// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright(c) 2022 Intel Corporation. All rights reserved.
 *
 */

#include <sof/init.h>
#include <rtos/alloc.h>
#include <sof/drivers/idc.h>
#include <rtos/interrupt.h>
#include <sof/drivers/interrupt-map.h>
#include <sof/lib/dma.h>
#include <sof/schedule/schedule.h>
#include <platform/drivers/interrupt.h>
#include <sof/lib/notifier.h>
#include <sof/lib/pm_runtime.h>
#include <sof/audio/pipeline.h>
#include <sof/audio/component_ext.h>
#include <sof/trace/trace.h>
#include <rtos/wait.h>

/* Zephyr includes */
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/policy.h>
#include <version.h>
#include <zephyr/sys/__assert.h>
#include <soc.h>

#define DEFAULT_HEAP_SUPPORTED	1
#define PREDEF_HEAP_SUPPORTED	IS_ENABLED(CONFIG_USE_PREDEFINED_HEAP)

#if PREDEF_HEAP_SUPPORTED
#include <zephyr/sys/predef_heap.h>
#endif	/* PREDEF_HEAP_SUPPORTED */

#if defined(CONFIG_ARCH_XTENSA) && !defined(CONFIG_KERNEL_COHERENCE)
#include <zephyr/arch/xtensa/cache.h>
#endif

#if CONFIG_SYS_HEAP_RUNTIME_STATS && CONFIG_IPC_MAJOR_4
#include <zephyr/sys/sys_heap.h>
#endif

LOG_MODULE_REGISTER(mem_allocator, CONFIG_SOF_LOG_LEVEL);

extern struct tr_ctx zephyr_tr;

/*
 * Memory - Create Zephyr HEAP for SOF.
 *
 * Currently functional but some items still WIP.
 */

#ifndef HEAP_RUNTIME_SIZE
#define HEAP_RUNTIME_SIZE	0
#endif

/* system size not declared on some platforms */
#ifndef HEAP_SYSTEM_SIZE
#define HEAP_SYSTEM_SIZE	0
#endif

/* The Zephyr heap */

#ifdef CONFIG_IMX
#define HEAPMEM_SIZE		(HEAP_SYSTEM_SIZE + HEAP_RUNTIME_SIZE + HEAP_BUFFER_SIZE)

/*
 * Include heapmem variable in .heap_mem section, otherwise the HEAPMEM_SIZE is
 * duplicated in two sections and the sdram0 region overflows.
 */
__section(".heap_mem") static uint8_t __aligned(64) heapmem[HEAPMEM_SIZE];

#elif CONFIG_ACE

/*
 * System heap definition for ACE is defined below.
 * It needs to be explicitly packed into dedicated section
 * to allow memory management driver to control unused
 * memory pages.
 */
__section(".heap_mem") static uint8_t __aligned(PLATFORM_DCACHE_ALIGN) heapmem[HEAPMEM_SIZE];

#elif defined(CONFIG_ARCH_POSIX)

/* Zephyr native_posix links as a host binary and lacks the automated heap markers */
#define HEAPMEM_SIZE (256 * 1024)
char __aligned(8) heapmem[HEAPMEM_SIZE];

#else

extern char _end[], _heap_sentry[];
#define heapmem ((uint8_t *)ALIGN_UP((uintptr_t)_end, PLATFORM_DCACHE_ALIGN))
#define HEAPMEM_SIZE ((uint8_t *)_heap_sentry - heapmem)

#endif


struct sof_mem_desc;

struct sof_heap_ops {
	void* (*alloc)(struct sof_mem_desc *desc, const size_t size, const size_t align);
	int (*free)(struct sof_mem_desc *desc, void* mem);
	size_t (*get_size)(struct sof_mem_desc *desc, void* mem);
};

struct sof_mem_desc {
	uintptr_t start, end;
	const struct sof_heap_ops *ops;
	struct k_spinlock lock;
};

#if DEFAULT_HEAP_SUPPORTED
struct sof_default_heap {
	struct sof_mem_desc desc;
	struct sys_heap heap;
};

