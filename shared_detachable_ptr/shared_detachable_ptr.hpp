#ifndef SHARED_DETACHABLE_PTR_HPP_INCLUDED
#define SHARED_DETACHABLE_PTR_HPP_INCLUDED

#include <memory>
#include <atomic>
#include <type_traits>
#include <cassert>

namespace gmb {
namespace utils {

struct SharedBlockAllocBase;

struct SharedBlockAllocVTable {
    void (*deallocate)(SharedBlockAllocBase*, void*, size_t);
};

struct SharedBlockAllocBase {
    SharedBlockAllocVTable* vtable;
};

template<typename T>
struct SharedBlock {
    T value;
    std::atomic_size_t ref_count;
    SharedBlockAllocBase* alloc;
};

template<typename T>
struct AllocatorWrapper : SharedBlockAllocBase {

    using InnerValueType = typename T::value_type;
    using BlockType = SharedBlock<InnerValueType>;

    static auto deallocate_impl(SharedBlockAllocBase* base, 
                                void* ptr, 
                                size_t n) 
    {
        auto* this_ = static_cast<AllocatorWrapper*>(base);
        
        using BlockRebind = typename
            std::allocator_traits<T>
                ::template rebind_alloc<BlockType>;

        using AllocRebind = typename
            std::allocator_traits<T>::template rebind_alloc<AllocatorWrapper>;

        AllocRebind alloc_alloc { this_->inner_ };
        BlockRebind block_alloc { this_->inner_ };

                                        
        std::allocator_traits<AllocRebind>::destroy(
            alloc_alloc,
            this_
        );

        std::allocator_traits<AllocRebind>::deallocate(
            alloc_alloc,
            this_,
            1
        );

        static_cast<BlockType*>(ptr)->~BlockType();

        std::allocator_traits<BlockRebind>::deallocate(
            block_alloc, 
            static_cast<BlockType*>(ptr),
            n
        );
    }

    AllocatorWrapper(T inner) :
        SharedBlockAllocBase { &wrapper_vtable_ }
    ,   inner_ { inner }
    { }

private:
    T inner_;
    static SharedBlockAllocVTable wrapper_vtable_;
};

template<typename Inner>
SharedBlockAllocVTable AllocatorWrapper<Inner>::wrapper_vtable_ = {
    &AllocatorWrapper<Inner>::deallocate_impl
};

template<typename T>
auto delete_shared_block(SharedBlock<T>* ptr) {
    ptr->alloc->vtable->deallocate(ptr->alloc, ptr, 1);
}

template<typename T>
auto bump_and_return(SharedBlock<T>* ptr) -> SharedBlock<T>* {
    if (ptr && ptr->ref_count.fetch_add(1) > 0) {
        return ptr;
    }

    return nullptr;
}

struct SharedDetachablePtrUnsafety { };

constexpr auto shared_detachable_ptr_unsafety = 
    SharedDetachablePtrUnsafety { };

template<typename T>
struct SharedDetachablePtr {

    static_assert(!std::is_reference_v<T>,
                  "SharedDetachablePtr cannot hold reference types!");
    static_assert(!std::is_array_v<T>,
                  "SharedDetachablePtr cannot hold array types!");

    using value_type = T;
    using pointer = value_type*;
    using const_pointer = std::conditional_t<
        std::is_const_v<std::remove_pointer_t<value_type>>,
        value_type*,
        value_type const*>;

    using reference = value_type&;
    using const_reference = std::conditional_t<
        std::is_const_v<std::remove_pointer_t<value_type>>,
        value_type&,
        value_type const&>;

    using block_type = SharedBlock<value_type>;

    SharedDetachablePtr() noexcept :
        ptr_ { nullptr }
    { }

    // UNSAFE:
    // The caller must guarantee that `ptr` contains
    // a non-zero reference count...
    SharedDetachablePtr(SharedDetachablePtrUnsafety, 
                        SharedBlock<value_type>* ptr) noexcept :
        ptr_ { ptr }
    { }

    SharedDetachablePtr(SharedDetachablePtr const& other) noexcept :
        ptr_ { bump_and_return(other.ptr_) }
    { }

    SharedDetachablePtr(SharedDetachablePtr&& other) noexcept :
        ptr_ { other.detach() }
    { }

