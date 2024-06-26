#include <stdint.h>
#include <stdbool.h>
#include <sof/lib/glitch.h>
#include <sof/lib/dai.h>
#include <sof/lib/dai-zephyr.h>
#include <sof/audio/component.h>
#include <zephyr/sys/util.h>
#include "../audio/copier/copier.h"
#include <ipc4/module.h>

bool ll_print;

LOG_MODULE_REGISTER(glitch, CONFIG_SOF_LOG_LEVEL);

struct debug_config {
	struct debug_dir_config playback;
	struct debug_dir_config capture;
};

const struct debug_config glitch_config = {
	.playback = {
		/* Przeszukanie zawartosci bufora w poszukiwaniu glitchy */
		.check_input = false,
		.check_output = false,
		/* Sprawdzenie styku pomiedzy kolejnymi chunkami czy nie ma glitcha */
		.cross_check_input = false,
		.mark_output = false,
		/* Znakowanie poczatku zawartosci bufora */
		.cross_check_output = false,

		/* Wyswietlanie wskaznikow pozycji bufora */
		.source_print_pos_in = false,
		.source_print_pos_out = false,
		.sink_print_pos_in = false,
		.sink_print_pos_out = false,
		.print_pos_diff = false,
		.component_print_buffers_pos_diff = false,
	},
	.capture= {
		/* Przeszukanie zawartosci bufora w poszukiwaniu glitchy */
		.check_input = false,
		.check_output = false,
		/* Sprawdzenie styku pomiedzy kolejnymi chunkami czy nie ma glitcha */
		.cross_check_input = false,
		.mark_output = false,
		/* Znakowanie poczatku zawartosci bufora */
		.cross_check_output = false,
		/* Wyswietlanie wskaznikow pozycji bufora */
		.source_print_pos_in = false,
		.source_print_pos_out = false,
		.sink_print_pos_in = false,
		.sink_print_pos_out = false,
		.print_pos_diff = false,
	},
};

static void debug_init(const struct comp_dev *dev, struct debug_data *debug)
{
	if (!debug->inited) {
		debug->inited = true;
		debug->under_debug = copier_under_debug(dev);
	}

	const struct debug_dir_config *config = (dev->direction == SOF_IPC_STREAM_PLAYBACK) ?
		&glitch_config.playback : &glitch_config.capture;
	debug->last_config = config;
}

bool copier_under_debug(const struct comp_dev *dev)
{
	return (
		(dev->ipc_config.id == 0x20004) || //capture input copier
		//(dev->ipc_config.id == 0x10004) || //playback output copier
		//(dev->ipc_config.id == 0x00004) || //playback input copier
	0);
}

void copier_init_callback(const struct comp_dev *dev, struct copier_data *cd)
{
	/*
	cd->debug.under_debug = copier_under_debug(dev);
	cd->debug.history_in.dev = dev;
	cd->debug.history_in.init = true;
	cd->debug.history_out.dev = dev;
	cd->debug.history_out.init = true;
	*/
}

void copier_dai_init_callback(const struct comp_dev *dev, struct copier_data *cd, struct dai_data *dd)
{
	//dd->debug = &cd->debug;
}

#define ALLOWED_DIFF	217000000

// Zeruje pierwsza probke w buforze
static void mark_buffer(struct comp_buffer *buf, uint16_t fill_symbol)
{
	uint32_t bytes = 1 * audio_stream_get_channels(&buf->stream) *
		audio_stream_sample_bytes(&buf->stream);
	bytes = MIN(buf->debug.bytes, bytes);

	uint32_t head_size = bytes;
	uint32_t tail_size = 0;

	/* check for potential wrap */
	if ((char *)buf->debug.ptr + bytes > (char *)buf->stream.end_addr) {
		head_size = (char *)buf->stream.end_addr - (char *)buf->debug.ptr;
		tail_size = bytes - head_size;
	}

	memset(buf->debug.ptr, fill_symbol >> 8, head_size);
	dcache_writeback_region(buf->debug.ptr, head_size);
	if (tail_size) {
		memset(buf->stream.addr, fill_symbol & 0xFF, tail_size);
		dcache_writeback_region(buf->stream.addr, tail_size);
	}
}

