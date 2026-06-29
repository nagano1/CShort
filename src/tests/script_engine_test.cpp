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

void mallocHeapTest() {
    ScriptEnv* env = ScriptEnv::newScriptEnv(nullptr);

    int* mem;
    for (int i = 0; i < 1024; i++) {
        mem = (int*)env->context->mallocHeapObject(sizeof(int));
        *mem = 53;
        if (i % 3 == 2) {
            env->context->freeHeapObject(mem);
        }
    }


    assert(*mem == 53);
    assert(env->context->memBufferForHeap.firstBufferBlock != env->context->memBufferForHeap.currentBufferBlock);

    ScriptEnv::deleteScriptEnv(env);
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



void testScript() {
            constexpr char source[] = R"(
fn Main()
{
    int b = 9
    int a = 500
    int c = 500
    
    return c * 2 - b * a
}
)";
        int ret = ScriptEnv::startScript(source);
        printf("ret 1 = %d\n", ret);
        assert(ret == -3500);
}

void testScript2() {
    constexpr const char expressionFirstAssignment[]  = u8R"(
fn Main() {
    int b = 9
    b = 5 + (10 + 1) - 2
    return b
}
    )";

    int ret = ScriptEnv::startScript(expressionFirstAssignment);
    printf("ret = %d\n", ret);
    assert(ret == 14);

}


void testHeapString() {
            constexpr char source[] = R"(
fn Main()
{
    string *ptr = "ijfowjio"
    String *ptr2 = ptr
    return ptr2
}
)";
        int ret = ScriptEnv::startScript(source);
        printf("ret = %d\n", ret);
        assert(ret != 0);
}

void testNull() {
            constexpr char source[] = R"(
fn Main()
{
    ?int *ptr = null
    #int ok = 3421
    return ptr
}
)";
        int ret = ScriptEnv::startScript(source);
        assert(ret == 0);
}

void testVarable() {
            constexpr char source[] = R"(
fn Main()
{
    let b = 9
    int c = b
    
    return c + b
}
)";
        int ret = ScriptEnv::startScript(source);
        printf("ret = %d\n", ret);
        assert(ret == 18);
}

void testi64 () {
            constexpr char source[] = R"(
fn Main()
{
    i64 b = 5L
    i64 a = 900L
    i64 c = 901L
    
    return c - (a + b)
}
)";
        ScriptEnv* env = ScriptEnv::loadScript((char*)source, sizeof(source) - 1);
        if (env->document->context->syntaxErrorInfo.hasError) {
            assert(false);
            return;// env->document->context->syntaxErrorInfo.errorItem.errorId;
        }

        env->document->context->typeManager->validateScript();
        if (env->context->semanticErrorInfo.hasError) {
            assert(false);
            env->context->setErrorPositions();
            return;// env->context->semanticErrorInfo.firstErrorItem->codeErrorItem.errorId;
        }
        auto* node = env->mainFunc->bodyNode.firstChildNode;
        while (node) {
            if (node->vtable == VTables::ReturnStatementVTable) {
                auto returnState = Cast::downcast<ReturnStatementNodeStruct*>(node);
                assert(returnState->expressionNode->vtable == VTables::BinaryOperationVTable);
                
                auto binary0 = Cast::downcast<BinaryOperationNodeStruct*>(returnState->expressionNode);

                auto *c = Cast::downcast<IdentifiersAccessNodeStruct*>(binary0->leftExprNode);

                auto pare = Cast::downcast<ParenthesesNodeStruct*>(binary0->rightExprNode);
                assert(pare->valueNode->vtable == VTables::BinaryOperationVTable);
                auto binary = Cast::downcast<BinaryOperationNodeStruct*>(pare->valueNode);
                auto *a = Cast::downcast<IdentifiersAccessNodeStruct*>(binary->leftExprNode);
                auto *b = Cast::downcast<IdentifiersAccessNodeStruct*>(binary->rightExprNode);

                // return c - (a + b)
                assert(c->calcRegEnum == GRPRegisterEnum::ebx);

                assert(a->calcRegEnum == GRPRegisterEnum::eax);
                assert(b->calcRegEnum == GRPRegisterEnum::ebx);
                
                assert(binary->calcRegEnum == GRPRegisterEnum::ecx);
                assert(binary0->rightExprNode->calcRegEnum == GRPRegisterEnum::ecx);

                // stack 
                assert(c->stackOffset == -24);
                assert(a->stackOffset == -16);
                assert(b->stackOffset == -8);
                assert(env->mainFunc->stackSize == 24);
            }
            
            node = node->nextNode;
        }
        int ret = env->runScript();
        assert(ret == -4);
}


#define CheckTextEq(x) checkTextEquality(#x, x)
void callTests()
{
    testCPURegister();
    mallocHeapTest();
    testStackMemoryPushPopTest();
    testStackMemoryMoveToFrom();
    testStackMemoryCallRet();
    testStackMemoryCallRet2();
    testStackMemoryOverflowPush();
    testStackMemoryOverflowLocalVariables();
    testStackMemoryOverflowCall();
    testHeapString();
    testNull();
    testScript();
    testScript2();
    testVarable();
    testi64();
    testStackMemoryFuncCall();
    testStackMemoryOverflowPush();
    testStackMemoryOverflowLocalVariables();
    testStackMemoryOverflowCall();
}
