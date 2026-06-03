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
    // Identifier Access node is an expression node which has list of IdentifierToken inside.
    // namespace::className.property
    // localVariable
    static CodeLine *appendToLine(IdentifierAccessNodeStruct *self, CodeLine *currentCodeLine) {
        return TokenVTableCall::callAppendTokenToLine(&self->identifierToken, currentCodeLine);
    }

    // not used for nodes currently, but we can use it for error reporting or code generation later if needed.
    static void copySelfText(IdentifierAccessNodeStruct *self, utf8byte *buf) {
    }

    // not used for nodes currently, but we can use it for error reporting or code generation later if needed.
    static int selfTextLength(IdentifierAccessNodeStruct *self) {
        return 0;
    }

    IdentifierAccessNodeStruct *Alloc::newIdentifierAccessNode(ParseContext *context, NodeBase *parentNode)
    {
        auto *node = context->newMem<IdentifierAccessNodeStruct>();
        INIT_NODE(node, context, parentNode, VTables::IdentifierAccessVTable);

        node->stackOffset = 0;
        auto *identifierToken = &node->identifierToken;
        Init::initIdentifierToken(identifierToken, context, parentNode);
        return node;
    }

    int Tokenizers::identifierAccessTokenizer(TokenizerParams_argNode_ch_start_context)
    {
        auto *variableNode = Alloc::newIdentifierAccessNode(context, Cast::upcast(argNode));
        auto result = Tokenizers::identifierTokenizer(&variableNode->identifierToken, ch, start, context);
        if (Search::IsTokenized(result)) {
            context->generatedPrimaryNode = Cast::upcast(variableNode);
        }

        return result;
    }

    static int IdentifierAccessNodeStruct_applyFuncToDescendants(IdentifierAccessNodeStruct *node, ApplyFunc_params3)
    {
        if (targetVTable == nullptr || node->vtable == targetVTable) {
            func(Cast::upcast(node), ApplyFunc_pass);
        }

        return 0;
    }

    static constexpr const char identifierAccessTypeText[] = "<IdentifierAccess>";

    static node_vtable _identifierAccessVTable = CREATE_VTABLE(IdentifierAccessNodeStruct, selfTextLength,
                                                       copySelfText, appendToLine,
                                                       IdentifierAccessNodeStruct_applyFuncToDescendants,
                                                       identifierAccessTypeText,
                                                         NodeTypeId::IdentifierAccess);
    const node_vtable *VTables::IdentifierAccessVTable = &_identifierAccessVTable;
}