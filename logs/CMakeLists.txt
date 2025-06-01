cmake_minimum_required(VERSION 3.14)
project(CoreLog)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 查找系统安装的 GTest
find_package(GTest REQUIRED)

enable_testing()
include(GoogleTest)

add_subdirectory(test)
