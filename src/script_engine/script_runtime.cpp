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

#include "../include/script_runtime.hpp"

namespace cshort {
    //------------------------------------------------------------------------------------------
    //
    //                                      TypeEntry
    //
    //------------------------------------------------------------------------------------------

    // returns false if the type entry list cannot be expanded due to memory allocation failure.
    static bool expandTypeEntryList(ScriptEnv *scriptEnv)
    {
        auto *oldListPointer = scriptEnv->typeEntryList;
        if (oldListPointer != nullptr) {
            if (scriptEnv->typeEntryListNextIndex < scriptEnv->typeEntryListCapacity) {
                return true;
            }
        }
        
        // calculate the new capacity for the type entry list, which is 8 more than the current capacity.
        const int newCapa = 8 + scriptEnv->typeEntryListCapacity;
        auto *newList = (TypeEntry**)malloc(newCapa * sizeof(TypeEntry*));
        if (newList) {
            scriptEnv->typeEntryList = newList;
            scriptEnv->typeEntryListCapacity = newCapa;

            if (oldListPointer != nullptr) {
                // copy the existing type entries from the old list to the new list.
                for (int i = 0; i < scriptEnv->typeEntryListNextIndex; i++) {
                    scriptEnv->typeEntryList[i] = (TypeEntry*)oldListPointer[i];
                }
                free(oldListPointer);
            }

            return true;
        }
        return false;
    }

    void ScriptEnv::registerTypeEntry(TypeEntry* typeEntry)
    {
        expandTypeEntryList(this);
        typeEntry->typeIndex = this->typeEntryListNextIndex;
        this->typeEntryList[typeEntry->typeIndex] = typeEntry;
        this->typeEntryListNextIndex++;
    }

    void ScriptEnv::addTypeAliasEntity(TypeEntry* typeEntry, char *aliasName , int length)
    {
        this->context->typeNameMap->put(aliasName, length, typeEntry);
    }


    TypeEntry *ScriptEnv::newTypeEntry() const
    {
        auto *emptyTypeEntry = this->context->memBuffer.newMem<TypeEntry>(1);
        emptyTypeEntry->toString = nullptr;
        emptyTypeEntry->binary_operate = nullptr;
        emptyTypeEntry->canAssignTypeImplicitly = nullptr;
        emptyTypeEntry->evaluateNode = nullptr;
        emptyTypeEntry->typeChars = nullptr;
        emptyTypeEntry->typeCharsLength = 0;
        emptyTypeEntry->typeId = BuildinTypeId::Null;
        emptyTypeEntry->dataSize = 0;
        emptyTypeEntry->isBuiltIn = false;
        emptyTypeEntry->isReferenceType = false;
        emptyTypeEntry->typeIndex = (int)TypeIndexEnum::NotAssigned;
        return emptyTypeEntry;
    }



    //------------------------------------------------------------------------------------------
    //
    //                                Built-in Types
    //
    //------------------------------------------------------------------------------------------

    /// @brief Converts an int32 value to a string.
    char* int32_toString(ScriptEngineContext *context, ValueBase *value)
    {
        // allocate memory for the string representation of the int32 value
        //auto *chars = (char*)malloc(sizeof(char) * 64);
        const int bufferSize = 13;
        auto *chars = context->memBufferForHeap.newText(bufferSize);
        // PRId32 means the format specifier for a 32-bit signed integer,
        // ensuring that the output is correctly formatted regardless of platform.
        // snprintf append null terminator automatically
        snprintf(chars, bufferSize, "%" PRId32, *(int32_t*)value->ptr);
        return chars;
    }

    int canAssignType_i32(ScriptEngineContext *context, _typeEntry *otherType)
    {
        return 0;
    }


    char* int64_toString(ScriptEngineContext *context, ValueBase *value)
    {
        const int bufferSize = 23;
        // snprintf append null terminator automatically.
        auto * chars = context->memBufferForHeap.newText(bufferSize);
        snprintf(chars, bufferSize, "%" PRId64, *(int64_t*)value->ptr);
        return chars;
    }

