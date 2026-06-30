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

#include "parser.hpp"

namespace cshort {

    static CodeLine *appendToLine(StringLiteralTokenStruct *self, CodeLine *currentCodeLine) {
        currentCodeLine = currentCodeLine->AddAttachedFormatTokens(self);
        return currentCodeLine->appendToken(self);
    }

    static void copySelfText(StringLiteralTokenStruct *self, utf8byte *buf) {
        TEXT_MEMCPY(buf, self->text, self->textLength);
    }

    static int selfTextLength(StringLiteralTokenStruct *self) {
        return self->textLength;
    }


    int Tokenizers::stringLiteralTokenizer(TokenizerParams_argNode_ch_start_context) {
        int strLength = 0; // including quotes, and escape characters


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
        bool succeededWithEndQuote = false;


        // find the closing quote, and count the length of the literal text
        for (int_fast32_t i = start + 1; i < context->length; i++) {
            strLength++;

            if (context->chars[i] == '\n' || context->chars[i] == '\r' || context->chars[i] == '\0') {
                context->setError(ErrorIndex::unexpected_line_break_or_null_in_string_literal, i);
                return Search::NOTFOUND;
            }

            if (escapeMode) {
                escapeMode = false;
                continue;
            }
            else if (context->chars[i] == '\\') {
                escapeMode = true;
                continue;
            }

            if (context->chars[i] == quoteChar) {
                succeededWithEndQuote = true;
                break;
            }
        }

        if (!succeededWithEndQuote) {
            context->setError(ErrorIndex::missing_closing_quote, start);
            return Search::NOTFOUND;
        }


        assert(strLength > 1); // at least has two quotes
        auto *strLiteralToken = context->newMem<StringLiteralTokenStruct>();
        Init::initStringLiteralNode(strLiteralToken, context, Cast::upcast(argNode));
        strLiteralToken->foundPos = start;
        context->mostLeftToken = Cast::upcastToken(strLiteralToken);

        strLiteralToken->text = context->memBuffer.newText(strLength);
        strLiteralToken->textLength = strLength;

        memcpy(strLiteralToken->text, context->chars + start, strLength);
        strLiteralToken->text[strLength] = '\0';


        strLiteralToken->literalType = literalType;
        strLiteralToken->str = strLiteralToken->text;
        strLiteralToken->strLength = strLength;

        return start + strLength;
    }

    static constexpr const char nameTypeText[] = "<string>";

    static int applyFuncToDescendants(StringLiteralTokenStruct *token, TokenApplyFunc_params3)
    {
        if (targetVTable == nullptr || token->vtable == targetVTable) {
            func(Cast::upcastToken(token), ApplyFunc_pass);
        }

        return 0;
    }

    static token_vtable _stringVTable = CREATE_TOKEN_VTABLE(StringLiteralTokenStruct,
                                                     selfTextLength,
                                                     copySelfText,
                                                     appendToLine,
                                                     applyFuncToDescendants,
                                                     nameTypeText,
                                                     TokenTypeId::StringLiteral);
    const token_vtable *VTables::StringLiteralTokenVTable = &_stringVTable;

    void Init::initStringLiteralNode(StringLiteralTokenStruct *name, ParseContext *context, NodeBase *parentNode)
    {
        INIT_TOKEN(name, context, parentNode, VTables::StringLiteralTokenVTable);
        name->text = nullptr;
        name->textLength = 0;
        name->str = nullptr;
        name->strLength = 0;
    }
}