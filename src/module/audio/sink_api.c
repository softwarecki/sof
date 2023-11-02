// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright(c) 2023 Intel Corporation. All rights reserved.
 */

#include <sof/audio/sink_api.h>
#include <sof/audio/sink_api_implementation.h>
#include <sof/audio/audio_stream.h>

/* This file contains public sink API functions that were too large to mark is as inline. */

int sink_get_buffer(struct sof_sink *sink, size_t req_size,
		    void **data_ptr, void **buffer_start, size_t *buffer_size)
{
	int ret;

	if (sink->requested_write_frag_size)
		return -EBUSY;

	ret = sink->ops->get_buffer(sink, req_size, data_ptr,
						buffer_start, buffer_size);

	if (!ret)
		sink->requested_write_frag_size = req_size;
	return ret;
}

int sink_commit_buffer(struct sof_sink *sink, size_t commit_size)
{
	int ret;

	/* check if there was a buffer obtained for writing by sink_get_buffer */
	if (!sink->requested_write_frag_size)
		return -ENODATA;

	/* limit size of data to be committed to previously obtained size */
	if (commit_size > sink->requested_write_frag_size)
		commit_size = sink->requested_write_frag_size;

	ret = sink->ops->commit_buffer(sink, commit_size);

	if (!ret)
		sink->requested_write_frag_size = 0;

	sink->num_of_bytes_processed += commit_size;
	return ret;
}

int sink_set_frm_fmt(struct sof_sink *sink, enum sof_ipc_frame frame_fmt)
{
	sink->audio_stream_params->frame_fmt = frame_fmt;

	/* notify the implementation */
	if (sink->ops->on_audio_format_set)
		return sink->ops->on_audio_format_set(sink);
	return 0;
}

size_t sink_get_free_frames(struct sof_sink *sink)
{
	return sink_get_free_size(sink) /
			sink_get_frame_bytes(sink);
}
