// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright(c) 2016 Intel Corporation. All rights reserved.
//
// Author: Liam Girdwood <liam.r.girdwood@linux.intel.com>
//         Keyon Jie <yang.jie@linux.intel.com>

#include <sof/audio/buffer.h>
#include <sof/audio/component.h>
#include <sof/audio/format.h>
#include <sof/audio/module_adapter/module/generic.h>
#include <sof/audio/pipeline.h>
#include <sof/audio/ipc-config.h>
#include <sof/common.h>
#include <rtos/panic.h>
#include <sof/ipc/msg.h>
#include <rtos/init.h>
#include <sof/lib/uuid.h>
#include <sof/list.h>
#include <sof/math/numbers.h>
#include <sof/platform.h>
#include <rtos/string.h>
#include <sof/trace/trace.h>
#include <sof/ut.h>
#include <ipc/stream.h>
#include <ipc/topology.h>
#include <ipc4/base-config.h>
#include <user/trace.h>
#include <stddef.h>
#include <stdint.h>

#include "mixer.h"

LOG_MODULE_REGISTER(mixer, CONFIG_SOF_LOG_LEVEL);

SOF_DEFINE_REG_UUID(mixer);

static int mixer_init(struct processing_module *mod)
{
	struct module_data *mod_data = &mod->priv;
	struct comp_dev *dev = mod->dev;
	struct mixer_data *md;

	comp_dbg(dev, "entry");

	md = mod_zalloc(mod, sizeof(*md));
	if (!md)
		return -ENOMEM;

	mod_data->private = md;
	mod->verify_params_flags = BUFF_PARAMS_CHANNELS;
	mod->no_pause = true;
	mod->max_sources = MIXER_MAX_SOURCES;

	return 0;
}

static int mixer_free(struct processing_module *mod)
{
	struct mixer_data *md = module_get_private_data(mod);
	struct comp_dev *dev = mod->dev;

	comp_dbg(dev, "entry");

	mod_free(mod, md);

	return 0;
}

/*
 * Mix N source PCM streams to one sink PCM stream. Frames copied is constant.
 */
static int mixer_process(struct processing_module *mod,
			 struct sof_source **sources, int num_of_sources,
			 struct sof_sink **sinks, int num_of_sinks)
{
	struct mixer_data *md = module_get_private_data(mod);
	struct comp_dev *dev = mod->dev;
	const struct source_fragment *sources_stream[PLATFORM_MAX_STREAMS];
	struct source_fragment source_fragments[PLATFORM_MAX_STREAMS];
	struct sink_fragment sink_fragment;
	int sources_indices[PLATFORM_MAX_STREAMS];
	int32_t i = 0, j = 0;
	uint32_t frames;
	uint32_t sink_bytes;
	int active_input_buffers = 0;
	int ret;

	comp_dbg(dev, "%d", num_of_sources);

	/* too many sources ? */
	if (num_of_sources >= PLATFORM_MAX_STREAMS)
		return -EINVAL;
	if (num_of_sinks != 1)
		return -EINVAL;

	if (md->frame_align == 0 || md->frame_bytes == 0)
		return -EINVAL;

	frames = (sink_get_free_size(sinks[0]) / (md->frame_align * md->frame_bytes)) *
		md->frame_align;

	/* check for underruns */
	for (i = 0; i < num_of_sources; i++) {
		uint32_t avail_frames;

		avail_frames = (source_get_data_available(sources[i]) /
				(source_get_frame_bytes(sources[i]) * md->frame_align)) *
			md->frame_align;

		/* if one source is inactive, skip it */
		if (avail_frames == 0)
			continue;

		active_input_buffers++;
		frames = MIN(frames, avail_frames);
	}

	if (!active_input_buffers) {
		/*
		 * Generate silence when sources are inactive. When
		 * sources change to active, additionally keep
		 * generating silence until at least one of the
		 * sources start to have data available (frames!=0).
		 */
		frames = MIN(frames ? frames : sink_get_free_frames(sinks[0]), (uint32_t)dev->frames);
		if (md->frame_align > 1)
			frames -= frames % md->frame_align;
		if (!frames)
			return 0;

		return sink_fill_with_silence(sinks[0], frames * md->frame_bytes);
	}

	frames = MIN(frames, (uint32_t)dev->frames);
	if (md->frame_align > 1)
		frames -= frames % md->frame_align;
	if (!frames)
		return 0;

	sink_bytes = frames * md->frame_bytes;

	comp_dbg(dev, "sink_bytes = 0x%x", sink_bytes);

	ret = sink_get_buffer_fragment(sinks[0], sink_bytes, &sink_fragment);
	if (ret)
		return ret;

	/* mix streams */
	for (i = 0; i < num_of_sources; i++) {
		uint32_t avail_frames;

		avail_frames = (source_get_data_available(sources[i]) /
				(source_get_frame_bytes(sources[i]) * md->frame_align)) *
			md->frame_align;

		/* if one source is inactive, skip it */
		if (avail_frames == 0)
			continue;

		ret = source_get_data_fragment(sources[i], frames * source_get_frame_bytes(sources[i]),
					      &source_fragments[j]);
		if (ret) {
			while (j--)
				source_release_data(sources[sources_indices[j]], 0);
			sink_commit_buffer(sinks[0], 0);
			return ret;
		}

		sources_indices[j] = i;
		sources_stream[j] = &source_fragments[j];
		j++;
	}

