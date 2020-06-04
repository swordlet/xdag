file(GLOB_RECURSE WEBSOCKET_SOURCES
        *.c
        *.cpp)

file(GLOB_RECURSE WEBSOCKET_HEADERS
        *.h)

add_library(websocket
        ${WEBSOCKET_SOURCES}
        ${WEBSOCKET_HEADERS})

set(WEBSOCKET_LIBRARY "websocket" CACHE STRING "websocket library")
set(WEBSOCKET_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE STRING "websocket include dir")