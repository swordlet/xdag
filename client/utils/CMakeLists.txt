file(GLOB_RECURSE UTILS_SOURCES
        *.c
        *.cpp)

file(GLOB_RECURSE UTILS_HEADERS
        *.h)

add_library(utils
        ${UTILS_SOURCES}
        ${UTILS_HEADERS})

set(UTILS_LIBRARY "utils" CACHE STRING "uthash library")
set(UTILS_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE STRING "utils include dir")