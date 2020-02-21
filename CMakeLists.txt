cmake_minimum_required(VERSION 2.8)
Project(dynaudnorm)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(compilerwarnings)
include(cppcheck)

set(COMMONFLAGS "-pipe")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMONFLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3 -DNDEBUG -fPIC -DPIC")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

add_definitions("-D_DEFAULT_SOURCE -D_XOPEN_SOURCE=600")

set(CMAKE_SHARED_MODULE_PREFIX "")

add_subdirectory(src)

file(GLOB HEADERS "${PROJECT_SOURCE_DIR}/include/*.h")
