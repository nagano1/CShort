#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4267) // conversion from 'size_t' to 'int', possible loss of data
#endif

#include <cstdint>
#include <cstring>

#include "common.hpp"

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
    //assert(reinterpret_cast<std::uintptr_t>(nextValue) % alignof(std::max_align_t) == 0);

    buffer.freeAll();
}

int main()
{
    testNewTextHasNullTerminator();
    testAllocationsReuseCurrentBlock();
    testDeletedNonLastBlockIsReleased();
    testLastBlockStaysAvailableAfterDelete();
    return 0;
}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
