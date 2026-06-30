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
    // --------------------------------- IdentifiersAccessNode ------------------------------- /
    // IdentifiersAccessNode is an expression node which consists of a list of IdentifierToken.
    // syntax:
    // - namespace::className.property
    // - jp.co.company::className.property
    // - localVariable
    // ----------------------------------------------------------------------------------------/
    static CodeLine *appendToLine(IdentifiersAccessNodeStruct *self, CodeLine *currentCodeLine) {
        return TokenVTableCall::callAppendTokenToLine(&self->identifierToken, currentCodeLine);
    }

    // not used for nodes currently, but we can use it for error reporting or code generation later if needed.
    static void copySelfText(IdentifiersAccessNodeStruct *self, utf8byte *buf) {
    }

    // not used for nodes currently, but we can use it for error reporting or code generation later if needed.
    static int selfTextLength(IdentifiersAccessNodeStruct *self) {
        return 0;
    }

    IdentifiersAccessNodeStruct *Alloc::newIdentifiersAccessNode(ParseContext *context, NodeBase *parentNode)
    {
        auto *node = context->newMem<IdentifiersAccessNodeStruct>();
        INIT_NODE(node, context, parentNode, VTables::IdentifiersAccessVTable);

        node->stackOffset = 0;
        Init::initIdentifierToken(&node->identifierToken, context, parentNode);
        return node;
    }

    int Tokenizers::identifiersAccessTokenizer(TokenizerParams_argNode_ch_start_context)
    {
        auto *identifiersAccess = Alloc::newIdentifiersAccessNode(context, Cast::upcast(argNode));
        auto result = Tokenizers::identifierTokenizer(&identifiersAccess->identifierToken, ch, start, context);
        if (Search::IsTokenized(result)) {
            context->generatedPrimaryNode = Cast::upcast(identifiersAccess);
        }

        return result;
    }

    static int applyFuncToDescendants(IdentifiersAccessNodeStruct *node, ApplyFunc_params3)
    {
        if (targetVTable == nullptr || node->vtable == targetVTable) {
            func(Cast::upcast(node), ApplyFunc_pass);
        }

        return 0;
    }

    static constexpr const char identifiersAccessTypeText[] = "<IdentifiersAccess>";
    static node_vtable _identifiersAccessVTable = CREATE_VTABLE(IdentifiersAccessNodeStruct,
                                                               selfTextLength, copySelfText,
                                                               appendToLine,
                                                               applyFuncToDescendants,
                                                               identifiersAccessTypeText,
                                                               NodeTypeId::IdentifiersAccess);
    const node_vtable *VTables::IdentifiersAccessVTable = &_identifiersAccessVTable;
}