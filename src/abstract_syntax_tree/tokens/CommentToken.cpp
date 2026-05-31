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

    
    static void copySelfText(SimpleTextTokenStruct *self, utf8byte *buf) {
        TEXT_MEMCPY(buf, self->text, self->textLength);
    }

    static int selfTextLength(SimpleTextTokenStruct *self) {
        return self->textLength;
    }

    static CodeLine *appendToLine(SimpleTextTokenStruct *self, CodeLine *currentCodeLine) {
        return currentCodeLine->AddAttachedFormatTokens(self)->appendToken(self);
    }

    static int SimpleTextTokenStruct_applyFuncToDescendants(SimpleTextTokenStruct *token, TokenApplyFunc_params3)
    {
        if (targetVTable == nullptr || token->vtable == targetVTable) {
            func(Cast::upcastToken(token), ApplyFunc_pass);
        }

        return 0;
    }


    // now LineCommentTokenStruct and BlockCommentFragmentStruct are alias of SimpleTextTokenStruct
    // since they have the same structure and behavior. If we need to add more specific behavior for them, we can change their structure and vtable later.

    static token_vtable _lineCommentVTable = CREATE_TOKEN_VTABLE(LineCommentTokenStruct,
                                                                 selfTextLength,
                                                                 copySelfText,
                                                                 appendToLine,
                                                                 SimpleTextTokenStruct_applyFuncToDescendants,
                                                                 "<Line Comment>", TokenTypeId::LineComment
    );

    const struct token_vtable *VTables::LineCommentVTable = &_lineCommentVTable;


    static token_vtable _blockCommentFragmentVTable = CREATE_TOKEN_VTABLE(BlockCommentFragmentStruct,
                                                                 selfTextLength,
                                                                 copySelfText,
                                                                 appendToLine,
                                                                 SimpleTextTokenStruct_applyFuncToDescendants,
                                                                 "<Comment Fragment>",
                                                                TokenTypeId::BlockCommentFragment);

    const struct token_vtable *VTables::BlockCommentFragmentVTable = &_blockCommentFragmentVTable;



    static void copySelfText_blockcomment(BlockCommentTokenStruct *self, char *buf) {
        return;
    }

    static int selfTextLength_blockcomment(BlockCommentTokenStruct *self) {
        return 0;
    }


    static CodeLine *appendToLineForBlockComment(BlockCommentTokenStruct *self, CodeLine *currentCodeLine)
    {
        currentCodeLine =  currentCodeLine->AddAttachedFormatTokens(self)->appendToken(self);

        auto *commentFragment = self->firstCommentFragment;
        while (commentFragment) {
            currentCodeLine = TokenVTableCall::callAppendTokenToLine(commentFragment, currentCodeLine);

            commentFragment = Cast::downcast<BlockCommentFragmentStruct*>(commentFragment->nextToken);
        }
        return currentCodeLine;
    }


    static int BlockCommentTokenStruct_applyFuncToDescendants(
            BlockCommentTokenStruct *token, TokenApplyFunc_params3)
    {
        if (targetVTable == nullptr || token->vtable == targetVTable) {
            func(Cast::upcastToken(token), ApplyFunc_pass);
        }

        return 0;
    }

    static token_vtable _blockCommentVTable = CREATE_TOKEN_VTABLE(BlockCommentTokenStruct,
                                                                  selfTextLength_blockcomment,
                                                                  copySelfText_blockcomment,
                                                                 appendToLineForBlockComment,
                                                           BlockCommentTokenStruct_applyFuncToDescendants, "<BlockComment>", TokenTypeId::BlockComment
    );

    const struct token_vtable *VTables::BlockCommentVTable = &_blockCommentVTable;





    LineCommentTokenStruct *Alloc::newLineCommentToken(ParseContext *context, NodeBase *parentNode)
    {
        auto *lineComment = context->newMemForNode<LineCommentTokenStruct>();
        auto *token = Cast::upcastToken(lineComment);

        INIT_TOKEN(token, context, parentNode, VTables::LineCommentVTable);
        return lineComment;
    }


    BlockCommentFragmentStruct *Alloc::newBlockCommentFragmentToken(ParseContext *context, NodeBase *parentNode)
    {
        auto *comment = context->newMemForNode<BlockCommentFragmentStruct>();
        auto *token = Cast::upcastToken(comment);

        INIT_TOKEN(token, context, parentNode, VTables::BlockCommentFragmentVTable);
        return comment;
    }

    BlockCommentTokenStruct *Alloc::newBlockCommentToken(ParseContext *context, NodeBase *parentNode)
    {
        auto *token = context->newMem<BlockCommentTokenStruct>();
        INIT_TOKEN(token, context, parentNode, VTables::BlockCommentVTable);

        token->firstCommentFragment = nullptr;
        token->tagText = nullptr;
        token->tagTextLength = 0;
        return token;
    }
}