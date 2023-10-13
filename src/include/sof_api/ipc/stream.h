/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2023 Intel Corporation. All rights reserved.
 *
 * Author: Liam Girdwood <liam.r.girdwood@linux.intel.com>
 *	   Keyon Jie <yang.jie@linux.intel.com>
 *	   Adrian Warecki <adrian.warecki@intel.com>
 */

#ifndef __SOF_API_IPC_STREAM_H__
#define __SOF_API_IPC_STREAM_H__

/* stream PCM frame format */
enum sof_ipc_frame {
	SOF_IPC_FRAME_S16_LE = 0,
	SOF_IPC_FRAME_S24_4LE,
	SOF_IPC_FRAME_S32_LE,
	SOF_IPC_FRAME_FLOAT,
	/* other formats here */
	SOF_IPC_FRAME_S24_3LE,
	SOF_IPC_FRAME_S24_4LE_MSB,
	SOF_IPC_FRAME_U8,
};

#endif /* __SOF_API_IPC_STREAM_H__ */
