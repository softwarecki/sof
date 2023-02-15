// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright(c) 2023 Intel Corporation. All rights reserved.
 */

#include <zephyr/sys/util.h>

#include <zephyr/sys/predef_heap.h>

 /* Heap section sizes for system runtime heap for primary core */
#define HEAP_SYS_RT_0_COUNT64		128
#define HEAP_SYS_RT_0_COUNT512		16
#define HEAP_SYS_RT_0_COUNT1024		4

 /* Heap section sizes for system runtime heap for secondary core */
#define HEAP_SYS_RT_X_COUNT64		64
#define HEAP_SYS_RT_X_COUNT512		8
#define HEAP_SYS_RT_X_COUNT1024		4

 /* Heap section counts base */
#define HEAP_COUNT64		128
#define HEAP_COUNT128		128
#define HEAP_COUNT256		96
#define HEAP_COUNT512		8
#define HEAP_COUNT1024		4
#define HEAP_COUNT2048		2
#define HEAP_COUNT4096		1

#define RT_TIMES	5
#define RT_SHARED_TIMES	4

 /* Heap section sizes for module pool */
#define HEAP_RT_COUNT64			(HEAP_COUNT64 * RT_TIMES)
#define HEAP_RT_COUNT128		(HEAP_COUNT128 * RT_TIMES)
#define HEAP_RT_COUNT256		(HEAP_COUNT256 * RT_TIMES)
#define HEAP_RT_COUNT512		(HEAP_COUNT512 * RT_TIMES)
#define HEAP_RT_COUNT1024		(HEAP_COUNT1024 * RT_TIMES)
#define HEAP_RT_COUNT2048		(HEAP_COUNT2048 * RT_TIMES)
#define HEAP_RT_COUNT4096		(HEAP_COUNT4096 * RT_TIMES)

 /* Heap section sizes for runtime shared heap */
#define HEAP_RUNTIME_SHARED_COUNT64	(HEAP_COUNT64 * RT_SHARED_TIMES)
#define HEAP_RUNTIME_SHARED_COUNT128	(HEAP_COUNT128 * RT_SHARED_TIMES)
#define HEAP_RUNTIME_SHARED_COUNT256	(HEAP_COUNT256 * RT_SHARED_TIMES)
#define HEAP_RUNTIME_SHARED_COUNT512	(HEAP_COUNT512 * RT_SHARED_TIMES)
#define HEAP_RUNTIME_SHARED_COUNT1024	(HEAP_COUNT1024 * RT_SHARED_TIMES)




#define SUM_HEAP_COUNT64	(HEAP_RT_COUNT64 + HEAP_RUNTIME_SHARED_COUNT64 +	\
				HEAP_SYS_RT_X_COUNT64 + HEAP_SYS_RT_0_COUNT64)
#define SUM_HEAP_COUNT128	(HEAP_RT_COUNT128 + HEAP_RUNTIME_SHARED_COUNT128)
#define SUM_HEAP_COUNT256	(HEAP_RT_COUNT256 + HEAP_RUNTIME_SHARED_COUNT256)
#define SUM_HEAP_COUNT512	(HEAP_SYS_RT_0_COUNT512 + HEAP_SYS_RT_X_COUNT512 +	\
				HEAP_RT_COUNT512 + HEAP_RUNTIME_SHARED_COUNT512)
#define SUM_HEAP_COUNT1024	(HEAP_SYS_RT_0_COUNT1024 + HEAP_SYS_RT_X_COUNT1024 +	\
				HEAP_RT_COUNT1024 + HEAP_RUNTIME_SHARED_COUNT1024)
#define SUM_HEAP_COUNT2048	(HEAP_RT_COUNT2048)
#define SUM_HEAP_COUNT4096	(HEAP_RT_COUNT4096)


/* Each bundle must be aligned to DCache linesize */
const struct predef_heap_config hp_buffers_config[] = {
	{ .count = SUM_HEAP_COUNT64, .size = 64 },
	{ .count = SUM_HEAP_COUNT128, .size = 128 },
	{ .count = SUM_HEAP_COUNT256, .size = 256 },
	{ .count = SUM_HEAP_COUNT512, .size = 512 },
	{ .count = SUM_HEAP_COUNT1024, .size = KB(1) },
	{ .count = SUM_HEAP_COUNT2048, .size = KB(2) },
	{ .count = SUM_HEAP_COUNT4096+32, .size = KB(4) },
	{ .count = 4, .size = KB(8) },
	//{ .count = 4, .size = KB(16) },
	{ .count = 3, .size = KB(32) },
	//{ .count = 1, .size = KB(64) },
	{ .count = 0 , .size = 0 }, /* Required empty entry as end marker! */

	//{ .count = 7, .size = 128 },
	//{ .count = 2, .size = 384 },
	//{ .count = 11, .size = 768 },
	//{ .count = 6, .size = 1536 },
	//{ .count = 3, .size = 2304 },
	//{ .count = 6, .size = KB(3) },
	//{ .count = 3, .size = 4224 },
	//{ .count = 1, .size = KB(40) },		/* dynamic */
	//{ .count = 1, .size = KB(48) },		/* dynamic */
	//{ .count = 1, .size = KB(96) },		/* static */
	//{ .count = 1, .size = KB(116) },	/* dynamic */
};

/* Each bundle must be aligned to Page Size */
/*
{
uint8_t hp_buffer_0[20480];
	uint8_t hp_buffer_1[40960];
	uint8_t hp_buffer_2[49152];
	uint8_t hp_buffer_3[118784];
} dynamic_hp_buffers;
*/

/* Each bundle must be aligned to DCache linesize */
const struct predef_heap_config lp_buffers_config[] = {
	{ .count = 8, .size = 128 },
	{ .count = 2, .size = 1536 },
	{ .count = 3, .size = 3072 },
	{ .count = 1, .size = KB(5) },
	{ .count = 1, .size = KB(38) }
};
