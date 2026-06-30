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
#include "types.hpp"

namespace cshort {
    TypeEntry *TypeManager::newTypeEntry(ParseContext *context) const
    {
        auto *emptyTypeEntry = context->memBuffer.newMem<TypeEntry>(1);
        //emptyTypeEntry->toString = nullptr;
        emptyTypeEntry->binary_operate = nullptr;
        emptyTypeEntry->canAssignTypeImplicitly = nullptr;
        //emptyTypeEntry->evaluateNode = nullptr;
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
    //                                      TypeEntry
    //
    //------------------------------------------------------------------------------------------

    // returns false if the type entry list cannot be expanded due to memory allocation failure.
    static bool expandTypeEntryList(TypeManager *scriptEnv)
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

    void TypeManager::registerTypeEntry(TypeEntry* typeEntry)
    {
        expandTypeEntryList(this);
        typeEntry->typeIndex = this->typeEntryListNextIndex;
        this->typeEntryList[typeEntry->typeIndex] = typeEntry;
        this->typeEntryListNextIndex++;
    }

    void TypeManager::addTypeAliasEntity(TypeEntry* typeEntry, char *aliasName , int length)
    {
        this->typeNameMap->put(aliasName, length, typeEntry);
    }



    //------------------------------------------------------------------------------------------
    //
    //                                Built-in Types
    //
    //------------------------------------------------------------------------------------------

    // Converts an int32 value to a string.
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

    int canAssignType_i32(ParseContext *context, TypeEntry *otherType)
    {
        return 0;
    }


    char* int64_toString(ParseContext *context, ValueBase *value)
    {
        /*
        const int bufferSize = 23;
        // snprintf append null terminator automatically.
        auto * chars = context->memBufferForHeap.newText(bufferSize);
        snprintf(chars, bufferSize, "%" PRId64, *(int64_t*)value->ptr);
        return chars;
        */
       return nullptr;
    }

    static void int32_evaluateNode(ParseContext *context, NumberNodeStruct *numberNode)
    {
        assert(numberNode != nullptr);
        assert(numberNode->typeIndex == BuiltInTypeIndex::int32);
        *(int32_t*)numberNode->calcReg = (int32_t)numberNode->num;
    }


    int canAssignType_i64(ParseContext *context, _typeEntry *otherType)
    {
        if (otherType->typeIndex == BuiltInTypeIndex::int32) {
            return 1;
        }
        return 0;
    }

    void int64_evaluateNode(ParseContext *context, NumberNodeStruct *numberNode)
    {
        assert(numberNode != nullptr);
        assert(numberNode->typeIndex == BuiltInTypeIndex::int64);
        *(int64_t*)numberNode->calcReg = numberNode->num;
    }

    int int32_binary_operate(ParseContext *context, BinaryOperationNodeStruct *binaryNode)
    {
            return BuiltInTypeIndex::int32;
    }

    int int64_binary_operate(ParseContext *context, BinaryOperationNodeStruct *binaryNode)
    {
        return BuiltInTypeIndex::int64;
    }


    void heapString_evaluateNode(ParseContext *context, LiteralValueNodeStruct *node)
    {
        /*
        StringLiteralTokenStruct *stringLiteralToken = node->stringLiteralToken;
        char *chars;
        int size = (1 + stringLiteralToken->strLength) * (int)sizeof(char);
        ValueBase *value = context->genValueBase(BuiltInTypeIndex::heapString, size, &chars);
        memcpy(chars, stringLiteralToken->str, stringLiteralToken->strLength);
        chars[stringLiteralToken->strLength] = '\0';
        *(ValueBase **)node->calcReg = value;
        */
    }

    char* heapString_toString(ParseContext *context, ValueBase* value)
    {
        return (char*)value->ptr;
    }


    int canAssignType_String(ParseContext *context, _typeEntry *otherType)
    {
        return 0;
    }

    int heapString_binary_operate(ParseContext *context, BinaryOperationNodeStruct *binaryNode)
    {
        return BuiltInTypeIndex::heapString;

    }


    char* null_toString(ParseContext *context, ValueBase* value)
    {
        return (char*)"null";
    }

    int null_binary_operate(ParseContext *context, BinaryOperationNodeStruct *binaryNode)
    {
        return BuiltInTypeIndex::null;
    }

    int canAssignType_null(ParseContext *context, _typeEntry *otherType)
    {
        return 0;
    }

    void null_evaluateNode(ParseContext *context, LiteralValueNodeStruct *node)
    {
        *(int64_t*)node->calcReg = 0;
    }

    void TypeManager::registerBuiltInTypes(ParseContext *context)
    {
        TypeManager *typeManager = context->typeManager;
        // int
        {

            TypeEntry *int32Type = TypeManager::newTypeEntry(context);
            int32Type->initAsBuiltInType(/*int32_toString,*/ int32_binary_operate, canAssignType_i32,
                                         // int32_evaluateNode,
                                         "int", BuildinTypeId::Int32, 4, false); // 4byte
            typeManager->registerTypeEntry(int32Type);
            BuiltInTypeIndex::int32 = int32Type->typeIndex;
            typeManager->addTypeAlias(int32Type, "int");
            typeManager->addTypeAlias(int32Type, "i32");
        }

        {
            // i64
            TypeEntry *int64Type = TypeManager::newTypeEntry(context);
            int64Type->initAsBuiltInType(int64_binary_operate, canAssignType_i64,
                                         //int64_evaluateNode,
                                         "i64", BuildinTypeId::Int64, 8, false); // 4byte
            typeManager->registerTypeEntry(int64Type);
            BuiltInTypeIndex::int64 = int64Type->typeIndex;
            typeManager->addTypeAlias(int64Type, "i64");
        }

        {
            // heap string
            TypeEntry *heapStringType = TypeManager::newTypeEntry(context);
            heapStringType->initAsBuiltInType(heapString_binary_operate,  canAssignType_String,
                                              //heapString_evaluateNode,
                                              "heapString", BuildinTypeId::HeapString, 8, /*heap only*/true); //
            typeManager->registerTypeEntry(heapStringType);
            typeManager->addTypeAlias(heapStringType, "String");
            typeManager->addTypeAlias(heapStringType, "string");
            BuiltInTypeIndex::heapString = heapStringType->typeIndex;
        }

        {
            // null
            TypeEntry *nullTypeEntry = TypeManager::newTypeEntry(context);
            nullTypeEntry->initAsBuiltInType(/*null_toString, */null_binary_operate,  canAssignType_null,
                                             //null_evaluateNode,
                                              "null", BuildinTypeId::Null, 8, /*heap only*/true); //
            typeManager->registerTypeEntry(nullTypeEntry);
            typeManager->addTypeAlias(nullTypeEntry, "Null");
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

    int TypeManager::typeFromNode(NodeBase *node)
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

        return (int)TypeIndexEnum::NotAssigned;
    }

    template<typename T>
    static void setTypeSelector(const node_vtable* vtable, int (*argToType)(ScriptEnv*, T*)) {
        ((node_vtable*)vtable)->typeSelector = reinterpret_cast<int (*)(void *, NodeBase *)>(argToType);
    }

    void TypeManager::setupBuiltInTypeSelectors()
    {
        setTypeSelector(VTables::NumberVTable, selectTypeFromNumberNode);
        setTypeSelector(VTables::FixedLiteralVTable, selectTypeFromConstNode);
    }
}