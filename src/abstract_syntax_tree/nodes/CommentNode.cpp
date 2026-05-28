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

    static int SimpleTextNodeStruct_applyFuncToDescendants(SimpleTextNodeStruct *node, ApplyFunc_params3)
    {
        if (targetVTable == nullptr || node->vtable == targetVTable) {
            func(Cast::upcast(node), ApplyFunc_pass);
        }

        return 0;
    }


    // now LineCommentNodeStruct and BlockCommentFragmentStruct are alias of SimpleTextNodeStruct
    // since they have the same structure and behavior. If we need to add more specific behavior for them, we can change their structure and vtable later.

    static node_vtable _lineCommentVTable = CREATE_VTABLE(LineCommentNodeStruct,
                                                           selfTextLength,
                                                           copySelfText,
                                                           appendToLine,
                                                          SimpleTextNodeStruct_applyFuncToDescendants,
                                                           "<Line Comment>", NodeTypeId::LineComment
    );

    const struct node_vtable *VTables::LineCommentVTable = &_lineCommentVTable;


    static node_vtable _blockCommentFragmentVTable = CREATE_VTABLE(BlockCommentFragmentStruct,
                                                                 selfTextLength,
                                                                 copySelfText,
                                                                 appendToLine,
                                                                                SimpleTextNodeStruct_applyFuncToDescendants,
                                                                 "<Comment Fragment>",
                                                                NodeTypeId::BlockCommentFragment);

    const struct node_vtable *VTables::BlockCommentFragmentVTable = &_blockCommentFragmentVTable;



    // virtual node does not have self text. underlying nodes will be appended to code line.
    static void copySelfText_blockcomment(BlockCommentNodeStruct *self, char *buf) {
        return;
    }

    static int selfTextLength_blockcomment(BlockCommentNodeStruct *self) {
        return 0;
    }


    static CodeLine *appendToLineForBlockComment(BlockCommentNodeStruct *self, CodeLine *currentCodeLine)
    {
        currentCodeLine =  currentCodeLine->AddAttachedFormatNodes(self)->appendNode(self);

        auto *commentFragment = self->firstCommentFragment;
        while (commentFragment) {
            currentCodeLine = VTableCall::callAppendToLine(commentFragment, currentCodeLine);

            commentFragment = Cast::downcast<BlockCommentFragmentStruct*>(commentFragment->nextNode);
        }
        return currentCodeLine;
    }


    static int BlockCommentNodeStruct_applyFuncToDescendants(
            BlockCommentNodeStruct *node, ApplyFunc_params3)
    {
        if (targetVTable == nullptr || node->vtable == targetVTable) {
            func(Cast::upcast(node), ApplyFunc_pass);
        }

        return 0;
    }

    static node_vtable _blockCommentVTable = CREATE_VTABLE(BlockCommentNodeStruct,
                                                                  selfTextLength_blockcomment,
                                                                  copySelfText_blockcomment,
                                                                 appendToLineForBlockComment,
                                                           BlockCommentNodeStruct_applyFuncToDescendants, "<BlockComment>", NodeTypeId::BlockComment
    );

    const struct node_vtable *VTables::BlockCommentVTable = &_blockCommentVTable;





    LineCommentNodeStruct *Alloc::newLineCommentNode(ParseContext *context, NodeBase *parentNode)
    {
        auto *lineComment = context->newMemForNode<LineCommentNodeStruct>();
        auto *node = Cast::upcast(lineComment);

        INIT_NODE(node, context, parentNode, VTables::LineCommentVTable);
        return lineComment;
    }


    BlockCommentFragmentStruct *Alloc::newBlockCommentFragmentNode(ParseContext *context, NodeBase *parentNode)
    {
        auto *comment = context->newMemForNode<BlockCommentFragmentStruct>();
        auto *node = Cast::upcast(comment);

        INIT_NODE(node, context, parentNode, VTables::BlockCommentFragmentVTable);
        return comment;
    }

    BlockCommentNodeStruct *Alloc::newBlockCommentNode(ParseContext *context, NodeBase *parentNode)
    {
        auto *node = context->newMem<BlockCommentNodeStruct>();
        INIT_NODE(node, context, parentNode, VTables::BlockCommentVTable);

        node->firstCommentFragment = nullptr;
        node->tagText = nullptr;
        node->tagTextLength = 0;
        return node;
    }
}