    static void int32_evaluateNode(ScriptEngineContext *context, NumberNodeStruct *numberNode)
    {
        *(int32_t*)numberNode->calcReg = (int32_t)numberNode->num;
    }


    int canAssignType_i64(ScriptEngineContext *context, _typeEntry *otherType)
    {
        if (otherType->typeIndex == BuiltInTypeIndex::int32) {
            return 1;
        }
        return 0;
    }

    void int64_evaluateNode(ScriptEngineContext *context, NumberNodeStruct *numberNode)
    {
        *(int64_t*)numberNode->calcReg = numberNode->num;
    }

    int int32_binary_operate(ScriptEngineContext *context, BinaryOperationNodeStruct *binaryNode, bool typeCheck)
    {
        if (typeCheck) {
            return BuiltInTypeIndex::int32;
        }

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

    int int64_binary_operate(ScriptEngineContext *context, BinaryOperationNodeStruct *binaryNode, bool typeCheck)
    {
        if (typeCheck) {
            return BuiltInTypeIndex::int64;
        }

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
        return 0;
    }


    void heapString_evaluateNode(ScriptEngineContext *context, LiteralValueNodeStruct *node)
    {
        StringLiteralTokenStruct *stringLiteralToken = node->stringLiteralToken;
        char *chars;
        int size = (1 + stringLiteralToken->strLength) * (int)sizeof(char);
        ValueBase *value = context->genValueBase(BuiltInTypeIndex::heapString, size, &chars);
        memcpy(chars, stringLiteralToken->str, stringLiteralToken->strLength);
        chars[stringLiteralToken->strLength] = '\0';
        *(ValueBase **)node->calcReg = value;
    }

    char* heapString_toString(ScriptEngineContext *context, ValueBase* value)
    {
        return (char*)value->ptr;
    }


    int canAssignType_String(ScriptEngineContext *context, _typeEntry *otherType)
    {
        return 0;
    }

    int heapString_binary_operate(ScriptEngineContext *context, BinaryOperationNodeStruct *binaryNode, bool typeCheck)
    {
        if (typeCheck) {
            return BuiltInTypeIndex::heapString;
        }

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
            auto *value = context->genValueBase(BuiltInTypeIndex::heapString, (int)size, &chars);
            memcpy(chars, leftValue->ptr, leftValue->size);
            memcpy(chars + leftValue->size, rightValue->ptr, rightValue->size);
            chars[size - 1] = '\0';
            binaryNode->calcReg = (st_byte*)&value;
        }

        return 0;
    }


    char* null_toString(ScriptEngineContext *context, ValueBase* value)
    {
        return (char*)"null";
    }

    int null_binary_operate(ScriptEngineContext *context, BinaryOperationNodeStruct *binaryNode, bool typeCheck)
    {
        if (typeCheck) {
            return BuiltInTypeIndex::null;
        }

        return 0;
    }

    int canAssignType_null(ScriptEngineContext *context, _typeEntry *otherType)
    {
        return 0;
    }

    void null_evaluateNode(ScriptEngineContext *context, LiteralValueNodeStruct *node)
    {
        *(int64_t*)node->calcReg = 0;
    }

