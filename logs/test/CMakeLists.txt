add_executable(util_test util_test.cc)
target_include_directories(util_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(util_test gtest gtest_main pthread)

add_executable(level_test level_test.cc)
target_include_directories(level_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(level_test gtest gtest_main pthread)

add_executable(format_test format_test.cc)
target_include_directories(format_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(format_test gtest gtest_main pthread)

include(GoogleTest)
gtest_discover_tests(util_test)
gtest_discover_tests(level_test)
gtest_discover_tests(format_test)