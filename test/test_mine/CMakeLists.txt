add_executable(test_rx_mine_cache
        test_rx_mine_cache.cpp)


include_directories(test_rx_mine_cache
        ${GTEST_INCLUDE_DIR}
        ${RANDOMX_INCLUDE_DIR}
        ${XDAG_CORE_INCLUDE_DIR}
        ${UTILS_INCLUDE_DIR})

target_link_libraries(test_rx_mine_cache
        PRIVATE
        ${GTEST_LIBRARY}
        ${RANDOMX_LIBRARY}
        ${XDAG_CORE_LIBRARY})