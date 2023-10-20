# This file is intended to be included from project's CMakeLists.txt.
# Prior to include, MODULES_LIST variable should be initialised with list
# of modules (subdirectories in modules dir) that should be built into
# project's loadable library.

if(NOT DEFINED MODULES_LIST)
	message(FATAL_ERROR "Please define MODULES_LIST: list of modules to be built into loadable library")
endif()

include(${CMAKE_CURRENT_LIST_DIR}/config.cmake)

foreach(MODULE ${MODULES_LIST})
	add_executable(${MODULE})
	add_subdirectory(${LMDK_BASE}/modules/${MODULE} ${MODULE}_module)

###	set_target_properties(${MODULE} PROPERTIES OUTPUT_NAME ${MODULE}.mod)
	target_compile_options(${MODULE} PUBLIC  "-g3")
	target_include_directories(${MODULE} PRIVATE
		"${LMDK_BASE}/include"
		"${RIMAGE_INCLUDE_DIR}"
		"${SOF_BASE}/src/include/sof_api"
	)

	# generate linker script
	get_target_property(HPSRAM_ADDR ${MODULE} HPSRAM_ADDR)

	if(NOT DEFINED HPSRAM_ADDR)
		message(FATAL_ERROR "Please define HPSRAM_ADDR for module ${MODULE}")
	endif()

	add_custom_command(TARGET ${MODULE} PRE_LINK
		COMMAND ${CMAKE_COMMAND}
		-DMODULE=${MODULE}
		-DHPSRAM_ADDR=${HPSRAM_ADDR}
		-P ${CMAKE_CURRENT_LIST_DIR}/ldscripts.cmake
	)

	target_link_options(${MODULE} PRIVATE
		#"-nostdlib" 
		#"-nodefaultlibs"
		#"-Wl,-lgcc"
#"-Wl,--entry=0"
"-Wl,--build-id=sha1"
#"-Wl,--print-gc-sections"
		"-Wl,--no-undefined" "-Wl,--unresolved-symbols=report-all" "-Wl,--error-unresolved-symbols"
		"-Wl,--gc-sections"	# may remove .bss and that will result in rimage error, do not use for now
		"-Wl,-Map,$<TARGET_FILE:${MODULE}>.map"	# optional: just for debug
		"-T" "${MODULE}_ldscripts/elf32xtensa.x"
		"-g"
	)

#LDFLAGS := -Wl,--no-undefined
#LDFLAGS += -Wl,--unresolved-symbols=report-all
#LDFLAGS += -Wl,--error-unresolved-symbols
#LDFLAGS += -Wl,--gc-sections
#LDFLAGS += -Wl,-z -Wl,relro
#LDFLAGS += -Wl,-z -Wl,combreloc
#LDFLAGS += -Wl,-z -Wl,now
#LDFLAGS += -Wl,-q
#LDFLAGS += -Wl,--entry=0
#LDFLAGS += -Wl,--build-id=sha1
#ifeq (1,$(VERBOSE))
#  LDFLAGS += -Wl,--print-gc-sections
#  LDFLAGS += -Wl,--verbose
#endif
#LDFLAGS += $(foreach libdir,$(sort $(dir $(STATIC_LIBRARIES))),-L$(libdir))
#LDLIBS := $(foreach libname,$(patsubst lib%.a,%,$(notdir $(STATIC_LIBRARIES))),-l$(libname))
endforeach()

set(RIMAGE_OUTPUT_FILE ${PROJECT_NAME}_noextmft)
set(OUTPUT_FILE ${PROJECT_NAME}.bin)

if(RIMAGE_INSTALL_DIR)
  cmake_path(ABSOLUTE_PATH RIMAGE_INSTALL_DIR BASE_DIRECTORY ${CMAKE_SOURCE_DIR} NORMALIZE)
endif()

# Create a hint - rimage may be installed to directory where SOF project installs it
cmake_path(APPEND SOF_BASE "../build-rimage" OUTPUT_VARIABLE RIMAGE_SOF_INSTALL_DIR)
cmake_path(NORMAL_PATH RIMAGE_SOF_INSTALL_DIR)
cmake_path(ABSOLUTE_PATH SIGNING_KEY BASE_DIRECTORY ${CMAKE_SOURCE_DIR} NORMALIZE)

find_program(RIMAGE_COMMAND NAMES rimage
  PATHS "${RIMAGE_INSTALL_DIR}"
  HINTS "${RIMAGE_SOF_INSTALL_DIR}"
  REQUIRED)

add_custom_target(${PROJECT_NAME}_target ALL
	DEPENDS ${MODULES_LIST}
	COMMAND ${RIMAGE_COMMAND} -k ${SIGNING_KEY} -f 2.0.0 -b 1 -o ${RIMAGE_OUTPUT_FILE} -c ${TOML} -e ${MODULES_LIST}
	COMMAND ${CMAKE_COMMAND} -E cat ${RIMAGE_OUTPUT_FILE}.xman ${RIMAGE_OUTPUT_FILE} > ${OUTPUT_FILE}
)