static void *default_heap_alloc(struct sof_mem_desc *desc, const size_t size, const size_t align)
{
	struct sof_default_heap *heap = container_of(desc, struct sof_default_heap, desc);
	k_spinlock_key_t key;
	void *ret;

	key = k_spin_lock(&desc->lock);
	ret = sys_heap_aligned_alloc(&heap->heap, align, size);
	k_spin_unlock(&desc->lock, key);

#if CONFIG_SYS_HEAP_RUNTIME_STATS && CONFIG_IPC_MAJOR_4
	struct sys_memory_stats stats;

	sys_heap_runtime_stats_get(&heap->heap, &stats);
	tr_info(&zephyr_tr, "heap allocated: %u free: %u max allocated: %u",
		stats.allocated_bytes, stats.free_bytes, stats.max_allocated_bytes);
#endif
	return ret;
}

static int default_heap_free(struct sof_mem_desc *desc, void *mem)
{

	struct sof_default_heap *heap = container_of(desc, struct sof_default_heap, desc);
	k_spinlock_key_t key;

	key = k_spin_lock(&desc->lock);
	sys_heap_free(&heap->heap, mem);
	k_spin_unlock(&desc->lock, key);

	return 0;
}

static size_t default_heap_get_size(struct sof_mem_desc *desc, void *mem)
{
	struct sof_default_heap *heap = container_of(desc, struct sof_default_heap, desc);

	return sys_heap_usable_size(&heap->heap, mem);
}

const static struct sof_heap_ops default_heap_ops = {
	.alloc = default_heap_alloc,
	.free = default_heap_free,
	.get_size = default_heap_get_size,
};

static void default_heap_init(struct sof_default_heap *heap, void *const start,
			      const size_t size)
{
	heap->desc.start = POINTER_TO_UINT(start);
	heap->desc.end = POINTER_TO_UINT(start) + size - 1;
	heap->desc.ops = &default_heap_ops;

	sys_heap_init(&heap->heap, UINT_TO_POINTER(start), size);
}
#endif	/* DEFAULT_HEAP_SUPPORTED */


#if PREDEF_HEAP_SUPPORTED
struct sof_predef_heap {
	struct sof_mem_desc desc;
	struct predef_heap heap;
	unsigned long config_store[128*4];
};

static void *predef_alloc(struct sof_mem_desc *desc, const size_t size, const size_t align)
{ 
	struct sof_predef_heap *heap = container_of(desc, struct sof_predef_heap, desc);
	k_spinlock_key_t key;
	void *ptr;

	key = k_spin_lock(&desc->lock);
	ptr = predef_heap_aligned_alloc(&heap->heap, align, size);
	k_spin_unlock(&desc->lock, key);
	/*
	if (ptr)
		tr_err(&zephyr_tr, "predef: requested: %d, allocated: %d", (int)size,
		       (int)predef_heap_usable_size(&heap->heap, ptr));
	*/
	if (!ptr)
		tr_err(&zephyr_tr, "predef: unable to alloc %d", (int)size);

	return ptr;
}

static int predef_free(struct sof_mem_desc *desc, void *mem)
{ 
	struct sof_predef_heap *heap = container_of(desc, struct sof_predef_heap, desc);
	k_spinlock_key_t key;
	int ret;

	key = k_spin_lock(&desc->lock);
	ret = predef_heap_free(&heap->heap, mem);
	k_spin_unlock(&desc->lock, key);
	assert(ret == 0);
	return ret;
}

static size_t predef_get_size(struct sof_mem_desc *desc, void *mem)
{
	struct sof_predef_heap *heap = container_of(desc, struct sof_predef_heap, desc);

	return predef_heap_usable_size(&heap->heap, mem);
}

const static struct sof_heap_ops predef_heap_ops = {
	.alloc = predef_alloc,
	.free = predef_free,
	.get_size = predef_get_size,
};

static int predef_init(struct sof_predef_heap *heap, void *const start, const size_t size,
			const struct predef_heap_config *const config)
{
	int count = 0;

	/* Count configured bundles */
	while (config[count].count)
		count++;

	heap->desc.start = POINTER_TO_UINT(start);
	heap->desc.end = POINTER_TO_UINT(start) + size - 1;
	heap->desc.ops = &predef_heap_ops;

	heap->heap.config = &heap->config_store;
	heap->heap.config_size= sizeof(heap->config_store);
	return predef_heap_init(&heap->heap, config, count, UINT_TO_POINTER(start), size);
}
#endif	/* PREDEF_HEAP_SUPPORTED */

