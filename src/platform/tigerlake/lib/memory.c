// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright(c) 2023 Intel Corporation. All rights reserved.
 */

#include <zephyr/sys/util.h>
#include <zephyr/sys/predef_heap.h>

const struct predef_heap_config hp_buffers_config[] = {
	{ .count = 7, .size = 128 },
	{ .count = 2, .size = 384 },
	{ .count = 11, .size = 768 },
	{ .count = 6, .size = 1536 },
	{ .count = 3, .size = 2304 },
	{ .count = 6, .size = KB(3) },
	{ .count = 3, .size = 4224 },
};

/*
{
	uint8_t hp_buffer_0[20480];
	uint8_t hp_buffer_1[40960];
	uint8_t hp_buffer_2[49152];
	uint8_t hp_buffer_3[118784];
} dynamic_hp_buffers;
*/

const struct predef_heap_config lp_buffers_config[] = {
	{ .count = 8, .size = 128 },
	{ .count = 2, .size = 1536 },
	{ .count = 4, .size = 3072 },
	{ .count = 1, .size = KB(5) },
	{ .count = 1, .size = KB(30) }
};
