add_executable(api-example2
        api-example2.cpp)

include_directories(api-example2
        ${GTEST_INCLUDE_DIR}
        ${RANDOMX_INCLUDE_DIR})

target_link_libraries(api-example2
        PRIVATE
        ${GTEST_LIBRARY}
        ${RANDOMX_LIBRARY})

add_executable(randomx-tests
  tests.cpp)

include_directories(randomx-tests
        ${GTEST_INCLUDE_DIR}
        ${RANDOMX_INCLUDE_DIR})

target_link_libraries(randomx-tests
    PRIVATE
    ${GTEST_LIBRARY}
    ${RANDOMX_LIBRARY})

add_executable(randomx-codegen
  code-generator.cpp)

include_directories(randomx-codegen
        ${GTEST_INCLUDE_DIR}
        ${RANDOMX_INCLUDE_DIR})

target_link_libraries(randomx-codegen
    PRIVATE
    ${GTEST_LIBRARY}
    ${RANDOMX_LIBRARY})

add_executable(randomx-benchmark
    benchmark.cpp
    affinity.cpp)

include_directories(randomx-benchmark
        ${GTEST_INCLUDE_DIR}
        ${RANDOMX_INCLUDE_DIR})

target_link_libraries(randomx-benchmark
        PRIVATE
        ${GTEST_LIBRARY}
        ${RANDOMX_LIBRARY}
        ${CMAKE_THREAD_LIBS_INIT})

add_executable(dynamic-seed
        dynamic-seed.cpp)

include_directories(dynamic-seed
        ${GTEST_INCLUDE_DIR}
        ${RANDOMX_INCLUDE_DIR})

target_link_libraries(dynamic-seed
        PRIVATE
        ${GTEST_LIBRARY}
        ${RANDOMX_LIBRARY}
        ${CMAKE_THREAD_LIBS_INIT})

add_executable(test_rx_seedheight
        test_rx_seedheight.cpp)

include_directories(test_rx_seedheight
        ${GTEST_INCLUDE_DIR}
        ${RANDOMX_INCLUDE_DIR})

target_link_libraries(test_rx_seedheight
        PRIVATE
        ${GTEST_LIBRARY}
        ${RANDOMX_LIBRARY}
        ${CMAKE_THREAD_LIBS_INIT})

if(RUN_TEST)
    #run randomx-test after build
    add_custom_target(test_randomx_all
            ALL DEPENDS randomx-tests)

    add_custom_command(TARGET randomx-tests
            POST_BUILD
            COMMAND randomx-tests)

    add_custom_command(TARGET randomx-codegen
            POST_BUILD
            COMMAND randomx-codegen)

    add_custom_command(TARGET test_rx_seedheight
            POST_BUILD
            COMMAND test_rx_seedheight)

    add_custom_target(benchmark_randomx_all
            ALL DEPENDS randomx-benchmark)

    add_custom_command(TARGET randomx-benchmark
            POST_BUILD
            COMMAND randomx-benchmark --mine --jit --threads 4 --auto)
endif()