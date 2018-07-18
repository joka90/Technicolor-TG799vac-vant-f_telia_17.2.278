cmake_minimum_required(VERSION 2.8.4)
project(miniupnpd-tch)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
find_package(LIBNFNETLINK REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Libuuid REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-strict-aliasing -fno-common -Wall -Wextra -Wstrict-prototypes -Wdeclaration-after-statement -DIPTABLES_143")

set(SOURCE_FILES src/daemonize.c src/minissdp.c src/miniupnpdpath.c src/options.c src/upnpevents.c src/upnppermissions.c src/upnpreplyparse.c
	src/getconnstatus.c src/miniupnpd.c src/minixml.c src/token.c src/upnpglobalvars.c src/upnppinhole.c src/upnpsoap.c src/getifaddr.c
	src/natpmp.c src/upnpdescgen.c src/upnphttp.c src/upnpredirect.c src/upnputils.c src/linux/getifstats.c
	src/linux/getroute.c  src/linux/ifacewatcher.c src/netfilter/iptcrdr.c src/netfilter/iptpinhole.c)

add_executable(miniupnpd ${SOURCE_FILES})

include_directories(${LIBNFNETLINK_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIRS} )
target_link_libraries(miniupnpd ${LIBNFNETLINK_LIBRARIES} ${OPENSSL_LIBRARIES} rt ip4tc)
INSTALL(TARGETS miniupnpd RUNTIME DESTINATION bin)