// Zaznacza ostatnie probki w buforze
static void mark_end(struct comp_buffer *buf)
{

	uint32_t bytes = 2 * buf->stream.runtime_stream_params.channels *
		audio_stream_sample_bytes(&buf->stream);
	char *ptr = (char *)buf->stream.w_ptr - bytes;

	uint32_t head_size = bytes;
	uint32_t tail_size = 0;
	/* check for potential wrap */
	if ((char *)buf->stream.addr > ptr) {
		head_size = (char *)(char *)buf->stream.addr - ptr;
		ptr = (char *)buf->stream.end_addr - head_size;
		tail_size = bytes - head_size;
	}
	memset(ptr, 0x55, head_size);
	dcache_writeback_region(ptr, head_size);
	if (tail_size) {
		memset(buf->stream.addr, 0x77, tail_size);
		dcache_writeback_region(buf->stream.addr, tail_size);
	}
}

// Przeglada bufor sprawdzajac czy nie wystepuje w nim glitch. Nie sprawdza pomiedzy chunkami!
static bool find_glitch(const struct comp_dev *dev, struct comp_buffer *buf)
{
	const uint32_t channels = audio_stream_get_channels(&buf->stream);

	int32_t last[channels];

	int32_t *src = (int32_t *)buf->debug.ptr;
	uint32_t samples = buf->debug.bytes / audio_stream_sample_bytes(&buf->stream);

	memcpy(last, src, sizeof(last));
	samples -= channels;
	src += channels;

	for (uint32_t i = 0; i < samples; i++, src++) {
		if (POINTER_TO_UINT(src) >= POINTER_TO_UINT(buf->stream.end_addr))
			src = (int32_t *)buf->stream.addr;

		int32_t dif;
		int32_t l = last[i % channels];

		dif = (l > *src) ? l - *src : *src - l;

		if (dif > ALLOWED_DIFF) {
			comp_err(dev, "Sample diff %d @ %u - %p", dif, i, (void*)buf);
			return true;
		}

		last[i % channels] = *src;
	}

	return false;
}

// Sprawdza zlaczenie bufora
static bool check_buffer_connect(const struct comp_dev *dev, struct sample_history *history,
			  const struct comp_buffer *buf, const char* dir)
{
	const uint32_t channels = audio_stream_get_channels(&buf->stream);
	if (channels > DEBUG_MAX_CHANNELS) {
		comp_err(dev, "Samples history too few channels! %d", channels);
		return false;
	}


	int32_t *src = (int32_t *)buf->debug.ptr;
	bool glitch = false;

	for (uint32_t i = 0; i < channels; i++, src++) {
		int32_t last = history->val[i];
		int32_t dif;

		dif = (last > *src) ? last - *src : *src - last;
		history->val[i] = *src;

		if (dif > ALLOWED_DIFF) {
			comp_err(dev, "Cross period %s glitch. diff %d, ch %u - %p", dir, dif, i, (void*)buf);
			glitch = true;
			break;
		}
	}

	src = (int32_t *)(POINTER_TO_UINT(buf->debug.ptr) + buf->debug.bytes -
			  channels * audio_stream_sample_bytes(&buf->stream));
	if (POINTER_TO_UINT(src) >= POINTER_TO_UINT(buf->stream.end_addr))
		src = UINT_TO_POINTER(POINTER_TO_UINT(buf->stream.addr) +
				      POINTER_TO_UINT(src) - POINTER_TO_UINT(buf->stream.end_addr));

	for (uint32_t i = 0; i < channels; i++, src++)
		history->val[i] = *src;

	return glitch;
}


static void check_buffer_config(struct comp_dev *dev, struct comp_buffer *buf)
{
	if (!buf->hw_params_configured)
		comp_err(dev, "buffer not configured! %p", (void*)buf);
}


