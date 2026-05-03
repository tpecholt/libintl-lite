cmake_minimum_required(VERSION 3.10)

project(libintl)
#include (CTest)
option(LIBINTL_BUILD_TESTS "Build the unit tests" ON)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(src)

if (LIBINTL_BUILD_TESTS)
    add_subdirectory(test)
endif ()