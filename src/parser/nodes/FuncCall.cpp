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

    // -----------------------------------------------------------------------------------
    //
    //                              FuncCallArgumentItemStruct
    //
    // -----------------------------------------------------------------------------------
    static CodeLine *FuncCallArg_appendToLine(FuncCallArgItemStruct *self, CodeLine *currentCodeLine) {

        if (self->exprNode) {
            currentCodeLine = VTableCall::callAppendNodeToLine(self->exprNode, currentCodeLine);
        }

        if (self->hasComma) {
            currentCodeLine = TokenVTableCall::callAppendTokenToLine(&self->follwingComma, currentCodeLine);
        }

        return currentCodeLine;
    };

    static void copySelfText_FuncCallArgItem(FuncCallArgItemStruct *self, utf8byte *buf) {
        return;
    }

    static int FuncCallArg_selfTextLength(FuncCallArgItemStruct *) {
        return 0;
    }


    static int FuncCallArgItemStruct_applyFuncToDescendants(
            FuncCallArgItemStruct *node, ApplyFunc_params3)
    {

        if (parentIsFirst) {
            if (targetVTable == nullptr || node->vtable == targetVTable) {
                func(Cast::upcast(node), ApplyFunc_pass);
            }
        }
        if (node->exprNode) {
            node->exprNode->vtable->applyFuncToDescendants(Cast::upcast(node->exprNode), ApplyFunc_pass2);
        }
        if (!parentIsFirst) {
            if (targetVTable == nullptr || node->vtable == targetVTable) {
                func(Cast::upcast(node), ApplyFunc_pass);
            }
        }
        return 0;
    }


    static node_vtable _funcArgItemVTable = CREATE_VTABLE(FuncCallArgItemStruct,
                                                               FuncCallArg_selfTextLength,
                                                               copySelfText_FuncCallArgItem,
                                                               FuncCallArg_appendToLine,
                                                               FuncCallArgItemStruct_applyFuncToDescendants,
                                                               "<FuncCallArgItem>",
                                                               NodeTypeId::FuncArgument);

    const struct node_vtable *VTables::FuncCallArgVTable  = &_funcArgItemVTable;

    FuncCallArgItemStruct *Alloc::newFuncCallArgItem(ParseContext *context, NodeBase *parentNode) {
        auto *keyValueItem = context->newMem<FuncCallArgItemStruct>();

        INIT_NODE(keyValueItem, context, parentNode, &_funcArgItemVTable);

        Init::initSymbolToken(&keyValueItem->follwingComma, context, keyValueItem, ',');

        keyValueItem->hasComma = false;
        keyValueItem->exprNode = nullptr;

        return keyValueItem;
    }



    // -----------------------------------------------------------------------------------
    //
    //                              FuncCall Node
    //
    // -----------------------------------------------------------------------------------

    static CodeLine *funcCall_appendToLine(FuncCallNodeStruct *self, CodeLine *currentCodeLine)
    {
        if (self->callerExprNode) {
            currentCodeLine = VTableCall::callAppendNodeToLine(self->callerExprNode, currentCodeLine);
        }

        currentCodeLine = TokenVTableCall::callAppendTokenToLine(&self->openParenthesisToken, currentCodeLine);

        int formerParentDepth = self->context->currentIndentDepth;
        self->context->currentIndentDepth += 1;

        auto *argItem = self->firstArgumentItem;
        while (argItem != nullptr) {
            currentCodeLine = VTableCall::callAppendNodeToLine(argItem, currentCodeLine);
            argItem = Cast::downcast<FuncCallArgItemStruct *>(argItem->nextNode);
        }

        self->context->currentIndentDepth = formerParentDepth;

        currentCodeLine = TokenVTableCall::callAppendTokenToLine(&self->closeParenthesisToken, currentCodeLine);

        return currentCodeLine;
    }


    static void copySelfText_FuncCall(FuncCallNodeStruct *self, utf8byte *buf)
    {
        return;
    }

    static int funcCall_selfTextLength(FuncCallNodeStruct *self)
    {
        return 0;
    }


    static constexpr const char funcCallNodeTypeText[] = "<FuncCall>";


    static inline void appendRootNode(FuncCallNodeStruct *arr, FuncCallArgItemStruct *arrayItem) {
        assert(arr != nullptr && arrayItem != nullptr);

        if (arr->firstArgumentItem == nullptr) {
            arr->firstArgumentItem = arrayItem;
        }
        if (arr->lastArgumentItem != nullptr) {
            arr->lastArgumentItem->nextNode = Cast::upcast(arrayItem);
        }
        arr->lastArgumentItem = arrayItem;
    }


    enum phase {
        EXPECT_VALUE = 0,
        EXPECT_COMMA = 3
    };



    static inline int parseNextValue(TokenizerParams_argNode_ch_start_context, FuncCallNodeStruct* funcCallNode)
    {
        int result;
        if (Search::IsTokenized(result = Tokenizers::tokenizeExpression(TokenizerParams_pass))) {
            auto *nextItem = Alloc::newFuncCallArgItem(context, Cast::upcast(argNode));

            nextItem->exprNode = context->generatedPrimaryNode;
            appendRootNode(funcCallNode, nextItem);
            funcCallNode->parsePhase = phase::EXPECT_COMMA;
            return result;
        }
        return Search::NOTFOUND;
    }


    static int tokenizeFuncCallInternal(TokenizerParams_argNode_ch_start_context) {
        auto *funcCallNode = Cast::downcast<FuncCallNodeStruct*>(argNode);

        if (ch == ')') {
            funcCallNode->closeParenthesisToken.foundPos = start;
            context->mostLeftToken = Cast::upcastToken(&funcCallNode->closeParenthesisToken);
            context->scanEnd = true;
            return start + 1;
        }

        if (funcCallNode->parsePhase == phase::EXPECT_VALUE) {
            return parseNextValue(TokenizerParams_pass, funcCallNode);
        }

        auto *currentKeyValueItem = funcCallNode->lastArgumentItem;

        if (funcCallNode->parsePhase == phase::EXPECT_COMMA) {
            if (ch == ',') { // try to find ',' which leads to next key-value
                currentKeyValueItem->hasComma = true;
                currentKeyValueItem->follwingComma.foundPos = start;
                context->mostLeftToken = Cast::upcastToken(&currentKeyValueItem->follwingComma);
                funcCallNode->parsePhase = phase::EXPECT_VALUE;
                return start + 1;
            }
            else if (context->isAfterLineBreak) {
                // comma is not required after a line break
                return parseNextValue(TokenizerParams_pass, funcCallNode);
            }
            return Search::NOTFOUND;
        }
        return Search::NOTFOUND;
    }


    // expression is already tokenized.
    // we will try to tokenize the function call by looking for '(' after the expression.
    int Tokenizers::tokenizeFuncCall(TokenizerParams_argNode_ch_start_context)
    {
        NodeBase *parent = Cast::upcast(argNode);

        if ('(' != ch) {
            return Search::NOTFOUND;
        }

        assert(context->generatedPrimaryNode != nullptr);

        auto *funcCallNode = Alloc::newFuncCallNode(context, parent);
        funcCallNode->openParenthesisToken.foundPos = start;

        auto *primaryExprNode = context->generatedPrimaryNode;

        int currentPos = start + 1;
        int resultPos = Scanner::scanLoop(funcCallNode, tokenizeFuncCallInternal, context, currentPos);
        if (Search::IsTokenized(resultPos)) {
            primaryExprNode->parentNode = Cast::upcast(funcCallNode);
            funcCallNode->callerExprNode = primaryExprNode;
            context->generatedPrimaryNode = Cast::upcast(funcCallNode);
            return resultPos;
        }
        return Search::NOTFOUND;
    }


    static int funcCall_applyFuncToDescendants(
            FuncCallNodeStruct *node, ApplyFunc_params3)
    {
        if (parentIsFirst) {
            if (targetVTable == nullptr || node->vtable == targetVTable) {
                func(Cast::upcast(node), ApplyFunc_pass);
            }
        }

        if (node->callerExprNode) {
            node->callerExprNode->vtable->applyFuncToDescendants(
                    Cast::upcast(node->callerExprNode), ApplyFunc_pass2);
        }

        auto *item = node->firstArgumentItem;
        while (item != nullptr) {
            item->vtable->applyFuncToDescendants(
                    Cast::upcast(item), ApplyFunc_pass2);
            item = Cast::downcast<FuncCallArgItemStruct *>(item->nextNode);
        }

        if (!parentIsFirst) {
            if (targetVTable == nullptr || node->vtable == targetVTable) {
                func(Cast::upcast(node), ApplyFunc_pass);
            }
        }
        return 0;
    }

    static node_vtable _funcCallVTable = CREATE_VTABLE(FuncCallNodeStruct,
                                                             funcCall_selfTextLength,
                                                             copySelfText_FuncCall,
                                                             funcCall_appendToLine,
                                                             funcCall_applyFuncToDescendants,
                                                             funcCallNodeTypeText,
                                                             NodeTypeId::FuncCall);

    const node_vtable *VTables::FuncCallVTable = &_funcCallVTable;


    FuncCallNodeStruct *Alloc::newFuncCallNode(ParseContext *context, NodeBase *parentNode)
    {
        auto *node = context->newMem<FuncCallNodeStruct>();
        INIT_NODE(node, context, parentNode, VTables::FuncCallVTable);
        node->callerExprNode = nullptr;
        node->parsePhase = phase::EXPECT_VALUE;

        Init::initSymbolToken(&node->openParenthesisToken, context, node, '(');
        Init::initSymbolToken(&node->closeParenthesisToken, context, node, ')');
        node->closeParenthesisToken.isEndFlag = true;

        node->firstArgumentItem = nullptr;
        node->lastArgumentItem = nullptr;

        return node;
    }
}