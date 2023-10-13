/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2023 Intel Corporation. All rights reserved.
 *
 */

#ifndef __SOF_API_AUDIO_SOURCE_API_H__
#define __SOF_API_AUDIO_SOURCE_API_H__

#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

#include "source_api_implementation.h"
#include "audio_stream.h"
#include "format.h"

/**
 * this is a definition of API to source of audio data
 *
 * THE SOURCE is any component in the system that have data stored somehow and can give the
 * data outside at request. The source API does not define who and how has produced the data
 *
 * The user - a module - sees this as a producer that PROVIDES data for processing
 * The IMPLEMENTATION - audio_stream, DP Queue - sees this API as a destination it must send data to
 * *
 * Examples of components that should expose source API:
 *  - DMIC
 *	Data are coming from the outside world, stores in tmp buffer and can be presented
 *	to the rest of the system using source_api
 *  - a memory ring buffer
 *	Data are coming from other module (usually using sink_api, but it does not matter in fact)
 *
 *  The main advantage of using source API instead of just taking pointers to the data is that
 *  the data may be prepared at the moment the data receiver is requesting it. i.e.
 *   - cache may be written back/invalidated if necessary
 *   - data may be moved from circular to linear space
 *   - part of the buffer may be locked to prevent writing
 *   etc.etc. it depends on implementation of the data source
 *
 * Data in general are provided as a circular buffer and the data receiver should be able to
 * deal with it. Of course if needed an implementation of source providing linear data can be
 * implemented and used as a mid-layer for modules needing it.
 *
 * NOTE: the module should get a complete portion of data it needs for processing, process it
 *       than release. The reason is - the depending on the implementation, the calls may be
 *       expensive - may involve some data moving in memory, cache writebacks, etc.
 */

/**
 * Retrieves size of available data (in bytes)
 * return number of bytes that are available for immediate use
 */
static inline size_t source_get_data_available(struct sof_source *source)
{
	return source->ops->get_data_available(source);
}

static inline enum sof_ipc_frame source_get_frm_fmt(struct sof_source *source)
{
	return source->audio_stream_params->frame_fmt;
}

static inline unsigned int source_get_channels(struct sof_source *source)
{
	return source->audio_stream_params->channels;
}

/** get size of a single audio frame (in bytes) */
static inline size_t source_get_frame_bytes(struct sof_source *source)
{
	return get_frame_bytes(source_get_frm_fmt(source), source_get_channels(source));
}

/**
 * Retrieves size of available data (in frames)
 * return number of frames that are available for immediate use
 */
static inline size_t source_get_data_frames_available(struct sof_source *source)
{
	return source_get_data_available(source) / source_get_frame_bytes(source);
}

/**
 * Retrieves a fragment of circular data to be used by the caller (to read)
 * After calling get_data, the data are guaranteed to be available
 * for exclusive use (read only)
 * if the provided pointers are cached, it is guaranteed that the caller may safely use it without
 * any additional cache operations
 *
 * The caller MUST take care of data circularity based on provided pointers
 *
 * Depending on implementation - there may be a way to have several receivers of the same
 * data, as long as the receiver respects that data are read-only and won'do anything
 * fancy with cache handling itself
 *
 * some implementation data may be stored in linear buffer
 * in that case:
 *  data_ptr = buffer_start
 *  buffer_end = data_ptr + req_size
 *  buffer_size = req_size
 *
 *  and the data receiver may use it as usual, rollover will simple never occur
 *  NOTE! the caller MUST NOT assume that pointers to start/end of the circular buffer
 *	  are constant. They may change between calls
 *
 * @param source a handler to source
 * @param [in] req_size requested size of data.
 * @param [out] data_ptr a pointer to data will be provided there
 * @param [out] buffer_start pointer to circular buffer start
 * @param [out] buffer_size size of circular buffer
 *
 * @retval -ENODATA if req_size is bigger than available data
 */
static inline int source_get_data(struct sof_source *source, size_t req_size,
				  void const **data_ptr, void const **buffer_start,
				  size_t *buffer_size)
{
	int ret;

	if (source->requested_read_frag_size)
		return -EBUSY;

	ret = source->ops->get_data(source, req_size, data_ptr, buffer_start, buffer_size);

	if (!ret)
		source->requested_read_frag_size = req_size;
	return ret;
}

/**
 * Releases fragment previously obtained by source_get_data()
 * Once called, the data are no longer available for the caller
 *
 * @param source a handler to source
 * @param free_size amount of data that the caller declares as "never needed again"
 *	  if free_size == 0 the source implementation MUST keep all data in memory and make them
 *	  available again at next get_data() call
 *	  if free_size is bigger than the amount of data obtained before by get_data(), only
 *	  the amount obtained before will be freed. That means - if somebody obtained some data,
 *	  processed it and won't need it again, it may simple call put_data with free_size==MAXINT
 *
 * @return proper error code (0  on success)
 */
static inline int source_release_data(struct sof_source *source, size_t free_size)
{
	int ret;

	/* Check if anything was obtained before for reading by source_get_data */
	if (!source->requested_read_frag_size)
		return -ENODATA;

	/* limit size of data to be freed to previously obtained size */
	if (free_size > source->requested_read_frag_size)
		free_size = source->requested_read_frag_size;

	ret = source->ops->release_data(source, free_size);

	if (!ret)
		source->requested_read_frag_size = 0;

	source->num_of_bytes_processed += free_size;
	return ret;
}

/** set of functions for retrieve audio parameters */
static inline enum sof_ipc_frame source_get_valid_fmt(struct sof_source *source)
{
	return source->audio_stream_params->valid_sample_fmt;
}
static inline unsigned int source_get_rate(struct sof_source *source)
{
	return source->audio_stream_params->rate;
}

static inline uint32_t source_get_buffer_fmt(struct sof_source *source)
{
	return source->audio_stream_params->buffer_fmt;
}

#endif /* __SOF_API_AUDIO_SOURCE_API_H__ */
