cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

option(WITH_RANDOMX         "Enable randomx support" ON)
option(WITH_LIBCPUID        "Enable libcpuid support" OFF)
option(WITH_HWLOC           "Enable hwloc support" OFF)
option(WITH_TLS             "Enable OpenSSL support" OFF)
option(WITH_ASM             "Enable ASM PoW implementations" OFF)

include (CheckIncludeFile)
include (cmake/cpu.cmake)
include (cmake/os.cmake)
#include (cmake/backend.cmake)

set(SOURCES_CRYPTO
        src/crypto/common/HugePagesInfo.cpp
        src/crypto/common/MemoryPool.cpp
        src/crypto/common/VirtualMemory.cpp
        )

if (WITH_HWLOC)
    list(APPEND HEADERS_CRYPTO
            src/crypto/common/NUMAMemoryPool.h
            )

    list(APPEND SOURCES_CRYPTO
            src/crypto/common/NUMAMemoryPool.cpp
            src/crypto/common/VirtualMemory_hwloc.cpp
            )
endif()

if (XMRIG_OS_WIN)
    list(APPEND SOURCES_OS
            res/app.rc
            src/crypto/common/VirtualMemory_win.cpp
            )

    set(EXTRA_LIBS ws2_32 psapi iphlpapi userenv)
elseif (XMRIG_OS_APPLE)
    list(APPEND SOURCES_OS
            src/crypto/common/VirtualMemory_unix.cpp
            )
else()
    list(APPEND SOURCES_OS
            src/crypto/common/VirtualMemory_unix.cpp
            )

    if (XMRIG_OS_ANDROID)
        set(EXTRA_LIBS pthread rt dl log)
    elseif (XMRIG_OS_LINUX)
        list(APPEND SOURCES_OS
                src/crypto/common/LinuxMemory.h
                src/crypto/common/LinuxMemory.cpp
                )

        set(EXTRA_LIBS pthread rt dl)
    elseif (XMRIG_OS_FREEBSD)
        set(EXTRA_LIBS kvm pthread)
    endif()
endif()

add_definitions(/D__STDC_FORMAT_MACROS)
add_definitions(/DUNICODE)

include(cmake/flags.cmake)
include(cmake/randomx.cmake)
include(cmake/argon2.cmake)
#include(cmake/cpu.cmake)

include_directories(src)
include_directories(src/3rdparty)

if (BUILD_STATIC)
    set(CMAKE_EXE_LINKER_FLAGS " -static")
endif()

if (WITH_DEBUG_LOG)
    add_definitions(/DAPP_DEBUG)
endif()

add_library(randomx
        ${HEADERS}
        ${SOURCES}
        ${SOURCES_OS}
        ${SOURCES_CPUID}
        ${HEADERS_CRYPTO}
        ${SOURCES_CRYPTO}
        ${SOURCES_SYSLOG}
        ${TLS_SOURCES}
        ${XMRIG_ASM_SOURCES}
        ${CN_GPU_SOURCES}
        ${SOURCES_BACKEND_CPU})
#target_link_libraries(${RX_LIBRARY} ${XMRIG_ASM_LIBRARY} ${OPENSSL_LIBRARIES} ${UV_LIBRARIES} ${EXTRA_LIBS} ${CPUID_LIB} ${ARGON2_LIBRARY})

set(RANDOMX_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/crypto/randomx" CACHE STRING "randomx include path")
set(RANDOMX_LIBRARY "randomx" CACHE STRING "randomx library")

if (WIN32)
    add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/bin/WinRing0/WinRing0x64.sys" $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>)
endif()
