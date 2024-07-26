
target_sources(example PRIVATE example.c)

set_target_properties(example PROPERTIES
	MAX_INSTANCES	"3"
	HPSRAM_ADDR	"0xa06a0000"
)
