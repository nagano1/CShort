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
    // Line Break Token
    static int selfTextLength(LineBreakTokenStruct *self) {
        return self->text[1] == '\0' ? 1 : 2;
    }

    static void copySelfText(LineBreakTokenStruct *self, utf8byte *buf) {
        buf[0] = self->text[0];
        if (self->text[1] != '\0') { // if it's "\r\n"
            buf[1] = self->text[1];
        }
    }

    static CodeLine *appendToLine(LineBreakTokenStruct *self, CodeLine *currentCodeLine) {
        auto *currentLineBreakItem = self;
        while (currentLineBreakItem) {
            currentCodeLine = currentCodeLine->AddAttachedFormatTokens(currentLineBreakItem);

            currentCodeLine->appendToken(currentLineBreakItem);

            // If there are multiple line breaks in a row, append them all and create one new CodeLine per break.
            // The new line depth is derived from the current parentDepth (indentation context) and does not accumulate per break.
            auto *newNextLine = self->context->newCodeLine();
            newNextLine->init(self->context);

            currentCodeLine->nextLine = newNextLine;
            currentCodeLine = newNextLine;

            currentCodeLine->depth = self->context->parentDepth + 1;

            currentLineBreakItem = currentLineBreakItem->nextLineBreak;
        }
        
        return currentCodeLine;
    }

    static int applyFuncToDescendants(LineBreakTokenStruct *token, TokenApplyFunc_params3) {

        if (targetVTable == nullptr || token->vtable == targetVTable) {
            func(Cast::upcastToken(token), ApplyFunc_pass);
        }

        return 0;
    }

    static constexpr const char LineBreakTypeText[] = "<LineBreak>";

    static token_vtable _lineBreakVTable = CREATE_TOKEN_VTABLE(LineBreakTokenStruct,
                                                        selfTextLength,
                                                        copySelfText,
                                                        appendToLine,
                                                        applyFuncToDescendants,
                                                        LineBreakTypeText,
                                                        TokenTypeId::LineBreak);

    const token_vtable *VTables::LineBreakVTable = &_lineBreakVTable;

    LineBreakTokenStruct *Alloc::newLineBreakToken(ParseContext *context, NodeBase *parentNode) {
        auto *lineBreakToken = context->newLineBreakToken();

        INIT_TOKEN(Cast::upcastToken(lineBreakToken), context, parentNode, VTables::LineBreakVTable);
        lineBreakToken->nextLineBreak = nullptr;
        lineBreakToken->text[0] = '\n';
        lineBreakToken->text[1] = '\0';

        return lineBreakToken;
    }
}