#if IS_ENABLED(CONFIG_USE_PREDEFINED_HEAP)
extern const struct predef_heap_config hp_buffers_config[];

static struct sof_predef_heap sof_heap;
#else
static struct sof_default_heap sof_heap;
#endif


#if CONFIG_L3_HEAP
static struct sof_default_heap l3_heap;

/**
 * Returns the start of L3 memory heap.
 * @return Pointer to the L3 memory location which can be used for L3 heap.
 */
static inline uintptr_t get_l3_heap_start(void)
{
	/*
	 * TODO: parse the actual offset using:
	 * - HfIMRIA1 register
	 * - rom_ext_load_offset
	 * - main_fw_load_offset
	 * - main fw size in manifest
	 */
	return (uintptr_t)z_soc_uncached_ptr((__sparse_force void __sparse_cache *)
					     ROUND_UP(IMR_L3_HEAP_BASE, L3_MEM_PAGE_SIZE));
}

/**
 * Returns the size of L3 memory heap.
 * @return Size of the L3 memory region which can be used for L3 heap.
 */
static inline size_t get_l3_heap_size(void)
{
	 /*
	  * Calculate the IMR heap size using:
	  * - total IMR size
	  * - IMR base address
	  * - actual IMR heap start
	  */
	return ROUND_DOWN(IMR_L3_HEAP_SIZE, L3_MEM_PAGE_SIZE);
}
#endif


static struct sof_mem_desc *const sof_heaps[] = {
	&sof_heap.desc, 
#if CONFIG_L3_HEAP
	&l3_heap.desc,
#endif
};

static void *heap_alloc_aligned(struct sof_mem_desc *desc, size_t min_align, size_t bytes)
{
	return desc->ops->alloc(desc, bytes, min_align);
}

static void __sparse_cache *heap_alloc_aligned_cached(struct sof_mem_desc *desc,
						      size_t min_align, size_t bytes)
{
	void __sparse_cache *ptr;

	/*
	 * Zephyr sys_heap stores metadata at start of each
	 * heap allocation. To ensure no allocated cached buffer
	 * overlaps the same cacheline with the metadata chunk,
	 * align both allocation start and size of allocation
	 * to cacheline. As cached and non-cached allocations are
	 * mixed, same rules need to be followed for both type of
	 * allocations.
	 */
#ifdef CONFIG_SOF_ZEPHYR_HEAP_CACHED
	min_align = MAX(PLATFORM_DCACHE_ALIGN, min_align);
	bytes = ALIGN_UP(bytes, min_align);
#endif

	ptr = (__sparse_force void __sparse_cache *)heap_alloc_aligned(desc, min_align, bytes);

#ifdef CONFIG_SOF_ZEPHYR_HEAP_CACHED
	if (ptr)
		ptr = z_soc_cached_ptr((__sparse_force void *)ptr);
#endif

	return ptr;
}

static void heap_free(void *mem)
{
	struct sof_mem_desc *const *const desc_end = sof_heaps + ARRAY_SIZE(sof_heaps);
	struct sof_mem_desc *const *desc;

#ifdef CONFIG_SOF_ZEPHYR_HEAP_CACHED
	void *mem_cached = mem;

	if (is_cached(mem)) {
		mem = z_soc_uncached_ptr((__sparse_force void __sparse_cache *)mem_cached);
	}
#endif

	/* Find the heap where the buffer come from. */
	for (desc = sof_heaps; desc < desc_end; desc++) {
		if ((POINTER_TO_UINT(mem) >= (*desc)->start) &&
		    (POINTER_TO_UINT(mem) <= (*desc)->end))
			break;
	}

	if (desc == desc_end)
		return;

#ifdef CONFIG_SOF_ZEPHYR_HEAP_CACHED
	if (mem != mem_cached) {
		z_xtensa_cache_flush_inv(mem, (*desc)->ops->get_size(*desc, mem));
	}
#endif
	(*desc)->ops->free(*desc, mem);
}

static inline bool zone_is_cached(enum mem_zone zone)
{
#ifdef CONFIG_SOF_ZEPHYR_HEAP_CACHED
	switch (zone) {
	case SOF_MEM_ZONE_SYS:
	case SOF_MEM_ZONE_SYS_RUNTIME:
	case SOF_MEM_ZONE_RUNTIME:
	case SOF_MEM_ZONE_BUFFER:
		return true;
	default:
		break;
	}
#endif

	return false;
}

