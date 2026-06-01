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


struct MemBufferBlock {
    void *chunk = nullptr;
    MemBufferBlock *next = nullptr;
    MemBufferBlock *prev = nullptr;
    bool isLast = true;
    int itemCount = 0;
};

struct MemBuffer {
    static constexpr st_size DEFAULT_BUFFER_SIZE = 255;

    MemBufferBlock *firstBufferBlock = nullptr;
    MemBufferBlock *currentBufferBlock = nullptr;
    st_uint currentMemOffset = DEFAULT_BUFFER_SIZE + 1;

    void init() {
        this->currentMemOffset = DEFAULT_BUFFER_SIZE + 1;
        this->firstBufferBlock = nullptr;
        this->currentBufferBlock = nullptr;
    }

    void freeAll() {
        MemBufferBlock *bufferList = this->firstBufferBlock;

        while (bufferList) {
            free(bufferList->chunk);

            auto *temp = bufferList;
            bufferList = bufferList->next;
            free(temp);
        }
    }

    template<typename Type>
    void tryDelete(Type *ptr) {
        if (this->firstBufferBlock == nullptr) {
            return;
        }

        MemBufferBlock *targetBufferList = *((MemBufferBlock **) ((st_byte *) ptr - sizeof(MemBufferBlock *)));
        targetBufferList->itemCount--;

        this->tryFreeMemoryBlock(targetBufferList);
    }

    inline void tryFreeMemoryBlock(MemBufferBlock *memBufferBlock)
    {
        assert(memBufferBlock != nullptr);

        if (memBufferBlock->itemCount == 0 && !memBufferBlock->isLast) {
            assert(memBufferBlock->next != nullptr);

            // can delete & free
            auto *prev = memBufferBlock->prev;
            if (prev == nullptr) {
                assert(memBufferBlock == this->firstBufferBlock);

                this->firstBufferBlock = memBufferBlock->next;
                memBufferBlock->next->prev = nullptr;
            } else {
                prev->next = memBufferBlock->next;
                memBufferBlock->next->prev = prev;
            }

            free(memBufferBlock->chunk);
            free(memBufferBlock);
        }
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
    utf8byte *newText(unsigned int count) {
        auto bytes = st_size_of(utf8byte) * (count + 1); // 1 for null terminator
        return (utf8byte*)this->newBytesMem(bytes);
    }

    void *newBytesMem(unsigned int bytes) {
        // Align header so that the returned pointer is aligned to max_align_t.
        // The back-pointer (MemBufferBlock*) is stored immediately before the
        // returned address, and the header is padded to the platform alignment
        // so the data region starts on a proper boundary.
        static constexpr size_t ALIGN = alignof(std::max_align_t);
        static constexpr size_t HEADER =
            (sizeof(MemBufferBlock*) + ALIGN - 1) & ~(ALIGN - 1);

        // Round total length up to ALIGN so successive allocations stay aligned.
        auto length = (st_size)((bytes + HEADER + ALIGN - 1) & ~(ALIGN - 1));


        if (currentMemOffset + length < DEFAULT_BUFFER_SIZE) {

        }
        else {
            MemBufferBlock* tryDeleteBlock = nullptr;

            st_size assign_size = DEFAULT_BUFFER_SIZE < length ? length : DEFAULT_BUFFER_SIZE;
            if (firstBufferBlock == nullptr) {
                firstBufferBlock = currentBufferBlock = (MemBufferBlock*)malloc(sizeof(MemBufferBlock));
                firstBufferBlock->chunk = (void *)calloc(assign_size, 1);
                firstBufferBlock->prev = nullptr;
            }
            else {
                auto *newNode = (MemBufferBlock*)malloc(sizeof(MemBufferBlock));
                newNode->chunk = (void *)calloc(assign_size, 1);

                currentBufferBlock->next = newNode;
                newNode->prev = currentBufferBlock;
                if (currentBufferBlock->itemCount == 0) {
                    tryDeleteBlock = currentBufferBlock;
                }

                currentBufferBlock->isLast = false;
                currentBufferBlock = newNode;
            }

            currentBufferBlock->isLast = true;
            currentBufferBlock->itemCount = 0;
            currentBufferBlock->next = nullptr;

            currentMemOffset = 0;

            if (tryDeleteBlock) {
                this->tryFreeMemoryBlock(tryDeleteBlock);
            }
        }
        currentBufferBlock->itemCount++;
        void *node = (void*)((st_byte*)(currentBufferBlock->chunk) + currentMemOffset);

        // Store back-pointer right before the data region.
        auto **address = (MemBufferBlock **)((st_byte*)node + HEADER - sizeof(MemBufferBlock*));
        *address = currentBufferBlock;

        this->currentMemOffset += length;

        return (void*)((st_byte*)node + HEADER);
    }
};