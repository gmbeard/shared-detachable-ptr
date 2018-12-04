#include "shared_detachable_ptr/shared_detachable_ptr.hpp"
#include <cassert>
#include <iostream>
#include <memory>

using namespace gmb::utils;

struct A {
    A(bool* val) :
        value_ { val }
    { }

    A(A&& other) noexcept :
        value_ { other.value_ }
    { other.value_ = nullptr; }

    A(A const&) = delete;

    ~A() {
        if (value_) {
            *value_ = true;
        }
    }

    auto operator=(A&& other) noexcept -> A& {
        A tmp { std::move(other) };
        std::swap(tmp.value_, value_);
        return *this;
    }

    auto operator=(A const&) -> A& = delete;

    bool* value_;
};

using TestType = auto (*)() -> void;
auto run_tests(std::initializer_list<TestType> tests) {
    for (auto& t : tests) {
        t();
    }
}

auto construct_test() {
    auto p = allocate_shared_detachable<size_t>(std::allocator<size_t> { },
                                                42);
    assert(*p == 42 && "*p != 42!");
}

auto destructor_test() {

    auto destroyed = false;

    {
        auto p = allocate_shared_detachable<A>(std::allocator<A> { },
                                               A { &destroyed });
        {
            auto p2 = p;
            assert(p2->value_ == p->value_ && "p2->value_ != p->value_!");
        }

        assert(!destroyed && "destroyed == true!");
    }

    assert(destroyed && "destroyed == false!");
}

auto detach_test() {
    SharedBlock<A>* block = nullptr;
    auto destroyed = false;

    {
        auto p = make_shared_detachable<A>(A { &destroyed });
        block = p.detach();
    }

    assert(block && "block == nullptr!");
    assert(block->ref_count.fetch_add(1) == 1 && "block->ref_count != 1!");
    assert(!destroyed && "destroyed == true!");

    {
        auto p = SharedDetachablePtr<A> { shared_detachable_ptr_unsafety, 
                                          block };
    }

    assert(!destroyed && "destroyed == true!");

    assert(block->ref_count == 1 && "block->ref_count != 1!");

    {
        auto p = SharedDetachablePtr<A> { shared_detachable_ptr_unsafety, 
                                          block };
    }

    assert(destroyed && "destroyed == false!");
}

auto pointer_element_test() {
    auto element = size_t { 42 };
    auto p = make_shared_detachable<size_t const*>(&element);

    assert(*p == &element && "*p != &element!");
}

template<typename...>
using VoidType = void;

template<typename Void, template <typename...> class Test, typename... Args>
struct TypeTest : std::false_type { };

template<template <typename...> class Test, typename... Args>
struct TypeTest<Test<Args...>, Test, Args...> : std::true_type { };

template<typename T, typename From>
using PtrAssignable = decltype(*std::declval<T>() = std::declval<From>());

auto const_test() {
    auto p = make_shared_detachable<size_t const>(42);

    constexpr auto v = TypeTest<void, 
                                PtrAssignable, 
                                decltype(p), 
                                size_t>::value;
    assert(!v && 
        "assignment to dereferenced const value shouldn't be allowed!");
}

auto block_is_standard_layout_test() {
    constexpr bool v = std::is_standard_layout_v<SharedBlock<size_t>>;
    assert(v && "SharedBlock<T> isn't standard layout!");

    auto p = make_shared_detachable<size_t>(42);

    auto* block = p.detach();
    **reinterpret_cast<size_t**>(block) = 43;

    assert(*block->value == 43 && "block->value != 43!");

    p = SharedDetachablePtr<size_t> { shared_detachable_ptr_unsafety,
                                      block };

    assert(*p == 43 && "*p != 43!");
}

auto main() -> int {
    run_tests({
        construct_test,
        destructor_test,
        detach_test,
        pointer_element_test,
        const_test,
        block_is_standard_layout_test
    });

    return 0;
}