	if (j)
		md->mix_func(dev, &sink_fragment, sources_stream, j, md->channels, frames);

	/* update source buffer consumed bytes */
	ret = 0;
	for (i = 0; i < j; i++) {
		int release_ret = source_release_data(sources[sources_indices[i]],
						    frames * source_get_frame_bytes(sources[sources_indices[i]]));

		if (!ret)
			ret = release_ret;
	}

	if (!ret)
		ret = sink_commit_buffer(sinks[0], sink_bytes);
	else
		sink_commit_buffer(sinks[0], 0);

	return ret;
}

static int mixer_reset(struct processing_module *mod)
{
	struct mixer_data *md = module_get_private_data(mod);
	struct comp_dev *dev = mod->dev;
	int dir = dev->pipeline->source_comp->direction;

	comp_dbg(dev, "entry");

	if (dir == SOF_IPC_STREAM_PLAYBACK) {
		struct comp_buffer *source;

		comp_dev_for_each_producer(dev, source) {
			/* FIXME: this is racy and implicitly protected by serialised IPCs */
			bool stop = false;

			if (comp_buffer_get_source_state(source) > COMP_STATE_READY)
				stop = true;

			/* only mix the sources with the same state with mixer */
			if (stop)
				/* should not reset the downstream components */
				return PPL_STATUS_PATH_STOP;
		}
	}

	md->mix_func = NULL;

	return 0;
}

/* init and calculate the aligned setting for available frames and free frames retrieve*/
static inline void mixer_set_frame_alignment(struct audio_stream *source)
{
#if XCHAL_HAVE_HIFI3 || XCHAL_HAVE_HIFI4

	/* Xtensa intrinsics ask for 8-byte aligned. 5.1 format SSE audio
	 * requires 16-byte aligned.
	 */
	const uint32_t byte_align = audio_stream_get_channels(source) == 6 ? 16 : 8;

	/*There is no limit for frame number, so set it as 1*/
	const uint32_t frame_align_req = 1;

	audio_stream_set_align(byte_align, frame_align_req, source);
#endif
}

static inline uint32_t mixer_frame_align_count(const struct audio_stream *stream)
{
	uint32_t byte_align = 1;
	uint32_t frame_bytes = audio_stream_frame_bytes(stream);

	if (!frame_bytes)
		return 0;

#if XCHAL_HAVE_HIFI3 || XCHAL_HAVE_HIFI4
	byte_align = audio_stream_get_channels(stream) == 6 ? 16 : 8;
#endif

	return byte_align / gcd(byte_align, frame_bytes);
}

static int mixer_prepare(struct processing_module *mod,
			 struct sof_source **sources, int num_of_sources,
			 struct sof_sink **sinks, int num_of_sinks)
{
	struct mixer_data *md = module_get_private_data(mod);
	struct comp_dev *dev = mod->dev;
	struct comp_buffer *sink;

	sink = comp_dev_get_first_data_consumer(dev);
	if (!sink) {
		comp_err(dev, "no sink");
		return -ENOTCONN;
	}

	md->mix_func = mixer_get_processing_function(dev, sink);
	if (!md->mix_func)
		return -EINVAL;
	md->channels = audio_stream_get_channels(&sink->stream);
	md->frame_bytes = audio_stream_frame_bytes(&sink->stream);
	md->frame_align = mixer_frame_align_count(&sink->stream);
	mixer_set_frame_alignment(&sink->stream);

	/* check each mixer source state */
	struct comp_buffer *source;

	comp_dev_for_each_producer(dev, source) {
		bool stop;

		/*
		 * FIXME: this is intrinsically racy. One of mixer sources can
		 * run on a different core and can enter PAUSED or ACTIVE right
		 * after we have checked it here. We should set a flag or a
		 * status to inform any other connected pipelines that we're
		 * preparing the mixer, so they shouldn't touch it until we're
		 * done.
		 */
		mixer_set_frame_alignment(&source->stream);
		stop = comp_buffer_get_source_state(source) == COMP_STATE_PAUSED ||
		       comp_buffer_get_source_state(source) == COMP_STATE_ACTIVE;

		/* only prepare downstream if we have no active sources */
		if (stop)
			return PPL_STATUS_PATH_STOP;
	}

	/* prepare downstream */
	return 0;
}

static const struct module_interface mixer_interface = {
	.init = mixer_init,
	.prepare = mixer_prepare,
	.process = mixer_process,
	.reset = mixer_reset,
	.free = mixer_free,
};

DECLARE_TR_CTX(mixer_tr, SOF_UUID(mixer_uuid), LOG_LEVEL_INFO);
DECLARE_MODULE_ADAPTER(mixer_interface, mixer_uuid, mixer_tr);
SOF_MODULE_INIT(mixer, sys_comp_module_mixer_interface_init);
