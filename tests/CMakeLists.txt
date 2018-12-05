add_executable(
    shared_detachable_ptr_tests
    shared_detachable_ptr_tests.cpp
)

target_link_libraries(
    shared_detachable_ptr_tests
    PRIVATE
        SharedDetachablePtr::SharedDetachablePtr
)

target_compile_options(
    shared_detachable_ptr_tests
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /permissive- /analyze>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Werror -Wextra>
)

add_test(
    NAME shared_detachable_ptr_tests
    COMMAND shared_detachable_ptr_tests
)

if(WIN32)
    add_executable(
        example
        example.cpp
    )

    target_link_libraries(
        example
        PRIVATE
            SharedDetachablePtr::SharedDetachablePtr
    )

    target_compile_options(
        example
        PRIVATE
            $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /permissive- /analyze>
            $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Werror -Wextra>
    )

    add_test(
        NAME Example
        COMMAND example
    )
endif()
