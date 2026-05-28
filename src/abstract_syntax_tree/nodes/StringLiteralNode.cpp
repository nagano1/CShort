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

namespace cshort {

    static CodeLine *appendToLine(StringLiteralNodeStruct *self, CodeLine *currentCodeLine) {
        currentCodeLine = currentCodeLine->AddAttachedFormatNodes(self);
        return currentCodeLine->appendNode(self);
    }

    static void copySelfText(StringLiteralNodeStruct *self, utf8byte *buf) {
        TEXT_MEMCPY(buf, self->text, self->textLength);
    }

    static int selfTextLength(StringLiteralNodeStruct *self) {
        return self->textLength;
    }


    int Tokenizers::stringLiteralTokenizer(TokenizerParams_argNode_ch_start_context) {
        int strLength = 0; // including quotes, and escape characters

        bool endsWithQuote = false;

        char quoteChar;
        int literalType;

        if (ch == '"') {
            strLength++;
            quoteChar = '"';
            literalType = 0;
        }
        else if (ch == '`'){
            strLength++;
            quoteChar = '`';
            literalType = 1;
        }
        else {
            return Search::NOTFOUND;
        }


        bool escapeMode = false;

        // find the closing quote, and count the length of the literal text
        for (int_fast32_t i = start + 1; i < context->length; i++) {
            strLength++;

            if (escapeMode) {
                escapeMode = false;
                continue;
            }

            if (context->chars[i] == '\\') {
                escapeMode = true;
                continue;
            }

            if (context->chars[i] == quoteChar) {
                endsWithQuote = true;
                break;
            }
        }

        if (!endsWithQuote) {
            context->setError(ErrorIndex::missing_closing_quote, start);
            return Search::NOTFOUND;
        }


        assert(strlength > 1); // at least has two quotes
        auto *strLiteralNode = context->newMem<StringLiteralNodeStruct>();
        Init::initStringLiteralNode(strLiteralNode, context, argNode);
        context->setCodeNode(strLiteralNode);

        strLiteralNode->text = context->memBuffer.newText(strLength);
        strLiteralNode->textLength = strLength;

        memcpy(strLiteralNode->text, context->chars + start, strLength);
        strLiteralNode->text[strLength] = '\0';


        strLiteralNode->literalType = literalType;
        strLiteralNode->str = strLiteralNode->text;
        strLiteralNode->strLength = strLength;

        return start + strLength;
    }

    static constexpr const char nameTypeText[] = "<string>";

    static int applyFuncToDescendants(StringLiteralNodeStruct *node, ApplyFunc_params3)
    {
        if (targetVTable == nullptr || node->vtable == targetVTable) {
            func(Cast::upcast(node), ApplyFunc_pass);
        }

        return 0;
    }

    static node_vtable _stringVTable = CREATE_VTABLE(StringLiteralNodeStruct,
                                                     selfTextLength,
                                                     copySelfText,
                                                     appendToLine,
                                                     applyFuncToDescendants,
                                                     nameTypeText,
                                                     NodeTypeId::StringLiteral);
    const node_vtable *VTables::StringLiteralVTable = &_stringVTable;

    void Init::initStringLiteralNode(StringLiteralNodeStruct *name, ParseContext *context, NodeBase *parentNode)
    {
        INIT_NODE(name, context, parentNode, VTables::StringLiteralVTable);
        name->text = nullptr;
        name->textLength = 0;
        name->str = nullptr;
        name->strLength = 0;
    }
}