void *rmalloc(enum mem_zone zone, uint32_t flags, uint32_t caps, size_t bytes)
{
	void *ptr;
	struct sof_mem_desc *heap;

	/* choose a heap */
	if (caps & SOF_MEM_CAPS_L3) {
#if CONFIG_L3_HEAP
		heap = &l3_heap.desc;
#else
		k_panic();
#endif
	} else {
		heap = &sof_heap.desc;
	}

	if (zone_is_cached(zone) && !(flags & SOF_MEM_FLAG_COHERENT)) {
		ptr = (__sparse_force void *)heap_alloc_aligned_cached(heap, 0, bytes);
	} else {
		/*
		 * XTOS alloc implementation has used dcache alignment,
		 * so SOF application code is expecting this behaviour.
		 */
		ptr = heap_alloc_aligned(heap, PLATFORM_DCACHE_ALIGN, bytes);
	}

	if (!ptr && zone == SOF_MEM_ZONE_SYS)
		k_panic();

	return ptr;
}

/* Use SOF_MEM_ZONE_BUFFER at the moment */
void *rbrealloc_align(void *ptr, uint32_t flags, uint32_t caps, size_t bytes,
		      size_t old_bytes, uint32_t alignment)
{
	void *new_ptr;

	if (!ptr) {
		/* TODO: Use correct zone */
		return rballoc_align(flags, caps, bytes, alignment);
	}

	/* Original version returns NULL without freeing this memory */
	if (!bytes) {
		/* TODO: Should we call rfree(ptr); */
		tr_err(&zephyr_tr, "realloc failed for 0 bytes");
		return NULL;
	}

	new_ptr = rballoc_align(flags, caps, bytes, alignment);
	if (!new_ptr)
		return NULL;

	if (!(flags & SOF_MEM_FLAG_NO_COPY))
		memcpy_s(new_ptr, bytes, ptr, MIN(bytes, old_bytes));

	rfree(ptr);

	tr_info(&zephyr_tr, "rbealloc: new ptr %p", new_ptr);

	return new_ptr;
}

/**
 * Similar to rmalloc(), guarantees that returned block is zeroed.
 *
 * @note Do not use  for buffers (SOF_MEM_ZONE_BUFFER zone).
 *       rballoc(), rballoc_align() to allocate memory for buffers.
 */
void *rzalloc(enum mem_zone zone, uint32_t flags, uint32_t caps, size_t bytes)
{
	void *ptr = rmalloc(zone, flags, caps, bytes);

	if (ptr)
		memset(ptr, 0, bytes);

	return ptr;
}

/**
 * Allocates memory block from SOF_MEM_ZONE_BUFFER.
 * @param flags see SOF_MEM_FLAG_...
 * @param caps Capabilities, see SOF_MEM_CAPS_...
 * @param bytes Size in bytes.
 * @param align Alignment in bytes.
 * @return Pointer to the allocated memory or NULL if failed.
 */
void *rballoc_align(uint32_t flags, uint32_t caps, size_t bytes,
		    uint32_t align)
{
	if (flags & SOF_MEM_FLAG_COHERENT)
		return heap_alloc_aligned(&sof_heap.desc, align, bytes);

	return (__sparse_force void *)heap_alloc_aligned_cached(&sof_heap.desc, align, bytes);
}

/*
 * Free's memory allocated by above alloc calls.
 */
void rfree(void *ptr)
{
	if (!ptr)
		return;

	heap_free(ptr);
}

static int heap_init(const struct device *unused)
{
	ARG_UNUSED(unused);
#if IS_ENABLED(CONFIG_USE_PREDEFINED_HEAP)
	int ret;

	ret = predef_init(&sof_heap, heapmem, HEAPMEM_SIZE, hp_buffers_config);
	assert(ret == 0);
#else
	default_heap_init(&sof_heap, heapmem, HEAPMEM_SIZE);
#endif

#if CONFIG_L3_HEAP
	default_heap_init(&l3_heap, UINT_TO_POINTER(get_l3_heap_start()), get_l3_heap_size());
#endif

	return 0;
}

SYS_INIT(heap_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);