    ~SharedDetachablePtr() {
        if (ptr_ && (1 == ptr_->ref_count.fetch_sub(1))) {
            delete_shared_block(ptr_);
        }
    }

    friend void swap(SharedDetachablePtr& lhs, 
                     SharedDetachablePtr& rhs) noexcept
    {
        using std::swap;
        swap(lhs.ptr_, rhs.ptr_);
    }

    auto operator=(SharedDetachablePtr const& other) noexcept
        -> SharedDetachablePtr&
    {
        SharedDetachablePtr tmp { other };
        swap(*this, tmp);
        return *this;
    }

    auto operator=(SharedDetachablePtr&& other) noexcept
        -> SharedDetachablePtr&
    {
        SharedDetachablePtr tmp { std::move(other) };
        swap(*this, tmp);
        return *this;
    }

    auto is_empty() const noexcept -> bool {
        return nullptr == ptr_;
    }

    operator bool() const noexcept {
        return !is_empty();
    }

    auto operator->() noexcept -> pointer {
        assert(!is_empty() && "Dereferencing empty SharedDetachablePtr!");
        return std::addressof(ptr_->value);
    }

    auto operator->() const noexcept -> const_pointer {
        return const_cast<SharedDetachablePtr*>(this)->operator->();
    }

    auto operator*() noexcept -> reference {
        return (*operator->());
    }

    auto operator*() const noexcept -> const_reference {
        return (*operator->());
    }

    auto detach() noexcept -> SharedBlock<T>* {
        auto* p = ptr_;
        ptr_ = nullptr;
        return p;
    }

    friend auto operator==(SharedDetachablePtr const& lhs,
                           SharedDetachablePtr const& rhs) noexcept -> bool
    {
        return lhs.ptr_ == rhs.ptr_;
    }

private:
    SharedBlock<value_type>* ptr_;
};

template<typename T>
auto operator!=(SharedDetachablePtr<T> const& lhs,
                SharedDetachablePtr<T> const& rhs) noexcept -> bool
{
    return !(lhs == rhs);
}

template<typename T, typename Alloc, typename... Args>
auto allocate_shared_detachable(Alloc alloc, Args&&... args) 
    -> SharedDetachablePtr<T>
{

    //  TODO(GB):
    //  This function could do with a `ERROR_GUARD`-style RAII
    //  mechanism.

    using AllocRebind = typename
        std::allocator_traits<Alloc>
            ::template rebind_alloc<AllocatorWrapper<Alloc>>;

    using BlockRebind = typename
        std::allocator_traits<Alloc>
            ::template rebind_alloc<SharedBlock<T>>;

    AllocRebind alloc_alloc { alloc };
    BlockRebind block_alloc { alloc };

    typename AllocRebind::value_type* inner_alloc = nullptr;

    inner_alloc = 
        std::allocator_traits<AllocRebind>::allocate(alloc_alloc, 1);

    std::allocator_traits<AllocRebind>::construct(alloc_alloc,
                                                  inner_alloc,
                                                  alloc);

    typename BlockRebind::value_type* block = nullptr;
    try {
        block =
            std::allocator_traits<BlockRebind>::allocate(
                block_alloc,
                1
            );

        new (block) SharedBlock<T> {
            std::forward<Args>(args)...,
            1,
            inner_alloc
        };

    }
    catch (...) {

        std::allocator_traits<AllocRebind>::destroy(alloc_alloc,
                                                    inner_alloc);

        std::allocator_traits<AllocRebind>::deallocate(alloc_alloc,
                                                       inner_alloc,
                                                       1);
        if (block) {
            std::allocator_traits<BlockRebind>::deallocate(block_alloc,
                                                           block,
                                                           1);
        }

        throw;
    }

    block->alloc = inner_alloc;

    return SharedDetachablePtr<T> { 
        shared_detachable_ptr_unsafety,
        block
    };
}

template<typename T, typename... Args>
auto make_shared_detachable(Args&&... args) -> SharedDetachablePtr<T> {
    return allocate_shared_detachable<T>(
        std::allocator<std::decay_t<T>> { },
        std::forward<Args>(args)...
    );
}

}
}
#endif // SHARED_DETACHABLE_PTR_HPP_INCLUDED
