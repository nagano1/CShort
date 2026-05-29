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



    void Init::initNameNode(NameNodeStruct *name, ParseContext *context, void *parentNode) {
        INIT_NODE(name, context, parentNode, VTables::NameVTable);
        name->name = nullptr;
        name->nameLength = 0;
    }
}