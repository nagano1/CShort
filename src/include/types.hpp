#pragma once

#include <stdlib.h>

#include "ParseUtil.hpp"
#include "common.hpp"
#include "parser.hpp"

namespace cshort
{
    // Node->TypeEntry mapping, used for type checking and type inference during script validation and execution.
    // node->typeIndex is the index of the type entry in the ScriptEnv->typeEntryList, which is used to get the TypeEntry for the node.
    // this is mainly because the node->typeIndex is an int (we want NodeBase to be independent to script engine as much as possible).
    using TypeEntry = struct _typeEntry {
        int typeIndex;
        int dataSize;
        bool isReferenceType; // class rather than struct

        int (*binary_operate)(ParseContext *context, BinaryOperationNodeStruct *binaryNode);
        int (*canAssignTypeImplicitly)(ParseContext *context, _typeEntry *typeEntry);
        void (*evaluateNode)(ParseContext *context, NodeBase *node);

        char *typeChars;
        int typeCharsLength;
        bool isBuiltIn;
        BuildinTypeId typeId;

        template<std::size_t SIZE>
        void initAsBuiltInType(decltype(binary_operate) f2, decltype(canAssignTypeImplicitly) f8,
                               const char(&f3)[SIZE], decltype(typeId) f4, decltype(dataSize) f5, decltype(isReferenceType) f7
        ) {
            this->binary_operate = f2;
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

    struct TypeManager
    {
        TypeEntry **typeEntryList;
        int typeEntryListLength;
        // next index to insert new type entry, which is also the count of type entries in the list
        int typeEntryListNextIndex;

        VoidHashMap *typeNameMap;

        TypeEntry* getTypeEntryByIndex(int typeIndex) {
            assert(TypeManager::isValidTypeIndex(typeIndex));
            assert(typeIndex >= 0 && typeIndex < this->typeEntryListNextIndex);
            return this->typeEntryList[typeIndex];
        }

        TypeEntry* getTypeEntryByName(const char *typeName, int length) {
            return (TypeEntry*)this->typeNameMap->get(typeName, length);
        }


        void initializeBuiltinTypeSelectors();

        int typeFromNode(NodeBase *expressionNode);

        TypeEntry *newTypeEntry(ParseContext *context) const;
        
        void registerTypeEntry(TypeEntry* typeEntry);

        void addTypeAliasEntity(TypeEntry* typeEntry, char *aliasName , int length);

        template<std::size_t SIZE>
        void addTypeAlias(TypeEntry* typeEntry, const char(&f3)[SIZE]) {
            this->addTypeAliasEntity(typeEntry , (char*)f3, SIZE-1);
        }

        void registerBuiltInTypes(ParseContext *context);

        static bool isValidTypeIndex(int typeIndex) {
            return typeIndex > 0; // && typeIndex < typeEntryListNextIndex;
        }

        ParseContext *context;
        void init(ParseContext *context) {
            this->context = context;
            typeEntryList = nullptr;
            typeEntryListNextIndex = 1;
            typeEntryListLength = 0;
            this->typeNameMap = context->memBufferForValidation.newMem<VoidHashMap>(1);
            this->typeNameMap->init(&context->memBufferForValidation);

            registerBuiltInTypes(context);
        }
    };
}