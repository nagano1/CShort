#define _CRT_SECURE_NO_WARNINGS

#include <cstdio>
#include <iostream>
#include <string>
#include <array>
#include <algorithm>
#include <cinttypes>

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

    // ---------------------------------------------------------------------------
    //
    //                            assign CalcReg
    //
    // ---------------------------------------------------------------------------
      
    static GRPRegisterEnum findUnusedReg1(GRPRegisterEnum reg1) {
        if (reg1 != GRPRegisterEnum::eax) {
            return GRPRegisterEnum::eax;
        }
        else  {
            return GRPRegisterEnum::ebx;
        }
    }

    static GRPRegisterEnum findUnusedReg2(GRPRegisterEnum reg1, GRPRegisterEnum reg2) {
        if (reg1 != GRPRegisterEnum::eax && reg2 != GRPRegisterEnum::eax) {
            return GRPRegisterEnum::eax;
        }
        else if (reg1 != GRPRegisterEnum::ebx && reg2 != GRPRegisterEnum::ebx) {
            return GRPRegisterEnum::ebx;
        }
        else if (reg1 != GRPRegisterEnum::ecx && reg2 != GRPRegisterEnum::ecx) {
            return GRPRegisterEnum::ecx;
        }

        //assert(reg1 != GRPRegisterEnum::edx && reg2 != GRPRegisterEnum::edx);
        return GRPRegisterEnum::edx;
    }

    static void assignCalcRegToNode(NodeBase *node, const ScriptEngineContext *scriptContext)
    {
        int typeIndex = node->typeIndex;
        if (typeIndex == -1) {
            typeIndex = BuiltInTypeIndex::int64;
        }

        auto *typeManager = scriptContext->parseContext->typeManager;
        auto *typeEntry = typeManager->getTypeEntryByIndex(typeIndex);
        int dataSize = typeEntry->getStackSizeForType();

        const GPRRegister *calcRegister = GetGPRRegisterByEnum(node->calcRegEnum, &scriptContext->cpuRegister);
        if (calcRegister != nullptr) {
            st_byte* dataPointer = GetDataPointerFromGPRRegister(calcRegister, dataSize);
            node->calcReg = dataPointer;
        }
    }

    // assigns calculation registers to the left and right expression nodes of a binary operation node, as well as to the value node of a parentheses node, and to the expression node of an assignment or return statement node.
    // parent comes first

    static int applyFunc_assignCalcOpRegister(NodeBase *node, ApplyFunc_params2)
    {
        assert(arg2 != nullptr);
        ScriptEngineContext *scriptContext = (ScriptEngineContext *)arg2;
        if (node->vtable == VTables::BinaryOperationVTable) {
            auto *binary = Cast::downcast<BinaryOperationNodeStruct *>(node);

            binary->leftExprNode->calcRegEnum = findUnusedReg1(binary->calcRegEnum);
            assignCalcRegToNode(binary->leftExprNode, scriptContext);

            // Find a register that is not used by the binary operation node or the left expression node, to avoid overwriting values during evaluation.
            binary->rightExprNode->calcRegEnum = findUnusedReg2(binary->calcRegEnum, binary->leftExprNode->calcRegEnum);
            assignCalcRegToNode(binary->rightExprNode, scriptContext);
        }

        if (node->vtable == VTables::ParenthesesVTable) {
            auto *parentheses = Cast::downcast<ParenthesesNodeStruct *>(node);

            assert(parentheses->valueNode != nullptr);
            parentheses->valueNode->calcRegEnum = parentheses->calcRegEnum;
            assignCalcRegToNode(parentheses->valueNode, scriptContext);
        }

        if (node->vtable == VTables::AssignmentVTable) {
            auto *assign = Cast::downcast<AssignmentNodeStruct *>(node);
            if (assign->expressionNode != nullptr) {
                assignCalcRegToNode(assign->expressionNode, scriptContext);
            }
        }

        if (node->vtable == VTables::ReturnStatementVTable) {
            auto *returnState = Cast::downcast<ReturnStatementNodeStruct *>(node);

            if (returnState->expressionNode) {
                assignCalcRegToNode(returnState->expressionNode, scriptContext);
            }
        }

        return 0;
    }


    static void assignCalcOpRegister(ScriptEngineContext *scriptContext, FuncDefNodeStruct *func)
    {
        func->bodyNode.vtable->applyFuncToDescendants(Cast::upcast(&func->bodyNode),
                                                         scriptContext->parseContext,
                                                         nullptr,
                                                         applyFunc_assignCalcOpRegister,
                                                         true, // parent first
                                                         nullptr,
                                                         scriptContext);
    }

    //------------------------------------------------------------------------------------------
    //
    //                                      evaluateExprNode
    //
    //------------------------------------------------------------------------------------------

    void ScriptEngineContext::evaluateExprNode(NodeBase *expressionNode)
    {
        assert(expressionNode != nullptr);
        assert(expressionNode->vtable != nullptr);

        auto *context = expressionNode->context;
        auto *typeManager = context->typeManager;

        if (expressionNode->vtable == VTables::IdentifiersAccessVTable) {
            auto* variableNode = Cast::downcast<IdentifiersAccessNodeStruct *>(expressionNode);
            TypeEntry *typeEntry = typeManager->getTypeEntryByIndex(variableNode->typeIndex);
            int dataSize = typeEntry->getStackSizeForType();
            this->stackMemory.moveFromStack(variableNode->stackOffset, dataSize, variableNode->calcReg);
            return;
        }
        
        if (expressionNode->vtable == VTables::ParenthesesVTable) {
            auto* parentheses = Cast::downcast<ParenthesesNodeStruct *>(expressionNode);
            evaluateExprNode(parentheses->valueNode);
            return;
        }

        // a + (b + c)
        if (expressionNode->vtable == VTables::BinaryOperationVTable) {
            auto* binaryNode = Cast::downcast<BinaryOperationNodeStruct *>(expressionNode);

            // evaluate right first, then left, to avoid overwriting registers
            this->evaluateExprNode(binaryNode->rightExprNode);
            uint64_t saved = *(uint64_t*)(binaryNode->rightExprNode->calcReg);
            this->evaluateExprNode(binaryNode->leftExprNode);
            *(uint64_t*)(binaryNode->rightExprNode->calcReg) = saved;

            auto *leftTypeEntry = typeManager->getTypeEntryByIndex(binaryNode->leftExprNode->typeIndex);
            leftTypeEntry->binary_operate(this, binaryNode);

            return;
        }

        // TODO: handle func calls, like 3 + funcA(100)
        // 1. evaluate caller expression to get func entry
        // 2. evaluate arguments and save their values to registers
        // 3. call func and save return value to register
        /*
        if (expressionNode->vtable == VTables::FuncCallVTable) {
            auto *funcCall = Cast::downcast<FuncCallNodeStruct *>(expressionNode);
            this->evaluateExprNode(funcCall->callerExprNode);
        }
        */

        TypeEntry *typeEntry = nullptr;
        int typeIndex = expressionNode->typeIndex;
        if (typeIndex > 0) {
            typeEntry = typeManager->getTypeEntryByIndex(typeIndex);
        }

        if (typeEntry) {
            typeEntry->evaluateNode(this, expressionNode);
        }
    }


    //------------------------------------------------------------------------------------------
    //
    //                                      BuiltIn Type operations
    //
    //------------------------------------------------------------------------------------------
    static void int32_evaluateNode(ScriptEngineContext *scriptContext, NumberNodeStruct *numberNode)
    {
        *(int32_t*)numberNode->calcReg = (int32_t)numberNode->num;
    }

    static void int32_binary_operate(ScriptEngineContext *scriptContext, BinaryOperationNodeStruct *binaryNode)
    {
        NodeBase *baseNode = binaryNode->useLeftAsBase ? binaryNode->leftExprNode : binaryNode->rightExprNode;
        NodeBase *otherNode = binaryNode->useLeftAsBase ? binaryNode->rightExprNode : binaryNode->leftExprNode;

        int32_t left32 = *(int32_t*)baseNode->calcReg;
        int32_t right32 = 0;
        if (otherNode->typeIndex == BuiltInTypeIndex::int32) {
            right32 = *(int32_t*)otherNode->calcReg;
        }
        else if (otherNode->typeIndex == BuiltInTypeIndex::int64) {
            right32 = (int32_t)(*(int64_t*)otherNode->calcReg);
        }


        switch (binaryNode->binaryOp) {
            case BinaryOperator::Add: {
                *(int32_t*)binaryNode->calcReg = left32 + right32;
                break;
            }
            case BinaryOperator::Subtract: {
                *(int32_t*)binaryNode->calcReg = left32 - right32;
                break;
            }
            case BinaryOperator::Multiply: {
                *(int32_t*)binaryNode->calcReg = left32 * right32;
                break;
            }
            case BinaryOperator::Divide: {
                if (right32 == 0) {
                    scriptContext->parseContext->addErrorWithNode(ErrorIndex::division_by_zero, binaryNode);
                    *(int32_t*)binaryNode->calcReg = 0;
                } else {
                    *(int32_t*)binaryNode->calcReg = left32 / right32;
                }
                break;
            }
        }
    }

    static void int64_evaluateNode(ScriptEngineContext *scriptContext, NumberNodeStruct *numberNode)
    {
        *(int64_t*)numberNode->calcReg = numberNode->num;
    }

    static void int64_binary_operate(ScriptEngineContext *scriptContext, BinaryOperationNodeStruct *binaryNode)
    {
        NodeBase *baseNode = binaryNode->useLeftAsBase ? binaryNode->leftExprNode : binaryNode->rightExprNode;
        NodeBase *otherNode = binaryNode->useLeftAsBase ? binaryNode->rightExprNode : binaryNode->leftExprNode;

        int64_t left64 = *(int64_t*)baseNode->calcReg;
        int64_t right64 = 0;
        if (otherNode->typeIndex == BuiltInTypeIndex::int32) {
            right64 = *(int32_t *) otherNode->calcReg;
        }
        else if (otherNode->typeIndex == BuiltInTypeIndex::int64) {
            right64 = *(int64_t *) otherNode->calcReg;
        }

        switch (binaryNode->binaryOp) {
            case BinaryOperator::Add: {
                *(int64_t*)binaryNode->calcReg = left64 + right64;
                break;
            }
            case BinaryOperator::Subtract: {
                *(int64_t*)binaryNode->calcReg = left64 - right64;
                break;
            }
            case BinaryOperator::Multiply: {
                *(int64_t*)binaryNode->calcReg = left64 * right64;
                break;
            }
            case BinaryOperator::Divide: {
                if (right64 == 0) {
                    scriptContext->parseContext->addErrorWithNode(ErrorIndex::division_by_zero, binaryNode);
                    *(int64_t*)binaryNode->calcReg = 0;
                } else {
                    *(int64_t*)binaryNode->calcReg = left64 / right64;
                }
                break;
            }
        }
    }


    static void heapString_evaluateNode(ScriptEngineContext *scriptContext, LiteralValueNodeStruct *node)
    {
        StringLiteralTokenStruct *stringLiteralToken = node->stringLiteralToken;
        char *chars;
        int size = (1 + stringLiteralToken->strLength) * (int)sizeof(char);
        TypedValue *value = scriptContext->generateTypedValue(BuiltInTypeIndex::heapString, size, &chars);
        memcpy(chars, stringLiteralToken->str, stringLiteralToken->strLength);
        chars[stringLiteralToken->strLength] = '\0';
        *(TypedValue **)node->calcReg = value;
    }

    static char* heapString_toString(ParseContext *context, TypedValue* value)
    {
        return (char*)value->ptr;
    }


    static int canAssignType_String(ParseContext *context, _typeEntry *otherType)
    {
        return 0;
    }

    static void heapString_binary_operate(ScriptEngineContext *scriptContext, BinaryOperationNodeStruct *binaryNode)
    {
        if (binaryNode->binaryOp != BinaryOperator::Add) {
            scriptContext->parseContext->addErrorWithNode(ErrorIndex::invalid_operator_for_string, binaryNode);
            return;
        }

        auto *leftNode = binaryNode->leftExprNode;
        auto *rightNode = binaryNode->rightExprNode;

        if (leftNode->typeIndex != BuiltInTypeIndex::heapString
             && rightNode->typeIndex != BuiltInTypeIndex::heapString) {
            scriptContext->parseContext->addErrorWithNode(ErrorIndex::invalid_operator_for_string, binaryNode);
            return;
        }

        auto *leftValue = *(TypedValue **)leftNode->calcReg;
        auto *rightValue = *(TypedValue **)rightNode->calcReg;

        // currently only support string concatenation with another string
        assert(leftValue->typeIndex == BuiltInTypeIndex::heapString);

        if (rightValue->typeIndex == BuiltInTypeIndex::heapString) {
            unsigned int size = (1 + leftValue->size + rightValue->size) * sizeof(char);
            char *chars;
            auto *value = scriptContext->generateTypedValue(BuiltInTypeIndex::heapString, (int)size, &chars);
            memcpy(chars, leftValue->ptr, leftValue->size);
            memcpy(chars + leftValue->size, rightValue->ptr, rightValue->size);
            chars[size - 1] = '\0';
            binaryNode->calcReg = (st_byte*)&value;
        }
    }


    static char* null_toString(ParseContext *context, TypedValue* value)
    {
        return (char*)"null";
    }

    static void null_binary_operate(ScriptEngineContext *scriptContext, BinaryOperationNodeStruct *binaryNode)
    {
    }

    static void null_evaluateNode(ScriptEngineContext *scriptContext, LiteralValueNodeStruct *node)
    {
        *(int64_t*)node->calcReg = 0;
    }







    //------------------------------------------------------------------------------------------
    //
    //                                Script Engine Context
    //
    //------------------------------------------------------------------------------------------

    TypedValue *ScriptEngineContext::newTypedValueForHeap()
    {
        auto *valueBase = (TypedValue *) memBufferForValueBase.newMem<TypedValue>(1);
        valueBase->ptr = nullptr;
        valueBase->size = 0;
        return valueBase;
    }


    static void reassignLineNumbers(DocumentStruct *docStruct)
    {
        int lineNumber = 0;
        auto *line = docStruct->firstCodeLine;
        while (line) {
            line->lineNumber = lineNumber++;
            line = line->nextLine;
        }
    }


    static TokenBase* findFirstNodeInLine(CodeLine *firstLine, CodeLine *lastLine)
    {
        CodeLine *currentLine = firstLine;
        while (currentLine) {
            TokenBase *node = currentLine->firstToken;
            if (node) {
                return node;
            }

            currentLine = currentLine->nextLine;
        }

        return nullptr;
    }

    static TokenBase* findLastNodeInLine(CodeLine *firstLine, CodeLine *lastLine)
    {
        TokenBase *returnNode = nullptr;
        CodeLine *currentLine = firstLine;
        while (currentLine) {
            returnNode = currentLine->lastToken;
            currentLine = currentLine->nextLine;
        }

        return returnNode;
    }


    TypedValue *ScriptEngineContext::generateTypedValue(int type, int size, void *ptr)
    {
        auto *value = this->newTypedValueForHeap();
        value->typeIndex = type;
        // value->ptr = context->memBufferForMalloc.newBytesMem(size); ////malloc(size);
        value->ptr = (void*)this->mallocHeapObject(size);
        *(void**)ptr = value->ptr;
        value->size = size;
        return value;
    }

    template<typename T>
    static void setBinaryOperateAndEvaluateForTypeEntry(ParseContext *context, int typeIndex
        , void (*binary_func)(ScriptEngineContext *, BinaryOperationNodeStruct *),
         void (*evalFunc)(ScriptEngineContext *, T *))
    {
        auto *typeEntry = context->typeManager->getTypeEntryByIndex(typeIndex);
        typeEntry->binary_operate = binary_func;// static_cast<int (*)(ParseContext *, BinaryOperationNodeStruct *)>(func);
        typeEntry->evaluateNode = reinterpret_cast<void (*)(ScriptEngineContext *, NodeBase *)>(evalFunc);
    }

    static void setBuiltinTypeOperations(ScriptEngineContext *context, ParseContext *parseContext) {
        TypeManager *typeManager = parseContext->typeManager;
        setBinaryOperateAndEvaluateForTypeEntry(parseContext, BuiltInTypeIndex::int32,
                                                              int32_binary_operate, 
                                                              int32_evaluateNode);

        setBinaryOperateAndEvaluateForTypeEntry(parseContext, BuiltInTypeIndex::int64,
                                                              int64_binary_operate,
                                                              int64_evaluateNode);

        setBinaryOperateAndEvaluateForTypeEntry(parseContext, BuiltInTypeIndex::heapString,
                                                              heapString_binary_operate,
                                                              heapString_evaluateNode);

        setBinaryOperateAndEvaluateForTypeEntry(parseContext, BuiltInTypeIndex::null,
                                                              null_binary_operate,
                                                              null_evaluateNode);
    }

    void ScriptEngineContext::init(ParseContext *context)
    {
        this->parseContext = context;
        
        this->memBuffer.init();
        this->memBufferForHeap.initWithHeapEntryEnabled();

        this->memBufferForValueBase.init();

        this->stackMemory.init();

        this->cpuRegister = CPUSim{}; // without this, the registers may contain garbage values, which can lead to unpredictable behavior during script execution.

        setBuiltinTypeOperations(this, context);
    }





    //------------------------------------------------------------------------------------------
    //
    //                              Script Runner (static functions)
    //
    //------------------------------------------------------------------------------------------

    // steps for running a CShort script using the script engine:
    // 1. load the script
    // 2. validate scripts
    //    - set typeIndex for each expression node
    //    - check type error
    //    - assign stack offset for variables
    //    - assign registers for expressions
    //    - find main entry func
    // 3. run script by running main func

    ScriptEngineContext *ScriptRunner::newScriptEngineContext(ParseContext *parseContext)
    {
        auto *scriptContext = mallocForType<ScriptEngineContext>();
        scriptContext->init(parseContext)   ;

        //parseContext->scriptEngineContext = scriptContext;
        return scriptContext;
    }

    void ScriptRunner::deleteScriptEngineContext(ScriptEngineContext *scriptContext)
    {
        scriptContext->freeAll();
        free(scriptContext);
    }


    struct TypeAndExpression {
        TypeEntry *typeEntry;
        NodeBase *expressionNode;
    };

    static TypeAndExpression executeFunc(FuncDefNodeStruct* mainFunc, ScriptEngineContext *scriptContext)
    {
        assignCalcOpRegister(scriptContext, mainFunc);

        auto *context = mainFunc->context;

        int stackSize = mainFunc->stackSize;
        int baseBytesMinusOne = scriptContext->stackMemory.baseBytes - 1;
        const int alignedStackSize = (stackSize + baseBytesMinusOne) & ~baseBytesMinusOne; // Align to base bytes

        auto *typeManager = mainFunc->context->typeManager;

        scriptContext->stackMemory.call(); // simulate call by pushing return address
        scriptContext->stackMemory.localVariables(alignedStackSize); // allocate space for local variables

        auto* statementNode = mainFunc->bodyNode.firstChildNode;
        while (statementNode != nullptr)
        {
            // perform assignment: int a = 3
            if (statementNode->vtable == VTables::AssignmentVTable) {
                auto* assignStatement = Cast::downcast<AssignmentNodeStruct *>(statementNode);
                if (assignStatement->expressionNode != nullptr) {
                    scriptContext->evaluateExprNode(assignStatement->expressionNode);
                    auto *typeEntry = typeManager->getTypeEntryByIndex(assignStatement->typeIndex);

                    auto dataSize = typeEntry->getStackSizeForType();
                    scriptContext->stackMemory.moveToStack(assignStatement->stackOffset, dataSize,
                                                     assignStatement->expressionNode->calcReg);
                }
            }

            // return 3
            if (statementNode->vtable == VTables::ReturnStatementVTable) {
                auto* returnNode = Cast::downcast<ReturnStatementNodeStruct*>(statementNode);
                scriptContext->evaluateExprNode(returnNode->expressionNode);
                auto* typeEntry = typeManager->getTypeEntryByIndex(returnNode->expressionNode->typeIndex);

                scriptContext->stackMemory.ret(); // simulate return by popping return address
                return TypeAndExpression{typeEntry, returnNode->expressionNode};
            }

            statementNode = statementNode->nextNode;
        }

        scriptContext->stackMemory.ret();
        return TypeAndExpression{nullptr, nullptr};
    }


    ScriptEngineContext* ScriptRunner::loadScript(const char* script, int byteLength)
    {
        auto* document = Alloc::newDocument(DocumentType::CodeDocument);
        auto* scriptContext = ScriptRunner::newScriptEngineContext(document->context);
        scriptContext->document = document;

        document->context->typeManager->initializeBuiltinTypeSelectors();

        DocumentUtils::parseText(document, script, byteLength);

        return scriptContext; // Adjusted since 'env' is no longer used
    }



    static int runScriptImpl(ScriptEngineContext *scriptContext)
    {
        assert(scriptContext->parseContext->syntaxErrorInfo.hasError == false);
        assert(scriptContext->parseContext->semanticErrorInfo.hasError == false);

        auto *document = scriptContext->document;
        int ret = 0;
        FuncDefNodeStruct *mainFunc = document->mainFunc;
        if (mainFunc != nullptr) {
            printf("main found <%s()>\n", mainFunc->funcNameToken.name);
            TypeAndExpression typeAndExpression = executeFunc(mainFunc, scriptContext);
            TypeEntry *typeEntry = typeAndExpression.typeEntry;
            if (typeEntry != nullptr) {
                if (typeEntry->getStackSizeForType() == 8) {
                    int64_t v = *(int64_t*)typeAndExpression.expressionNode->calcReg;
                    ret = (int32_t)v; // return of main entry func is always int32, so cast to int32
                }
                else {
                    ret = *(int32_t*)typeAndExpression.expressionNode->calcReg;
                }
            }
        }

        auto *rootNode = document->firstRootNode;
        while (rootNode != nullptr) {
            if (rootNode->vtable == VTables::ClassVTable) {
                // class
            }
            else if (rootNode->vtable == VTables::FuncDefVTable) {
                // fn
            }

            rootNode = rootNode->nextNode;
        }

        return ret;
    }


    int ScriptRunner::runScriptWithLength(const char* script, int scriptLength)
    {
        // Load the script
        auto *scriptContext = ScriptRunner::loadScript(script, scriptLength);
        auto *document = scriptContext->document;

        // Return if there's a syntax error
        if (scriptContext->parseContext->syntaxErrorInfo.hasError) {
            return scriptContext->parseContext->syntaxErrorInfo.errorItem.errorId;
        }

        // Validate types, values and finding Main(entry) function
        Validator::validateScript(document);
        
        if (scriptContext->parseContext->semanticErrorInfo.hasError) {
            return scriptContext->parseContext->semanticErrorInfo.firstErrorItem->codeErrorItem.errorId;
        }

        // Run script
        int ret = runScriptImpl(scriptContext);

        Alloc::deleteDocument(document);
        ScriptRunner::deleteScriptEngineContext(scriptContext);

        return ret;
    }
}