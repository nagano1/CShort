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
        assert(typeIndex != (int)TypeIndexConst::NotAssigned);

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
            auto *assignment = Cast::downcast<AssignmentNodeStruct *>(node);
            if (assignment->expressionNode != nullptr) {
                assignCalcRegToNode(assignment->expressionNode, scriptContext);
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

    static void evaluateExprNode(ScriptEngineContext *scriptContext, NodeBase *expressionNode)
    {
        assert(expressionNode != nullptr);
        assert(expressionNode->vtable != nullptr);

        auto *context = expressionNode->context;
        auto *typeManager = context->typeManager;

        if (expressionNode->vtable == VTables::IdentifiersAccessVTable) {
            auto* variableNode = Cast::downcast<IdentifiersAccessNodeStruct *>(expressionNode);
            TypeEntry *typeEntry = typeManager->getTypeEntryByIndex(variableNode->typeIndex);
            int dataSize = typeEntry->getStackSizeForType();
            scriptContext->stackMemory.moveFromStack(variableNode->stackOffset, dataSize, variableNode->calcReg);
            return;
        }
        
        if (expressionNode->vtable == VTables::ParenthesesVTable) {
            auto* parentheses = Cast::downcast<ParenthesesNodeStruct *>(expressionNode);
            evaluateExprNode(scriptContext, parentheses->valueNode);
            return;
        }

        // a + (b + c)
        if (expressionNode->vtable == VTables::BinaryOperationVTable) {
            auto* binaryNode = Cast::downcast<BinaryOperationNodeStruct *>(expressionNode);

            // evaluate right first, then left, to avoid overwriting registers
            evaluateExprNode(scriptContext, binaryNode->rightExprNode);
            auto *rightTypeEntry = typeManager->getTypeEntryByIndex(binaryNode->rightExprNode->typeIndex);
            const int rightSize = rightTypeEntry->getStackSizeForType();
             assert(rightSize <= 8 && "expression value larger than GPR register");

            std::array<st_byte, 8> saved{}; // max GPR size
            memcpy(saved.data(), binaryNode->rightExprNode->calcReg, (size_t)rightSize);
            evaluateExprNode(scriptContext, binaryNode->leftExprNode);
            memcpy(binaryNode->rightExprNode->calcReg, saved.data(), (size_t)rightSize);

            auto *leftTypeEntry = typeManager->getTypeEntryByIndex(binaryNode->leftExprNode->typeIndex);
            leftTypeEntry->binary_operate(scriptContext, binaryNode);

            return;
        }

        // TODO: handle func calls, like 3 + funcA(100)
        // 1. evaluate caller expression to get func entry
        // 2. evaluate arguments and save their values to registers
        // 3. call func and save return value to register
        /*
        if (expressionNode->vtable == VTables::FuncCallVTable) {
            auto *funcCall = Cast::downcast<FuncCallNodeStruct *>(expressionNode);
            evaluateExprNode(scriptContext, funcCall->callerExprNode);
        }
        */

        TypeEntry *typeEntry = nullptr;
        int typeIndex = expressionNode->typeIndex;
        if (typeIndex > 0) {
            typeEntry = typeManager->getTypeEntryByIndex(typeIndex);
        }

        if (typeEntry) {
            typeEntry->evaluateNode(scriptContext, expressionNode);
        }
    }


    //------------------------------------------------------------------------------------------
    //
    //                                      BuiltIn Type operations
    //
    //------------------------------------------------------------------------------------------
    static void int32_evaluateNode(ScriptEngineContext *scriptContext, NodeBase *nodeBase)
    {
        NumberNodeStruct *numberNode = Cast::downcast<NumberNodeStruct *>(nodeBase);
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

    static bool int32_convert_value_implicit(st_byte *dest, st_byte *src, ParseContext *context, TypeEntry *srcType, TypeEntry *destType) {
        return false;
    }

    static void int64_evaluateNode(ScriptEngineContext *scriptContext, NodeBase *nodeBase)
    {
        NumberNodeStruct *numberNode = Cast::downcast<NumberNodeStruct *>(nodeBase);
        *(int64_t*)numberNode->calcReg = numberNode->num;
    }

    static bool int64_convert_value_implicit(st_byte *dest, st_byte *src, ParseContext *context, TypeEntry *srcType, TypeEntry *destType) {
        // i64 a = 100
        if (srcType->typeIndex == BuiltInTypeIndex::int32 && destType->typeIndex == BuiltInTypeIndex::int64) {
            // Implicit conversion from int32 to int64
            *(int64_t*)dest = (int64_t)*(int32_t*)src;
            return true;
        }

        return false;
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


    static void heapString_evaluateNode(ScriptEngineContext *scriptContext, NodeBase *nodeBase)
    {
        assert(nodeBase->vtable == VTables::FixedLiteralVTable);
        LiteralValueNodeStruct *node = Cast::downcast<LiteralValueNodeStruct *>(nodeBase);

        StringLiteralTokenStruct *stringLiteralToken = node->stringLiteralToken;
        char *chars;
        int size = (1 + stringLiteralToken->strLength) * (int)sizeof(char);
        TypedValue *value = scriptContext->generateTypedValue(BuiltInTypeIndex::heapString, size, &chars);
        memcpy(chars, stringLiteralToken->str, stringLiteralToken->strLength);
        chars[stringLiteralToken->strLength] = '\0';
        *(TypedValue **)node->calcReg = value;
    }

    static bool heapString_convert_value_implicit(st_byte *dest, st_byte *src, ParseContext *context, TypeEntry *srcType, TypeEntry *destType) {
        // currently, heapString does not support implicit conversion from other types
        return false;
    }

    static char* heapString_toString(ParseContext *context, TypedValue* value)
    {
        return (char*)value->ptr;
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
             || rightNode->typeIndex != BuiltInTypeIndex::heapString) {
            scriptContext->parseContext->addErrorWithNode(ErrorIndex::invalid_operator_for_string, binaryNode);
            return;
        }

        auto *leftTypedValue = *(TypedValue **)leftNode->calcReg;
        auto *rightTypedValue = *(TypedValue **)rightNode->calcReg;

         if (leftTypedValue == nullptr || rightTypedValue == nullptr ||
             leftTypedValue->typeIndex != BuiltInTypeIndex::heapString ||
             rightTypedValue->typeIndex != BuiltInTypeIndex::heapString) {
             scriptContext->parseContext->addErrorWithNode(ErrorIndex::invalid_operator_for_string, binaryNode);
             return;
         }

        if (rightTypedValue->typeIndex == BuiltInTypeIndex::heapString) {
            const unsigned int leftLen = leftTypedValue->size > 0 ? leftTypedValue->size - 1 : 0;
            const unsigned int rightLen = rightTypedValue->size > 0 ? rightTypedValue->size - 1 : 0;
            const unsigned int size = leftLen + rightLen + 1; // includes trailing '\0'
            char *chars;
            auto *value = scriptContext->generateTypedValue(BuiltInTypeIndex::heapString, (int)size, &chars);
            memcpy(chars, leftTypedValue->ptr, leftLen);
            memcpy(chars + leftLen, rightTypedValue->ptr, rightLen);
            chars[size - 1] = '\0';
            *(TypedValue **)binaryNode->calcReg = value;
        }
    }

    static void null_binary_operate(ScriptEngineContext *scriptContext, BinaryOperationNodeStruct *binaryNode)
    {
        // currently null is not supported as base node for binary operation, so this function should not be called.
        // null + "string" will be handled by the string binary operation function
    }

    static void null_evaluateNode(ScriptEngineContext *scriptContext, NodeBase *nodeBase)
    {
        assert(nodeBase->vtable == VTables::FixedLiteralVTable);
        LiteralValueNodeStruct *node = Cast::downcast<LiteralValueNodeStruct *>(nodeBase);
        assert(node->calcReg != nullptr);
        *(int64_t*)node->calcReg = 0;
    }

    static bool null_convert_value_implicit(st_byte *dest, st_byte *src, ParseContext *context, TypeEntry *srcType, TypeEntry *destType) {
        // currently null does not support implicit conversion from other types
        return false;
    }

    static void bool_binary_operate(ScriptEngineContext *scriptContext, BinaryOperationNodeStruct *binaryNode)
    {
        NodeBase *baseNode = binaryNode->useLeftAsBase ? binaryNode->leftExprNode : binaryNode->rightExprNode;
        NodeBase *otherNode = binaryNode->useLeftAsBase ? binaryNode->rightExprNode : binaryNode->leftExprNode;

        bool leftBool = *(bool*)baseNode->calcReg;
        bool rightBool = *(bool*)otherNode->calcReg;

        switch (binaryNode->binaryOp) {
            case BinaryOperator::And: {
                *(bool*)binaryNode->calcReg = leftBool && rightBool;
                break;
            }
            case BinaryOperator::Or: {
                *(bool*)binaryNode->calcReg = leftBool || rightBool;
                break;
            }
            default: {
                assert(false);
                // this should not happen, as the type selector should have already filtered out invalid operators for bool type
                //scriptContext->parseContext->addErrorWithNode(ErrorIndex::invalid_operator_for_bool, binaryNode);
                //*(bool*)binaryNode->calcReg = false;
                break;
            }
        }
    }

    static void bool_evaluateNode(ScriptEngineContext *scriptContext, NodeBase *nodeBase) {
        assert(nodeBase->vtable == VTables::FixedLiteralVTable);
        LiteralValueNodeStruct *node = Cast::downcast<LiteralValueNodeStruct *>(nodeBase);
        assert(node->calcReg != nullptr);
        *(bool*)node->calcReg = node->isTrue ? 1 : 0; // internally represent bool as 1 or 0, but when printing, print as true or false
    }

    static bool bool_convert_value_implicit(st_byte *dest, st_byte *src, ParseContext *context, TypeEntry *srcType, TypeEntry *destType) {
        // currently bool does not support implicit conversion from other types
        return false;
    }







    //------------------------------------------------------------------------------------------
    //
    //                                Script Engine Context
    //
    //------------------------------------------------------------------------------------------

    TypedValue *ScriptEngineContext::generateTypedValue(int type, int size, void *ptr)
    {
        auto *typedValue = (TypedValue *) memBufferForTypedValue.newMem<TypedValue>(1);

        typedValue->typeIndex = type;
        // typedValue->ptr = context->memBufferForMalloc.newBytesMem(size); ////malloc(size);
        typedValue->ptr = (void*)this->mallocHeapObject(size);
        *(void**)ptr = typedValue->ptr;
        typedValue->size = size;
        return typedValue;
    }
    

    static void setBinaryOperateAndEvaluateForTypeEntry(ParseContext *context, type_index typeIndex
        , void (*binary_func)(ScriptEngineContext *, BinaryOperationNodeStruct *),
         void (*evalFunc)(ScriptEngineContext *, NodeBase *),
         bool (*convert_value_implicit)(st_byte *dest, st_byte *src, ParseContext *context, TypeEntry *srcType, TypeEntry *destType)
        
        )
    {
        auto *typeEntry = context->typeManager->getTypeEntryByIndex(typeIndex);
        typeEntry->binary_operate = binary_func;
        typeEntry->evaluateNode = evalFunc;
        typeEntry->convert_value_implicit = convert_value_implicit;
    }

    static void setBuiltinTypeOperations(ScriptEngineContext *context, ParseContext *parseContext) {
        TypeManager *typeManager = parseContext->typeManager;
        setBinaryOperateAndEvaluateForTypeEntry(parseContext, BuiltInTypeIndex::int32,
                                                              int32_binary_operate, 
                                                              int32_evaluateNode,
                                                              int32_convert_value_implicit);

        setBinaryOperateAndEvaluateForTypeEntry(parseContext, BuiltInTypeIndex::int64,
                                                              int64_binary_operate,
                                                              int64_evaluateNode,
                                                              int64_convert_value_implicit);

        setBinaryOperateAndEvaluateForTypeEntry(parseContext, BuiltInTypeIndex::heapString,
                                                              heapString_binary_operate,
                                                              heapString_evaluateNode,
                                                              heapString_convert_value_implicit);

        setBinaryOperateAndEvaluateForTypeEntry(parseContext, BuiltInTypeIndex::null,
                                                              null_binary_operate,
                                                              null_evaluateNode,
                                                              null_convert_value_implicit);

        setBinaryOperateAndEvaluateForTypeEntry(parseContext, BuiltInTypeIndex::boolIdx,
                                                              bool_binary_operate,
                                                              bool_evaluateNode,
                                                              bool_convert_value_implicit);


    }

    void ScriptEngineContext::init(ParseContext *context)
    {
        this->parseContext = context;
        
        this->memBufferForHeap.initWithHeapEntryEnabled();
        this->memBufferForTypedValue.init();

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


    struct TypeAndExpression {
        TypeEntry *typeEntry;
        NodeBase *expressionNode;
    };


    static st_byte *ConvertImplicitForAssignment(int dstTypeIndex, int srcTypeIndex, st_byte* srcCalcReg) {
        if (dstTypeIndex == srcTypeIndex) {
            return srcCalcReg;
        }
        else if (dstTypeIndex == BuiltInTypeIndex::int64 && srcTypeIndex == BuiltInTypeIndex::int32) {
            // Implicit conversion from int32 to int64
            int64_t* tmp = new int64_t((int64_t)*(int32_t*)srcCalcReg);
            return (st_byte*)tmp;
        }
        else {
            assert(false && "unsupported implicit conversion in assignment");
            return nullptr;
        }
    }

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
            // execute assignment: int a = 3
            if (statementNode->vtable == VTables::AssignmentVTable) {
                auto* assignStatement = Cast::downcast<AssignmentNodeStruct *>(statementNode);
                if (assignStatement->expressionNode != nullptr) {
                    evaluateExprNode(scriptContext, assignStatement->expressionNode);
                    TypeEntry *dstTypeEntry = typeManager->getTypeEntryByIndex(assignStatement->typeIndex);
                    // int variableSize = assignStatement->typeOrLet.hasNullableMark dstTypeEntry->getStackSizeForType();
                    TypeEntry *srcTypeEntry = typeManager->getTypeEntryByIndex(assignStatement->expressionNode->typeIndex);
                    const int dstSize = dstTypeEntry->getStackSizeForType();

                    st_byte tmpBuffer[8] = {0}; // temporary buffer for implicit conversion, 8 is the maximum size of a GPR register (for int64)
                    st_byte* dstPtr;
                    if (assignStatement->typeIndex == assignStatement->expressionNode->typeIndex || srcTypeEntry->getStackSizeForType() == dstTypeEntry->getStackSizeForType()) {
                        dstPtr = assignStatement->expressionNode->calcReg;
                    }
                    else {
                        bool handled = dstTypeEntry->convert_value_implicit(tmpBuffer, assignStatement->expressionNode->calcReg, context,
                                                         typeManager->getTypeEntryByIndex(assignStatement->expressionNode->typeIndex),
                                                         dstTypeEntry);
                        assert(handled && "unsupported implicit conversion in assignment");
                        dstPtr = tmpBuffer;
                    }

                    scriptContext->stackMemory.moveToStack(assignStatement->stackOffset, dstSize, dstPtr);
                }
            }

            // return 3
            if (statementNode->vtable == VTables::ReturnStatementVTable) {
                auto* returnNode = Cast::downcast<ReturnStatementNodeStruct*>(statementNode);
                evaluateExprNode(scriptContext, returnNode->expressionNode);
                auto* typeEntry = typeManager->getTypeEntryByIndex(returnNode->expressionNode->typeIndex);

                scriptContext->stackMemory.ret(); // simulate return by popping return address
                return TypeAndExpression{typeEntry, returnNode->expressionNode};
            }

            statementNode = statementNode->nextNode;
        }

        scriptContext->stackMemory.ret();
        return TypeAndExpression{nullptr, nullptr};
    }


    static int64_t runScriptImpl(DocumentStruct *document, ScriptEngineContext *scriptContext)
    {
        assert(scriptContext->parseContext->syntaxErrorInfo.hasError == false);
        assert(scriptContext->parseContext->semanticErrorInfo.hasError == false);

        int64_t ret = 0;
        FuncDefNodeStruct *mainFunc = document->mainFunc;
        if (mainFunc != nullptr) {
            // printf("main found <%s()>\n", mainFunc->funcNameToken.name);
            TypeAndExpression typeAndExpression = executeFunc(mainFunc, scriptContext);
            TypeEntry *typeEntry = typeAndExpression.typeEntry;
            if (typeEntry != nullptr && typeAndExpression.expressionNode != nullptr) {
                const int size = typeEntry->getStackSizeForType();
                st_byte *p = typeAndExpression.expressionNode->calcReg;
                switch (size) {
                    case 1: ret = *(uint8_t*)p; break;
                    case 2: ret = *(uint16_t*)p; break;
                    case 4: ret = *(int32_t*)p; break;
                    case 8: ret = *(int64_t*)p; break;
                    default: assert(false && "unsupported return size"); ret = 0; break;
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


    int64_t ScriptRunner::runScriptWithLength(const char* script, int scriptLength)
    {
        // Load the script
        auto* document = Alloc::newDocument(DocumentType::CodeDocument);
        document->context->typeManager->initializeBuiltinTypeSelectors();

        DocumentUtils::parseText(document, script, scriptLength);

        // Quit if there's a syntax error
        if (document->context->syntaxErrorInfo.hasError) {
             int err = document->context->syntaxErrorInfo.errorItem.errorId;
             Alloc::deleteDocument(document);
             return err;
        }

        auto *scriptContext = mallocForType<ScriptEngineContext>();
        scriptContext->init(document->context);


        // Validate types, values and finding Main(entry) function
        Validator::validateScript(document);
        
        if (document->context->semanticErrorInfo.hasError) {
            // Return the error ID of the first semantic error encountered
            int err = document->context->semanticErrorInfo.firstErrorItem->codeErrorItem.errorId;
            scriptContext->freeAll();
            free(scriptContext);
            Alloc::deleteDocument(document);
            return err;
        }

        // Run script
        int64_t ret = runScriptImpl(document, scriptContext);

        if (document->context->semanticErrorInfo.hasError) {
            // Return the error ID of the first semantic error encountered during execution
            ret = (int64_t)document->context->semanticErrorInfo.firstErrorItem->codeErrorItem.errorId;
        }


        Alloc::deleteDocument(document);

        scriptContext->freeAll();
        free(scriptContext);

        return ret;
    }
}