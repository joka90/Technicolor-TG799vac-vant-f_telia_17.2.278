cmake_minimum_required(VERSION 2.6)

PROJECT(ipset-helper-tch C)

SET(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

find_package(LIBUCI REQUIRED)

ADD_DEFINITIONS(-Os -Wall -Werror -std=gnu99 -g3)

LIST(APPEND iptc_libs ip4tc)
LIST(APPEND iptc_libs ip6tc)
LIST(APPEND ext_libs iptext)
LIST(APPEND ext_libs iptext4)
LIST(APPEND ext_libs iptext6)

INCLUDE_DIRECTORIES(${LIBUCI_INCLUDE_DIRS})

SET(SOURCES src/main.c src/utils.c src/options.c src/ipsets.c src/ipset_entries.c)

ADD_EXECUTABLE(ipset-helper ${SOURCES})
TARGET_LINK_LIBRARIES(ipset-helper ${LIBUCI_LIBRARIES} xtables ${iptc_libs} ${ext_libs})

SET(CMAKE_INSTALL_PREFIX /usr)
INSTALL(TARGETS ipset-helper RUNTIME DESTINATION sbin)
