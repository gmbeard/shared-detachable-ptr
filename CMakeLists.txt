cmake_minimum_required(VERSION 3.8)

project(SharedDetachablePtr CXX)

add_library(
    SharedDetachablePtr
    INTERFACE
)

target_include_directories(
    SharedDetachablePtr
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(
    SharedDetachablePtr
    INTERFACE
        cxx_decltype_auto   # >= C++14
)

add_library(
    SharedDetachablePtr::SharedDetachablePtr
    ALIAS
    SharedDetachablePtr
)

enable_testing()
add_subdirectory(tests)
