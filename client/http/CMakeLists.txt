file(GLOB_RECURSE HTTP_SOURCES
        *.c)

file(GLOB_RECURSE HTTP_HEADERS
        *.h)

add_library(http
        ${HTTP_SOURCES}
        ${HTTP_HEADERS})
target_include_directories(http
        PRIVATE
        ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(http
        PRIVATE
        ${OPENSSL_SSL_LIBRARY}
        ${OPENSSL_CRYPTO_LIBRARY}
        )

set(HTTP_LIBRARY "http" CACHE STRING "http library")
set(HTTP_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE STRING "http include dir")
