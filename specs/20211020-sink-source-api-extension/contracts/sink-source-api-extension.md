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

### 2. SOF-side acquisition wrappers

These wrappers preserve the existing reservation semantics while packaging the
acquired tuple for helper-layer users.

```c
int source_get_data_fragment(struct sof_source *source, size_t req_size,
			     struct source_fragment *fragment);

int sink_get_buffer_fragment(struct sof_sink *sink, size_t req_size,
			     struct sink_fragment *fragment);
```

### 3. Generic fragment-navigation helpers

These helpers operate on acquired fragments and do not require new transport semantics.

```c
int audio_fragment_bytes_without_wrap(const void *ptr,
				      const void *buffer_start,
				      size_t buffer_size);

int audio_fragment_rewind_bytes_without_wrap(const void *ptr,
					     const void *buffer_start);

const void *audio_fragment_wrap(const void *ptr,
				const void *buffer_start,
				size_t buffer_size);

void *audio_fragment_wrap_w(void *ptr,
			    void *buffer_start,
			    size_t buffer_size);

const void *audio_fragment_rewind_wrap(const void *ptr,
				       const void *buffer_start,
				       size_t buffer_size);

void *audio_fragment_rewind_wrap_w(void *ptr,
				   void *buffer_start,
				   size_t buffer_size);
```

### 4. Frame and sample distance helpers

These wrappers keep format-aware calculations close to sink/source handles rather than forcing modules to reconstruct them.

```c
uint32_t source_fragment_frames_without_wrap(struct sof_source *source,
					     const struct source_fragment *fragment,
					     const void *ptr);

uint32_t sink_fragment_frames_without_wrap(struct sof_sink *sink,
					   const struct sink_fragment *fragment,
					   const void *ptr);

int source_fragment_samples_without_wrap_s16(const struct source_fragment *fragment,
						     const void *ptr);
int source_fragment_samples_without_wrap_s24(const struct source_fragment *fragment,
						     const void *ptr);
int source_fragment_samples_without_wrap_s32(const struct source_fragment *fragment,
						     const void *ptr);

int sink_fragment_samples_without_wrap_s16(const struct sink_fragment *fragment,
						   const void *ptr);
int sink_fragment_samples_without_wrap_s24(const struct sink_fragment *fragment,
						   const void *ptr);
int sink_fragment_samples_without_wrap_s32(const struct sink_fragment *fragment,
						   const void *ptr);
```

### 5. Source-side latest feeding time

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
- `source_get_data_fragment()` and `sink_get_buffer_fragment()` must remain thin wrappers over the existing acquisition calls and must not introduce extra reservation state.
- The helper layer must not assume linear storage; it must work on circular fragments returned by current implementations.
- `source_get_last_feeding_time()` must return `UINT32_MAX` when a provider has not implemented `source_ops.get_lft()` yet.

## Explicitly Rejected Additions

These were considered during analysis and are intentionally not part of the first-phase contract:

- New buffer-format getters: rejected because the current API already provides them.
- Atomic multi-source or multi-sink acquisition APIs: rejected because reservation state is already tracked per source or sink.
- Non-reserving free-space or metadata getters for codecs: rejected because current size and metadata getters already cover the analyzed prepare paths.
- Direct `audio_stream` exposure to modules: rejected because it preserves the legacy coupling the roadmap is trying to remove.