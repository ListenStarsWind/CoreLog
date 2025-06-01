find_package(GTest REQUIRED)

set(TEST_TARGETS util_test level_test format_test sink_test)

add_executable(util_test util_test.cc)
target_include_directories(util_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(util_test PRIVATE GTest::gtest_main pthread)

add_executable(level_test level_test.cc)
target_include_directories(level_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(level_test PRIVATE GTest::gtest_main pthread)

add_executable(format_test format_test.cc)
target_include_directories(format_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(format_test PRIVATE GTest::gtest_main pthread)

add_executable(sink_test sink_test.cc)
target_include_directories(sink_test PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(sink_test PRIVATE GTest::gtest_main pthread)

foreach(test_target IN LISTS TEST_TARGETS)
    gtest_discover_tests(${test_target})
endforeach()