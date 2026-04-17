// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright(c) 2022 Intel Corporation. All rights reserved.
//
// Author: Andrula Song <xiaoyuan.song@intel.com>

#include <sof/common.h>

#include "mixer.h"

#ifdef MIXER_GENERIC

#if CONFIG_FORMAT_S16LE
/* Mix n 16 bit PCM source streams to one sink stream */
static void mix_n_s16(struct comp_dev *dev, struct sink_fragment *sink,
		      const struct source_fragment **sources, uint32_t num_sources,
		      uint32_t channels, uint32_t frames)
{
	int16_t *src[PLATFORM_MAX_STREAMS];
	int16_t *dest = sink->data_ptr;
	int32_t val;
	int nmax;
	int i, j, n, ns;
	int processed = 0;
	int samples = frames * channels;

	for (j = 0; j < num_sources; j++)
		src[j] = (int16_t *)sources[j]->data_ptr;

	while (processed < samples) {
		nmax = samples - processed;
		n = sink_fragment_samples_without_wrap_s16(sink, dest);
		n = MIN(n, nmax);
		for (i = 0; i < num_sources; i++) {
			ns = source_fragment_samples_without_wrap_s16(sources[i], src[i]);
			n = MIN(n, ns);
		}
		for (i = 0; i < n; i++) {
			val = 0;
			for (j = 0; j < num_sources; j++) {
				val += *src[j];
				src[j]++;
			}

			/* Saturate to 16 bits */
			*dest = sat_int16(val);
			dest++;
		}
		processed += n;
		dest = sink_fragment_wrap(sink, dest);
		for (i = 0; i < num_sources; i++)
			src[i] = (int16_t *)source_fragment_wrap(sources[i], src[i]);
	}
}
#endif /* CONFIG_FORMAT_S16LE */

#if CONFIG_FORMAT_S24LE
/* Mix n 24 bit PCM source streams to one sink stream */
static void mix_n_s24(struct comp_dev *dev, struct sink_fragment *sink,
		      const struct source_fragment **sources, uint32_t num_sources,
		      uint32_t channels, uint32_t frames)
{
	int32_t *src[PLATFORM_MAX_STREAMS];
	int32_t *dest = sink->data_ptr;
	int32_t val;
	int32_t x;
	int nmax;
	int i, j, n, ns;
	int processed = 0;
	int samples = frames * channels;

	for (j = 0; j < num_sources; j++)
		src[j] = (int32_t *)sources[j]->data_ptr;

	while (processed < samples) {
		nmax = samples - processed;
		n = sink_fragment_samples_without_wrap_s24(sink, dest);
		n = MIN(n, nmax);
		for (i = 0; i < num_sources; i++) {
			ns = source_fragment_samples_without_wrap_s24(sources[i], src[i]);
			n = MIN(n, ns);
		}
		for (i = 0; i < n; i++) {
			val = 0;
			for (j = 0; j < num_sources; j++) {
				x = *src[j] << 8;
				val += x >> 8; /* Sign extend */
				src[j]++;
			}

			/* Saturate to 24 bits */
			*dest = sat_int24(val);
			dest++;
		}
		processed += n;
		dest = sink_fragment_wrap(sink, dest);
		for (i = 0; i < num_sources; i++)
			src[i] = (int32_t *)source_fragment_wrap(sources[i], src[i]);
	}
}
#endif /* CONFIG_FORMAT_S24LE */

#if CONFIG_FORMAT_S32LE
/* Mix n 32 bit PCM source streams to one sink stream */
static void mix_n_s32(struct comp_dev *dev, struct sink_fragment *sink,
		      const struct source_fragment **sources, uint32_t num_sources,
		      uint32_t channels, uint32_t frames)
{
	int32_t *src[PLATFORM_MAX_STREAMS];
	int32_t *dest = sink->data_ptr;
	int64_t val;
	int nmax;
	int i, j, n, ns;
	int processed = 0;
	int samples = frames * channels;

	for (j = 0; j < num_sources; j++)
		src[j] = (int32_t *)sources[j]->data_ptr;

	while (processed < samples) {
		nmax = samples - processed;
		n = sink_fragment_samples_without_wrap_s32(sink, dest);
		n = MIN(n, nmax);
		for (i = 0; i < num_sources; i++) {
			ns = source_fragment_samples_without_wrap_s32(sources[i], src[i]);
			n = MIN(n, ns);
		}
		for (i = 0; i < n; i++) {
			val = 0;
			for (j = 0; j < num_sources; j++) {
				val += *src[j];
				src[j]++;
			}

			/* Saturate to 32 bits */
			*dest = sat_int32(val);
			dest++;
		}
		processed += n;
		dest = sink_fragment_wrap(sink, dest);
		for (i = 0; i < num_sources; i++)
			src[i] = (int32_t *)source_fragment_wrap(sources[i], src[i]);
	}
}
#endif /* CONFIG_FORMAT_S32LE */

const struct mixer_func_map mixer_func_map[] = {
#if CONFIG_FORMAT_S16LE
	{ SOF_IPC_FRAME_S16_LE, mix_n_s16 },
#endif
#if CONFIG_FORMAT_S24LE
	{ SOF_IPC_FRAME_S24_4LE, mix_n_s24 },
#endif
#if CONFIG_FORMAT_S32LE
	{ SOF_IPC_FRAME_S32_LE, mix_n_s32 },
#endif
};

const size_t mixer_func_count = ARRAY_SIZE(mixer_func_map);

#endif
