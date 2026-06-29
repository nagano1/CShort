#pragma once

#include <stdlib.h>
//#include <array>
//
//#include <cstdlib>
//#include <cassert>
//#include <cstdio>
//#include <chrono>
//#include <unordered_map>
//
//#include <cstdint> // uint64_t, int_fast32_t
//#include <ctime>
//
//#include <string.h> // memcpy
//
#include "ParseUtil.hpp"
#include "common.hpp"
#include "parser.hpp"

namespace cshort
{
    struct _typeEntry;

    struct TypeManager {

        void setupBuiltInTypeSelectors();

        struct _typeEntry **typeEntryList;
        int typeEntryListCapacity;
        // next index to insert new type entry, which is also the count of type entries in the list
        int typeEntryListNextIndex;

        struct _typeEntry* getTypeEntryByIndex(int typeIndex) {
            assert(typeIndex >= 0 && typeIndex < this->typeEntryListNextIndex);
            return this->typeEntryList[typeIndex];
        }
        int typeFromNode(NodeBase *expressionNode);

        void validateScript(DocumentStruct *document);

        void validateFuncDef(FuncDefNodeStruct* funcDefNode);
        int runScript();

        void registerTypeEntry(struct _typeEntry* typeEntry);

        template<std::size_t SIZE>
        void addTypeAlias(struct _typeEntry* typeEntry, const char(&f3)[SIZE]) {
            this->addTypeAliasEntity(typeEntry , (char*)f3, SIZE-1);
        }

        void registerBuiltInTypes(ParseContext *context);
        struct _typeEntry *newTypeEntry(ParseContext *context) const;


        void addTypeAliasEntity(struct _typeEntry* typeEntry, char *aliasName , int length);

        ParseContext *context;
        void init(ParseContext *context) {
            this->context = context;
            typeEntryList = nullptr;
            typeEntryListNextIndex = 1;
            typeEntryListCapacity = 0;

            registerBuiltInTypes(context);

        }
    };

    // Node->TypeEntry mapping, used for type checking and type inference during script validation and execution.
    // node->typeIndex is the index of the type entry in the ScriptEnv->typeEntryList, which is used to get the TypeEntry for the node.
    // this is mainly because the node->typeIndex is an int (we want NodeBase to be independent to script engine as much as possible).
    using TypeEntry = struct _typeEntry {
        int typeIndex;
        int dataSize;
        bool isReferenceType; // class rather than struct

        //char *(*toString)(ParseContext *context, ValueBase* value);
        int (*binary_operate)(ParseContext *context, BinaryOperationNodeStruct *binaryNode);
        //int (*binary_operate)(ScriptEngineContext *context, BinaryOperationNodeStruct *binaryNode, bool typeCheck);
        int (*canAssignTypeImplicitly)(ParseContext *context, _typeEntry *typeEntry);
        void (*evaluateNode)(ParseContext *context, NodeBase *node);

        char *typeChars;
        int typeCharsLength;
        bool isBuiltIn;
        BuildinTypeId typeId;

        //template<typename T, std::size_t SIZE>
        template<std::size_t SIZE>
        void initAsBuiltInType(/*decltype(toString) f1, */decltype(binary_operate) f2, decltype(canAssignTypeImplicitly) f8,
                               //void(*evaluateNode2)(ScriptEngineContext *context, T *node),
                               const char(&f3)[SIZE], decltype(typeId) f4, decltype(dataSize) f5, decltype(isReferenceType) f7
        ) {
            //this->toString = f1;
            this->binary_operate = f2;
            this->canAssignTypeImplicitly = f8;
            //this->evaluateNode = (void(*)(ScriptEngineContext *context, NodeBase *node))evaluateNode2;
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
}