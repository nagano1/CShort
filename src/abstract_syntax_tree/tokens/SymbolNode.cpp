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

    static CodeLine *appendToLine(SymbolTokenStruct *self, CodeLine *currentCodeLine) {
        return currentCodeLine->AddAttachedFormatTokens(self)->appendToken(self);
    }

    static void copySelfText(SymbolTokenStruct *self, utf8byte *buf)
    {
        buf[0] = self->symbol[0];
    }

    static int selfTextLength(SymbolTokenStruct *) {
        return 1;
    }

    static int SymbolTokenStruct_applyFuncToDescendants(SymbolTokenStruct *token, TokenApplyFunc_params3) {

        if (targetVTable == nullptr || token->vtable == targetVTable) {
            func(Cast::upcastToken(token), ApplyFunc_pass);
        }

        return 0;
    }


    static constexpr const char SymbolTypeText[] = "<Symbol>";

    static token_vtable _symbolVTable = CREATE_TOKEN_VTABLE(SymbolTokenStruct, selfTextLength, copySelfText,
                                                   appendToLine, SymbolTokenStruct_applyFuncToDescendants, SymbolTypeText, TokenTypeId::Symbol);
    const token_vtable *VTables::SymbolVTable = &_symbolVTable;


    void Init::initSymbolToken(SymbolTokenStruct *token, ParseContext *context, void *parentNode, utf8byte letter) {
        INIT_TOKEN(token, context, parentNode, VTables::SymbolVTable);
        token->isEnabled = false;
        token->symbol[0] = letter;
        token->symbol[1] = '\0';
    }

}