static void debug_buffer_save_pointers(struct comp_buffer *buf, bool source, uint32_t bytes)
{
	buf->debug.ptr = source ? buf->stream.r_ptr : buf->stream.w_ptr;
	buf->debug.bytes = bytes;
	buf->debug.last_r = buf->stream.r_ptr;
	buf->debug.last_w = buf->stream.w_ptr;
}

static void debug_buffer_print_pos(struct comp_dev *dev, struct comp_buffer *buf)
{
	comp_err(dev, "buf %p	rd %lu	wr %lu	size %u	av %u	fr %u", (void*)buf->stream.addr,
		 POINTER_TO_UINT(buf->stream.r_ptr) - POINTER_TO_UINT(buf->stream.addr),
		 POINTER_TO_UINT(buf->stream.w_ptr) - POINTER_TO_UINT(buf->stream.addr),
		 buf->stream.size, buf->stream.avail, buf->stream.free);
}

static uintptr_t calc_pos_diff(struct comp_buffer *buf, void *prev_pos, void *cur_pos)
{
	uintptr_t cur = POINTER_TO_UINT(cur_pos);
	uintptr_t prev = POINTER_TO_UINT(prev_pos);

	if (cur >= prev)
		return cur - prev;
	else
		return (cur - POINTER_TO_UINT(buf->stream.addr)) +
			(POINTER_TO_UINT(buf->stream.end_addr) - prev);
}

static void debug_buffer_print_pos_diff(struct comp_dev *dev, struct comp_buffer *buffer, bool source)
{
	uint32_t id = buf_get_id(buffer);

	id = source ? IPC4_SRC_QUEUE_ID(id) : IPC4_SINK_QUEUE_ID(id);

	uint32_t diff_r = calc_pos_diff(buffer, buffer->debug.last_r, buffer->stream.r_ptr);
	uint32_t diff_w = calc_pos_diff(buffer, buffer->debug.last_w, buffer->stream.w_ptr);
	comp_err(dev, "%p %s.%u: read %u, write %u, avail %u, free %u", (void *)buffer,
		 source ? "source" : "sink", id,
		 diff_r, diff_w, buffer->stream.avail, buffer->stream.free);


	
}

static void debug_buffers_print_pos_diff(struct comp_dev *dev, struct comp_buffer *source, struct comp_buffer *sink)
{
	uint32_t source_r, source_w, sink_r, sink_w;

	source_r = calc_pos_diff(source, source->debug.last_r, source->stream.r_ptr);
	source_w = calc_pos_diff(source, source->debug.last_w, source->stream.w_ptr);
	sink_r = calc_pos_diff(sink, sink->debug.last_r, sink->stream.r_ptr);
	sink_w = calc_pos_diff(sink, sink->debug.last_w, sink->stream.w_ptr);
	comp_err(dev, "source %p: read %u, write %u	sink %p: read %u, write %u",
		 (void *)source, source_r, source_w,
		 (void *)sink, sink_r, sink_w);
}


int ll_counter = 0;

void glitcher_ll_begin()
{
	ll_counter++;
	if (ll_counter >= 1000)
		ll_counter = 0;

	ll_print = ll_counter > 990;
}

void glitcher_ll_end()
{

}