    static void _registerBuiltInTypes(ScriptEnv* scriptEnv)
    {
        // int
        {
            TypeEntry *int32Type = scriptEnv->newTypeEntry();
            int32Type->initAsBuiltInType(int32_toString, int32_binary_operate, canAssignType_i32,
                                         int32_evaluateNode,
                                         "int", BuildinTypeId::Int32, 4, false); // 4byte
            scriptEnv->registerTypeEntry(int32Type);
            BuiltInTypeIndex::int32 = int32Type->typeIndex;
            scriptEnv->addTypeAlias(int32Type, "int");
            scriptEnv->addTypeAlias(int32Type, "i32");
        }

        {
            // i64
            TypeEntry *int64Type = scriptEnv->newTypeEntry();
            int64Type->initAsBuiltInType(int64_toString, int64_binary_operate, canAssignType_i64,
                                         int64_evaluateNode,
                                         "i64", BuildinTypeId::Int64, 8, false); // 4byte
            scriptEnv->registerTypeEntry(int64Type);
            BuiltInTypeIndex::int64 = int64Type->typeIndex;
            scriptEnv->addTypeAlias(int64Type, "i64");
        }

        {
            // heap string
            TypeEntry *heapStringType = scriptEnv->newTypeEntry();
            heapStringType->initAsBuiltInType(heapString_toString,
                                              heapString_binary_operate,  canAssignType_String,
                                              heapString_evaluateNode,
                                              "heapString", BuildinTypeId::HeapString, 8, /*heap only*/true); //
            scriptEnv->registerTypeEntry(heapStringType);
            scriptEnv->addTypeAlias(heapStringType, "String");
            scriptEnv->addTypeAlias(heapStringType, "string");
            BuiltInTypeIndex::heapString = heapStringType->typeIndex;
        }

        {
            // null
            TypeEntry *nullTypeEntry = scriptEnv->newTypeEntry();
            nullTypeEntry->initAsBuiltInType(null_toString, null_binary_operate,  canAssignType_null,
                                             null_evaluateNode,
                                              "null", BuildinTypeId::Null, 8, /*heap only*/true); //
            scriptEnv->registerTypeEntry(nullTypeEntry);
            scriptEnv->addTypeAlias(nullTypeEntry, "Null");
            BuiltInTypeIndex::null = nullTypeEntry->typeIndex;
        }
    }

    int BuiltInTypeIndex::int32 = 0;
    int BuiltInTypeIndex::int64 = 0;
    int BuiltInTypeIndex::boolIdx = 0;
    int BuiltInTypeIndex::heapString = 0;
    int BuiltInTypeIndex::null = 0;

    //------------------------------------------------------------------------------------------
    //
    //                                 Type Selector from Node (static)
    //
    //------------------------------------------------------------------------------------------

    int ScriptEnv::typeFromNode(NodeBase *node)
    {
        if (node->vtable->typeSelector != nullptr) {
            return node->vtable->typeSelector(this, node);
        }
        return -1;
    }

    static int selectTypeFromNumberNode(ScriptEnv *env, NumberNodeStruct *numberNode)
    {
        if (numberNode->unit == 64) {
            numberNode->typeIndex = BuiltInTypeIndex::int64;
        }
        else {
            numberNode->typeIndex = BuiltInTypeIndex::int32;
        }

        return numberNode->typeIndex;
    }

    static int selectTypeFromConstNode(ScriptEnv *env, LiteralValueNodeStruct *nodeBase)
    {
        if (nodeBase->isStringLiteral) {
            return nodeBase->typeIndex = BuiltInTypeIndex::heapString;
        }
        else if (nodeBase->isNull) {
            return nodeBase->typeIndex = BuiltInTypeIndex::null;
        }
        else if (nodeBase->isTrue || nodeBase->isFalse) {
            return nodeBase->typeIndex = BuiltInTypeIndex::boolIdx;
        }
        assert(false); // Unknown literal type
    }

    static int selectTypeFromNullNode(ScriptEnv *env, LiteralValueNodeStruct *nodeBase)
    {
        return nodeBase->typeIndex = BuiltInTypeIndex::null;
    }

    template<typename T>
    static void setTypeSelector(const node_vtable* vtable, int (*argToType)(ScriptEnv*, T*)) {
        ((node_vtable*)vtable)->typeSelector = reinterpret_cast<int (*)(void *, NodeBase *)>(argToType);
    }

