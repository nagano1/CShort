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

    static void copySelfText(SimpleTextNodeStruct *self, utf8byte *buf) {
        TEXT_MEMCPY(buf, self->text, self->textLength);
    }

    static int selfTextLength(SimpleTextNodeStruct *self) {
        return self->textLength;
    }


    static CodeLine *appendToLine(SimpleTextNodeStruct *self, CodeLine *currentCodeLine) {
        return currentCodeLine->AddAttachedFormatNodes(self)->appendNode(self);
    }

    static int SimpleTextNodeStruct_applyFuncToDescendants(
            SimpleTextNodeStruct *node, ApplyFunc_params3)
    {
        if (targetVTable == nullptr || node->vtable == targetVTable) {
            func(Cast::upcast(node), ApplyFunc_pass);
        }

        return 0;
    }


    static constexpr const char simpleTextTypeText[] = "<SimpleText>";

    static struct node_vtable simpleTextVTABLE = CREATE_VTABLE(SimpleTextNodeStruct,
                                                                selfTextLength,
                                                                copySelfText,
                                                                appendToLine,
                                                               SimpleTextNodeStruct_applyFuncToDescendants,
                                                                simpleTextTypeText
                                                                  , NodeTypeId::SimpleText);
    const struct node_vtable *VTables::SimpleTextVTable = &simpleTextVTABLE;








    SimpleTextNodeStruct *Alloc::newSimpleTextNode(ParseContext *context, NodeBase *parentNode) {
        auto *node = context->newMemForNode<SimpleTextNodeStruct>();
        Init::initSimpleTextNode(node, context, parentNode, 0);
        return node;
    }


    void Init::initSimpleTextNode(SimpleTextNodeStruct *textNode, ParseContext *context, void *parentNode, int charLen)
    {
        INIT_NODE(textNode, context, parentNode, VTables::SimpleTextVTable);

        textNode->text = context->memBuffer.newText(charLen);
        textNode->textLength = charLen;

        //TEXT_MEMCPY(boolNode->text, context->chars + start, length);
        textNode->text[charLen] = '\0';
    }

    void Init::assignText_SimpleTextNode(SimpleTextNodeStruct *textNode, ParseContext *context, int pos, int charLen)
    {
        textNode->text = context->memBuffer.newText(charLen);
        textNode->textLength = charLen;

        if (charLen > 0) {
            TEXT_MEMCPY(textNode->text, context->chars + pos, charLen);
        }
        textNode->text[charLen] = '\0';
    }
}