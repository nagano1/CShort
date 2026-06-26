#include <cstdio>

#include "code_nodes.hpp"
#include "ParseUtil.hpp"
#include "script_runtime.hpp"

using namespace cshort;

void callTests();

int main()
{
    printf("-------------------- script engine tests ---------------------\n");
    fflush(stdout);

    callTests();

    printf("-------------------- passed successfully ---------------------\n");
    return 0;
}

void testCPURegister() {
    CPUSim reg{};
    assert(EAX(&reg) == 0);
    assert(sizeof(reg.rax) == 8);
    assert(sizeof(bool) == 1);

    RAX(&reg) = 0xFFFFFFFFFFFFFFFF;
    AX(&reg) = 0;
    assert(RAX(&reg) == 0xFFFFFFFFFFFF0000);


    RAX(&reg) = 0xFFFFFFFFFFFFFFFF;
    EAX(&reg) = 0x0;
    assert(RAX(&reg) == 0xFFFFFFFF00000000);


    EAX(&reg) = 0xFFFFFFFF;
    AX(&reg) = 0;
    assert(EAX(&reg) == 0xFFFF0000);


    AX(&reg) = 0xFFFF;
    AL(&reg) = 0x00;
    assert(AX(&reg) == 0xFF00);
}

void testStackMemoryPushPopTest() {
    StackMemory stackMemory;
    stackMemory.init();
    auto* stackPointer1 = stackMemory.stackPointer;

    stackMemory.push(5);
    stackMemory.push(6);

    assert((uint64_t)stackPointer1 != (uint64_t)stackMemory.stackPointer);

    assert(6 == stackMemory.pop());
    assert(5 == stackMemory.pop());

    assert((uint64_t)stackPointer1 == (uint64_t)stackMemory.stackPointer);
}

void testStackMemoryMoveToFrom() {
    StackMemory stackMemory;
    stackMemory.init();

    uint32_t value = 0x12345678;
    stackMemory.moveTo(-4, 4, (st_byte*)&value);

    uint32_t readValue = 0;
    stackMemory.moveFrom(-4, 4, (st_byte*)&readValue);

    assert(value == readValue);
}

void testStackMemoryCallRet() {

    StackMemory stackMemory;
    stackMemory.init();

    auto* stackPointer1 = stackMemory.stackPointer;
    auto* stackBasePointer1 = stackMemory.stackBasePointer;

    stackMemory.call();

    assert((uint64_t)stackPointer1 != (uint64_t)stackMemory.stackPointer);
    assert((uint64_t)stackBasePointer1 != (uint64_t)stackMemory.stackBasePointer);

    stackMemory.ret();

    assert((uint64_t)stackPointer1 == (uint64_t)stackMemory.stackPointer);
    assert((uint64_t)stackBasePointer1 == (uint64_t)stackMemory.stackBasePointer);
}

void testStackMemoryCallRet2 () {
    StackMemory stackMemory;
    stackMemory.init();

    auto *basePointer0 = stackMemory.stackBasePointer;
    auto *stackPointer0 = stackMemory.stackPointer;

    stackMemory.call();
    stackMemory.localVariables(8*4);

    //stackMemory.localVariables(1);
    uint32_t a = 100;
    uint32_t b;
    stackMemory.moveTo(-4, 4, (st_byte*)&a);
    stackMemory.moveFrom(-4, 4, (st_byte*)&b);
    assert(100 == b);
    stackMemory.ret();

    assert((uint64_t)basePointer0 == (uint64_t)stackMemory.stackBasePointer);
    assert((uint64_t)stackPointer0 == (uint64_t)stackMemory.stackPointer);

}


void testStackMemoryCallRet3() {
    StackMemory stackMemory;
    stackMemory.init();

    auto* basePointer0 = stackMemory.stackBasePointer;
    auto* stackPointer0 = stackMemory.stackPointer;

    // call func1
    stackMemory.call();

    stackMemory.push(8);
    auto* stackPointer1 = stackMemory.stackPointer;
    auto* basePointer1 = stackMemory.stackBasePointer;


    // call func2
    stackMemory.call();

    stackMemory.localVariables(8 * 4);

    stackMemory.ret();
    // returned from func2

    assert((uint64_t)basePointer1 == (uint64_t)stackMemory.stackBasePointer);
    assert((uint64_t)stackPointer1 == (uint64_t)stackMemory.stackPointer);

    stackMemory.ret();
    // returned from func1

    assert((uint64_t)basePointer0 == (uint64_t)stackMemory.stackBasePointer);
    assert((uint64_t)stackPointer0 == (uint64_t)stackMemory.stackPointer);
}

void testStackMemoryOverflowPush() {
   StackMemory stackMemory;
   stackMemory.init();

   for (int i = 0; i < stackMemory.stackSize/stackMemory.baseBytes - 1; i++) {
       stackMemory.push(5);
   }
   assert(stackMemory.isOverflowed == false);
   stackMemory.push(5);

   assert(stackMemory.isOverflowed == true);
}

void testStackMemoryOverflowLocalVariables() {
    StackMemory stackMemory;
    stackMemory.init();

    stackMemory.localVariables(stackMemory.stackSize - 1);
    assert(stackMemory.isOverflowed == false);
    stackMemory.localVariables(1);
    assert(stackMemory.isOverflowed == true);
}

void testStackMemoryOverflowCall() {
    StackMemory stackMemory;
    stackMemory.init();

    for (int i = 0; i < stackMemory.stackSize / stackMemory.baseBytes - 1; i++) {
        stackMemory.call();
    }
    assert(stackMemory.isOverflowed == false);
    stackMemory.call();

    assert(stackMemory.isOverflowed == true);
}

#define CheckTextEq(x) checkTextEquality(#x, x)
void callTests()
{
    testCPURegister();
    testStackMemoryPushPopTest();
    testStackMemoryMoveToFrom();
    testStackMemoryCallRet();
    testStackMemoryCallRet2();
    testStackMemoryCallRet3();
    testStackMemoryOverflowPush();
    testStackMemoryOverflowLocalVariables();
    testStackMemoryOverflowCall();
}
