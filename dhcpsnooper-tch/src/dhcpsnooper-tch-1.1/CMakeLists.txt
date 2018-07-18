cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0015 NEW)

# Project Definition
project(dhcpsnooper-tch C)

set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -std=c99")

add_definitions(-D_GNU_SOURCE -Wall -Werror -Wextra)

add_executable(dhcpsnooper src/dhcpsnooper.c src/config.c src/ubus.c)
target_link_libraries(dhcpsnooper ubox uci ubus netfilter_queue)

# Installation
install(TARGETS dhcpsnooper DESTINATION bin/)


# Packaging information
set(CPACK_PACKAGE_VERSION "1")
set(CPACK_PACKAGE_CONTACT "Alin Nastac <alin.nastac@technicolor.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "dhcpsnooper")
set(CPACK_GENERATOR "DEB;RPM;STGZ")
set(CPACK_STRIP_FILES true)

SET(CPACK_DEBIAN_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION})
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}_${CPACK_DEBIAN_PACKAGE_VERSION}")

include(CPack)

