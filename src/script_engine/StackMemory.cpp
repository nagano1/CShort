#define _CRT_SECURE_NO_WARNINGS

#include <cstdio>
#include <iostream>
#include <string>
#include <array>
#include <algorithm>
#include <cinttypes>
#include <cstring>

#include <cstdlib>
#include <cassert>
#include <cstdio>
#include <chrono>
#include <unordered_map>
#include <vector>

#include <cstdint>
#include <ctime>
#include <cstdint>

#include "script_runtime.hpp"

namespace cshort {

    //------------------------------------------------------------------------------------------
    //
    //                                       StackMemory
    //
    //------------------------------------------------------------------------------------------

    void StackMemory::init()
    {
        // size of the basic unit of data that can be pushed onto the stack.
        this->baseBytes = 8; // 8 bytes for 64-bit architecture, 4 bytes for 32-bit architecture.
        // This is used for alignment of local variables, return address, base pointer, and arguments.

        this->isOverflowed = false;

        this->stackSize = 2 * 1024 * 1024; // 2MB
        this->stackChunk = (st_byte*)malloc(this->stackSize);
         if (this->stackChunk == nullptr) {
             this->isOverflowed = true;
             this->stackPointer = nullptr;
             this->stackBasePointer = nullptr;
             return;
         }
         this->returnValue = 0;

        // stack grows downwards, so the initial stack pointer is at the end of the allocated stackChunk.
        this->stackPointer = this->stackChunk + this->stackSize;
        // stack base pointer is used to keep track of the base of the current stack frame.
        // in function calls, the base pointer is typically saved and restored to manage local variables and function arguments.
        this->stackBasePointer = this->stackChunk + this->stackSize;
    }

    void StackMemory::freeAll()
    {
        if (this->stackChunk != nullptr) {
            free(this->stackChunk);
            this->stackChunk = nullptr;
        }
    }
    
    void StackMemory::push(uint64_t data)
    {
        if (this->stackPointer - this->baseBytes <= this->stackChunk) {
            overflowed();
            return;
        }
        this->stackPointer -= this->baseBytes;
        // store the value at the current stack pointer location
        *(uint64_t*)this->stackPointer = data;
    }

    // this method is used for allocating space for local variables on the stack.
    void StackMemory::localVariables(int bytes)
    {
        assert(bytes % this->baseBytes == 0); // ensure that the requested bytes is a multiple of baseBytes for alignment
        if (this->stackPointer - bytes <= this->stackChunk) {
            overflowed();
            return;
        }

        this->stackPointer -= bytes;
    }

    // pop retrieves the value at the current stack pointer location and
    // then increments the stack pointer to effectively remove that value from the stack.
    uint64_t StackMemory::pop()
    {
        if (this->stackChunk == nullptr || this->stackPointer >= this->stackChunk + this->stackSize) {
            overflowed();
            return 0;
        }
        uint64_t data = *(uint64_t*)this->stackPointer;
        this->stackPointer += this->baseBytes;
        return data;
    }

    // moveTo copies data from the provided pointer to the stack at the specified offset from the base pointer.
    // this is used for writing values to the stack, such as local variables or function arguments. 
    void StackMemory::moveTo(int offsetFromBase, int byteCount, st_byte*ptr)
    {
        if (this->stackBasePointer + offsetFromBase <= this->stackChunk) {
            overflowed();
            return;
        }

        // copy the data from the provided pointer to the stack at the specified offset
        // from the base pointer.
        if (byteCount == 1) { // 8bit
            *(uint8_t*)(this->stackBasePointer + offsetFromBase) = *(uint8_t*)ptr;
        }
        else if (byteCount == 2) { // 16bit
            *(uint16_t*)(this->stackBasePointer + offsetFromBase) = *(uint16_t*)ptr;
        }
        else if (byteCount == 4) { // 32bit
            auto vl = *(uint32_t*)ptr;
            *(uint32_t*)(this->stackBasePointer + offsetFromBase) = vl;//*(uint32_t*)ptr;
        }
        else if (byteCount == 8) { // 64bit
            *(uint64_t*)(this->stackBasePointer + offsetFromBase) = *(uint64_t*)ptr;
        }
        else {
            // for larger data sizes, use memcpy to copy the data from the provided pointer
            // to the stack.
            memcpy(this->stackBasePointer + offsetFromBase, ptr, byteCount);
        }
    }

    // moveFrom retrieves data from the stack at the specified offset from the base pointer
    // and copies it to the provided pointer.
    // this is used for reading values from the stack, such as local variables or function arguments without popping them.
    void StackMemory::moveFrom(int offsetFromBase, int byteCount, st_byte* ptr) const
    {
        auto* target = this->stackBasePointer + offsetFromBase;
        auto* stackEnd = this->stackChunk + this->stackSize;
        // ensure that the stackChunk is not null, the byteCount is positive, and the target address is within the valid stack range.
        assert(this->stackChunk != nullptr);
        assert(byteCount > 0);
        printf("stackPointer: %p, target: %p, stackEnd: %p\n", this->stackPointer, target, stackEnd);
        assert(this->stackPointer > target);
        assert(target >= this->stackChunk);
        assert(target + byteCount <= stackEnd);

        if (byteCount == 1) { // 8bit
            *(uint8_t*)(ptr) = *(uint8_t*)(this->stackBasePointer + offsetFromBase);
        }
        else if (byteCount == 2) { // 16bit
            *(uint16_t*)(ptr) = *(uint16_t*)(this->stackBasePointer + offsetFromBase);
        }
        else if (byteCount == 4) { // 32bit
            auto vl = *(uint32_t*)(this->stackBasePointer + offsetFromBase);
            *(uint32_t*)(ptr) = vl;//*(uint32_t*)(this->stackBasePointer + offsetFromBase);
        }
        else if (byteCount == 8) { // 64bit
            *(uint64_t*)(ptr) = *(uint64_t*)(this->stackBasePointer + offsetFromBase);
        }
        else {
            // for larger data sizes, use memcpy to copy the data from the stack to the provided pointer
            memcpy(ptr, this->stackBasePointer + offsetFromBase, byteCount);
        }
    }

    // call simulates a function call by saving the current base pointer onto
    // the stack and updating the base pointer to the current stack pointer.
    void StackMemory::call()
    {
        // save the current base pointer onto the stack
        this->push((uint64_t)this->stackBasePointer);

        if (this->isOverflowed) {
            return;
        }
        // update the base pointer to the current stack pointer, establishing a new stack frame for the called function.
        this->stackBasePointer = this->stackPointer;
    }

    // ret simulates returning from a function call by restoring the base pointer
    // from the stack and resetting the stack pointer to the base pointer.
    void StackMemory::ret()
    {
        this->stackPointer = this->stackBasePointer;
        this->stackBasePointer = (st_byte*)this->pop();
    }
}