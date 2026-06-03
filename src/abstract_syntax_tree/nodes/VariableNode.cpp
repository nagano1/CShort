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
    // Variable node is an expression node which has just an IdentifierToken, and it will be used in variable assignment and variable usage.
    
    static CodeLine *appendToLine(VariableNodeStruct *self, CodeLine *currentCodeLine) {
        return TokenVTableCall::callAppendTokenToLine(&self->identifierToken, currentCodeLine);
    }

    static void copySelfText(VariableNodeStruct *self, utf8byte *buf) {
        //TokenVTableCall::copySelfText(&self->identifierToken, buf);
    }

    static int selfTextLength(VariableNodeStruct *self) {
        return 0;//TokenVTableCall::selfTextLength(Cast::upcastToken(&self->identifierToken));
    }

    VariableNodeStruct *Alloc::newVariableNode(ParseContext *context, NodeBase *parentNode)
    {
        auto *node = context->newMem<VariableNodeStruct>();
        INIT_NODE(node, context, parentNode, VTables::VariableVTable);

        node->stackOffset = 0;
        auto *identifierToken = &node->identifierToken;
        Init::initIdentifierToken(identifierToken, context, parentNode);
        return node;
    }

    int Tokenizers::variableTokenizer(TokenizerParams_argNode_ch_start_context)
    {
        auto *variableNode = Alloc::newVariableNode(context, Cast::upcast(argNode));
        auto result = Tokenizers::identifierTokenizer(&variableNode->identifierToken, ch, start, context);
        if (Search::IsTokenized(result)) {
            context->generatedPrimaryNode = Cast::upcast(variableNode);
        }

        return result;
    }

    static int VariableNodeStruct_applyFuncToDescendants(VariableNodeStruct *node, ApplyFunc_params3)
    {
        if (targetVTable == nullptr || node->vtable == targetVTable) {
            func(Cast::upcast(node), ApplyFunc_pass);
        }

        return 0;
    }



static constexpr const char variableTypeText[] = "<Variable>";

    static node_vtable _variableVTable = CREATE_VTABLE(VariableNodeStruct, selfTextLength,
                                                         copySelfText, appendToLine,
                                                       VariableNodeStruct_applyFuncToDescendants,
                                                       variableTypeText,
                                                         NodeTypeId::Variable);
    const node_vtable *VTables::VariableVTable = &_variableVTable;
}