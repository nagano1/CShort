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

#include "common.hpp"

void MemBuffer::init() {
    this->currentMemOffset = DEFAULT_BUFFER_SIZE + 1;
    this->firstBufferBlock = nullptr;
    this->currentBufferBlock = nullptr;
}

void MemBuffer::initWithHeapEntryEnabled() {
    this->init();

    this->isHeapEntryEnabled = true;
}

void MemBuffer::freeAll() {
    MemBufferBlock *bufferList = this->firstBufferBlock;

    while (bufferList != nullptr) {
        free(bufferList->chunk);

        auto *temp = bufferList;
        bufferList = bufferList->next;
        free(temp);
    }
}



void MemBuffer::tryFreeMemoryBlock(MemBufferBlock *memBufferBlock)
{
    if (memBufferBlock == nullptr) {
        return;
    }

    if (memBufferBlock->itemCount == 0 && !memBufferBlock->isLast) {
        assert(memBufferBlock->next != nullptr);

        // can delete & free
        auto *prev = memBufferBlock->prev;
        if (prev == nullptr) {
            assert(memBufferBlock == this->firstBufferBlock);

            this->firstBufferBlock = memBufferBlock->next;
            memBufferBlock->next->prev = nullptr;
        }
        else {
            prev->next = memBufferBlock->next;
            memBufferBlock->next->prev = prev;
        }

        if (memBufferBlock == this->currentBufferBlock) {
            this->currentBufferBlock = prev;
            if (this->currentBufferBlock == nullptr) {
                // all blocks are freed, reset to initial state
                this->currentMemOffset = DEFAULT_BUFFER_SIZE + 1;
            }
        }

        free(memBufferBlock->chunk);
        free(memBufferBlock);
    }
}


utf8byte * MemBuffer::newText(unsigned int count) {
    auto bytes = st_size_of(utf8byte) * (count + 1); // 1 for null terminator
    return (utf8byte*)this->newBytesMem(bytes);
}
utf8byte * MemBuffer::newTextAssign(const utf8byte *text, unsigned int count) {
    assert(text != nullptr || count == 0);
    auto bytes = st_size_of(utf8byte) * (count + 1); // 1 for null terminator
    utf8byte *newText = (utf8byte*)this->newBytesMem(bytes);
    for (unsigned int i = 0; i < count; i++) {
        newText[i] = text[i];
    }
    newText[count] = '\0';
    return newText;
}


// Align header so that the returned pointer is aligned to max_align_t.
// The back-pointer (MemBufferBlock*) is stored immediately before the
// returned address, and the header is padded to the platform alignment
// so the data region starts on a proper boundary.
static constexpr size_t ALIGN = alignof(std::max_align_t);
static constexpr size_t HEADER = (sizeof(MemBufferBlock*) + ALIGN - 1) & ~(ALIGN - 1);

void *MemBuffer::newBytesMem(unsigned int bytes) {
    // Round total length up to ALIGN so successive allocations stay aligned.
    auto length = (st_size)((bytes + HEADER + ALIGN - 1) & ~(ALIGN - 1));
    int padding_tail_size = isHeapEntryEnabled ? HEADER : 0;
    if (currentMemOffset + length < DEFAULT_BUFFER_SIZE) {

    }
    else {
        MemBufferBlock* tryDeleteBlock = nullptr;
        st_size assign_size = DEFAULT_BUFFER_SIZE < length ? length : DEFAULT_BUFFER_SIZE;
        assign_size += padding_tail_size; // add padding size to the end of the buffer block to ensure that the next pointer is  invalid and not accidentally pointing to a valid memory address.

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

        // If the previous block has no items, we can free it to save memory.
        this->tryFreeMemoryBlock(tryDeleteBlock);
    }
    currentBufferBlock->itemCount++;
    void *node = (void*)((st_byte*)(currentBufferBlock->chunk) + currentMemOffset);

    // Store back-pointer right before the data region.
    auto **address = (MemBufferBlock **)((st_byte*)node + HEADER - sizeof(MemBufferBlock*));
    *address = currentBufferBlock;

    this->currentMemOffset += length;

    if (isHeapEntryEnabled) {
        // Zero out the padding bytes at end of current void* to ensure that the next pointer is invalid and not accidentally pointing to a valid memory address.
        for (st_size i = 0; i < padding_tail_size; i++) {
            ((char*)this->currentBufferBlock->chunk + currentMemOffset)[i] = 0;
        }
    }

    return (void*)((st_byte*)node + HEADER);
}

// ----------------------------------------------------------------
//
//                    MemBuffer Extension : heap malloc
//
// ----------------------------------------------------------------

/*

MemBufferBlock
-------------------------------------------------
|MemBufferBlock*|HeapEntry|MemBufferBlock*|HeapEntry|...
-------------------------------------------------
*/


static constexpr size_t HEAP_HEADER_SIZE = (sizeof(HeapEntry*) + ALIGN - 1) & ~(ALIGN - 1);

// allocation methods of heap objects in running script, which will be freed all together
// after script execution finishes even if user couldn't handle freeing the objects in their script.
void* MemBuffer::mallocHeapEntry(int bytes) {
    auto *heapEntry = this->newMem<HeapEntry>(1);
    heapEntry->freed = false;

    heapEntry->ptr = malloc(bytes + HEAP_HEADER_SIZE); // allocate extra space for back-pointer

    // Store a back-pointer to the HeapEntry struct immediately before the allocated memory.
    HeapEntry** addressPtr = (HeapEntry**)heapEntry->ptr;
    *addressPtr = heapEntry;

    return ((char*)heapEntry->ptr) + HEAP_HEADER_SIZE;
}

void MemBuffer::freeHeapEntry(void *ptr) {
    // Retrieve the back-pointer to the HeapEntry struct, which is stored immediately before the allocated memory.
    HeapEntry *item = *(HeapEntry**)((char*)ptr - HEAP_HEADER_SIZE);
    assert(item != nullptr);
    if (!item->freed) { // prevent double free
        free(item->ptr);
        item->freed = true;
    }
    this->tryDelete<HeapEntry>(item);
}

// Free all heap entries that have not been freed yet.
void MemBuffer::freeAllHeapEntries() {
    auto *block = this->firstBufferBlock;
    while (block != nullptr) {
        if (block->itemCount > 0) {
            int currentOffset = 0;
            // this strategy can be used only for same size items
            while (currentOffset < DEFAULT_BUFFER_SIZE) {
                MemBufferBlock* blockRef = *(MemBufferBlock**)((char*)block->chunk + currentOffset + HEADER - sizeof(MemBufferBlock*));
                if (blockRef == block) { // found an item that belongs to this block
                    HeapEntry* heapEntry = (HeapEntry*)((char*)block->chunk + currentOffset + HEADER);
                    if (!heapEntry->freed) {
                        free(heapEntry->ptr);
                        heapEntry->freed = true;
                    }
                    currentOffset += HEADER + sizeof(HeapEntry);
                }
                else {
                    // reached the end of items in this block
                    break;
                }
            }
        }
        block = block->next;
    }
}