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
    //------------------------------------------------------------------------------------------
    //
    //                                      TypeEntry
    //
    //------------------------------------------------------------------------------------------

    TypeEntry *TypeManager::newTypeEntry(ParseContext *context) const
    {
        auto *typeEntry = context->memBufferForValidation.newMem<TypeEntry>(1);
        //typeEntry->toString = nullptr;
        typeEntry->binary_operate = nullptr;
        typeEntry->canAssignTypeImplicitly = nullptr;
        //typeEntry->evaluateNode = nullptr;
        typeEntry->typeChars = nullptr;
        typeEntry->typeCharsLength = 0;
        typeEntry->typeId = BuildinTypeId::Null;
        typeEntry->dataSize = 0;
        typeEntry->isBuiltIn = false;
        typeEntry->isReferenceType = false;
        typeEntry->typeIndex = (int)TypeIndexConst::NotAssigned;
        return typeEntry;
    }

    /// returns false if the type entry list cannot be expanded due to memory allocation failure.
    static bool expandTypeEntryList(TypeManager *scriptEnv)
    {
        auto *oldListPointer = scriptEnv->typeEntryList;
        if (oldListPointer != nullptr) {
            // check if there is enough capacity to add a new type entry.
            if (scriptEnv->typeEntryListNextIndex < scriptEnv->typeEntryListLength) {
                return true;
            }
        }
        
        // calculate the new capacity for the type entry list
        const int INCREMENT_VALUE = 32;
        const int newCapa = INCREMENT_VALUE + scriptEnv->typeEntryListLength;
        auto *newList = (TypeEntry**)malloc(newCapa * sizeof(TypeEntry*));
        if (newList == nullptr) {
            return false; // memory allocation failed
        }

        scriptEnv->typeEntryList = newList;
        scriptEnv->typeEntryListLength = newCapa;

        if (oldListPointer != nullptr) {
            // copy the existing type entries from the old list to the new list.
            for (int i = 0; i < scriptEnv->typeEntryListNextIndex; i++) {
                scriptEnv->typeEntryList[i] = (TypeEntry*)oldListPointer[i];
            }
            free(oldListPointer);
        }
        return true;
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

    int canAssignType_i32(ParseContext *context, TypeEntry *otherType)
    {
        return 0;
    }

    int canAssignType_i64(ParseContext *context, TypeEntry *otherType)
    {
        if (otherType->typeIndex == BuiltInTypeIndex::int32) {
            return 1;
        }
        return 0;
    }

    int int32_binary_operate(ParseContext *context, BinaryOperationNodeStruct *binaryNode)
    {
        return BuiltInTypeIndex::int32;
    }

    int int64_binary_operate(ParseContext *context, BinaryOperationNodeStruct *binaryNode)
    {
        return BuiltInTypeIndex::int64;
    }


    int canAssignType_String(ParseContext *context, _typeEntry *otherType)
    {
        return 0;
    }

    int heapString_binary_operate(ParseContext *context, BinaryOperationNodeStruct *binaryNode)
    {
        return BuiltInTypeIndex::heapString;

    }

    int null_binary_operate(ParseContext *context, BinaryOperationNodeStruct *binaryNode)
    {
        return BuiltInTypeIndex::null;
    }

    int canAssignType_null(ParseContext *context, TypeEntry *otherType)
    {
        return 0;
    }

    void TypeManager::registerBuiltInTypes(ParseContext *context)
    {
        TypeManager *typeManager = context->typeManager;
        // int
        {
            TypeEntry *int32Type = TypeManager::newTypeEntry(context);
            int32Type->initAsBuiltInType(int32_binary_operate, canAssignType_i32,
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
                                         "i64", BuildinTypeId::Int64, 8, false); // 4byte
            typeManager->registerTypeEntry(int64Type);
            BuiltInTypeIndex::int64 = int64Type->typeIndex;
            typeManager->addTypeAlias(int64Type, "i64");
        }

        {
            // heap string
            TypeEntry *heapStringType = TypeManager::newTypeEntry(context);
            heapStringType->initAsBuiltInType(heapString_binary_operate,
                                              canAssignType_String,
                                              "heapString", BuildinTypeId::HeapString, 8, /*heap only*/true); //
            typeManager->registerTypeEntry(heapStringType);
            typeManager->addTypeAlias(heapStringType, "String");
            typeManager->addTypeAlias(heapStringType, "string");
            BuiltInTypeIndex::heapString = heapStringType->typeIndex;
        }

        {
            // null
            TypeEntry *nullTypeEntry = TypeManager::newTypeEntry(context);
            nullTypeEntry->initAsBuiltInType(null_binary_operate,  canAssignType_null,
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
    //                                 Type Selector for Node/VTable
    //
    //------------------------------------------------------------------------------------------

    int TypeManager::typeFromNode(NodeBase *node)
    {
        assert(node->vtable->typeSelector != nullptr);
        if (node->vtable->typeSelector != nullptr) {
            return node->vtable->typeSelector(node);
        }
        return -1;// node->typeIndex;
    }

    static int selectTypeFromNumberNode(NumberNodeStruct *numberNode)
    {
        if (numberNode->unit == 64) {
            numberNode->typeIndex = BuiltInTypeIndex::int64;
        }
        else {
            numberNode->typeIndex = BuiltInTypeIndex::int32;
        }

        return numberNode->typeIndex;
    }

    static int selectTypeFromConstNode(LiteralValueNodeStruct *nodeBase)
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
        return (int)TypeIndexConst::NotAssigned;
    }

    template<typename T>
    static void setTypeSelector(const node_vtable* vtable, int (*argToType)(T*)) {
        ((node_vtable*)vtable)->typeSelector = reinterpret_cast<int (*)(NodeBase *)>(argToType);
    }

    void TypeManager::initializeBuiltinTypeSelectors()
    {
        setTypeSelector(VTables::NumberVTable, selectTypeFromNumberNode);
        setTypeSelector(VTables::FixedLiteralVTable, selectTypeFromConstNode);
    }
}