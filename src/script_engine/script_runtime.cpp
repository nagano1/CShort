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

    static void assignCalcRegToNode(NodeBase *node, const ParseContext *context)
    {
        int typeIndex = node->typeIndex;
        if (typeIndex == -1) {
            typeIndex = BuiltInTypeIndex::int64;
        }

        auto *typeManager = context->typeManager;
        auto *typeEntry = typeManager->getTypeEntryByIndex(typeIndex);
        int dataSize = typeEntry->getStackSizeForType();

        const GPRRegister *calcRegister = GetGPRRegisterByEnum(node->calcRegEnum, &context->scriptEngineContext->cpuRegister);
        if (calcRegister != nullptr) {
            st_byte* dataPointer = GetDataPointerFromGPRRegister(calcRegister, dataSize);
            node->calcReg = dataPointer;
        }
    }

    // assigns calculation registers to the left and right expression nodes of a binary operation node, as well as to the value node of a parentheses node, and to the expression node of an assignment or return statement node.
    // parent comes first

    static int applyFunc_assignCalcOpRegister(NodeBase *node, ApplyFunc_params2)
    {
        //auto *context = (ParseContext *)context;

        if (node->vtable == VTables::BinaryOperationVTable) {
            auto *binary = Cast::downcast<BinaryOperationNodeStruct *>(node);

            binary->leftExprNode->calcRegEnum = findUnusedReg1(binary->calcRegEnum);
            assignCalcRegToNode(binary->leftExprNode, context);

            binary->rightExprNode->calcRegEnum = findUnusedReg2(binary->calcRegEnum, binary->leftExprNode->calcRegEnum);
            assignCalcRegToNode(binary->rightExprNode, context);
        }

        if (node->vtable == VTables::ParenthesesVTable) {
            auto *parentheses = Cast::downcast<ParenthesesNodeStruct *>(node);

            assert(parentheses->valueNode != nullptr);
            parentheses->valueNode->calcRegEnum = parentheses->calcRegEnum;
            assignCalcRegToNode(parentheses->valueNode, context);
        }

        if (node->vtable == VTables::AssignmentVTable) {
            auto *assign = Cast::downcast<AssignmentNodeStruct *>(node);
            if (assign->expressionNode != nullptr) {
                assignCalcRegToNode(assign->expressionNode, context);
            }
        }

        if (node->vtable == VTables::ReturnStatementVTable) {
            auto *returnState = Cast::downcast<ReturnStatementNodeStruct *>(node);

            if (returnState->expressionNode) {
                assignCalcRegToNode(returnState->expressionNode, context);
            }
        }

        return 0;
    }


    static void assignCalcOpRegister(ParseContext *context, FuncDefNodeStruct *func)
    {
        func->bodyNode.vtable->applyFuncToDescendants(Cast::upcast(&func->bodyNode),
                                                         context,
                                                         nullptr,
                                                         applyFunc_assignCalcOpRegister,
                                                         /*parent first*/true,
                                                         nullptr,
                                                         nullptr);
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

        auto *context = this->scriptEnv->document->context;
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
            leftTypeEntry->binary_operate(parseContext, binaryNode);

            return;
        }

        // To handle func call, like 3 + funcA(100)
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
            typeEntry->evaluateNode(this->parseContext, expressionNode);
        }
    }


    //------------------------------------------------------------------------------------------
    //
    //                                      BuiltIn Type operations
    //
    //------------------------------------------------------------------------------------------
    static void int32_evaluateNode(ParseContext *context, NumberNodeStruct *numberNode)
    {
        *(int32_t*)numberNode->calcReg = (int32_t)numberNode->num;
    }
    static int int32_binary_operate(ParseContext *context, BinaryOperationNodeStruct *binaryNode)
    {
        int32_t left32 = *(int32_t*)binaryNode->leftExprNode->calcReg;
        int32_t right32 = 0;
        if (binaryNode->rightExprNode->typeIndex == BuiltInTypeIndex::int32) {
            right32 = *(int32_t*)binaryNode->rightExprNode->calcReg;
        }
        else if (binaryNode->rightExprNode->typeIndex == BuiltInTypeIndex::int64) {
            right32 = (int32_t)(*(int64_t*)binaryNode->rightExprNode->calcReg);
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
                    context->addErrorWithNode(ErrorIndex::division_by_zero, binaryNode);
                    *(int32_t*)binaryNode->calcReg = 0;
                } else {
                    *(int32_t*)binaryNode->calcReg = left32 / right32;
                }
                break;
            }
        }

        return BuiltInTypeIndex::int32;
    }

    static void int64_evaluateNode(ParseContext *context, NumberNodeStruct *numberNode)
    {
        *(int64_t*)numberNode->calcReg = numberNode->num;
    }

    static int int64_binary_operate(ParseContext *context, BinaryOperationNodeStruct *binaryNode)
    {
        int64_t left64 = *(int64_t*)binaryNode->leftExprNode->calcReg;
        int64_t right64 = 0;
        if (binaryNode->rightExprNode->typeIndex == BuiltInTypeIndex::int32) {
            right64 = *(int32_t *) binaryNode->rightExprNode->calcReg;
        }
        else if (binaryNode->rightExprNode->typeIndex == BuiltInTypeIndex::int64) {
            right64 = *(int64_t *) binaryNode->rightExprNode->calcReg;
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
                    context->addErrorWithNode(ErrorIndex::division_by_zero, binaryNode);
                    *(int64_t*)binaryNode->calcReg = 0;
                } else {
                    *(int64_t*)binaryNode->calcReg = left64 / right64;
                }
                break;
            }
        }
        return BuiltInTypeIndex::int64;
    }


    static void heapString_evaluateNode(ParseContext *context, LiteralValueNodeStruct *node)
    {
        StringLiteralTokenStruct *stringLiteralToken = node->stringLiteralToken;
        char *chars;
        int size = (1 + stringLiteralToken->strLength) * (int)sizeof(char);
        ValueBase *value = context->scriptEngineContext->genValueBase(BuiltInTypeIndex::heapString, size, &chars);
        memcpy(chars, stringLiteralToken->str, stringLiteralToken->strLength);
        chars[stringLiteralToken->strLength] = '\0';
        *(ValueBase **)node->calcReg = value;
    }

    static char* heapString_toString(ParseContext *context, ValueBase* value)
    {
        return (char*)value->ptr;
    }


    static int canAssignType_String(ParseContext *context, _typeEntry *otherType)
    {
        return 0;
    }

    static int heapString_binary_operate(ParseContext *context, BinaryOperationNodeStruct *binaryNode)
    {
        if (binaryNode->binaryOp != BinaryOperator::Add) {
            context->addErrorWithNode(ErrorIndex::invalid_operator_for_string, binaryNode);
            return BuiltInTypeIndex::heapString;
        }

        auto *leftNode = binaryNode->leftExprNode;
        auto *rightNode = binaryNode->rightExprNode;

        if (leftNode->typeIndex != BuiltInTypeIndex::heapString
             && rightNode->typeIndex != BuiltInTypeIndex::heapString) {
            context->addErrorWithNode(ErrorIndex::invalid_operator_for_string, binaryNode);
            return BuiltInTypeIndex::heapString;
        }

        auto *leftValue = *(ValueBase **)leftNode->calcReg;
        auto *rightValue = *(ValueBase **)rightNode->calcReg;

        // currently only support string concatenation with another string
        assert(leftValue->typeIndex == BuiltInTypeIndex::heapString);

        if (rightValue->typeIndex == BuiltInTypeIndex::heapString) {
            unsigned int size = (1 + leftValue->size + rightValue->size) * sizeof(char);
            char *chars;
            auto *value = context->scriptEngineContext->genValueBase(BuiltInTypeIndex::heapString, (int)size, &chars);
            memcpy(chars, leftValue->ptr, leftValue->size);
            memcpy(chars + leftValue->size, rightValue->ptr, rightValue->size);
            chars[size - 1] = '\0';
            binaryNode->calcReg = (st_byte*)&value;
        }

        return BuiltInTypeIndex::heapString;
    }


    static char* null_toString(ParseContext *context, ValueBase* value)
    {
        return (char*)"null";
    }

    static int null_binary_operate(ParseContext *context, BinaryOperationNodeStruct *binaryNode)
    {
        return BuiltInTypeIndex::null;
    }

    static int canAssignType_null(ParseContext *context, _typeEntry *otherType)
    {
        return 0;
    }

    static void null_evaluateNode(ParseContext *context, LiteralValueNodeStruct *node)
    {
        *(int64_t*)node->calcReg = 0;
    }




























    //------------------------------------------------------------------------------------------
    //
    //                                Script Engine Context
    //
    //------------------------------------------------------------------------------------------

    ValueBase *ScriptEngineContext::newValueForHeap()
    {
        auto *valueBase = (ValueBase *) memBufferForValueBase.newMem<ValueBase>(1);
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

/*
    // return: utf16
    static int getPosInLine(NodeBase *node, bool beginningPos)
    {
        auto *codeLine = node->codeLine;

        int utf16Pos = 0;
        assert(codeLine);

        NodeBase *currenNode = codeLine->firstNode;
        while (currenNode) {

            utf16Pos += currenNode->precedingSpaceCount;

            if (currenNode == node) {
                if (beginningPos) {
                    break;
                }
            }

            int textLength = VTableCall::selfTextLength(currenNode);
            char *text = (char*)VTableCall::selfText(currenNode);

            utf16Pos += ParseUtil::utf16_length(text, textLength);

            if (currenNode == node) {
                break;
            }

            currenNode = currenNode->nextNodeInLine;
        }

        return utf16Pos;
    }
*/


    void ScriptEngineContext::setErrorPositions()
    {
        /*
        reassignLineNumbers(this->scriptEnv->document);

        auto *context = this->scriptEnv->document->context;
        context->appendLineMode = AppendLineMode::DetectErrorSpanNodes;

        auto *errorItem = this->semanticErrorInfo.firstErrorItem;
        while (errorItem) {
            auto *node = errorItem->node;

            auto* firstCodeLine = context->newCodeLine();
            firstCodeLine->init(context);
            auto *lastCodeLine = VTableCall::callAppendToLine(node, firstCodeLine);

            auto *firstNode = findFirstNodeInLine(firstCodeLine, lastCodeLine);
            auto *lastNode = findLastNodeInLine(firstCodeLine, lastCodeLine);

            int a = getPosInLine(firstNode, true);
            int b = getPosInLine(lastNode, false);

            errorItem->codeErrorItem.charPos1 = a;
            errorItem->codeErrorItem.charPos2 = b;
            errorItem->codeErrorItem.charPosition = a;
            errorItem->codeErrorItem.charPosition2 = b;
            errorItem->codeErrorItem.linePos1 = firstNode->codeLine->lineNumber;
            errorItem->codeErrorItem.linePos2 = lastNode->codeLine->lineNumber;

            errorItem = errorItem->next;
        }

        // DocumentUtils::regenerateCodeLines(docStruct);
        static_assert(true, "not implemented");
        */
    }
    // //int32_t *int32ptr;
    //            //auto *value = this->context->genValueBase(BuiltInTypeIndex::int32, sizeof(int32_t), &int32ptr);
    //            //*int32ptr = numberNode->num;
    //            \param type
    ValueBase *ScriptEngineContext::genValueBase(int type, int size, void *ptr)
    {
        auto *value = this->newValueForHeap();
        value->typeIndex = type;
        // value->ptr = context->memBufferForMalloc.newBytesMem(size); ////malloc(size);
        value->ptr = (void*)this->mallocHeapObject(size);
        *(void**)ptr = value->ptr;
        value->size = size;
        return value;
    }

    template<typename T>
    static void setBinaryOperateAndEvaluateForTypeEntry(ParseContext *context, int typeIndex
        , int (*binary_func)(ParseContext *, BinaryOperationNodeStruct *),
         void (*evalFunc)(ParseContext *, T *))
    {
        auto *typeEntry = context->typeManager->getTypeEntryByIndex(typeIndex);
        typeEntry->binary_operate = binary_func;// static_cast<int (*)(ParseContext *, BinaryOperationNodeStruct *)>(func);
        typeEntry->evaluateNode = reinterpret_cast<void (*)(ParseContext *, NodeBase *)>(evalFunc);
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
/*
        typeManager->getTypeEntryByIndex(BuiltInTypeIndex::int64)->binary_operate = int64_binary_operate;
        typeManager->getTypeEntryByIndex(BuiltInTypeIndex::int64)->evaluateNode = int64_evaluateNode;

        typeManager->getTypeEntryByIndex(BuiltInTypeIndex::heapString)->binary_operate = heapString_binary_operate;
        typeManager->getTypeEntryByIndex(BuiltInTypeIndex::heapString)->evaluateNode = heapString_evaluateNode;
        
        typeManager->getTypeEntryByIndex(BuiltInTypeIndex::null)->binary_operate = null_binary_operate;
        typeManager->getTypeEntryByIndex(BuiltInTypeIndex::null)->evaluateNode = null_evaluateNode;
*/
    }

    void ScriptEngineContext::init(ScriptEnv *scriptEnvArg, ParseContext *context)
    {
        this->scriptEnv = scriptEnvArg;
        this->parseContext = context;

        this->semanticErrorInfo.hasError = false;
        this->semanticErrorInfo.count = 0;
        this->semanticErrorInfo.firstErrorItem = nullptr;
        this->semanticErrorInfo.lastErrorItem = nullptr;


        this->memBuffer.init();
        this->memBufferForHeap.initWithHeapEntryEnabled();
        this->memBufferForError.init();
        this->memBufferForValueBase.init();
        this->stackMemory.init();

        // this->variableMap2 = this->memBuffer.newMem<VoidHashMap>(1);
        // this->variableMap2->init(&this->memBuffer);

        // this->typeNameMap = this->memBuffer.newMem<VoidHashMap>(1);
        // this->typeNameMap->init(&this->memBuffer);
        this->cpuRegister = CPUSim{};

        setBuiltinTypeOperations(this, context);
    }





    //------------------------------------------------------------------------------------------
    //
    //                                       Script Engine
    //
    //------------------------------------------------------------------------------------------

    // Script engine steps:
    // 1. load scripts.
    // 2. validate scripts
    //    - set typeIndex for each expression node
    //    - check type error
    //    - assign stack offset for variables
    //    - assign registers for expressions
    //    - find main entry func
    // 3. run scripts (evaluate nodes)

    ScriptEnv *ScriptEnv::newScriptEnv(ParseContext *parseContext)
    {
        auto *scriptEnv = (ScriptEnv*)malloc(sizeof(ScriptEnv));
        if (scriptEnv != nullptr) {
            auto *context = mallocForType<ScriptEngineContext>();

            scriptEnv->context = context;
            context->init(scriptEnv, parseContext);
            parseContext->scriptEngineContext = context;

            scriptEnv->mainFunc = nullptr;
        }
        return scriptEnv;
    }

    void ScriptEnv::deleteScriptEnv(ScriptEnv *scriptEnv)
    {
        scriptEnv->context->freeAll();
        free(scriptEnv->context);
        free(scriptEnv);
    }


    struct TypeAndExpression {
        TypeEntry *typeEntry;
        NodeBase *expressionNode;
    };

    static TypeAndExpression executeFunc(ScriptEnv* env, FuncDefNodeStruct* mainFunc)
    {
        assignCalcOpRegister(mainFunc->context, mainFunc);

        int stackSize = mainFunc->stackSize;
        int baseBytesMinusOne = env->context->stackMemory.baseBytes - 1;
        const int alignedStackSize = (stackSize + baseBytesMinusOne) & ~baseBytesMinusOne; // Align to base bytes

        auto *typeManager = mainFunc->context->typeManager;

        env->context->stackMemory.call(); // simulate call by pushing return address
        env->context->stackMemory.localVariables(alignedStackSize); // allocate space for local variables

        auto* statementNode = mainFunc->bodyNode.firstChildNode;
        while (statementNode != nullptr)
        {
            // call func: funcA(100)
            /*
            if (statementNode->vtable == VTables::FuncCallVTable) {
                auto* funcCall = Cast::downcast<FuncCallNodeStruct*>(statementNode);
                FuncCallArgItemStruct *arg = funcCall->firstArgumentItem;
                while (arg != nullptr) {
                    printf("arg = <%s>\n", arg->exprNode->vtable->typeChars);
                    env->context->evaluateExprNode(arg->exprNode);
                    if (arg->nextNode == nullptr) {
                        break;
                    }
                    arg = Cast::downcast<FuncCallArgItemStruct *>(arg->nextNode);
                }
            }
            */

            // assignment: int a = 3
            if (statementNode->vtable == VTables::AssignmentVTable) {
                auto* assignStatement = Cast::downcast<AssignmentNodeStruct *>(statementNode);
                if (assignStatement->expressionNode != nullptr) {
                    env->context->evaluateExprNode(assignStatement->expressionNode);
                    auto *typeEntry = typeManager->getTypeEntryByIndex(assignStatement->typeIndex);

                    // 
                    auto dataSize = typeEntry->getStackSizeForType();
                    env->context->stackMemory.moveToStack(assignStatement->stackOffset, dataSize,
                                                     assignStatement->expressionNode->calcReg);
                }
            }

            // return 3
            if (statementNode->vtable == VTables::ReturnStatementVTable) {
                auto* returnNode = Cast::downcast<ReturnStatementNodeStruct*>(statementNode);
                env->context->evaluateExprNode(returnNode->expressionNode);
                auto* typeEntry = typeManager->getTypeEntryByIndex(returnNode->expressionNode->typeIndex);

                env->context->stackMemory.ret(); // simulate return by popping return address
                return TypeAndExpression{typeEntry, returnNode->expressionNode};
            }

            statementNode = statementNode->nextNode;
        }

        env->context->stackMemory.ret();
        return TypeAndExpression{nullptr, nullptr};
    }


    _ScriptEnv* ScriptEnv::loadScript(char* script, int byteLength)
    {
        auto* document = Alloc::newDocument(DocumentType::CodeDocument);
        ScriptEnv* env = ScriptEnv::newScriptEnv(document->context);

        document->context->typeManager->initializeBuiltinTypeSelectors();

        DocumentUtils::parseText(document, script, byteLength);
        env->document = document;

        return env;
    }



    int ScriptEnv::runScript()
    {
        assert(this->document->context->syntaxErrorInfo.hasError == false);
        assert(this->context->parseContext->semanticErrorInfo.hasError == false);

        int ret = 0;
        auto *mainFunc = this->mainFunc;
        if (mainFunc) {
            // printf("main found <%s()>\n", mainFunc2->nameNode.name);
            TypeAndExpression typeAndExpression = executeFunc(this, mainFunc);
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

        auto *rootNode = this->document->firstRootNode;
        while (rootNode != nullptr) {
            if (rootNode->vtable == VTables::ClassVTable) {
                // class
            }
            else if (rootNode->vtable == VTables::FuncDefVTable) {
                // fn
            }

            rootNode = rootNode->nextNode;
        }

        Alloc::deleteDocument(this->document);
        ScriptEnv::deleteScriptEnv(this);

        return ret;
    }


    int ScriptEnv::startScriptInternal(char* script, int scriptLength)
    {
        // Load the script
        ScriptEnv *env = ScriptEnv::loadScript(script, scriptLength);

        // Return if there's a syntax error
        if (env->document->context->syntaxErrorInfo.hasError) {
            return env->document->context->syntaxErrorInfo.errorItem.errorId;
        }

        // Validate types, values and finding Main(entry) function
        Validator::validateScript(env->document);
        
        if (env->context->parseContext->semanticErrorInfo.hasError) {
            env->context->setErrorPositions();
            return env->context->parseContext->semanticErrorInfo.firstErrorItem->codeErrorItem.errorId;
        }

        // Run script
        return env->runScript();
    }
}