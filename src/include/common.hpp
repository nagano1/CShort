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
#include <type_traits>


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
        assert(!isHeapEntryEnabled || (std::is_same<Type, HeapEntry>::value)); // only HeapEntry is allowed when heap mode is enabled
        auto bytes = st_size_of(Type) * count;
        return (Type*)this->newBytesMem(bytes);
    }
    
    utf8byte *newText(unsigned int count);
    utf8byte *newTextAssign(const utf8byte *text, unsigned int count);

    void *newBytesMem(unsigned int bytes);

    // Extension for heap malloc: objects can be freed all together after script execution finishes, simplifying memory management in the script engine.
    void* mallocHeapEntry(int bytes);
    void freeHeapEntry(void *ptr);
    void freeAllHeapEntries();
};


#define HashNode_TABLE_SIZE 104

struct VoidHashNode {
    VoidHashNode *next;
    char *key;
    int keyLength;
    void *voidPtrItem;
};

struct VoidHashMap {
    // entries is an array of pointers to the first node in each linked list (bucket) of the hash table.
    VoidHashNode **entries;
    size_t entries_length; // number of entries in the hash table
    MemBuffer *memBuffer;

    void init(MemBuffer *memBuffer1);

    template<std::size_t SIZE>
    static int calc_hash_x(const char(&f4)[SIZE], size_t max) {
        return VoidHashMap::calc_hash_impl(f4, static_cast<int>(SIZE - 1), max);
    }
    int calc_hash(const char *key, int keyLength) {
        return VoidHashMap::calc_hash_impl(key, keyLength, this->entries_length);
    }
    // Calculate the hash value of a key, given the key and its length,
    // and the maximum value for the hash (usually the size of the hash table).
    static int calc_hash_impl(const char *key, int keyLength, size_t max);
    
    void put(const char *keyA, int keyLength, void *val);
    void *get(const char *key, int keyLength);
    bool hasKey(const char *key, int keyLength);
    void deleteKey(const char *key, int keyLength);

    template<std::size_t SIZE>
    void *get_x(const char(&f4)[SIZE]) {
        return this->get(f4, static_cast<int>(SIZE - 1));
    }
    template<std::size_t SIZE>
    void put_x(const char(&f4)[SIZE], void *val) {
        this->put(f4, static_cast<int>(SIZE - 1), val);
    }
};