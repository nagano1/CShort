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

    static CodeLine *appendToLine(NameNodeStruct *self, CodeLine *currentCodeLine) {
        currentCodeLine = currentCodeLine->AddAttachedFormatNodes(self);
        currentCodeLine->appendNode(self);

        return currentCodeLine;
    }

    static void copySelfText(NameNodeStruct *self, utf8byte *buf) {
        TEXT_MEMCPY(buf, self->name, self->nameLength);
    }

    static int selfTextLength(NameNodeStruct *self) {
        return self->nameLength;
    }

    int Tokenizers::nameTokenizer(TokenizerParams_argNode_ch_start_context) {
        int found_count = 0;
        // TODO: first letter should be letter or _. simple solution is to use a flag to indicate if it's the first letter, and only allow letter or _ for the first letter, but it will add some overhead, since we need to check the flag for every letter. better solution is to use a separate loop to check the first letter, and then use another loop to check the rest of the letters, since it's common that the first letter is not valid, so we can fail fast without checking the rest of the letters.
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
            auto *nameNode = Cast::downcast<NameNodeStruct *>(argNode);

            context->setCodeNode(nameNode);
            nameNode->name = context->memBuffer.newText(found_count);
            nameNode->nameLength = found_count;
            nameNode->foundPos = start;

            memcpy(nameNode->name, context->chars + start, found_count);
            nameNode->name[found_count] = '\0';

            return start + found_count;
        }

        return Search::NOTFOUND;
    }


    VariableNodeStruct *Alloc::newVariableNode(ParseContext *context, NodeBase *parentNode)
    {
        auto *node = context->newMem<VariableNodeStruct>();
        INIT_NODE(node, context, parentNode, VTables::VariableVTable);

        node->stackOffset = 0;
        node->name = nullptr;
        node->nameLength = 0;   
        // segmentation fault 
        // Init::initNameNode(reinterpret_cast<NameNodeStruct *>(&node), context, parentNode);
        return node;
    }

    int Tokenizers::variableTokenizer(TokenizerParams_argNode_ch_start_context)
    {
        auto *variableNode = Alloc::newVariableNode(context, argNode);
        return Tokenizers::nameTokenizer(reinterpret_cast<NodeBase *>(variableNode), ch, start, context);
    }

    static int NameNodeStruct_applyFuncToDescendants(NameNodeStruct *node, ApplyFunc_params3)
    {
        if (targetVTable == nullptr || node->vtable == targetVTable) {
            func(Cast::upcast(node), ApplyFunc_pass);
        }

        return 0;
    }


    static constexpr const char nameTypeText[] = "<Name>";

    static node_vtable _nameVTable = CREATE_VTABLE(NameNodeStruct, selfTextLength,
                                                         copySelfText, appendToLine,
                                                   NameNodeStruct_applyFuncToDescendants,
                                                         nameTypeText, NodeTypeId::Name);
    const node_vtable *VTables::NameVTable = &_nameVTable;



    static constexpr const char variableTypeText[] = "<Variable>";

    static node_vtable _variableVTable = CREATE_VTABLE(VariableNodeStruct, selfTextLength,
                                                         copySelfText, appendToLine,
                                                       NameNodeStruct_applyFuncToDescendants,
                                                       variableTypeText,
                                                         NodeTypeId::Variable);
    const node_vtable *VTables::VariableVTable = &_variableVTable;


    void Init::initNameNode(NameNodeStruct *name, ParseContext *context, void *parentNode) {
        INIT_NODE(name, context, parentNode, VTables::NameVTable);
        name->name = nullptr;
        name->nameLength = 0;
    }
}