/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2023 Intel Corporation. All rights reserved.
 *
 */

#ifndef __SOF_SOURCE_API_IMPLEMENTATION_H__
#define __SOF_SOURCE_API_IMPLEMENTATION_H__

#include <sof/common.h>
#include <stdint.h>
#include <stdbool.h>

#include <sof_api/audio/source_api_implementation.h>

/**
 * Init of the API, must be called before any operation
 *
 * @param source pointer to the structure
 * @param ops pointer to API operations
 * @param audio_stream_params pointer to structure with audio parameters
 *	  note that the audio_stream_params must be accessible by the caller core
 *	  the implementation must ensure coherent access to the parameteres
 *	  in case of i.e. cross core shared queue, it must be located in non-cached memory
 */
void source_init(struct sof_source *source, const struct source_ops *ops,
		 struct sof_audio_stream_params *audio_stream_params);

#endif /* __SOF_SOURCE_API_IMPLEMENTATION_H__ */
