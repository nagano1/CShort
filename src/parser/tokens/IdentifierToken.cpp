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

    // --------------------- IdentifierToken ---------------------- /
    // used for identifiers in code, including variable names, function names, class names, etc.
    // IdentifierToken has additional check for valid identifier characters,
    // and it cannot be keywords, while SimpleTextToken can be any text.
    // -------------------------------------------------------------/

    static CodeLine *appendToLine(IdentifierTokenStruct *self, CodeLine *currentCodeLine) {
        currentCodeLine = currentCodeLine->AddAttachedFormatTokens(self);
        currentCodeLine->appendToken(self);

        return currentCodeLine;
    }

    static void copySelfText(IdentifierTokenStruct *self, utf8byte *buf) {
        TEXT_MEMCPY(buf, self->name, self->nameLength);
    }

    static int selfTextLength(IdentifierTokenStruct *self) {
        return self->nameLength;
    }


    /// @param argNode: an IdentifierTokenStruct pointer
    /// @return the position after the tokenized identifier, or NOTFOUND if tokenization failed.
    int Tokenizers::identifierTokenizer(TokenizerParams_argNode_ch_start_context) {
        // First character cannot be a digit (but allow '_' and non-ASCII bytes).
        if (!(('A' <= ch && ch <= 'Z') || ('a' <= ch && ch <= 'z') || ch == '_' || ((static_cast<unsigned char>(ch) & 0x80u) != 0))) {
            return Search::NOTFOUND;
        }

        int found_count = 0;
        for (int_fast32_t i = start; i < context->length; i++) {
            if (ParseUtil::isIdentifierLetter(context->chars[i])) {
                found_count++;
            }
            else {
                break;
            }
        }

        if (found_count > 0) {
            // ban keywords
            if (ParseUtil::IsKeyword(context->chars + start, found_count)) {
                return Search::NOTFOUND;
            }
            auto *identifierToken = Cast::downcast<IdentifierTokenStruct *>(argNode);

            context->mostLeftToken = Cast::upcastToken(identifierToken);
            identifierToken->name = context->memBuffer.newText(found_count);
            identifierToken->nameLength = found_count;
            identifierToken->foundPos = start;

            memcpy(identifierToken->name, context->chars + start, found_count);
            identifierToken->name[found_count] = '\0';

            return start + found_count;
        }

        return Search::NOTFOUND;
    }


    static int IdentifierTokenStruct_applyFuncToDescendants(IdentifierTokenStruct *token, TokenApplyFunc_params3)
    {
        if (targetVTable == nullptr || token->vtable == targetVTable) {
            func(Cast::upcastToken(token), ApplyFunc_pass);
        }

        return 0;
    }


    static constexpr const char nameTypeText[] = "<Identifier>";

    static token_vtable _identifierVTable = CREATE_TOKEN_VTABLE(IdentifierTokenStruct, selfTextLength,
                                                         copySelfText, appendToLine,
                                                   IdentifierTokenStruct_applyFuncToDescendants,
                                                         nameTypeText, TokenTypeId::Identifier);
    const token_vtable *VTables::IdentifierTokenVTable = &_identifierVTable;



    void Init::initIdentifierToken(IdentifierTokenStruct *identifier, ParseContext *context, void *parentNode) {
        INIT_TOKEN(identifier, context, parentNode, VTables::IdentifierTokenVTable);
        identifier->name = nullptr;
        identifier->nameLength = 0;
    }
}