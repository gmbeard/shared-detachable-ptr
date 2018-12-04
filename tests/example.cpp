#include "shared_detachable_ptr/shared_detachable_ptr.hpp"
#include <type_traits>
#include <windows.h>
#include <cassert>

namespace gbu = gmb::utils;

struct Buffer {
    uint8_t* data;
    DWORD length;
};

struct AsyncTask {
    OVERLAPPED ovr;
    Buffer buffer;
};

auto allocate_buffer(DWORD length) -> Buffer {
    auto alloc = std::allocator<uint8_t> { };
    
    return {
        alloc.allocate(length),
        length
    };
}

auto deallocate_buffer(Buffer buffer) {
    auto alloc = std::allocator<uint8_t> { };
    alloc.deallocate(buffer.data, buffer.length);
}

auto main() -> int {
    using BlockType = 
        typename gbu::SharedDetachablePtr<AsyncTask>::block_type;

    static_assert(
        std::is_standard_layout_v<BlockType>, 
        "AsyncTask is not StandardLayoutType!");

    auto ptr = 
        gbu::make_shared_detachable<AsyncTask>(OVERLAPPED { },
                                               allocate_buffer(256));

    auto* inner_block = ptr.detach();

    auto handle = INVALID_HANDLE_VALUE;

    auto result = 
        ::ReadFile(handle,
                   inner_block->value.buffer.data,
                   inner_block->value.buffer.length,
                   nullptr,
                   reinterpret_cast<LPOVERLAPPED>(inner_block));

    assert(!result && "result == TRUE!");
}
