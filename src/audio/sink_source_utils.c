// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright(c) 2023 Intel Corporation. All rights reserved.
//

#include <sof/audio/sink_source_utils.h>
#include <sof/audio/sink_api.h>
#include <sof/audio/source_api.h>
#include <sof/common.h>
#include <rtos/panic.h>
#include <rtos/string.h>
#include <rtos/symbol.h>
#include <sof/math/numbers.h>
#include <limits.h>

static inline uint8_t *audio_fragment_end_addr(void *buffer_start, size_t buffer_size)
{
	return (uint8_t *)buffer_start + buffer_size;
}

static uint32_t audio_fragment_frames_without_wrap_common(const void *ptr,
						   const void *buffer_start,
						   size_t buffer_size,
						   size_t frame_bytes)
{
	if (!frame_bytes)
		return 0;

	return audio_fragment_bytes_without_wrap(ptr, buffer_start, buffer_size) / frame_bytes;
}

static int audio_fragment_samples_without_wrap_common(const void *ptr,
						      const void *buffer_start,
						      size_t buffer_size,
						      size_t sample_bytes)
{
	if (!sample_bytes)
		return 0;

	return audio_fragment_bytes_without_wrap(ptr, buffer_start, buffer_size) / sample_bytes;
}

int audio_fragment_bytes_without_wrap(const void *ptr, const void *buffer_start, size_t buffer_size)
{
	void *buffer_end = audio_fragment_end_addr((void *)buffer_start, buffer_size);

	if (ptr > buffer_end)
		return 0;

	return (intptr_t)buffer_end - (intptr_t)ptr;
}
EXPORT_SYMBOL(audio_fragment_bytes_without_wrap);

int audio_fragment_rewind_bytes_without_wrap(const void *ptr, const void *buffer_start)
{
	if (ptr < buffer_start)
		return 0;

	return (intptr_t)ptr - (intptr_t)buffer_start;
}
EXPORT_SYMBOL(audio_fragment_rewind_bytes_without_wrap);

const void *audio_fragment_wrap(const void *ptr, const void *buffer_start, size_t buffer_size)
{
	void *buffer_end = audio_fragment_end_addr((void *)buffer_start, buffer_size);

	if (ptr >= buffer_end)
		ptr = (uint8_t *)buffer_start + ((const uint8_t *)ptr - (uint8_t *)buffer_end);

	return ptr;
}
EXPORT_SYMBOL(audio_fragment_wrap);

void *audio_fragment_wrap_w(void *ptr, void *buffer_start, size_t buffer_size)
{
	return (void *)audio_fragment_wrap(ptr, buffer_start, buffer_size);
}
EXPORT_SYMBOL(audio_fragment_wrap_w);

const void *audio_fragment_rewind_wrap(const void *ptr, const void *buffer_start,
				       size_t buffer_size)
{
	void *buffer_end = audio_fragment_end_addr((void *)buffer_start, buffer_size);

	if (ptr < buffer_start)
		ptr = (uint8_t *)buffer_end - ((uint8_t *)buffer_start - (const uint8_t *)ptr);

	return ptr;
}
EXPORT_SYMBOL(audio_fragment_rewind_wrap);

void *audio_fragment_rewind_wrap_w(void *ptr, void *buffer_start, size_t buffer_size)
{
	return (void *)audio_fragment_rewind_wrap(ptr, buffer_start, buffer_size);
}
EXPORT_SYMBOL(audio_fragment_rewind_wrap_w);

uint32_t source_fragment_frames_without_wrap(struct sof_source *source,
					     const struct source_fragment *fragment,
					     const void *ptr)
{
	return audio_fragment_frames_without_wrap_common(ptr, fragment->buffer_start,
							 fragment->buffer_size,
							 source_get_frame_bytes(source));
}
EXPORT_SYMBOL(source_fragment_frames_without_wrap);

uint32_t sink_fragment_frames_without_wrap(struct sof_sink *sink,
					   const struct sink_fragment *fragment,
					   const void *ptr)
{
	return audio_fragment_frames_without_wrap_common(ptr, fragment->buffer_start,
							 fragment->buffer_size,
							 sink_get_frame_bytes(sink));
}
EXPORT_SYMBOL(sink_fragment_frames_without_wrap);

int source_fragment_samples_without_wrap_s16(const struct source_fragment *fragment,
						    const void *ptr)
{
	return audio_fragment_samples_without_wrap_common(ptr, fragment->buffer_start,
							  fragment->buffer_size,
							  sizeof(int16_t));
}
EXPORT_SYMBOL(source_fragment_samples_without_wrap_s16);

int source_fragment_samples_without_wrap_s24(const struct source_fragment *fragment,
						    const void *ptr)
{
	return audio_fragment_samples_without_wrap_common(ptr, fragment->buffer_start,
							  fragment->buffer_size,
							  sizeof(int32_t));
}
EXPORT_SYMBOL(source_fragment_samples_without_wrap_s24);

