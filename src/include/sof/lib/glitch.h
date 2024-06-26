#ifndef GLITCH_ANALISER
#define GLITCH_ANALISER

#include <stdint.h>
#include <stdbool.h>

struct comp_dev;
struct copier_data;
struct comp_buffer;
struct dai_data;

bool copier_under_debug(const struct comp_dev *dev);
void copier_init_callback(const struct comp_dev *dev, struct copier_data *cd);
void copier_dai_init_callback(const struct comp_dev *dev, struct copier_data *cd, struct dai_data *dd);
void glitcher_ll_begin();
void glitcher_ll_end();

struct debug_dir_config {
	bool check_input;		/* Przeszukanie zawartosci bufora w poszukiwaniu glitchy */
	bool check_output;
	bool cross_check_input;		/* Sprawdzenie styku pomiedzy kolejnymi chunkami czy nie ma glitcha */
	bool cross_check_output;
	bool mark_output;		/* Znakowanie poczatku zawartosci bufora */
	bool source_print_pos_in;	/* Wyswietlanie wskaznikow pozycji bufora */
	bool source_print_pos_out;
	bool sink_print_pos_in;
	bool sink_print_pos_out;
	bool print_pos_diff;		/* Wyswietl ile bajtow zjedzono i ile wyprodukowano */
	bool component_print_buffers_pos_diff; /* Wyswietl ile bajtow zjedzono i ile wyprodukowano po stronie sink i source dowolnego komponentu */
};

#define DEBUG_MAX_CHANNELS	2

struct sample_history {
	int32_t val[DEBUG_MAX_CHANNELS];
};

struct debug_buffer {
	void *ptr;
	uint32_t bytes;

	void *last_r;
	void *last_w;

	struct sample_history history_in;
	struct sample_history history_out;
};

struct debug_data {
	bool inited;
	bool under_debug;
	bool print;
	struct comp_buffer *last_source;
	struct comp_buffer *last_sink;
	const struct debug_dir_config *last_config;
};

void debug_comp_copy_pre(struct comp_dev *dev);
void debug_comp_copy_post(struct comp_dev *dev, int ret);

void debug_copy_pre(struct comp_dev *dev, struct comp_buffer *source, struct comp_buffer *sink,
		    void* process, uint32_t sink_bytes);

void debug_copy_done(struct comp_dev *dev);


#define FUN() if (copier_under_debug(dev)) { comp_err(dev, "FUN %s() !!!", __FUNCTION__); }

#endif /* GLITCH_ANALISER */
