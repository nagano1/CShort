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
        if (self->pointerAsterisk.foundPos > -1) {
            currentCodeLine = TokenVTableCall::callAppendTokenToLine(&self->pointerAsterisk, currentCodeLine);
        }
        return currentCodeLine;
    }

    static void copySelfText(TypeNodeStruct *self, utf8byte *buf)
    {
        TokenVTableCall::copySelfText(&self->typeTextToken, buf);
    }

    static int selfTextLength(TypeNodeStruct *self)
    {
        return self->typeTextToken.textLength;
    }


    int asteriskTokenizer(TokenizerParams_argNode_ch_start_context) {
        if (ch == '*') {
            auto *token = Cast::downcast<SymbolTokenStruct *>(argNode);
            token->foundPos = start;
            token->symbol[0] = '*';
            token->symbol[1] = '\0';
            context->mostLeftToken = Cast::upcastToken(token);
            return start + 1;
        }

        return Search::NOTFOUND;
    }


    // include ? or # as leading letter, and include * if it's pointer type, for example:
    // ?string *
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
        int result = Tokenizers::identifierTokenizer(Cast::upcastToken(&typeNode->nameNode), context->chars[currentPos],  currentPos, context);

        bool isLet = false;
        if (result == Search::NOTFOUND) {
            // find let
            bool found = ParseUtil::matchWordWithTerminatableEnd(context->chars, context->length, currentPos, let_chars);
            if (found) {
                isLet = true;
                result = currentPos + size_of_let;
            }
        }

        if (Search::IsTokenized(result)) {
            typeNode->typeTextToken.foundPos = start;
            typeNode->nameNode.foundPos = currentPos;
            typeNode->isLet = isLet;

            Init::assignText_SimpleTextToken(&typeNode->typeTextToken, context, context->chars + start, result - start);

            context->mostLeftToken = Cast::upcastToken(&typeNode->typeTextToken);

            // find pointer asterisk
            int res = Scanner::scanOnce(&typeNode->pointerAsterisk, asteriskTokenizer, context, result);
            if (Search::IsTokenized(res)) {
                result = res;
            }

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

        node->text = nullptr;
        node->textLength = 0;

        node->hasImmutableMark = false;
        node->hasNullableMark = false;
        node->isLet = false;

        Init::initSymbolToken(&node->pointerAsterisk, context, node, '*');
        Init::initIdentifierToken(&node->nameNode, context, node);
        Init::initSimpleTextToken(&node->typeTextToken, context, node, 0);
    }
}