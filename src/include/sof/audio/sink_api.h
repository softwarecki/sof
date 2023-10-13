/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2023 Intel Corporation. All rights reserved.
 *
 */

#ifndef __SOF_SINK_API_H__
#define __SOF_SINK_API_H__

#include <sof_api/audio/sink_api.h>

/**
 * Get total number of bytes processed by the sink (meaning - committed by sink_commit_buffer())
 *
 * @param sink a handler to sink
 * @return total number of processed data
 */
size_t sink_get_num_of_processed_bytes(struct sof_sink *sink);

/**
 * sets counter of total number of bytes processed  to zero
 */
void sink_reset_num_of_processed_bytes(struct sof_sink *sink);

/** set of functions for retrieve audio parameters */
bool sink_get_overrun(struct sof_sink *sink);

/** set of functions for setting audio parameters */
int sink_set_frm_fmt(struct sof_sink *sink, enum sof_ipc_frame frame_fmt);
int sink_set_valid_fmt(struct sof_sink *sink, enum sof_ipc_frame valid_sample_fmt);
int sink_set_rate(struct sof_sink *sink, unsigned int rate);
int sink_set_channels(struct sof_sink *sink, unsigned int channels);
int sink_set_overrun(struct sof_sink *sink, bool overrun_permitted);
int sink_set_buffer_fmt(struct sof_sink *sink, uint32_t buffer_fmt);
void sink_set_min_free_space(struct sof_sink *sink, size_t min_free_space);
size_t sink_get_min_free_space(struct sof_sink *sink);

/**
 * initial set of audio parameters, provided in sof_ipc_stream_params
 *
 * @param sink a handler to sink
 * @param params the set of parameters
 * @param force_update tells the implementation that the params should override actual settings
 * @return 0 if success
 */
int sink_set_params(struct sof_sink *sink,
		    struct sof_ipc_stream_params *params, bool force_update);

/**
 * Set frame_align_shift and frame_align of stream according to byte_align and
 * frame_align_req alignment requirement. Once the channel number,frame size
 * are determined, the frame_align and frame_align_shift are determined too.
 * these two feature will be used in audio_stream_get_avail_frames_aligned
 * to calculate the available frames. it should be called in component prepare
 * or param functions only once before stream copy. if someone forgets to call
 * this first, there would be unexampled error such as nothing is copied at all.
 *
 * @param sink a handler to sink
 * @param byte_align Processing byte alignment requirement.
 * @param frame_align_req Processing frames alignment requirement.
 *
 * @return 0 if success
 */
int sink_set_alignment_constants(struct sof_sink *sink,
				 const uint32_t byte_align,
				 const uint32_t frame_align_req);

#endif /* __SOF_SINK_API_H__ */
