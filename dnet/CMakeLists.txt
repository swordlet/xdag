file(GLOB_RECURSE DNET_SOURCES
        *.c)

file(GLOB_RECURSE DNET_HEADERS
        *.h)

add_library(dnet
        ${DNET_SOURCES}
        ${DNET_HEADERS})

target_include_directories(dnet
        PRIVATE
        ${ALGO_INCLUDE_DIR}
        )

target_link_libraries(dnet
        PRIVATE
        ${ALGO_LIBRARY}
        )

set(DNET_LIBRARY "dnet" CACHE STRING "dnet library")
set(DNET_INCLUDE_DIR "." CACHE STRING "dnet include dir")
