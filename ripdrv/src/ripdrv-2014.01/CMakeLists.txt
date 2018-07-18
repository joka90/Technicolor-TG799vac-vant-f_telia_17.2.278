cmake_minimum_required(VERSION 2.6)
PROJECT(rip C)
option(ENABLE_TESTS  "Enable the compilation of the unittests" OFF)

set(BUILDTYPE "userspace" CACHE STRING "Defines buildtype target (userspace/bootloader)")
set(RIP2_OFFSET "0x20000" CACHE STRING "start of RIP2 sector")

add_definitions(-std=gnu99 -Wall -Werror -D_GNU_SOURCE -g -DRIP2_OFFSET=${RIP2_OFFSET})
include_directories(${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/target/${BUILDTYPE}/include)

include_directories(${EXTRA_INCLUDES})

IF(BUILDTYPE MATCHES "cfe_bootloader")
   set(EFU_SOURCES efu_common.c efu_privileged.c)
   set(CRYPTO_SOURCES_TARGET target/${BUILDTYPE}/crypto_bl.c)
   add_definitions(-DBUILDTYPE=${BUILDTYPE} -DBUILDTYPE_${BUILDTYPE})
ENDIF()

if (BUILD_SECUREBOOT)
   set(CRYPTO_SOURCES crypto.c target/${BUILDTYPE}/crypto_api.c ${CRYPTO_SOURCES_TARGET})
   add_definitions(-DCONFIG_RIPDRV_CRYPTO_SUPPORT)
ENDIF()

add_library(riplib STATIC rip2lib.c target/${BUILDTYPE}/platform.c crc32.c ${EFU_SOURCES} ${CRYPTO_SOURCES})

IF(BUILDTYPE MATCHES "userspace")
   add_executable(ripv2gen rip2_builder.c rip2lib.c target/${BUILDTYPE}/platform.c crc32.c)
   add_executable(ripv2getconfig rip2_getconfig.c rip2lib.c target/${BUILDTYPE}/platform.c crc32.c)
   install (TARGETS ripv2gen ripv2getconfig DESTINATION bin)
   add_definitions(-fPIC)
ENDIF()

IF (ENABLE_TESTS)
   include(${CMAKE_SOURCE_DIR}/cmake/unittest.cmake)
ENDIF()

