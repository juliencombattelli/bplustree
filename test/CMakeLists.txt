add_executable(bplustree-private-unit-tests private-tests.cpp)
target_link_libraries(bplustree-private-unit-tests PUBLIC bplustree gtest_main)
target_enable_coverage(bplustree-private-unit-tests)

add_test(NAME bplustree-private-unit-tests COMMAND bplustree-private-unit-tests)

add_executable(bplustree-public-unit-tests public-tests.cpp)
target_link_libraries(bplustree-public-unit-tests PUBLIC bplustree gtest_main)
target_enable_coverage(bplustree-public-unit-tests)

add_test(NAME bplustree-public-unit-tests COMMAND bplustree-public-unit-tests)
