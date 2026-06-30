#include <cstdio>

#include "parser.hpp"
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


    RAX(&reg) = 25;
    RAX(&reg) = RAX(&reg) + 35; // add 35 to RAX
    assert(RAX(&reg) == 60);
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
    stackMemory.freeAll();
}

void testStackMemoryMoveToFrom() {
    StackMemory stackMemory;
    stackMemory.init();

    stackMemory.push(0xFFFFFFFFFFFFFFFF);
    stackMemory.push(0xFFFFFFFFFFFFFFFF);

    uint32_t value = 0x12345678;
    stackMemory.moveToStack(-4, 4, (st_byte*)&value);

    uint32_t readValue = 0;
    stackMemory.moveFromStack(-4, 4, (st_byte*)&readValue);

    assert(value == readValue);
    stackMemory.freeAll();
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

    stackMemory.freeAll();
}

void testStackMemoryCallRet2 () {
    StackMemory stackMemory;
    stackMemory.init();

    auto *basePointer0 = stackMemory.stackBasePointer;
    auto *stackPointer0 = stackMemory.stackPointer;

    stackMemory.call();
    stackMemory.localVariables(8);
    assert((uint64_t)stackMemory.stackPointer == (uint64_t)stackMemory.stackBasePointer - 8);

    uint64_t a = 100;
    uint64_t b;
    // copy the value of a to the stack at offset -8 from the base pointer, then read it back into b
    stackMemory.moveToStack(-8, 8, (st_byte*)&a);
    stackMemory.moveFromStack(-8, 8, (st_byte*)&b);

    assert(100 == b);
    stackMemory.ret();

    assert((uint64_t)basePointer0 == (uint64_t)stackMemory.stackBasePointer);
    assert((uint64_t)stackPointer0 == (uint64_t)stackMemory.stackPointer);

    stackMemory.freeAll();
}


void testStackMemoryFuncCall() {
    /*
    Consider the following C code:
    int main(void) {
        foo(1, 2);
    }

    void foo(int a, int b) {
        return a + b;
    }
    */
    StackMemory stackMemory;
    stackMemory.init();

    auto* basePointer0 = stackMemory.stackBasePointer;
    auto* stackPointer0 = stackMemory.stackPointer;


    // call main()
    stackMemory.call();

    stackMemory.push(2); // push argument 2
    stackMemory.push(1); // push argument 1

    auto* stackPointer1 = stackMemory.stackPointer;
    auto* basePointer1 = stackMemory.stackBasePointer;
    
    // call foo()
    stackMemory.call();
    stackMemory.localVariables(8 * 2); // allocate space for local variables in func2

    uint64_t arg1 = 0, arg2 = 0;
    stackMemory.moveFromStack(8, 8, (st_byte*)&arg1); // get argument 1
    stackMemory.moveFromStack(16, 8, (st_byte*)&arg2); // get argument 2
    printf("arg1: %llu, arg2: %llu\n", (unsigned long long)arg1, (unsigned long long)arg2);
    assert(arg1 == 1);
    assert(arg2 == 2);
    stackMemory.returnValue = arg1 + arg2; // return a + b


    stackMemory.ret();
    // returned from foo()

    assert((uint64_t)basePointer1 == (uint64_t)stackMemory.stackBasePointer);
    assert((uint64_t)stackPointer1 == (uint64_t)stackMemory.stackPointer);

    assert(stackMemory.returnValue == 3);

    stackMemory.ret();
    // returned from main()

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

    stackMemory.localVariables(stackMemory.stackSize - stackMemory.baseBytes);
    assert(stackMemory.isOverflowed == false);
    stackMemory.localVariables(stackMemory.baseBytes);
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
    testStackMemoryFuncCall();
    testStackMemoryOverflowPush();
    testStackMemoryOverflowLocalVariables();
    testStackMemoryOverflowCall();
}
