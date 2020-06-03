set(test_randomx_sources
        test_randomx.cpp
        ${SOURCES_OS})

set(test_randomx_headers)

add_executable(test_randomx
        ${test_randomx_sources}
        ${test_randomx_headers})





#if (XMRIG_OS_WIN)
#    list(APPEND SOURCES_OS
#            res/app.rc
#            src/crypto/common/VirtualMemory_win.cpp
#            )
#
#    set(EXTRA_LIBS ws2_32 psapi iphlpapi userenv)
#elseif (XMRIG_OS_APPLE)
#    list(APPEND SOURCES_OS
#            src/crypto/common/VirtualMemory_unix.cpp
#            )
#else()
#    list(APPEND SOURCES_OS
#            src/crypto/common/VirtualMemory_unix.cpp
#            )
#
#    if (XMRIG_OS_ANDROID)
#        set(EXTRA_LIBS pthread rt dl log)
#    elseif (XMRIG_OS_LINUX)
#        list(APPEND SOURCES_OS
#                src/crypto/common/LinuxMemory.h
#                src/crypto/common/LinuxMemory.cpp
#                )
#
#        set(EXTRA_LIBS pthread rt dl)
#    elseif (XMRIG_OS_FREEBSD)
#        set(EXTRA_LIBS kvm pthread)
#    endif()
#endif()

include_directories(test_core
        ${GTEST_INCLUDE_DIR}
        ../../randomx/src/crypto/randomx
        ../../randomx/src/crypto/common
        ${RANDOMX_INCLUDE})

target_link_libraries(test_randomx
        PRIVATE
        ${GTEST_LIBRARY}
        ${RANDOMX_LIBRARY})

set_property(TARGET test_randomx
        PROPERTY
        FOLDER "test")

add_test(NAME   test_randomx
        COMMAND test_randomx)

if(RUN_TEST)
    add_custom_target(test_randomx_all
            ALL DEPENDS test_randomx)

    add_custom_command(TARGET test_randomx_all
            POST_BUILD
            COMMAND           test_randomx)
endif()