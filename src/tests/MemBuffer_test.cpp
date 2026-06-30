#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4267) // conversion from 'size_t' to 'int', possible loss of data
#endif

#include <cstdint>
#include <cstring>

#include "common.hpp"
#include "parser.hpp"

using namespace cshort;

void testNewTextHasNullTerminator()
{
    MemBuffer buffer;
    buffer.init();

    auto *text = buffer.newText(4);
    std::memcpy(text, "test", 4);

    assert(text[4] == '\0');

    buffer.freeAll();
}

void testAllocationsReuseCurrentBlock()
{
    MemBuffer buffer;
    buffer.init();

    auto *firstValue = buffer.newMem<int>(1);
    auto *firstBlock = buffer.currentBufferBlock;
    auto *secondValue = buffer.newMem<int>(1);

    assert(firstValue != nullptr);
    assert(secondValue != nullptr);
    assert(buffer.currentBufferBlock == firstBlock);
    assert(buffer.firstBufferBlock == firstBlock);
    assert(firstBlock->itemCount == 2);

    buffer.freeAll();
}

void testDeletedNonLastBlockIsReleased()
{
    MemBuffer buffer;
    buffer.init();

    auto *largeText = buffer.newText(MemBuffer::DEFAULT_BUFFER_SIZE);
    auto *firstBlock = buffer.firstBufferBlock;
    auto *smallValue = buffer.newMem<int>(1);
    auto *secondBlock = buffer.currentBufferBlock;

    assert(largeText != nullptr);
    assert(smallValue != nullptr);
    assert(firstBlock != secondBlock);
    assert(firstBlock->next == secondBlock);
    assert(secondBlock->prev == firstBlock);

    buffer.tryDelete(largeText);

    assert(buffer.firstBufferBlock == secondBlock);
    assert(buffer.currentBufferBlock == secondBlock);
    assert(secondBlock->prev == nullptr);
    assert(secondBlock->itemCount == 1);

    buffer.freeAll();
}

void testLastBlockStaysAvailableAfterDelete()
{
    MemBuffer buffer;
    buffer.init();

    auto *value = buffer.newMem<int>(1);
    auto *block = buffer.currentBufferBlock;

    buffer.tryDelete(value);

    assert(buffer.firstBufferBlock == block);
    assert(buffer.currentBufferBlock == block);
    assert(block->itemCount == 0);
    assert(block->isLast == true);

    auto *nextValue = buffer.newMem<std::max_align_t>(1);
    assert(nextValue != nullptr);
    assert(buffer.currentBufferBlock == block);
    assert(reinterpret_cast<std::uintptr_t>(nextValue) % alignof(std::max_align_t) == 0);

    buffer.freeAll();
}


void testMallocHeapEntry() {
    MemBuffer memBufferForHeap{};
    memBufferForHeap.initWithHeapEntryEnabled();

    int* mem;
    for (int i = 0; i < 1024; i++) {
        mem = (int*)memBufferForHeap.mallocHeapEntry(sizeof(int));
        assert(mem != nullptr);
        *mem = 53;
        if (i % 3 == 2) {
            memBufferForHeap.freeHeapEntry(mem);
        }
    }


    assert(*mem == 53);
    assert(memBufferForHeap.firstBufferBlock != memBufferForHeap.currentBufferBlock);

    memBufferForHeap.freeAllHeapEntries();
    memBufferForHeap.freeAll();
}

void testMallocHeapEntrySmallAndLargeFree() {
    MemBuffer memBufferForHeap{};
    memBufferForHeap.initWithHeapEntryEnabled();

    constexpr size_t ALIGN = alignof(std::max_align_t);
    constexpr size_t HEAP_HEADER_SIZE = (sizeof(HeapEntry*) + ALIGN - 1) & ~(ALIGN - 1);

    auto *smallMem = (char*)memBufferForHeap.mallocHeapEntry(1);
    auto *largeMem = (char*)memBufferForHeap.mallocHeapEntry(64 * 1024);

    assert(smallMem != nullptr);
    assert(largeMem != nullptr);

    smallMem[0] = 's';
    largeMem[0] = 'L';
    largeMem[(64 * 1024) - 1] = 'E';

    assert(smallMem[0] == 's');
    assert(largeMem[0] == 'L');
    assert(largeMem[(64 * 1024) - 1] == 'E');

    auto *smallEntry = *(HeapEntry**)((char*)smallMem - HEAP_HEADER_SIZE);
    auto *largeEntry = *(HeapEntry**)((char*)largeMem - HEAP_HEADER_SIZE);
    assert(smallEntry != nullptr);
    assert(largeEntry != nullptr);
    assert(!smallEntry->freed);
    assert(!largeEntry->freed);

    memBufferForHeap.freeHeapEntry(smallMem);
    assert(smallEntry->freed);
    memBufferForHeap.freeHeapEntry(nullptr);

    memBufferForHeap.freeHeapEntry(largeMem);
    assert(largeEntry->freed);

    memBufferForHeap.freeAllHeapEntries();
    memBufferForHeap.freeAll();
}


void testHashMap() {
    {
        auto hashKey = VoidHashMap::calc_hash_x("ak", 10000);
        auto hashKey2 = VoidHashMap::calc_hash_x("ka", 10000);
        assert(hashKey != hashKey2);
    }

    {
        auto hashKey = VoidHashMap::calc_hash_x("N01", 10000);
        auto hashKey2 = VoidHashMap::calc_hash_x("N01234C", 10000);
        assert(hashKey != hashKey2);
    }


    auto *context = (ParseContext *)malloc(sizeof(ParseContext));
    if (context != nullptr) {
        context->init();
    }

    for (int i = 0; i < 100; i++) {

        auto *hashMap = context->newMem<VoidHashMap>();
        if (hashMap != nullptr) {
            hashMap->init(&context->memBuffer);
        }

        auto *firstItem = (void*)(context->newMem<DocumentStruct>());
        const char firstItemKey[] = "firstAA";
        const char secondItemKey[] = "secondB";
        const char thirdItemKey[] = "thirdC";
        hashMap->put_x(firstItemKey, context->newMem<DocumentStruct>());
        hashMap->put_x(firstItemKey, firstItem); // replace

        hashMap->put_x(secondItemKey, context->newMem<DocumentStruct>());
        hashMap->put_x(thirdItemKey, context->newMem<DocumentStruct>());
        hashMap->put_x(thirdItemKey, context->newMem<DocumentStruct>());

        auto *node = (NodeBase*)hashMap->get_x(firstItemKey);
        assert(node == firstItem);

        assert(hashMap->hasKey(firstItemKey, sizeof(firstItemKey) - 1));
        hashMap->deleteKey(firstItemKey, sizeof(firstItemKey) - 1);
        assert(hashMap->get_x(firstItemKey) == nullptr);

        node = (NodeBase*)hashMap->get_x(thirdItemKey);
        assert(node != nullptr);
        
        {
            auto *node2 = hashMap->get_x("empty");
            assert(node2 == nullptr);
        }
    }
    context->dispose();
    free(context);
}

int main()
{
    testNewTextHasNullTerminator();
    testAllocationsReuseCurrentBlock();
    testDeletedNonLastBlockIsReleased();
    testLastBlockStaysAvailableAfterDelete();
    testMallocHeapEntry();
    testMallocHeapEntrySmallAndLargeFree();

    testHashMap();
    return 0;
}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
