#define _CRT_SECURE_NO_WARNINGS

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
    /*
     LiteralToken: true, false, null 
    */
    static CodeLine *appendToLine_LiteralToken(ConstLiteralTokenStruct *self, CodeLine *currentCodeLine) {
        return currentCodeLine->AddAttachedFormatTokens(self)->appendToken(self);
    }

    static void copySelfText_LiteralToken(ConstLiteralTokenStruct *self, utf8byte *buf) {
        TEXT_MEMCPY(buf, self->text, self->textLength);
    }

    static int selfTextLength_LiteralToken(ConstLiteralTokenStruct *self) {
        return self->textLength;
    }


    static int ConstLiteralTokenStruct_applyFuncToDescendants(ConstLiteralTokenStruct *token, TokenApplyFunc_params3) {
        if (targetVTable == nullptr || token->vtable == targetVTable) {
            func(Cast::upcastToken(token), ApplyFunc_pass);
        }

        return 0;
    }



    static constexpr const char literalNodeTypeText[] = "<const literal>";
    static token_vtable _literalVTable = CREATE_TOKEN_VTABLE(ConstLiteralTokenStruct,
                                                   selfTextLength_LiteralToken,
                                                   copySelfText_LiteralToken, appendToLine_LiteralToken,
                                                   ConstLiteralTokenStruct_applyFuncToDescendants,
                                                   literalNodeTypeText, TokenTypeId::ConstLiteral);

    const token_vtable *VTables::ConstLiteralVTable = &_literalVTable;

    ConstLiteralTokenStruct* Alloc::newConstLiteralToken(ParseContext *context, NodeBase *parentNode) {
        auto *token = context->newMem<ConstLiteralTokenStruct>();
        INIT_TOKEN(token, context, parentNode, VTables::ConstLiteralVTable);
        token->text = nullptr;
        token->textLength = 0;
        return token;
    }
}