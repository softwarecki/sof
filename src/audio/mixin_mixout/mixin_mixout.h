/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright 2022 Intel Corporation. All rights reserved.
 */

/*
 * This file contains structures that are exact copies of an existing ABI used
 * by IOT middleware. They are Intel specific and will be used by one middleware.
 *
 * Some of the structures may contain programming implementations that makes them
 * unsuitable for generic use and general usage.
 *
 * This code is mostly copied "as-is" from existing C++ interface files hence the use of
 * different style in places. The intention is to keep the interface as close as possible to
 * original so it's easier to track changes with IPC host code.
 */

/**
 * \file include/ipc4/mixin_mixout.h
 * \brief IPC4 mixin/mixout definitions.
 */

#ifndef __SOF_IPC4_MIXIN_MIXOUT_H__
#define __SOF_IPC4_MIXIN_MIXOUT_H__

#include <stdint.h>
#include <rtos/bit.h>
#include <sof/audio/buffer.h>
#include <sof/audio/component.h>
#include <sof/audio/format.h>
#include <sof/platform.h>
#include <stddef.h>

enum ipc4_mixin_config_param {
	/* config_param param id to set/get underrun notification period. */
	IPC4_MIXER_UNDERRUN_NOTIF_PERIOD = 0,
	/* large_config_set param id for ipc4_mixer_mode_config */
	IPC4_MIXER_MODE = 1
};

/* Number of supported output pins (sinks) */
#define IPC4_MIXIN_MODULE_MAX_OUTPUT_QUEUES 3

/* Number of supported input pins that are mixed together */
#define IPC4_MIXOUT_MODULE_MAX_INPUT_QUEUES 8

/* Each mixin instance by default has set default notification
 * period to avoid notification flooding.
 */
#define MIXIN_MODULE_DEFAULT_UNDERRUN_NOTIFICATION_PERIOD	10
#define MIXIN_MODULE_MIN_UNDERRUN_NOTIFICATION_PERIOD		1
#define MIXIN_MODULE_MAX_UNDERRUN_NOTIFICATION_PERIOD		65535

enum ipc4_mixer_mode {
	/* Normal mode, just mixing */
	IPC4_MIXER_NORMAL_MODE = 0,

	/* Mixing with channel remapping */
	IPC4_MIXER_CHANNEL_REMAPPING_MODE = 1,
};

struct ipc4_mixer_mode_sink_config {
	/* Index of output queue (aka sink) this config is for,
	 * range from 0 to IPC4_MIXIN_MODULE_MAX_OUTPUT_QUEUES - 1
	 */
	uint32_t output_queue_id;

	/* Operational mode for given output queue index. enum ipc4_mixer_mode */
	uint32_t mixer_mode;

	/* These two below are used in channel remapping mode. */
	uint32_t output_channel_count;

	/* Output channel map for given output queue index. Each nibble (where nibble index is
	 * equivalent for output channel index) contains source channel index. Value 0xF in nibble
	 * means that output channel cannot be modified.
	 */
	uint32_t output_channel_map;

	/* Gain to be applied to input signal. Valid range: 0x0..0x400 (0.0 <= gain <= 1.0). Values
	 * greater than 0x400 are treated as 0x400 (unity gain). To apply gain, multiply sample
	 * by "gain" and divide by 1024.
	 */
	uint16_t gain;
	uint16_t reserved;
} __packed __aligned(4);

#define IPC4_MIXIN_GAIN_SHIFT 10
#define IPC4_MIXIN_UNITY_GAIN BIT(IPC4_MIXIN_GAIN_SHIFT)

/* Payload for large_config_set IPC4_MIXER_MODE param id */
struct ipc4_mixer_mode_config {
	/* Total number of given ipc4_mixer_mode_sink_config. Size of passed structure is
	 * determined by this number.
	 */
	uint32_t mixer_mode_config_count;

	/* Array of settings for sinks, size is mixer_mode_config_count. */
	struct ipc4_mixer_mode_sink_config mixer_mode_sink_configs[1];
} __packed __aligned(4);

/* Pointer to data in circular buffer together with buffer boundaries */
struct cir_buf_ptr {
	void *buf_start;
	void *buf_end;
	void *ptr;
};

/**
 * \brief mixin processing function interface
 */
typedef void (*mix_func)(struct cir_buf_ptr *sink, int32_t start_sample,
			 int32_t mixed_samples,
			 const struct cir_buf_ptr *source,
			 int32_t sample_count, uint16_t gain);

/**
 * @brief mixin processing functions map.
 */
struct mix_func_map {
	uint16_t frame_fmt;	/* frame format */
	mix_func mix;		/* faster mixing func without gain support */
	mix_func gain_mix;	/* slower mixing func with gain support */
};

extern const struct mix_func_map mix_func_map[];
extern const size_t mix_count;
/**
 * \brief Retrievies mixin processing function.
 * \param[in] fmt  stream PCM frame format
 */
static inline bool mixin_get_processing_functions(int fmt, mix_func *mix, mix_func *gain_mix)
{
	int i;

	*mix = NULL;
	*gain_mix = NULL;

	/* map mixin processing function for source and sink buffers */
	for (i = 0; i < mix_count; i++) {
		if (fmt == mix_func_map[i].frame_fmt) {
			*mix = mix_func_map[i].mix;
			*gain_mix = mix_func_map[i].gain_mix;
			return true;
		}
	}

	return false;
}

#endif	/* __SOF_IPC4_MIXIN_MIXOUT_H__ */
