find_package(GTest REQUIRED)
include(GoogleTest)

set(TEST_TARGETS util_test level_test format_test sink_test synclogger_test buffer_test)

foreach(test_target IN LISTS TEST_TARGETS)
    add_executable(${test_target} ${test_target}.cc)
    target_include_directories(${test_target} PRIVATE ${PROJECT_SOURCE_DIR}/include)
    target_link_libraries(${test_target} PRIVATE GTest::gtest_main pthread)

    # 开启调试信息
    target_compile_options(${test_target} PRIVATE -g3)
    target_link_options(${test_target} PRIVATE -g)

    gtest_discover_tests(${test_target})
endforeach()