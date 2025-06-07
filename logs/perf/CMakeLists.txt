cmake_minimum_required(VERSION 3.14)
project(LogBench)

set(EXAMPLE_TARGETS bench)

foreach(example_target IN LISTS EXAMPLE_TARGETS)
    add_executable(${example_target} ${example_target}.cc)
    target_include_directories(${example_target} PRIVATE ${PROJECT_SOURCE_DIR}/../include)
    target_link_libraries(${example_target} PRIVATE pthread)

    target_compile_options(${example_target} PRIVATE -g3)
    target_link_options(${example_target} PRIVATE -g)
endforeach()