void debug_copy_pre(struct comp_dev *dev, struct comp_buffer *source, struct comp_buffer *sink,
		    void* process, uint32_t sink_bytes)
{
	struct debug_data *debug = &dev->debug;
	debug_init(dev, debug);
	const struct debug_dir_config *config = debug->last_config;
	if (!debug->under_debug) return;

	debug->last_source = source;
	debug->last_sink = sink;

	debug_buffer_save_pointers(source, true, sink_bytes);
	buffer_stream_invalidate(source, sink_bytes);

	debug_buffer_save_pointers(sink, false, sink_bytes);

	// Sprawdza zlaczenie bufora wejsciowego
	if (config->cross_check_input) {
		if (check_buffer_connect(dev, &source->debug.history_in, source, "in"))
			mark_buffer(source, 0x5566);
	}

	// Przeprowadza analize zawartosci od wskaznika odczytu
	if (config->check_input) {
		if (find_glitch(dev, source))
			mark_buffer(source, 0xAABB);
	}

	if (config->source_print_pos_in)
		debug_buffer_print_pos(dev, source);

	if (config->sink_print_pos_in)
		debug_buffer_print_pos(dev, sink);

	//const bool playback = dev->direction == SOF_IPC_STREAM_PLAYBACK;

	/*
	if (dd->debug->print)
		comp_err(dev, "Sink ID: %d; convert %p", j, (void*)converter[j]);

	if (dd->debug->print)
		comp_err(dev, "sink ID: %d; convert %p", IPC4_SINK_QUEUE_ID(buf_get_id(dd->local_buffer)), (void*)dd->process);

	*/
}

void debug_copy_done(struct comp_dev *dev)
{
	struct debug_data *debug = &dev->debug;
	if (!debug->under_debug) return;

	const struct debug_dir_config *config = debug->last_config;
	struct comp_buffer *source = debug->last_source;
	struct comp_buffer *sink = debug->last_sink;

	// Sprawdza zlaczenie bufora wyjsciowego
	if (config->cross_check_output) {
		if (check_buffer_connect(dev, &sink->debug.history_out, sink, "out"))
			mark_buffer(sink, 0x5566);
	}

	// Przeprowadza analize zawartosci od wskaznika odczytu
	if (config->check_output) {
		if (find_glitch(dev, sink))
			mark_buffer(sink, 0xAABB);
	}

	if (config->mark_output) {
		mark_buffer(sink, 0);
	}

	if (config->source_print_pos_out)
		debug_buffer_print_pos(dev, source);

	if (config->sink_print_pos_out)
		debug_buffer_print_pos(dev, sink);

	if (config->print_pos_diff)
		debug_buffers_print_pos_diff(dev, source, sink);

	/*

#if PLAYBACK_OUT
	if (dd->debug->under_debug && find_glitch(&mark))
		mark_fill(&mark);
#endif
#if PLAYBACK_MARK_OUT
	mark_fill(&mark);
#endif
	//mark_end(dd->dma_buffer);

	//------------------------
#if CAPTURE_OUT
	if (dd->debug->under_debug && find_glitch(&mark))
		mark_fill(&mark);
#endif
	*/
}

void debug_comp_copy_pre(struct comp_dev *dev)
{
	struct comp_buffer *buffer;
	struct list_item *blist;
	struct debug_data *debug = &dev->debug;
	int ret, i = 0;

	debug_init(dev, debug);
	const struct debug_dir_config *config = debug->last_config;

	if (!debug->under_debug || !config->component_print_buffers_pos_diff)
		return;

	list_for_item(blist, &dev->bsource_list) {
		buffer = container_of(blist, struct comp_buffer, sink_list);
		check_buffer_config(dev, buffer);
		debug_buffer_save_pointers(buffer, true, 0);
	}

	list_for_item(blist, &dev->bsink_list) {
		buffer = container_of(blist, struct comp_buffer, source_list);
		check_buffer_config(dev, buffer);
		debug_buffer_save_pointers(buffer, false, 0);
	}

}

void debug_comp_copy_post(struct comp_dev *dev, int ret)
{
	struct debug_data *debug = &dev->debug;
	const struct debug_dir_config *config = debug->last_config;
	struct comp_buffer *buffer;
	struct list_item *blist;

	if (!debug->under_debug || !config->component_print_buffers_pos_diff)
		return;

	list_for_item(blist, &dev->bsource_list) {
		buffer = container_of(blist, struct comp_buffer, sink_list);
		debug_buffer_print_pos_diff(dev, buffer, true);
	}

	list_for_item(blist, &dev->bsink_list) {
		buffer = container_of(blist, struct comp_buffer, source_list);
		debug_buffer_print_pos_diff(dev, buffer, false);
	}
}