file(GLOB XDAG_CORE_SOURCES
        *.c
        *.cpp)

file(GLOB XDAG_CORE_HEADERS
        *.h)

list(FILTER XDAG_CORE_SOURCES EXCLUDE REGEX "main.c")
list(FILTER XDAG_CORE_SOURCES EXCLUDE REGEX ".*cgi.c$")
list(FILTER XDAG_CORE_HEADERS EXCLUDE REGEX ".*dvstime.c$")
list(FILTER XDAG_CORE_HEADERS EXCLUDE REGEX ".*statsdaemon.c$")

add_library(xdagcore
        ${XDAG_CORE_SOURCES}
        ${XDAG_CORE_HEADERS})

target_include_directories(xdagcore
        PRIVATE
        ${ALGO_INCLUDE_DIR}
        ${HTTP_INCLUDE_DIR}
        ${JSON_RPC_INCLUDE_DIR}
        ${UTHASH_INCLUDE_DIR}
        ${UTILS_INCLUDE_DIR}
        ${WEBSOCKET_INCLUDE_DIR}
        ${RANDOMX_INCLUDE_DIR}
        ${RocksDB_INCLUDE_DIR}
        ${SECP256K1_INCLUDE_DIR}
        ${OPENSSL_INCLUDE_DIR}
        ${TCMALLOC_INCLUDE_DIR}
        ${UV_INCLUDE_DIR}
        ${RANDOMX_INCLUDE_DIR}
        )

target_link_libraries(xdagcore
        PUBLIC
        ${DNET_LIBRARY}
        ${ALGO_LIBRARY}
        ${HTTP_LIBRARY}
        ${JSON_RPC_LIBRARY}
        ${UTILS_LIBRARY}
        ${WEBSOCKET_LIBRARY}
        ${RADOMX_LIBRARY}
        ${CMAKE_THREAD_LIBS_INIT}
        ${RocksDB_LIBRARY}
        ${SECP256K1_LIBRARY}
        ${OPENSSL_SSL_LIBRARY}
        ${OPENSSL_CRYPTO_LIBRARY}
        ${TCMALLOC_LIBRARY}
        ${PROFILER_LIBRARY}
        ${UV_LIBRARY}
        ${RANDOMX_LIBRARY}
        m
        dl
)

set(XDAG_CORE_LIBRARY "xdagcore" CACHE STRING "xdag core library")
set(XDAG_CORE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE STRING "xdag core include dir")