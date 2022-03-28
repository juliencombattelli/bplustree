add_executable(bplustree-unit-tests main.cpp)
target_link_libraries(bplustree-unit-tests PUBLIC bplustree)
target_enable_coverage(bplustree-unit-tests)

add_test(NAME bplustree-unit-tests COMMAND bplustree-unit-tests)
