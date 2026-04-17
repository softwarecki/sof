// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright(c) 2020 Google LLC. All rights reserved.
//
// Author: Sebastiano Carlucci <scarlucci@google.com>

#include <stdint.h>
#include <sof/audio/component.h>
#include <sof/audio/format.h>

#include "dcblock.h"

#if SOF_USE_HIFI(NONE, DCBLOCK)

LOG_MODULE_DECLARE(dcblock, CONFIG_SOF_LOG_LEVEL);

/**
 *
 * Genereric processing function. Input is 32 bits.
 *
 */
static int32_t dcblock_generic(struct dcblock_state *state,
			       int64_t R, int32_t x)
{
	/*
	 * R: Q2.30, y_prev: Q1.31
	 * R * y_prev: Q3.61
	 */
	int64_t out = ((int64_t)x) - state->x_prev +
		      Q_SHIFT_RND(R * state->y_prev, 61, 31);

	state->y_prev = sat_int32(out);
	state->x_prev = x;

	return state->y_prev;
}

#if CONFIG_FORMAT_S16LE
static void dcblock_s16_default(struct comp_data *cd,
				const struct source_fragment *source_fragment,
				struct sink_fragment *sink_fragment,
				uint32_t frames)
{
	struct dcblock_state *state;
	const int16_t *x = source_fragment->data_ptr;
	int16_t *y = sink_fragment->data_ptr;
	int32_t R;
	int32_t tmp;
	int idx;
	int ch;
	int i, n, nmax;
	int nch = cd->channels;
	int samples = nch * frames;

	while (samples) {
		nmax = source_fragment_samples_without_wrap_s16(source_fragment, x);
		n = MIN(samples, nmax);
		nmax = sink_fragment_samples_without_wrap_s16(sink_fragment, y);
		n = MIN(n, nmax);
		for (ch = 0; ch < nch; ch++) {
			state = &cd->state[ch];
			R = cd->R_coeffs[ch];
			idx = ch;
			for (i = 0; i < n; i += nch) {
				tmp = dcblock_generic(state, R, x[idx] << 16);
				y[idx] = sat_int16(Q_SHIFT_RND(tmp, 31, 15));
				idx += nch;
			}
		}
		samples -= n;
		x = source_fragment_wrap(source_fragment, x + n);
		y = sink_fragment_wrap(sink_fragment, y + n);
	}

}
#endif /* CONFIG_FORMAT_S16LE */

#if CONFIG_FORMAT_S24LE
static void dcblock_s24_default(struct comp_data *cd,
				const struct source_fragment *source_fragment,
				struct sink_fragment *sink_fragment,
				uint32_t frames)
{
	struct dcblock_state *state;
	const int32_t *x = source_fragment->data_ptr;
	int32_t *y = sink_fragment->data_ptr;
	int32_t R;
	int32_t tmp;
	int idx;
	int ch;
	int i, n, nmax;
	int nch = cd->channels;
	int samples = nch * frames;

	while (samples) {
		nmax = source_fragment_samples_without_wrap_s24(source_fragment, x);
		n = MIN(samples, nmax);
		nmax = sink_fragment_samples_without_wrap_s24(sink_fragment, y);
		n = MIN(n, nmax);
		for (ch = 0; ch < nch; ch++) {
			state = &cd->state[ch];
			R = cd->R_coeffs[ch];
			idx = ch;
			for (i = 0; i < n; i += nch) {
				tmp = dcblock_generic(state, R, x[idx] << 8);
				y[idx] = sat_int24(Q_SHIFT_RND(tmp, 31, 23));
				idx += nch;
			}
		}
		samples -= n;
		x = source_fragment_wrap(source_fragment, x + n);
		y = sink_fragment_wrap(sink_fragment, y + n);
	}

}
#endif /* CONFIG_FORMAT_S24LE */

#if CONFIG_FORMAT_S32LE
static void dcblock_s32_default(struct comp_data *cd,
				const struct source_fragment *source_fragment,
				struct sink_fragment *sink_fragment,
				uint32_t frames)
{
	struct dcblock_state *state;
	const int32_t *x = source_fragment->data_ptr;
	int32_t *y = sink_fragment->data_ptr;
	int32_t R;
	int idx;
	int ch;
	int i, n, nmax;
	int nch = cd->channels;
	int samples = nch * frames;

	while (samples) {
		nmax = source_fragment_samples_without_wrap_s32(source_fragment, x);
		n = MIN(samples, nmax);
		nmax = sink_fragment_samples_without_wrap_s32(sink_fragment, y);
		n = MIN(n, nmax);
		for (ch = 0; ch < nch; ch++) {
			state = &cd->state[ch];
			R = cd->R_coeffs[ch];
			idx = ch;
			for (i = 0; i < n; i += nch) {
				y[idx] = dcblock_generic(state, R, x[idx]);
				idx += nch;
			}
		}
		samples -= n;
		x = source_fragment_wrap(source_fragment, x + n);
		y = sink_fragment_wrap(sink_fragment, y + n);
	}
}
#endif /* CONFIG_FORMAT_S32LE */

const struct dcblock_func_map dcblock_fnmap[] = {
/* { SOURCE_FORMAT , PROCESSING FUNCTION } */
#if CONFIG_FORMAT_S16LE
	{ SOF_IPC_FRAME_S16_LE, dcblock_s16_default },
#endif /* CONFIG_FORMAT_S16LE */
#if CONFIG_FORMAT_S24LE
	{ SOF_IPC_FRAME_S24_4LE, dcblock_s24_default },
#endif /* CONFIG_FORMAT_S24LE */
#if CONFIG_FORMAT_S32LE
	{ SOF_IPC_FRAME_S32_LE, dcblock_s32_default },
#endif /* CONFIG_FORMAT_S32LE */
};

const size_t dcblock_fncount = ARRAY_SIZE(dcblock_fnmap);
#endif
