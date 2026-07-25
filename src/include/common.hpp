#pragma once

#include <iostream>
#include <condition_variable>
#include <array>

#include <string.h>
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



/**
 * BinaryDataBuilder - A utility struct for building binary data in memory.
 *
 * Appends byte data to a dynamically growing internal buffer.
 * The buffer grows by a fixed increment when it is exhausted.
 */
struct BinaryDataBuilder{
    uint8_t *data = nullptr;      /* Pointer to the internal buffer */
    size_t   size = 0;      /* Number of bytes currently written */
    size_t   capacity = 0;  /* Total allocated capacity in bytes */

    /**
     * Initialise a BinaryDataBuilder with an initial capacity.
     * Returns true on success, false if memory allocation failed.
     */
    bool init(size_t initial_capacity);

    /**
     * Release all resources held by the builder.
     * After this call the struct should not be used without re-initialising.
     */
    void freeAll();

    /**
     * Append a single byte.
     * Returns true on success, false on allocation failure.
     */
    bool append_byte(uint8_t byte);

    /**
     * Append a raw buffer of `len` bytes.
     * Returns true on success, false on allocation failure.
     */
    bool append_bytes(const uint8_t *src, size_t len);

    /**
     * Append an unsigned 16-bit integer in little-endian order.
     */
    bool append_u16le(uint16_t value);

    /**
     * Append an unsigned 32-bit integer in little-endian order.
     */
    bool append_u32le(uint32_t value);

    /**
     * Append an unsigned 64-bit integer in little-endian order.
     */
    bool append_u64le(uint64_t value);

    /**
     * Reset the builder (size → 0) without freeing the buffer,
     * so the buffer can be reused without a new allocation.
     */
    void reset();
} ;

/* Default initial capacity (when initial_capacity == 0) and growth step (bytes) */

#define BINARY_DATA_BUILDER_DEFAULT_CAPACITY 2048

struct StringBuilder {
    utf8byte *str = nullptr;
    size_t currentStrLength = 0;
    size_t currentCapacity = 0;

    void initWithInitialCapacity(size_t initialCapacity) {
        assert(currentCapacity == 0 && str == nullptr); // ensure not initialized yet
        assert(initialCapacity > 0);

        currentStrLength = 0;
        str = (utf8byte *)malloc(initialCapacity);
        if (str == nullptr) {
            currentCapacity = 0;
            return; // allocation failed, do nothing
        }
        currentCapacity = initialCapacity;
        str[0] = '\0';
    }

    void freeAll() {
        if (str != nullptr) {
            free(str);
            str = nullptr;
            currentStrLength = 0;
            currentCapacity = 0;
        }
    }

    template<std::size_t SIZE>
    void appendWithAutoLength(const char (&text)[SIZE]) {
        appendImpl(text, SIZE - 1);
    }
    void append(const char *text) {
        appendImpl(text, strlen(text));
    }

    static constexpr size_t CapacityGrowthAmount = 200;
    void appendImpl(const char *text, size_t length) {
        if (text == nullptr || length == 0) {
            assert(false && "text is nullptr or length is 0"); // if length is not 0, text should not be nullptr
            return; // nothing to append
        }

        size_t needed = currentStrLength + length + 1; // +1 for null terminator
        if (needed < currentStrLength) {
            return; // overflow, do nothing
        }

        if (needed > currentCapacity) {
            size_t newCapacity = needed + CapacityGrowthAmount;
            if (newCapacity < needed) {
                return; // overflow, do nothing
            }

            utf8byte *newStr = (utf8byte *)malloc(newCapacity);
            if (newStr == nullptr) {
                return; // allocation failed, do nothing
            }

            if (str != nullptr) {
                if (currentStrLength > 0) {
                    memcpy(newStr, str, currentStrLength);
                }
                free(str);
            }

            str = newStr;
            currentCapacity = newCapacity;
            str[currentStrLength] = '\0';
        }

        memcpy(str + currentStrLength, text, length);
        str[currentStrLength + length] = '\0';
        currentStrLength += length;
    }

    const char *c_str() const {
        assert(str != nullptr && "StringBuilder is not initialized or has been freed");
        return str;
    }

    size_t length() const {
        return currentStrLength;
    }
};

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
    void _freeAllHeapEntries();// this is called in freeAll() when heap mode is enabled, so this is kinda private, but it is public for testing purposes.
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