#include <cstdio>
#include <iostream>
#include <string>
#include <array>
#include <algorithm>


#include <cstdlib>
#include <cassert>
#include <cstdio>
#include <chrono>
#include <unordered_map>
#include <vector>

#include <cstdint>
#include <ctime>

#include "code_nodes.hpp"

namespace cshort
{
    const constexpr char immutableMarkChar = '#';
    const constexpr char nullableMarkChar = '?';
    
    static constexpr const char let_chars[] = "let";
    static constexpr int size_of_let = sizeof(let_chars) - 1;

    static CodeLine *appendToLine(TypeNodeStruct *self, CodeLine *currentCodeLine)
    {
        currentCodeLine = TokenVTableCall::callAppendTokenToLine(&self->typeTextToken, currentCodeLine);
        return currentCodeLine;
    }

    static void copySelfText(TypeNodeStruct *self, utf8byte *buf)
    {
        /*
        bool hasImmutableOrNullableMark = self->hasImmutableMark || self->hasNullableMark;

        if (self->hasImmutableMark) {
            buf[0] = immutableMarkChar;
        } else if (self->hasNullableMark) {
            buf[0] = nullableMarkChar;
        }
        */
        VTableCall::copySelfText(&self->typeTextToken, buf);
        //VTableCall::copySelfText(&self->nameNode, buf + (hasImmutableOrNullableMark ? 1 : 0));
    }

    static int selfTextLength(TypeNodeStruct *self)
    {
        return self->typeTextToken.textLength;
        //bool hasImmutableOrNullableMark = self->hasImmutableMark || self->hasNullableMark;
        //return (hasImmutableOrNullableMark ? 1 : 0) + VTableCall::selfTextLength(Cast::upcast(&self->nameNode));
    }




    int Tokenizers::typeTokenizer(TokenizerParams_argNode_ch_start_context) {
        TypeNodeStruct *typeNode  = Cast::downcast<TypeNodeStruct*>(argNode);

        int currentPos = start;

        bool hasImmutableMark = false;
        bool hasNullableMark = false;

        if (immutableMarkChar == ch) { // # is immutable mark
            hasImmutableMark = true;
            currentPos += 1;
        }
        else if (nullableMarkChar == ch) { // ? is nullable mark
            hasNullableMark = true;
            currentPos += 1;
        }

        if (currentPos >= context->length) {
            return Search::NOTFOUND;
        }

        if (context->chars[currentPos] == nullableMarkChar || context->chars[currentPos] == immutableMarkChar) {
            return Search::NOTFOUND; // error: invalid type name like "??int", "##int", "#?int"
        }

        typeNode->hasNullableMark = hasNullableMark;
        typeNode->hasImmutableMark = hasImmutableMark;
        int result = Tokenizers::identifierTokenizer(Cast::upcast(&typeNode->nameNode), context->chars[currentPos], currentPos, context);

        if (Search::IsTokenized(result)) {
            typeNode->isLet = ParseUtil::equals(typeNode->typeTextToken.text, typeNode->typeTextToken.textLength, let_chars, size_of_let);

            Init::assignText_SimpleTextToken(&typeNode->typeTextToken, context, context->chars + start, result - start);

            context->mostLeftToken = Cast::upcastToken(&typeNode->typeTextToken);
            return result;
        }

        return Search::NOTFOUND;
    }


    static int applyFuncToDescendants(TypeNodeStruct *node, ApplyFunc_params3)
    {
        if (targetVTable == nullptr || node->vtable == targetVTable) {
            func(Cast::upcast(node), ApplyFunc_pass);
        }

        return 0;
    }

    static constexpr const char typeTypeText[] = "<Type>";

    static node_vtable _typeVTable = CREATE_VTABLE(TypeNodeStruct, selfTextLength,
                                                         copySelfText,
                                                         appendToLine,
                                                         applyFuncToDescendants,
                                                         typeTypeText,
                                                         NodeTypeId::Type);
    const node_vtable *VTables::TypeVTable = &_typeVTable;

    TypeNodeStruct *Alloc::newTypeNode(ParseContext *context, NodeBase *parentNode) {
        auto *node = context->newMem<TypeNodeStruct>();
        Init::initTypeNode(node, context, parentNode);
        return node;
    }

    void Init::initTypeNode(TypeNodeStruct *node, ParseContext *context, void *parentNode) {
        INIT_NODE(node, context, parentNode, VTables::TypeVTable);

        node->hasImmutableMark = false;
        node->hasNullableMark = false;
        node->isLet = false;

        Init::initIdentifierToken(&node->nameNode, context, node);
        Init::initSimpleTextToken(&node->typeTextToken, context, node, 0);
    }

}