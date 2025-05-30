add_executable(util_test util_test.cc)

target_include_directories(util_test PRIVATE ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(util_test gtest gtest_main pthread)

include(GoogleTest)
gtest_discover_tests(util_test)
