/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright(c) 2024 Intel Corporation. All rights reserved.
 *
 * Author: Adrian Warecki <adrian.warecki@intel.com>
 */

#ifndef __EXAMPLE_MODULE__
#define __EXAMPLE_MODULE__

#include <module/handle.h>

#define EXAMPLE_UUID		0x4c, 0x6d, 0x64, 0x6b, 0x45, 0x78, 0x61, 0x6d, \
				0x70, 0x6c, 0x65, 0x2d, 0x4b, 0x6f, 0x6b, 0x6f
#define EXAMPLE_MAX_INSTANCES	5
#define EXAMPLE_AFFINITY	0x01

struct example_data {
	/* Module must provide module_handle as a first field of the module private data structure. */
	struct module_handle handle;

	/* TODO: Place the module variables here */
};

#endif /* __EXAMPLE_MODULE__ */
