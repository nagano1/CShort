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

    static void copySelfText(SimpleTextTokenStruct *self, utf8byte *buf) {
        TEXT_MEMCPY(buf, self->text, self->textLength);
    }

    static int selfTextLength(SimpleTextTokenStruct *self) {
        return self->textLength;
    }


    static CodeLine *appendToLine(SimpleTextTokenStruct *self, CodeLine *currentCodeLine) {
        return currentCodeLine->AddAttachedFormatTokens(self)->appendToken(self);
    }

    static int SimpleTextTokenStruct_applyFuncToDescendants(SimpleTextTokenStruct *token, TokenApplyFunc_params3)
    {
        if (targetVTable == nullptr || token->vtable == targetVTable) {
            func(Cast::upcastToken(token), ApplyFunc_pass);
        }

        return 0;
    }


    static constexpr const char simpleTextTypeText[] = "<SimpleText>";

    static struct token_vtable simpleTextVTABLE = CREATE_TOKEN_VTABLE(SimpleTextTokenStruct,
                                                                selfTextLength,
                                                                copySelfText,
                                                                appendToLine,
                                                               SimpleTextTokenStruct_applyFuncToDescendants,
                                                                simpleTextTypeText
                                                                  , TokenTypeId::SimpleText);
    const struct token_vtable *VTables::SimpleTextVTable = &simpleTextVTABLE;








    SimpleTextTokenStruct *Alloc::newSimpleTextToken(ParseContext *context, NodeBase *parentNode) {
        auto *token = context->newMemForNode<SimpleTextTokenStruct>();
        Init::initSimpleTextToken(token, context, parentNode, 0);
        return token;
    }


    void Init::initSimpleTextToken(SimpleTextTokenStruct *textToken, ParseContext *context, void *parentNode, int charLen)
    {
        INIT_TOKEN(textToken, context, parentNode, VTables::SimpleTextVTable);

        textToken->text = context->memBuffer.newText(charLen);
        textToken->textLength = charLen;

        //TEXT_MEMCPY(boolNode->text, context->chars + start, length);
        textToken->text[charLen] = '\0';
    }

    void Init::assignText_SimpleTextToken(SimpleTextTokenStruct *name, ParseContext *context, const utf8byte *text, int charLen)
    {
        name->text = context->memBuffer.newText(charLen);
        name->textLength = charLen;

        if (charLen > 0) {
            TEXT_MEMCPY(name->text, text, charLen);
        }
        name->text[charLen] = '\0';
    }
}