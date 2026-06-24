#pragma once

#include <iostream>
#include <string>
#include <condition_variable>
#include <array>

#include <cstdlib>
#include <cassert>
#include <cstdio>
#include <chrono>
#include <sstream>
#include <unordered_map>
#include <vector>

#include <cstdint>
#include <cstddef> // std::max_align_t
#include <ctime>


using utf8byte = char;
using utf8chars = const utf8byte *;

using st_size = unsigned long;
using st_uint = unsigned long;
using st_int = long;

using st_byte = unsigned char;

#define UNUSED(x) (void)(x)

#define st_size_of(T) ((int)sizeof(T))

namespace Search {
    // Parse result code
    constexpr int NOTFOUND = -1;
    constexpr int DONE_WITH_PREVIOUS_POSITION = -2;

    static inline bool IsTokenized(int result) {
        return result != NOTFOUND;
    }

    static inline bool IsPositionChanged(int result) {
        return result > -1;
    }
}

// malloc using type. e.g. auto *node = mallocForType<NodeStruct>();
template<class T>
static inline T *mallocForType() {
    return (T *) malloc(sizeof(T));
}




// heap entry on MemBuffer
using HeapEntry = struct Item {
    void *ptr{nullptr};
    bool freed{false};
};

struct MemBufferBlock {
    void *chunk = nullptr;
    MemBufferBlock *next = nullptr;
    MemBufferBlock *prev = nullptr;
    bool isLast = true;
    int itemCount = 0;
};

struct MemBuffer {
    static constexpr st_size DEFAULT_BUFFER_SIZE = 255;

    bool isHeapEntryEnabled = false;

    MemBufferBlock *firstBufferBlock = nullptr;
    MemBufferBlock *currentBufferBlock = nullptr;
    // currentMemOffset is the offset of the next allocation in the current buffer block, it is initialized to DEFAULT_BUFFER_SIZE + 1 to trigger the allocation of the first buffer block when newBytesMem is called for the first time.
    st_uint currentMemOffset = DEFAULT_BUFFER_SIZE + 1;

    void init();
    void initWithHeapEntryEnabled();

    void freeAll();

    void tryFreeMemoryBlock(MemBufferBlock *memBufferBlock);

    template<typename Type>
    void tryDelete(Type *ptr) {
        if (this->firstBufferBlock == nullptr) {
            return;
        }

        MemBufferBlock *targetBufferList = *((MemBufferBlock **) ((st_byte *) ptr - sizeof(MemBufferBlock *)));
        targetBufferList->itemCount--;

        this->tryFreeMemoryBlock(targetBufferList);
    }

    template<typename T>
    T *newMemArray(st_size len) {
        return (T *) this->newMem<T>(len);
    }

    template<typename Type>
    Type *newMem(unsigned int count) {
        auto bytes = st_size_of(Type) * count;
        return (Type*)this->newBytesMem(bytes);
    }
    
    utf8byte *newText(unsigned int count);

    void *newBytesMem(unsigned int bytes);
};