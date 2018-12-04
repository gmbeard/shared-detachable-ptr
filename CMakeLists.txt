cmake_minimum_required(VERSION 3.8)

project(SharedDetachablePtr CXX)

option(
    SHARED_DETACHABLE_PTR_ENABLE_TESTS
    "Enable tests for ${PROJECT_NAME}"
    OFF
)

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

install(
    DIRECTORY
        ${CMAKE_CURRENT_LIST_DIR}/shared_detachable_ptr/
    DESTINATION
        include/shared_detachable_ptr
)

install(
    TARGETS
        SharedDetachablePtr
    EXPORT
        SharedDetachablePtrTargets
)

install(
    FILES
        ${CMAKE_CURRENT_LIST_DIR}/cmake/SharedDetachablePtrConfig.cmake
    DESTINATION
        lib/cmake/SharedDetachablePtr
)

install(
    EXPORT
        SharedDetachablePtrTargets
    DESTINATION
        lib/cmake/SharedDetachablePtr
    FILE
        SharedDetachablePtrTargets.cmake
    NAMESPACE
        SharedDetachablePtr::
)

if (SHARED_DETACHABLE_PTR_ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
