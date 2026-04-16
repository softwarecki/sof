# Contract: Sink/Source API Extension

## Scope

This contract defines the minimum public and internal interface changes needed for the first sink/source API extension phase.

It intentionally excludes:

- direct bind
- buffer-factory policy
- copier internal-storage exposure
- any reintroduction of `struct audio_stream` or `struct comp_buffer` into module-facing code

## Existing API Reused As-Is

The following already cover legacy prepare-path metadata needs and must not be duplicated:

- `source_get_frm_fmt()` / `sink_get_frm_fmt()`
- `source_get_valid_fmt()` / `sink_get_valid_fmt()`
- `source_get_rate()` / `sink_get_rate()`
- `source_get_channels()` / `sink_get_channels()`
- `source_get_buffer_fmt()` / `sink_get_buffer_fmt()`
- `source_get_data_available()` / `sink_get_free_size()`
- `source_get_data_frames_available()` / `sink_get_free_frames()`

## Proposed Additions

### 1. Fragment helper descriptors

These are helper-layer carriers around existing acquisition results.

```c
struct source_fragment {
	const void *data_ptr;
	const void *buffer_start;
	size_t buffer_size;
};

struct sink_fragment {
	void *data_ptr;
	void *buffer_start;
	size_t buffer_size;
};
```

### 2. Generic fragment-navigation helpers

These helpers operate on acquired fragments and do not require new transport semantics.

```c
size_t audio_fragment_bytes_until_wrap(const void *ptr,
				      const void *buffer_start,
				      size_t buffer_size);

size_t audio_fragment_bytes_before_wrap_reverse(const void *ptr,
					       const void *buffer_start,
					       size_t buffer_size);

const void *audio_fragment_wrap(const void *ptr,
				const void *buffer_start,
				size_t buffer_size);

void *audio_fragment_wrap_w(void *ptr,
			    void *buffer_start,
			    size_t buffer_size);
```

### 3. Frame and sample distance helpers

These wrappers keep format-aware calculations close to sink/source handles rather than forcing modules to reconstruct them.

```c
size_t source_fragment_frames_until_wrap(struct sof_source *source,
					 const struct source_fragment *fragment);

size_t sink_fragment_frames_until_wrap(struct sof_sink *sink,
				       const struct sink_fragment *fragment);

size_t source_fragment_samples_until_wrap_s16(const struct source_fragment *fragment);
size_t source_fragment_samples_until_wrap_s24(const struct source_fragment *fragment);
size_t source_fragment_samples_until_wrap_s32(const struct source_fragment *fragment);

size_t sink_fragment_samples_until_wrap_s16(const struct sink_fragment *fragment);
size_t sink_fragment_samples_until_wrap_s24(const struct sink_fragment *fragment);
size_t sink_fragment_samples_until_wrap_s32(const struct sink_fragment *fragment);
```

### 4. Source-side latest feeding time

This is the only proposed core timing accessor addition in this phase.

```c
struct source_ops {
	...
	uint32_t (*get_lft)(struct sof_source *source);
};

uint32_t source_get_last_feeding_time(struct sof_source *source);
```

## Compatibility Rules

- The helper layer must be additive; existing sink/source users must continue to compile unchanged.
- Modules must still acquire and release source and sink fragments through `source_get_data()`, `source_release_data()`, `sink_get_buffer()`, and `sink_commit_buffer()`.
- The helper layer must not assume linear storage; it must work on circular fragments returned by current implementations.
- If a source implementation cannot provide source-side LFT immediately, the API must define an explicit fallback or error convention.

## Explicitly Rejected Additions

These were considered during analysis and are intentionally not part of the first-phase contract:

- New buffer-format getters: rejected because the current API already provides them.
- Atomic multi-source or multi-sink acquisition APIs: rejected because reservation state is already tracked per source or sink.
- Non-reserving free-space or metadata getters for codecs: rejected because current size and metadata getters already cover the analyzed prepare paths.
- Direct `audio_stream` exposure to modules: rejected because it preserves the legacy coupling the roadmap is trying to remove.