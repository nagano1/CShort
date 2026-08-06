#pragma once

#include <stdlib.h>

#include "ParseUtil.hpp"
#include "common.hpp"
#include "parser.hpp"

namespace cshort
{
    struct _scriptEngineContext;

    enum class CanAssignResult {
        CanAssign,
        CannotAssign,
        CannotAssignWitoutExplicitConversion, // e.g. int64 cannot be assigned to int32 without explicit conversion
        CanAssignWithConversion // e.g. int32 can be assigned to int64 with conversion
    };

    // Node->TypeEntry mapping, used for type checking and type inference during script validation and execution.
    // node->typeIndex is the index of the type entry in the TypeManager->typeEntryList, which is used to get the TypeEntry for the node.
    // this is mainly because the node->typeIndex is an int (we want NodeBase to be independent to script engine as much as possible).
    using TypeEntry = struct _typeEntry {
        int typeIndex;
        int dataSize;
        bool isReferenceType; // class rather than struct

        type_index (*selectTypeOnBinaryOperation)(ParseContext *context, BinaryOperationNodeStruct *binaryNode);
        CanAssignResult (*canAssignTypeImplicitly)(ParseContext *context, _typeEntry *typeEntry);

        // script engine
        void (*evaluateNode)(struct _scriptEngineContext *context, NodeBase *node);
        void (*binary_operate)(struct _scriptEngineContext *context, BinaryOperationNodeStruct *binaryNode);
        bool (*convert_value_implicit)(st_byte *dest, st_byte *src, ParseContext *context, _typeEntry *srcType, _typeEntry *destType);

        char *typeChars;
        int typeCharsLength;
        bool isBuiltIn;
        BuildinTypeId typeId;

        template<std::size_t SIZE>
        void initAsBuiltInType(decltype(selectTypeOnBinaryOperation) f2, decltype(canAssignTypeImplicitly) f8,
                               const char(&f3)[SIZE], decltype(typeId) f4, decltype(dataSize) f5, decltype(isReferenceType) f7
        ) {
            this->selectTypeOnBinaryOperation = f2;
            this->canAssignTypeImplicitly = f8;
            this->typeChars = (char*)f3;
            this->typeCharsLength = SIZE;
            this->typeId = f4;
            this->dataSize = f5;
            this->isReferenceType = f7;
            this->isBuiltIn = true;
        }

        // get the stack size for this type, if it is a reference type, the stack size is 8 bytes (64bit), otherwise it is the data size of the type.
        int getStackSizeForType() const {
            return this->isReferenceType ? 8 : this->dataSize;
        }
    };


    struct Validator {
        static void validateScript(DocumentStruct *document);

        static void validateFuncDef(FuncDefNodeStruct* funcDefNode);
    };

    // Handle types and type checking
    struct TypeManager
    {
        TypeEntry **typeEntryList;
        int typeEntryListLength;
        // next index to insert new type entry, which is also the count of type entries in the list
        int typeEntryListNextIndex;

        VoidHashMap *typeNameMap;

        TypeEntry* getTypeEntryByIndex(type_index typeIndex) {
            assert(TypeManager::isValidTypeIndex(typeIndex));
            assert(typeIndex >= 0 && typeIndex < this->typeEntryListNextIndex);
            return this->typeEntryList[typeIndex];
        }

        TypeEntry* getTypeEntryByName(const char *typeName, int length) {
            auto *typeEntry = this->typeNameMap->get(typeName, length);
            return typeEntry != nullptr ? (TypeEntry*)*typeEntry : nullptr;
        }



        int typeFromNode(NodeBase *expressionNode);

        TypeEntry *newTypeEntry(ParseContext *context) const;
        
        void registerTypeEntry(TypeEntry* typeEntry);

        void addTypeAliasEntity(TypeEntry* typeEntry, char *aliasName , int length);

        template<std::size_t SIZE>
        void addTypeAlias(TypeEntry* typeEntry, const char(&f3)[SIZE]) {
            this->addTypeAliasEntity(typeEntry , (char*)f3, SIZE-1);
        }

        void registerBuiltInTypes(ParseContext *context);
        void initializeBuiltinTypeSelectors();

        static bool isValidTypeIndex(int typeIndex) {
            return typeIndex > 0; // && typeIndex < typeEntryListNextIndex;
        }

        void init(ParseContext *context) {
            typeEntryList = nullptr;
            typeEntryListNextIndex = 1;
            typeEntryListLength = 0;
            this->typeNameMap = context->memBufferForValidation.newMem<VoidHashMap>(1);
            this->typeNameMap->init(&context->memBufferForValidation);

            registerBuiltInTypes(context);
        }
    };
}