int source_fragment_samples_without_wrap_s32(const struct source_fragment *fragment,
						    const void *ptr)
{
	return audio_fragment_samples_without_wrap_common(ptr, fragment->buffer_start,
							  fragment->buffer_size,
							  sizeof(int32_t));
}
EXPORT_SYMBOL(source_fragment_samples_without_wrap_s32);

int sink_fragment_samples_without_wrap_s16(const struct sink_fragment *fragment,
						  const void *ptr)
{
	return audio_fragment_samples_without_wrap_common(ptr, fragment->buffer_start,
							  fragment->buffer_size,
							  sizeof(int16_t));
}
EXPORT_SYMBOL(sink_fragment_samples_without_wrap_s16);

int sink_fragment_samples_without_wrap_s24(const struct sink_fragment *fragment,
						  const void *ptr)
{
	return audio_fragment_samples_without_wrap_common(ptr, fragment->buffer_start,
							  fragment->buffer_size,
							  sizeof(int32_t));
}
EXPORT_SYMBOL(sink_fragment_samples_without_wrap_s24);

int sink_fragment_samples_without_wrap_s32(const struct sink_fragment *fragment,
						  const void *ptr)
{
	return audio_fragment_samples_without_wrap_common(ptr, fragment->buffer_start,
							  fragment->buffer_size,
							  sizeof(int32_t));
}
EXPORT_SYMBOL(sink_fragment_samples_without_wrap_s32);

int source_to_sink_copy(struct sof_source *source,
			struct sof_sink *sink, bool free, size_t size)
{
	struct source_fragment source_fragment;
	struct sink_fragment sink_fragment;
	uint8_t const *src_ptr;
	uint8_t *dst_ptr;
	int ret;

	if (!size)
		return 0;
	if (size > source_get_data_available(source))
		return -EFBIG;
	if (size > sink_get_free_size(sink))
		return -ENOSPC;

	ret = source_get_data_fragment(source, size, &source_fragment);
	if (ret)
		return ret;

	src_ptr = source_fragment.data_ptr;

	ret = sink_get_buffer_fragment(sink, size, &sink_fragment);
	if (ret) {
		source_release_data(source, 0);
		return ret;
	}

	dst_ptr = sink_fragment.data_ptr;
	while (size) {
		uint32_t src_to_buf_overlap = source_fragment_bytes_without_wrap(&source_fragment,
									 src_ptr);
		uint32_t dst_to_buf_overlap = sink_fragment_bytes_without_wrap(&sink_fragment,
								   dst_ptr);
		uint32_t to_copy = MIN(src_to_buf_overlap, dst_to_buf_overlap);

		to_copy = MIN(to_copy, size);
		ret = memcpy_s(dst_ptr, dst_to_buf_overlap, src_ptr, to_copy);
		if (ret) {
			source_release_data(source, 0);
			sink_commit_buffer(sink, 0);
			return -EINVAL;
		}

		size -= to_copy;
		src_ptr += to_copy;
		dst_ptr += to_copy;
		src_ptr = source_fragment_wrap(&source_fragment, src_ptr);
		dst_ptr = sink_fragment_wrap(&sink_fragment, dst_ptr);
	}

	source_release_data(source, free ? INT_MAX : 0);
	sink_commit_buffer(sink, INT_MAX);
	return 0;
}
EXPORT_SYMBOL(source_to_sink_copy);

int sink_fill_with_silence(struct sof_sink *sink, size_t size)
{
	struct sink_fragment sink_fragment;
	uint8_t *dst_ptr;
	int ret;

	if (!size)
		return 0;
	if (size > sink_get_free_size(sink))
		return -ENOSPC;

	ret = sink_get_buffer_fragment(sink, size, &sink_fragment);
	if (ret)
		return ret;

	dst_ptr = sink_fragment.data_ptr;
	while (size) {
		uint32_t dst_to_buf_overlap = sink_fragment_bytes_without_wrap(&sink_fragment,
								   dst_ptr);
		uint32_t to_fill = MIN(dst_to_buf_overlap, size);

		ret = memset_s(dst_ptr, dst_to_buf_overlap, 0, to_fill);
		if (ret) {
			sink_commit_buffer(sink, 0);
			return -EINVAL;
		}

		size -= to_fill;
		dst_ptr += to_fill;
		dst_ptr = sink_fragment_wrap(&sink_fragment, dst_ptr);
	}

	sink_commit_buffer(sink, INT_MAX);
	return 0;
}
EXPORT_SYMBOL(sink_fill_with_silence);

int source_drop_data(struct sof_source *source, size_t size)
{
	struct source_fragment source_fragment;
	int ret;

	if (!size)
		return 0;
	if (size > source_get_data_available(source))
		return -EFBIG;

	ret = source_get_data_fragment(source, size, &source_fragment);
	if (ret)
		return ret;

	source_release_data(source, INT_MAX);
	return 0;
}
EXPORT_SYMBOL(source_drop_data);
