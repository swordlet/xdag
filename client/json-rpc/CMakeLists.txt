file(GLOB_RECURSE JSON_RPC_SOURCES
        *.c
        *.cpp
        )

file(GLOB_RECURSE JSON_RPC_HEADERS
        *.h)

add_library(jsonrpc
        ${JSON_RPC_SOURCES}
        ${JSON_RPC_HEADERS})

set(JSON_RPC_LIBRARY "jsonrpc" CACHE STRING "jsonrpc library")
set(JSON_RPC_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE STRING "json rpc include dir")
