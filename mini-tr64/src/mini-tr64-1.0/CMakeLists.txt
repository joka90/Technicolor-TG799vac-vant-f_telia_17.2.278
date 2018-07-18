cmake_minimum_required(VERSION 2.8.4)
project(minitr064d)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
find_package(LIBNFNETLINK REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Libuuid REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-strict-aliasing -fno-common -Wall -Wextra -Wstrict-prototypes -Wdeclaration-after-statement")

set(SOURCE_FILES src/minitr064d.c src/upnphttp.c src/upnpreplyparse.c src/minixml.c src/upnpsoap.c src/upnpglobalvars.c src/minissdp.c
	src/options.c src/daemonize.c src/upnputils.c src/upnpdescgen.c src/getifaddr.c src/asyncsendto.c src/upnphttpdigest.c
	src/linux/getroute.c src/linux/ifacewatcher.c src/transformer/message.c src/transformer/proxy.c src/transformer/helper.c
	src/soapmethods/soaphelper.c src/soapmethods/deviceinfo.c src/soapmethods/deviceconfig.c 
	src/soapmethods/wanethernetinterfaceconfig.c src/soapmethods/wancommoninterfaceconfig.c src/soapmethods/wandsllinkconfig.c 
	src/soapmethods/wandslinterfaceconfig.c src/soapmethods/wanpppconnection.c src/soapmethods/lanethernetinterfaceconfig.c
	src/soapmethods/wlanconfiguration.c src/soapmethods/lanconfigsecurity.c src/soapmethods/wanipconnection.c src/token.c
	src/minitr064dpath.c)

add_executable(minitr064d ${SOURCE_FILES})
add_executable(minitr064ctl src/minitr064ctl.c)
add_executable(computeHA1 src/computeHA1.c src/upnphttpdigest.c src/upnpglobalvars.c)

include_directories(${LIBNFNETLINK_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIRS} )
target_link_libraries(minitr064d ${LIBNFNETLINK_LIBRARIES} ${OPENSSL_LIBRARIES} rt)
target_link_libraries(computeHA1 ${OPENSSL_LIBRARIES} rt)
INSTALL(TARGETS minitr064d  RUNTIME DESTINATION bin)
INSTALL(TARGETS computeHA1 RUNTIME DESTINATION bin)
