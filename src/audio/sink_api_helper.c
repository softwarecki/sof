// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright(c) 2023 Intel Corporation. All rights reserved.
 */

#include <sof/audio/sink_api.h>
#include <sof/audio/sink_api_implementation.h>
#include <sof/audio/audio_stream.h>

/* This file contains private sink API functions intended for use only by the sof. */

void sink_init(struct sof_sink *sink, const struct sink_ops *ops,
	       struct sof_audio_stream_params *audio_stream_params)
{
	sink->ops = ops;
	sink->audio_stream_params = audio_stream_params;
}

size_t sink_get_num_of_processed_bytes(struct sof_sink *sink)
{
	return sink->num_of_bytes_processed;
}

void sink_reset_num_of_processed_bytes(struct sof_sink *sink)
{
	sink->num_of_bytes_processed = 0;
}

bool sink_get_overrun(struct sof_sink *sink)
{
	return sink->audio_stream_params->overrun_permitted;
}
x
int sink_set_valid_fmt(struct sof_sink *sink,
		       enum sof_ipc_frame valid_sample_fmt)
{
	sink->audio_stream_params->valid_sample_fmt = valid_sample_fmt;
	if (sink->ops->on_audio_format_set)
		return sink->ops->on_audio_format_set(sink);
	return 0;
}
x
int sink_set_rate(struct sof_sink *sink, unsigned int rate)
{
	sink->audio_stream_params->rate = rate;
	if (sink->ops->on_audio_format_set)
		return sink->ops->on_audio_format_set(sink);
	return 0;
}
x
int sink_set_channels(struct sof_sink *sink, unsigned int channels)
{
	sink->audio_stream_params->channels = channels;
	if (sink->ops->on_audio_format_set)
		return sink->ops->on_audio_format_set(sink);
	return 0;
}
x
int sink_set_buffer_fmt(struct sof_sink *sink, uint32_t buffer_fmt)
{
	sink->audio_stream_params->buffer_fmt = buffer_fmt;
	if (sink->ops->on_audio_format_set)
		return sink->ops->on_audio_format_set(sink);
	return 0;
}
x
int sink_set_overrun(struct sof_sink *sink, bool overrun_permitted)
{
	sink->audio_stream_params->overrun_permitted = overrun_permitted;
	if (sink->ops->on_audio_format_set)
		return sink->ops->on_audio_format_set(sink);
	return 0;
}
x
int sink_set_params(struct sof_sink *sink,
		    struct sof_ipc_stream_params *params, bool force_update)
{
	if (sink->ops->audio_set_ipc_params)
		return sink->ops->audio_set_ipc_params(sink, params, force_update);
	return 0;
}
x
int sink_set_alignment_constants(struct sof_sink *sink,
				 const uint32_t byte_align,
				 const uint32_t frame_align_req)
{
	if (sink->ops->set_alignment_constants)
		return sink->ops->set_alignment_constants(sink, byte_align, frame_align_req);
	return 0;
}

void sink_set_min_free_space(struct sof_sink *sink, size_t min_free_space)
{
	sink->min_free_space = min_free_space;
}
x
size_t sink_get_min_free_space(struct sof_sink *sink)
{
	return sink->min_free_space;
}
