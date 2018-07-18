find_package(UNITTESTCHECK REQUIRED)
find_package(CHECK REQUIRED)


INCLUDE_DIRECTORIES(
  ${UNITTESTCHECK_INCLUDE_DIR} ${CHECK_INCLUDE_DIR} ${CHECK_INCLUDE_DIR}/src
)

#set_source_files_properties(
#    rip2lib.c
#    PROPERTIES
#    COMPILE_FLAGS "--coverage"
#)

enable_testing()

add_library(unittest_rip SHARED 
    test/unit/rip2lib/main.c 
    test/unit/rip2lib/rip2lib.c
)
target_link_libraries(unittest_rip riplib)

set_target_properties(
    unittest_rip
    PROPERTIES
    LINK_FLAGS --coverage
    COMPILE_DEFINITIONS USERSPACE
)

add_test (RipUnitTests ${UNITTESTCHECK_BINARY} --nf --nml unittest_rip)

