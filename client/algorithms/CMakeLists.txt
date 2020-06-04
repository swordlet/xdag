file(GLOB_RECURSE ALGO_SOURCES
        *.c
        *.cpp)

file(GLOB_RECURSE ALGO_HEADERS
        *.h)

if(APPLE)
    list(APPEND ALGO_SOURCES
            sha256-mb-x86_64-mac.s
            x86_64cpuid-mac.s
            )
else()
    list(APPEND ALGO_SOURCES
            sha256-mb-x86_64.s
            x86_64cpuid.s)
endif()

add_library(algorithms
        ${ALGO_SOURCES}
        ${ALGO_HEADERS})

set(ALGO_LIBRARY "algorithms" CACHE STRING "algorithms library")
set(ALGO_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE STRING "algorithms include dir")
