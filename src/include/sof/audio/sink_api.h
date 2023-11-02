/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2023 Intel Corporation. All rights reserved.
 *
 */

#ifndef __SOF_SINK_API_H__
#define __SOF_SINK_API_H__

#include <module/audio/sink_api.h>

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
void sink_set_min_free_space(struct sof_sink *sink, size_t min_free_space);

#endif /* __SOF_SINK_API_H__ */
