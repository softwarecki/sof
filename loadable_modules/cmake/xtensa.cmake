
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)

cmake_path(CONVERT $ENV{XTENSA_TOOLCHAIN_PATH} TO_CMAKE_PATH_LIST XTENSA_TOOLCHAIN_PATH)
#message(FATAL_ERROR $ENV{XTENSA_TOOLCHAIN_PATH})
set(TOOLCHAIN_PATH ${XTENSA_TOOLCHAIN_PATH}/XtensaTools)
set(CROSS_COMPILE ${TOOLCHAIN_PATH}/bin/xt-)

# these could be replaced with clang and clang++
find_program(CMAKE_C_COMPILER NAMES ${CROSS_COMPILE}xcc NO_DEFAULT_PATH)
find_program(CMAKE_CXX_COMPILER NAMES ${CROSS_COMPILE}xc++ NO_DEFAULT_PATH)

find_program(CMAKE_LD NAMES ${CROSS_COMPILE}ld NO_DEFAULT_PATH)
find_program(CMAKE_AR NAMES ${CROSS_COMPILE}ar NO_DEFAULT_PATH)
find_program(CMAKE_RANLIB NAMES ${CROSS_COMPILE}ranlib NO_DEFAULT_PATH)
find_program(CMAKE_OBJCOPY NAMES ${CROSS_COMPILE}objcopy NO_DEFAULT_PATH)
find_program(CMAKE_OBJDUMP NAMES ${CROSS_COMPILE}objdump NO_DEFAULT_PATH)

set(CMAKE_FIND_ROOT_PATH ${TOOLCHAIN_PATH}/xtensa-elf)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

#message(FATAL_ERROR WYPIERDALAJ)