    static void setupBuiltInTypeSelectors(ScriptEnv *env)
    {
        setTypeSelector(VTables::NumberVTable, selectTypeFromNumberNode);
        setTypeSelector(VTables::FixedLiteralVTable, selectTypeFromConstNode);
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

    void ScriptEngineContext::init(ScriptEnv *scriptEnvArg)
    {
        this->scriptEnv = scriptEnvArg;
        this->semanticErrorInfo.hasError = false;
        this->semanticErrorInfo.count = 0;
        this->semanticErrorInfo.firstErrorItem = nullptr;
        this->semanticErrorInfo.lastErrorItem = nullptr;


        this->memBuffer.init();
        this->memBufferForHeap.initWithHeapEntryEnabled();
        this->memBufferForError.init();
        this->memBufferForValueBase.init();
        this->stackMemory.init();

        this->variableMap2 = this->memBuffer.newMem<VoidHashMap>(1);
        this->variableMap2->init(&this->memBuffer);

        this->typeNameMap = this->memBuffer.newMem<VoidHashMap>(1);
        this->typeNameMap->init(&this->memBuffer);
        this->cpuRegister = CPUSim{};
    }




    //------------------------------------------------------------------------------------------
    //
    //                                       Validate Script
    //
    //------------------------------------------------------------------------------------------
    
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

    static void assignCalcRegToNode(NodeBase *node, const ScriptEngineContext *context)
    {
        int typeIndex = node->typeIndex;
        if (typeIndex == -1) {
            typeIndex = BuiltInTypeIndex::int64;
        }

        auto *typeEntry = context->scriptEnv->getTypeEntryByIndex(typeIndex);
        int dataSize = typeEntry->dataSize;
        if (typeEntry->isReferenceType) {
            dataSize = 8;
        }

        const GPRRegister *calcRegister = GetGPRRegisterByEnum(node->calcRegEnum, &context->cpuRegister);
        if (calcRegister != nullptr) {
            st_byte* dataPointer = GetDataPointerFromGPRRegister(calcRegister, dataSize);
            node->calcReg = dataPointer;
        }
    }

    // assigns calculation registers to the left and right expression nodes of a binary operation node, as well as to the value node of a parentheses node, and to the expression node of an assignment or return statement node.
    // parent comes first

    static int applyFunc_assignCalcOpRegister(NodeBase *node, ApplyFunc_params2)
    {
        auto context = (ScriptEngineContext*)scriptEngineContext;

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

/*
        if (node->vtable == VTables::VariableVTable) {
            //node->calcRegEnum = PrimitiveCalcRegisterEnum::eax;
//            node->calcReg = (st_byte*)&__EX(&context->cpuRegister.rax);

            assignCalcRegToNode(node, context);
        }
*/
        /*
         * TODO: make calcFunc always returns with same register
        if (node->vtable == VTables::CallFuncVTable) {
            auto *callFunc = Cast::downcast<FuncCallNodeStruct *>(node);

            node->calcResultReg = PrimitiveCalcRegister::eax;
        }
        */
        return 0;
    }


    static void assignCalcOpRegister(ScriptEngineContext *context,
                                     FuncDefNodeStruct *func)
    {
        func->bodyNode.vtable->applyFuncToDescendants(Cast::upcast(&func->bodyNode),
                                                         (void*)context,
                                                         nullptr,
                                                         applyFunc_assignCalcOpRegister,
                                                         /*parent first*/true,
                                                         nullptr,
                                                         nullptr);
    }

    static void validateDeclaredTypeAssignmentNode(AssignmentNodeStruct *assign,
                                                   void *topLevelNodeInBody,
                                                   ScriptEngineContext *context)
    {
        TypeEntry *declaredType = nullptr;
        if (assign->hasTypeDecl()) {
            declaredType = (TypeEntry *) context->typeNameMap->get(
                assign->typeOrLet.nameNode.name,
                assign->typeOrLet.nameNode.nameLength
            );
            if (declaredType != nullptr) {
                assert(declaredType->typeIndex > 0);
                assign->typeIndex = declaredType->typeIndex;
            }
            else {
                // error: no type found
                context->addErrorWithNode(ErrorIndex::type_not_found, &assign->typeOrLet);
            }
        }

        if (assign->expressionNode) { // int b = 8, let b = 8
            assert(assign->expressionNode->typeIndex > 0);
            int childTypeIndex = assign->expressionNode->typeIndex;

            if (assign->typeOrLet.isLet) { // let b = 8
                assign->typeIndex = childTypeIndex;

                TypeEntry *typeEntry = context->scriptEnv->getTypeEntryByIndex(assign->typeIndex);
                assign->typeIndex = childTypeIndex;
            }
            else { // int b = 8
                if (declaredType != nullptr) {
                    if (declaredType->typeIndex != childTypeIndex) {
                        if (childTypeIndex == BuiltInTypeIndex::null){
                            if (!assign->typeOrLet.hasNullableMark) {
                                context->addErrorWithNode(ErrorIndex::assign_null_to_unnullable, assign);
                            }
                        }
                        else {
                            // check assignable
                            auto *targetTypeEntry = context->scriptEnv->getTypeEntryByIndex(assign->expressionNode->typeIndex);
                            bool canAssign = declaredType->canAssignTypeImplicitly(context, targetTypeEntry);
                            if (!canAssign) {
                                context->addErrorWithNode(ErrorIndex::type_is_not_assignable, assign);
                            }
                        }
                    }
                }
            }
        }
        else { // no value: int b, let b
            if (assign->typeOrLet.hasImmutableMark) {
                // #int a
                context->addErrorWithNode(ErrorIndex::cant_put_immutable_mark_for_non_value_assignment, &assign->typeOrLet);
            }

            if (assign->typeOrLet.isLet) { // let b
                context->addErrorWithNode(ErrorIndex::let_without_value, assign);
            }
        }
    }

    static void validateOnlyAssignmentNode(AssignmentNodeStruct *assign,
                                           void *topLevelNodeInBody,
                                           ScriptEngineContext *context
    ) {
        auto *currentStatement = Cast::downcast<NodeBase*>(topLevelNodeInBody);
        auto *bodyNode = Cast::downcast<FuncBodyNodeStruct *>(currentStatement->parentNode);
        assert(bodyNode->vtable == VTables::FuncBodyVTable);
        assert(assign->expressionNode);

        int childTypeIndex = assign->expressionNode->typeIndex;
        assign->typeIndex = childTypeIndex;

        auto *child = bodyNode->firstChildNode;
        bool hit = false;
        while (child) {
            if (child == currentStatement) {
                break;
            }
            if (child->vtable == VTables::AssignmentVTable) {
                auto *declAssign = Cast::downcast<AssignmentNodeStruct *>(child);
                if (declAssign->hasTypeOrLet) {
                    if (ParseUtil::equals(assign->variableNameToken.name, assign->variableNameToken.nameLength,
                                            declAssign->variableNameToken.name, declAssign->variableNameToken.nameLength)) {
                        if (declAssign->typeOrLet.hasImmutableMark) {
                            context->addErrorWithNode(ErrorIndex::assign_to_immutable, assign);
                        }

                        assign->stackOffset = declAssign->stackOffset;
                        hit = true;
                        if (childTypeIndex != declAssign->typeIndex) {
                            if (childTypeIndex > 0) {
                                auto *targetTypeEntry = context->scriptEnv->getTypeEntryByIndex(childTypeIndex);
                                bool canAssign = targetTypeEntry->canAssignTypeImplicitly(context, targetTypeEntry);
                                if (!canAssign) {
                                    // error
                                    context->addErrorWithNode(ErrorIndex::type_is_not_assignable, assign);
                                }
                            }
                        }

                        // if (assign->expressionNode->typeAtHeap != declAssign->typeAtHeap) {
                        //     // error
                        //     context->addErrorWithNode(ErrorIndex::type_is_not_assignable, assign);
                        // }
                    }
                }
            }
            child = child->nextNode;
        }

        if (!hit) {
            // error: no decl found
            context->addErrorWithNode(ErrorIndex::no_variable_defined, assign);
        }
    }


    // This function validates an assignment node by checking type declarations, type compatibility, and other constraints.
    static void validateAssignmentNode(NodeBase *node, void *topLevelNodeInBody, ScriptEngineContext *context) {
        auto *assign = Cast::downcast<AssignmentNodeStruct *>(node);

        if (assign->hasTypeOrLet) { // int b = 8, let b = 8, b = 4
            validateDeclaredTypeAssignmentNode(assign, topLevelNodeInBody, context);
        }
        else { // b = 4
            validateOnlyAssignmentNode(assign, topLevelNodeInBody, context);
        }
    }

    // children come first
    static int callTypeSelectorOnExpressions(NodeBase *node, ApplyFunc_params2)
    {
        auto *context = (ScriptEngineContext *)scriptEngineContext;

        if (node->vtable == VTables::BinaryOperationVTable) {
            auto *binary = Cast::downcast<BinaryOperationNodeStruct *>(node);

            int leftTypeIndex = binary->leftExprNode->typeIndex;
            int rightTypeIndex = binary->rightExprNode->typeIndex;
             assert(leftTypeIndex > -1 && rightTypeIndex > -1);

            auto *baseTypeEntry = context->scriptEnv->getTypeEntryByIndex(leftTypeIndex);
            auto *targetTypeEntry = context->scriptEnv->getTypeEntryByIndex(rightTypeIndex);
            if (baseTypeEntry->typeIndex == BuiltInTypeIndex::null) {
                auto *temp = targetTypeEntry;
                targetTypeEntry = baseTypeEntry;
                baseTypeEntry = temp;
            }

            if (baseTypeEntry->typeIndex == BuiltInTypeIndex::null) {
                context->addErrorWithNode(ErrorIndex::invalid_operator_for_type, binary);
                return 0;
            }

            int binaryType = baseTypeEntry->binary_operate(context, binary, true);
            assert(binaryType > -1);
            binary->typeIndex = binaryType;
        }
        else if (node->vtable == VTables::ParenthesesVTable) {
            auto *parentheses = Cast::downcast<ParenthesesNodeStruct *>(node);
            if (parentheses->valueNode != nullptr) {
                assert(parentheses->valueNode != nullptr && parentheses->valueNode->typeIndex > -1);
                parentheses->typeIndex = parentheses->valueNode->typeIndex;
            }
            else {
                parentheses->typeIndex = (int)TypeIndexEnum::Empty; // empty parentheses (syntax error)
            }
        }
        else if (node->vtable == VTables::ReturnStatementVTable) {
            auto *returnState = Cast::downcast<ReturnStatementNodeStruct*>(node);
            if (returnState->expressionNode != nullptr) {
                assert(returnState->expressionNode->typeIndex > -1);
                returnState->typeIndex = returnState->expressionNode->typeIndex;
            }
            else {
                returnState->typeIndex = (int)TypeIndexEnum::Empty; // return for void function
            }
        }
        else if (node->vtable == VTables::AssignmentVTable) {
            validateAssignmentNode(node, topLevelNodeInBody, context);
        }
        else if (node->vtable == VTables::IdentifiersAccessVTable) {
            auto *currentStatement = Cast::downcast<NodeBase*>(topLevelNodeInBody);
            auto *bodyNode = Cast::downcast<FuncBodyNodeStruct *>(currentStatement->parentNode);
            assert(bodyNode->vtable == VTables::FuncBodyVTable);

            auto *vari = Cast::downcast<IdentifiersAccessNodeStruct*>(node);
            auto varibleName = vari->identifierToken;//.name;

            auto *child = bodyNode->firstChildNode;
            while (child) {
                if (child == currentStatement) {
                    break;
                }
                if (child->vtable == VTables::AssignmentVTable) {
                    auto *declAssign = Cast::downcast<AssignmentNodeStruct *>(child);
                    if (declAssign->hasTypeOrLet) {
                        if (ParseUtil::equals(varibleName.name, varibleName.nameLength,
                                             declAssign->variableNameToken.name, declAssign->variableNameToken.nameLength)) {
                            vari->stackOffset = declAssign->stackOffset;
                            vari->typeIndex = declAssign->typeIndex;
                        }
                    }
                }
                child = child->nextNode;
            }
        }
        else {
            // for other nodes, like number, string literal, etc.
            node->typeIndex = context->scriptEnv->typeFromNode(node);
        }

        return 0; // varDefFound ? 1 : 0;
    }


    static FuncDefNodeStruct* findMainFunc(DocumentStruct* document)
    {
        auto* rootNode = document->firstRootNode;
        while (rootNode != nullptr) {
            if (rootNode->vtable == VTables::FuncDefVTable) {
                // fn
                auto* fnNode = Cast::downcast<FuncDefNodeStruct*>(rootNode);
                auto* nameNode = &fnNode->funcNameToken;
                if (ParseUtil::equals(nameNode->name, nameNode->nameLength, "Main", 4))
                {
                    return fnNode;
                }
            }
            rootNode = rootNode->nextNode;
        }
        return nullptr;
    }


    /// <summary>
    /// Sets typeIndex and Assigns stackOffset
    /// </summary>
    static void callTypeSelectorsOnExpressions2(ScriptEngineContext *context, FuncDefNodeStruct *func)
    {
        func->localVariableMap = context->memBuffer.newMem<VoidHashMap>(1);

        auto *statement = func->bodyNode.firstChildNode;
        int currentStackOffset = 0;
        while (statement) {
            statement->vtable->applyFuncToDescendants(
                    statement,
                    (void*)context,
                    nullptr,
                    callTypeSelectorOnExpressions,
                    /*children first*/false,
                    (void *) statement,
                    nullptr);

            // int a = 3
            if (statement->vtable == VTables::AssignmentVTable) {
                auto* assign = Cast::downcast<AssignmentNodeStruct*>(statement);
                if (assign->hasTypeOrLet && assign->typeIndex > -1) {
                    TypeEntry *typeEntry = context->scriptEnv->getTypeEntryByIndex(assign->typeIndex);
                    if (typeEntry->isReferenceType) {
                        currentStackOffset -= 8;
                    }
                    else {
                        currentStackOffset -= typeEntry->dataSize;
                    }
                    assign->stackOffset = currentStackOffset;
                }
            }

            statement = statement->nextNode;
        }
        func->stackSize = -currentStackOffset;
    }


    void ScriptEnv::validateFuncDef(FuncDefNodeStruct *func)
    {
        int errorCount = this->context->semanticErrorInfo.count;

        // set typeIndex to all expressions and assignments
        callTypeSelectorsOnExpressions2(context, func);

        if (errorCount == this->context->semanticErrorInfo.count) {
            assignCalcOpRegister(context, func);
        }

    }

    void ScriptEnv::validateScript()
    {
        assert(this->document->context->syntaxErrorInfo.hasError == false);

        // search all funcs
        auto *rootNode = document->firstRootNode;
        while (rootNode != nullptr) {
            if (rootNode->vtable == VTables::FuncDefVTable) {
                // fn
                auto *fnNode = Cast::downcast<FuncDefNodeStruct*>(rootNode);
                this->validateFuncDef(fnNode);
            }
            rootNode = rootNode->nextNode;
        }

        this->mainFunc = findMainFunc(this->document);
        if (this->mainFunc == nullptr) {
            // error: entry func not found
        }
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

        if (expressionNode->vtable == VTables::IdentifiersAccessVTable) {
            auto* variableNode = Cast::downcast<IdentifiersAccessNodeStruct *>(expressionNode);
            TypeEntry *typeEntry = this->scriptEnv->getTypeEntryByIndex(variableNode->typeIndex);
            if (typeEntry->isReferenceType) {
                this->stackMemory.moveFromStack(variableNode->stackOffset, 8, variableNode->calcReg);
            }
            else {
                int dataSize = typeEntry->dataSize;
                this->stackMemory.moveFromStack(variableNode->stackOffset, dataSize, variableNode->calcReg);
            }
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

            auto *leftTypeEntry = this->scriptEnv->getTypeEntryByIndex(binaryNode->leftExprNode->typeIndex);
            leftTypeEntry->binary_operate(this, binaryNode, /**/false);

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
            typeEntry = this->scriptEnv->getTypeEntryByIndex(typeIndex);
        }

        if (typeEntry) {
            typeEntry->evaluateNode(this, expressionNode);
        }
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

    ScriptEnv *ScriptEnv::newScriptEnv()
    {
        auto *scriptEnv = (ScriptEnv*)malloc(sizeof(ScriptEnv));
        if (scriptEnv != nullptr) {
            auto *context = mallocForType<ScriptEngineContext>();

            scriptEnv->context = context;
            context->init(scriptEnv);

            scriptEnv->mainFunc = nullptr;
            scriptEnv->typeEntryList = nullptr;
            scriptEnv->typeEntryListNextIndex = 1;
            scriptEnv->typeEntryListCapacity = 0;

            _registerBuiltInTypes(scriptEnv);
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
        int stackSize = mainFunc->stackSize;
        int baseBytesMinusOne = env->context->stackMemory.baseBytes - 1;
        const int alignedStackSize = (stackSize + baseBytesMinusOne) & ~baseBytesMinusOne; // Align to base bytes

        env->context->stackMemory.call(); // simulate call by pushing return address
        env->context->stackMemory.localVariables(alignedStackSize); // allocate space for local variables

        auto* statementNode = mainFunc->bodyNode.firstChildNode;
        while (statementNode != nullptr)
        {
            // call func: funcA(100)
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

            // assignment: int a = 3
            if (statementNode->vtable == VTables::AssignmentVTable) {
                auto* assignStatement = Cast::downcast<AssignmentNodeStruct *>(statementNode);
                if (assignStatement->expressionNode != nullptr) {
                    env->context->evaluateExprNode(assignStatement->expressionNode);
                    auto *typeEntry = env->getTypeEntryByIndex(assignStatement->typeIndex);

                    // 
                    auto dataSize = typeEntry->isReferenceType ? 8 : typeEntry->dataSize;
                    env->context->stackMemory.moveToStack(assignStatement->stackOffset, dataSize,
                                                     assignStatement->expressionNode->calcReg);
                }
            }

            // return 3
            if (statementNode->vtable == VTables::ReturnStatementVTable) {
                auto* returnNode = Cast::downcast<ReturnStatementNodeStruct*>(statementNode);
                env->context->evaluateExprNode(returnNode->expressionNode);
                auto* typeEntry = env->getTypeEntryByIndex(returnNode->expressionNode->typeIndex);

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
        ScriptEnv* env = ScriptEnv::newScriptEnv();
        setupBuiltInTypeSelectors(env);

        auto* document = Alloc::newDocument(DocumentType::CodeDocument);
        DocumentUtils::parseText(document, script, byteLength);
        env->document = document;

        return env;
    }



    int ScriptEnv::runScript()
    {
        assert(this->document->context->syntaxErrorInfo.hasError == false);
        assert(this->context->semanticErrorInfo.hasError == false);

        int ret = 0;
        auto *mainFunc = this->mainFunc;
        if (mainFunc) {
            // printf("main found <%s()>\n", mainFunc2->nameNode.name);
            TypeAndExpression typeAndExpression = executeFunc(this, mainFunc);
            if (typeAndExpression.typeEntry != nullptr) {
                if (typeAndExpression.typeEntry->dataSize == 8 || typeAndExpression.typeEntry->isReferenceType) {
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
        env->validateScript();
        if (env->context->semanticErrorInfo.hasError) {
            env->context->setErrorPositions();
            return env->context->semanticErrorInfo.firstErrorItem->codeErrorItem.errorId;
        }

        // Run script
        return env->runScript();
    }
}