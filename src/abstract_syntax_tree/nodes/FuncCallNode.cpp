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
    //                              FuncArgumentItemStruct
    //
    // -----------------------------------------------------------------------------------
    static CodeLine *FuncArgument_appendToLine2(FuncArgumentItemStruct *self, CodeLine *currentCodeLine) {
        currentCodeLine = currentCodeLine->AddAttachedFormatNodes(self);

        currentCodeLine->appendNode(self);

        if (self->exprNode) {
            currentCodeLine = VTableCall::callAppendToLine(self->exprNode, currentCodeLine);
        }

        if (self->hasComma) {
            currentCodeLine = VTableCall::callAppendToLine(&self->follwingComma, currentCodeLine);
        }

        return currentCodeLine;
    };


    // virtual node does not have self text. underlying nodes will be appended to code line.
    static void copySelfText_FuncArgument(FuncArgumentItemStruct *self, utf8byte *buf) {
        return;
    }

    static int FuncArgument_selfTextLength2(FuncArgumentItemStruct *) {
        return 0;
    }


    static int FuncArgumentItemStruct_applyFuncToDescendants(
            FuncArgumentItemStruct *node, ApplyFunc_params3)
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


    static node_vtable _funcArgumentItemVTable = CREATE_VTABLE(FuncArgumentItemStruct,
                                                                     FuncArgument_selfTextLength2,
                                                                     copySelfText_FuncArgument,
                                                                     FuncArgument_appendToLine2,
                                                               FuncArgumentItemStruct_applyFuncToDescendants,
                                                                     "<FuncArgument>",
                                                                     NodeTypeId::FuncArgument);

    const struct node_vtable *VTables::FuncArgumentVTable  = &_funcArgumentItemVTable;

    FuncArgumentItemStruct *Alloc::newFuncArgumentItem(ParseContext *context, NodeBase *parentNode) {
        auto *keyValueItem = context->newMem<FuncArgumentItemStruct>();

        INIT_NODE(keyValueItem, context, parentNode, &_funcArgumentItemVTable);

        Init::initSymbolNode(&keyValueItem->follwingComma, context, keyValueItem, ',');

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
        if (self->exprNode) {
            currentCodeLine = VTableCall::callAppendToLine(self->exprNode, currentCodeLine);
        }

        //int formerParentDepth = self->context->parentDepth;
        //self->context->parentDepth += 1;
        //self->context->parentDepth = formerParentDepth;

        currentCodeLine = VTableCall::callAppendToLine(&self->openNode, currentCodeLine);

        self->context->parentDepth += 1;

        auto *item = self->firstArgumentItem;
        while (item != nullptr) {
            currentCodeLine = VTableCall::callAppendToLine(item, currentCodeLine);
            item = Cast::downcast<FuncArgumentItemStruct *>(item->nextNode);
        }

        self->context->parentDepth -= 1;


        currentCodeLine = VTableCall::callAppendToLine(&self->closeNode2, currentCodeLine);

        return currentCodeLine;
    }


    // virtual node does not have self text. underlying nodes will be appended to code line.
    static void copySelfText_FuncCall(FuncCallNodeStruct *self, utf8byte *buf)
    {
        return;
    }

    static int funcCall_selfTextLength(FuncCallNodeStruct *self)
    {
        return 0;
    }


    static constexpr const char funcCallNodeTypeText[] = "<FuncCall>";


    static inline void appendRootNode(FuncCallNodeStruct *arr, FuncArgumentItemStruct *arrayItem) {
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
            auto *nextItem = Alloc::newFuncArgumentItem(context, argNode);

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
            context->setCodeNode(&funcCallNode->closeNode2);
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
                context->setCodeNode(&currentKeyValueItem->follwingComma);
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
        NodeBase *parent = argNode;

        if ('(' != ch) {
            return Search::NOTFOUND;
        }

        assert(context->generatedPrimaryNode != nullptr);

        auto *funcCallNode = Alloc::newFuncCallNode(context, parent);

        funcCallNode->exprNode = context->generatedPrimaryNode;
        funcCallNode->exprNode->parentNode = Cast::upcast(funcCallNode);

        auto *leftNode = context->mostLeftNode; // this is also the generatedPrimaryNode which is the expression before '('

        int currentPos = start + 1;
        int resultPos = Scanner::scanLoop(funcCallNode, tokenizeFuncCallInternal, context, currentPos);
        if (Search::IsTokenized(resultPos)) {
            context->generatedPrimaryNode = Cast::upcast(funcCallNode);
            context->mostLeftNode = leftNode;
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

        if (node->exprNode) {
            node->exprNode->vtable->applyFuncToDescendants(
                    Cast::upcast(node->exprNode), ApplyFunc_pass2);
        }

        auto *item = node->firstArgumentItem;
        while (item != nullptr) {
            item->vtable->applyFuncToDescendants(
                    Cast::upcast(item), ApplyFunc_pass2);
            item = Cast::downcast<FuncArgumentItemStruct *>(item->nextNode);
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
        node->exprNode = nullptr;
        node->parsePhase = phase::EXPECT_VALUE;

        Init::initSymbolNode(&node->openNode, context, node, '(');
        Init::initSymbolNode(&node->closeNode2, context, node, ')');

        node->firstArgumentItem = nullptr;
        node->lastArgumentItem = nullptr;

        return node;
    }
}