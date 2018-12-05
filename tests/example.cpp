#include "shared_detachable_ptr/shared_detachable_ptr.hpp"
#include <type_traits>
#include <windows.h>
#include <cassert>
#include <iostream>
#include <memory>
#include <algorithm>
#include <iterator>

namespace gbu = gmb::utils;

struct Buffer {
    uint8_t* data;
    DWORD length;
};

struct AsyncTask {
    OVERLAPPED ovr;
    Buffer buffer;
};

namespace std {
template<>
class allocator<typename gbu::SharedDetachablePtr<AsyncTask>::block_type> {
public:
    using value_type = 
        gbu::SharedDetachablePtr<AsyncTask>::block_type;

    static constexpr auto kMaxBufferSize = size_t { 256 };
    static constexpr auto kBlocksRequired = kMaxBufferSize / sizeof(value_type);
    static constexpr DWORD kActualBufferSize = 
        (kBlocksRequired - 1) * sizeof(value_type);

    template<typename U>
    allocator(allocator<U> const&) noexcept { }

    template<typename U>
    struct rebind {
        using type = allocator<U>;
    };

    auto allocate(size_t n) -> value_type* {
        assert(n == 1 && "Only one block at a time is allowed!");
        std::cerr << "Allocating block\n";
        return static_cast<value_type*>(
            std::malloc(sizeof(value_type) * n * kBlocksRequired)
        );
    }

    auto deallocate(value_type* ptr, size_t n) {
        assert(n == 1 && "Only one block at a time is allowed!");
        std::cerr << "Deallocating block\n";
        std::free(ptr);
    }

    template<typename... Args>
    auto construct(value_type* ptr, Args&&... args) {
        std::cerr << "Constructing block\n";
        auto p = ::new (ptr) value_type { std::forward<Args>(args)... };
        p->value.buffer.data = reinterpret_cast<uint8_t*>(ptr + 1);
        p->value.buffer.length = kActualBufferSize;
        std::fill_n(p->value.buffer.data,
                    p->value.buffer.length,
                    u8'd');
    }

    auto destroy(value_type* ptr) {
        std::cerr << "Destroying block\n";
        ptr->~value_type();
    }
};
}

constexpr auto ensure_block_is_std_layout() {
    using BlockType = 
        typename gbu::SharedDetachablePtr<AsyncTask>::block_type;
    return std::is_standard_layout_v<BlockType>;
}

auto main() -> int {
    static_assert(ensure_block_is_std_layout(), 
                  "AsyncTask is not StandardLayoutType!");

    auto ptr = gbu::make_shared_detachable<AsyncTask>();

    std::cout << "Buffer size: " << ptr->buffer.length << "\n";
    std::copy_n(ptr->buffer.data,
                ptr->buffer.length,
                std::ostream_iterator<char> { std::cout });
    std::cout << "\n";
}
