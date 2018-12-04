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

auto main() -> int {
    run_tests({
        construct_test,
        destructor_test,
        detach_test
    });

    return 0;
}
