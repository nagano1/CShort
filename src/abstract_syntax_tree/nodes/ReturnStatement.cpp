#include <cstdio>
#include <iostream>
#include <array>
#include <algorithm>


#include <cstdlib>
#include <cassert>
#include <cstdio>
#include <chrono>
#include <vector>

#include <cstdint>
#include <ctime>
#include <cstring>

#include "code_nodes.hpp"


namespace cshort {

    // --------------------- Defines ReturnStatement VTable ---------------------- /

    // virtual node does not have self text. underlying nodes will be appended to code line.
    static int selfTextLength(ReturnStatementNodeStruct *)
    {
        return 0;
    }

    static void copySelfText(ReturnStatementNodeStruct *self, utf8byte *buf)
    {
    }

    static CodeLine *appendToLine(ReturnStatementNodeStruct*self, CodeLine *currentCodeLine) {
        currentCodeLine = TokenVTableCall::callAppendTokenToLine(&self->returnText, currentCodeLine);

        auto *prevCodeLine = currentCodeLine;
        auto formerParentDepth = self->context->parentDepth;

        if (self->expressionNode) {
            currentCodeLine = VTableCall::callAppendNodeToLine(self->expressionNode, currentCodeLine);

            if (prevCodeLine != currentCodeLine) {
                currentCodeLine->depth = formerParentDepth + 1;
            }
        }

        self->context->parentDepth = formerParentDepth;

        return currentCodeLine;
    }


    static int ReturnStatementNodeStruct_applyFuncToDescendants(
            ReturnStatementNodeStruct *node, ApplyFunc_params3)
    {
        if (parentIsFirst) {
            if (targetVTable == nullptr || node->vtable == targetVTable) {
                func(Cast::upcast(node), ApplyFunc_pass);
            }
        }
        if (node->expressionNode) {
            node->expressionNode->vtable->applyFuncToDescendants(node->expressionNode, ApplyFunc_pass2);
        }

        if (!parentIsFirst) {
            if (targetVTable == nullptr || node->vtable == targetVTable) {
                func(Cast::upcast(node), ApplyFunc_pass);
            }
        }
        return 0;
    }

    static constexpr const char assignTypeText[] = "<ReturnStatement>";

    /*
     * return statement
     */
    static node_vtable _returnVTable = CREATE_VTABLE(ReturnStatementNodeStruct,
                                                          selfTextLength,
                                                          copySelfText,
                                                          appendToLine,
                                                          ReturnStatementNodeStruct_applyFuncToDescendants,
                                                          assignTypeText
                                                          , NodeTypeId::ReturnStatement);

    const struct node_vtable *VTables::ReturnStatementVTable = &_returnVTable;


    // -------------------- Implements AssignStatement Allocator --------------------- //
    ReturnStatementNodeStruct *Alloc::newReturnStatement(ParseContext *context, NodeBase *parentNode) {
        auto *returnStatement = context->newMem<ReturnStatementNodeStruct>();

        returnStatement->expressionNode = nullptr;

        Init::initReturnStatement(context, parentNode, returnStatement);
        return returnStatement;
    }

    constexpr char returnWord[] = "return";
    constexpr int returnTextSize = sizeof(returnWord) - 1;

    void Init::initReturnStatement(ParseContext *context, NodeBase *parentNode, ReturnStatementNodeStruct *returnStatement) {
        INIT_NODE(returnStatement, context, parentNode, &_returnVTable);

        returnStatement->expressionNode = nullptr;

        Init::initSimpleTextToken(&returnStatement->returnText, context, returnStatement, returnTextSize);
    }

    // "return" already matched, now try to match expression after return keyword
    static int tokenizeExpressionForReturnInternal(TokenizerParams_argNode_ch_start_context) {
        if (context->isAfterLineBreak) {
            // if there is line break after return keyword, it means there is no value for return statement.
            return Search::NOTFOUND;
        }

        auto *returnNode = Cast::downcast<ReturnStatementNodeStruct *>(argNode);
        int result = Tokenizers::tokenizeExpression(Cast::upcast(returnNode), ch,start, context);
        if (Search::IsTokenized(result)) {
            returnNode->expressionNode = context->generatedPrimaryNode;
            return result;
        }
        else {
            // no value for return statement. e.g. "return" or "return\n"
            return Search::NOTFOUND;
        }
    }

    // return 1234
    int Tokenizers::returnStatementTokenizer(TokenizerParams_argNode_ch_start_context) {
        // return
        if ('r' != ch) {
            return Search::NOTFOUND;
        }

        bool matched = ParseUtil::matchWordWithTerminatableEnd(context->chars, context->length, start, returnWord);
        if (matched) {
            auto *parent = Cast::upcast(argNode);
            auto *returnNode = Alloc::newReturnStatement(context, parent);
            Init::assignText_SimpleTextToken(&returnNode->returnText, context, context->chars + start, returnTextSize);

            int currentPos = start + returnTextSize;
            int resultPos = Scanner::scanOnce(returnNode, tokenizeExpressionForReturnInternal, context, currentPos);
            if (Search::IsTokenized(resultPos)) {
                context->mostLeftToken = Cast::upcastToken(&returnNode->returnText);
                context->generatedPrimaryNode = Cast::upcast(returnNode);
                return resultPos;
            }
            else { // no value for return statement. 
                context->mostLeftToken = Cast::upcastToken(&returnNode->returnText);
                context->generatedPrimaryNode = Cast::upcast(returnNode);
                return currentPos;
            }
        }

        return Search::NOTFOUND;
    }
}