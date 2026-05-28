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
    // Line Break Node implementation
    static int selfTextLength(LineBreakNodeStruct *self) {
        return self->text[1] == '\0' ? 1 : 2;
    }

    static void copySelfText(LineBreakNodeStruct *self, utf8byte *buf) {
        buf[0] = self->text[0];
        if (self->text[1] != '\0') { // if it's "\r\n"
            buf[1] = self->text[1];
        }
    }

    static CodeLine *appendToLine(LineBreakNodeStruct *self, CodeLine *currentCodeLine) {
        auto *currentLineBreakItem = self;
        while (currentLineBreakItem) {
            currentCodeLine = currentCodeLine->AddAttachedFormatNodes(currentLineBreakItem);

            currentCodeLine->appendNode(Cast::upcast(currentLineBreakItem));

            // if there are multiple line breaks in a row, we need to add them all to the code line,
            // and the depth of the code line will be increased by 1 for each line break,
            // so that the nodes after the line breaks will be in the new line with correct indentation
            auto *newNextLine = self->context->newCodeLine();
            newNextLine->init(self->context);

            currentCodeLine->nextLine = newNextLine;
            currentCodeLine = newNextLine;

            currentCodeLine->depth = self->context->parentDepth + 1;

            currentLineBreakItem = currentLineBreakItem->nextLineBreakNode;
        }
        
        return currentCodeLine;
    }

    static int applyFuncToDescendants(LineBreakNodeStruct *node, ApplyFunc_params3) {

        if (targetVTable == nullptr || node->vtable == targetVTable) {
            func(Cast::upcast(node), ApplyFunc_pass);
        }

        return 0;
    }

    static constexpr const char LineBreakTypeText[] = "<LineBreak>";

    static node_vtable _lineBreakVTable = CREATE_VTABLE(LineBreakNodeStruct,
                                                        selfTextLength,
                                                        copySelfText,
                                                        appendToLine,
                                                        applyFuncToDescendants,
                                                        LineBreakTypeText,
                                                        NodeTypeId::LineBreak);

    const node_vtable *VTables::LineBreakVTable = &_lineBreakVTable;

    LineBreakNodeStruct *Alloc::newLineBreakNode(ParseContext *context, NodeBase *parentNode) {
        auto *lineNode = context->newLineBreakNode();
        auto *node = Cast::upcast(lineNode);

        INIT_NODE(node, context, parentNode, VTables::LineBreakVTable);
        lineNode->nextLineBreakNode = nullptr;
        lineNode->text[0] = '\n';
        lineNode->text[1] = '\0';

        return lineNode;
    }
}