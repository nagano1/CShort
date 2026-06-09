#define _CRT_SECURE_NO_WARNINGS

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

    // Op Item
    static constexpr const char opItem_NodeTypeText[] = "<OpItem>";
    static CodeLine *opItem_appendToLine(OpItemNodeStruct *self, CodeLine *currentCodeLine)
    {
        if (!self->isFirstOp) {
            assert(self->opToken.foundPos > -1); 
            assert(self->opToken.symbol[0] != '_');
            currentCodeLine = TokenVTableCall::callAppendTokenToLine(&self->opToken, currentCodeLine);
        }

        currentCodeLine = VTableCall::callAppendNodeToLine(self->rightExprNode, currentCodeLine);
        
        if (self->isFirstOp) {
            self->context->incrementDepthOnNextLine = true;
        }

        return currentCodeLine;
    }

    static void copySelfText_opItem(OpItemNodeStruct *self, utf8byte *buf)
    {
    }

    static int opItem_selfTextLength(OpItemNodeStruct *self)
    {
        return 0;
    }

    static int OpItemNodeStruct_applyFuncToDescendants(OpItemNodeStruct *node, ApplyFunc_params3)
    {
        if (parentIsFirst) {
            if (targetVTable == nullptr || node->vtable == targetVTable) {
                func(Cast::upcast(node), ApplyFunc_pass);
            }
        }

        if (!node->isFirstOp) {
            // op token is not a node, so we cannot call applyFuncToDescendants on it, but we can still call func on it if the targetVTable is for op tokens
            if (targetVTable == nullptr || &node->opToken.vtable == targetVTable) {
                //func(&node->opToken, ApplyFunc_pass);
            }
        }

        if (node->rightExprNode) {
            node->rightExprNode->vtable->applyFuncToDescendants(node->rightExprNode,
                                                        ApplyFunc_pass2);
        }


        if (!parentIsFirst) {
            if (targetVTable == nullptr || node->vtable == targetVTable) {
                func(Cast::upcast(node), ApplyFunc_pass);
            }
        }
        return 0;
    }

    static node_vtable opItem_VTable = CREATE_VTABLE(OpItemNodeStruct ,
                                                       opItem_selfTextLength,
                                                       copySelfText_opItem,
                                                       opItem_appendToLine,
                                                       OpItemNodeStruct_applyFuncToDescendants,
                                                       opItem_NodeTypeText,
                                                       NodeTypeId::OpItem);

    const node_vtable *VTables::OpItemVTable = &opItem_VTable;



    //    +--------------------------+
    //    |  Binary Operation        |
    //    +--------------------------+

    /*
        343 + 23142
            - 1234132
            * 1234

        (343 - 1234132 - (3241
            + 3241 + 2412
            + fjowie()
        ))

        longlonglonglonglonglongType
            longlonglonglonglonglonglonglonglonglonglongVariable
                = 3214213

        (
            343
                - 1234132
                - (
                    324321 + 214
                )
        )
        
        int a = 3421432 + (3124
            - 421
            - 421
        )

        true + false + (
            324112 - 32142
        )
    
    */
    static CodeLine *binaryop_appendToLine(BinaryOperationNodeStruct *self, CodeLine *currentCodeLine)
    {
        OpItemNodeStruct *opItemNode = self->firstOpNode;
        while (opItemNode != nullptr) {
            currentCodeLine = VTableCall::callAppendNodeToLine(opItemNode, currentCodeLine);
            opItemNode = opItemNode->nextOpNode;
        }
        return currentCodeLine;
    }

    static void copySelfText_binaryOp(BinaryOperationNodeStruct *self, utf8byte *buf)
    {
    }

    static int binaryop_selfTextLength(BinaryOperationNodeStruct *self)
    {
        return 0;
    }

    static int BinaryOperationNodeStruct_applyFuncToDescendants(BinaryOperationNodeStruct *node, ApplyFunc_params3)
    {
        if (parentIsFirst) {
            if (targetVTable == nullptr || node->vtable == targetVTable) {
                func(Cast::upcast(node), ApplyFunc_pass);
            }
        }
/*
        if (node->leftExprNode) {
            node->leftExprNode->vtable->applyFuncToDescendants(node->leftExprNode,
                                                           ApplyFunc_pass2);
        }

        if (node->rightExprNode) {
            node->rightExprNode->vtable->applyFuncToDescendants(node->rightExprNode,
                                                            ApplyFunc_pass2);
        }
*/
        if (!parentIsFirst) {
            if (targetVTable == nullptr || node->vtable == targetVTable) {
                func(Cast::upcast(node), ApplyFunc_pass);
            }
        }
        return 0;
    }


    static constexpr const char binaryop_NodeTypeText[] = "<binary op>";

    static node_vtable binaryop_VTable = CREATE_VTABLE(BinaryOperationNodeStruct ,
                                                       binaryop_selfTextLength,
                                                       copySelfText_binaryOp,
                                                       binaryop_appendToLine,
                                                       BinaryOperationNodeStruct_applyFuncToDescendants,
                                                       binaryop_NodeTypeText,
                                                       NodeTypeId::BinaryOperation);

    const node_vtable *VTables::BinaryOperationVTable = &binaryop_VTable;



    static int inner_op_binaryOpTokenizerLoop(TokenizerParams_argNode_ch_start_context)
    {
        OpItemNodeStruct *prevOpItem = Cast::downcast<OpItemNodeStruct*>(context->generatedPrimaryNode);

        if (ch == '+' || ch == '*' || ch == '-' || ch == '/' || ch == '%' || ch == '&' || ch == '|') {
            auto *parent = Cast::upcast(argNode);
            auto *newOpNode = Alloc::newOpItemNode(context, parent, ch);
            prevOpItem->nextOpNode = newOpNode;
            newOpNode->opToken.foundPos = start;

            int resultPos = Scanner::scanOnce(newOpNode, Tokenizers::tokenizeExpression, context, start + 1);
            if (Search::IsTokenized(resultPos)) {
                newOpNode->rightExprNode = context->generatedPrimaryNode;
                context->mostLeftToken = Cast::upcastToken(&newOpNode->opToken);
                context->generatedPrimaryNode = Cast::upcast(newOpNode);

                return resultPos;
            }
            else {
                context->scanEnd = true;
                context->setError(ErrorIndex::expected_expression_after_operator, start);
                return Search::NOTFOUND;
            }
        }
        else {
            context->scanEnd = true;
            if (prevOpItem->isFirstOp) {
                return Search::NOTFOUND;
            }
            return Search::DONE_WITH_PREVIOUS_POSITION;
        }
    }


    // tokenizer for binary operation, e.g. a + b
    // left expression is already tokenized. this tokenizer will try to tokenize operator and right expression.
    int Tokenizers::binaryOperationTokenizer(TokenizerParams_argNode_ch_start_context)
    {
        NodeBase *parent = Cast::upcast(argNode);
        assert(context->generatedPrimaryNode != nullptr);

        auto *binaryOpNode = Alloc::newBinaryOperationNode(context, parent, ch);

        auto *leftExpressionNode = context->generatedPrimaryNode;
        auto *firstOpItem = Alloc::newOpItemNode(context, Cast::upcast(binaryOpNode), '_');
        firstOpItem->rightExprNode = leftExpressionNode;
        firstOpItem->isFirstOp = true;
        context->generatedPrimaryNode = Cast::upcast(firstOpItem);

        binaryOpNode->firstOpNode = firstOpItem;
        
        // + 320 - 1123
        int resultPos = Scanner::scanLoop(binaryOpNode, inner_op_binaryOpTokenizerLoop, context, start);

        if (Search::IsTokenized(resultPos)) {
            context->generatedPrimaryNode = Cast::upcast(binaryOpNode);
            // mostLeftToken is handled in tokenizeExpression.
            return resultPos;
        }

        context->generatedPrimaryNode = leftExpressionNode;
        return Search::NOTFOUND;
    }


    BinaryOperationNodeStruct *Alloc::newBinaryOperationNode(ParseContext *context, NodeBase *parentNode, char op)
    {
        auto *node = context->newMem<BinaryOperationNodeStruct>();
        INIT_NODE(node, context, parentNode, VTables::BinaryOperationVTable);

        node->firstOpNode = nullptr;
        //Init::initSymbolToken(&node->opToken, context, node, op);
        return node;
    }

    OpItemNodeStruct *Alloc::newOpItemNode(ParseContext *context, NodeBase *parentNode, char op)
    {
        auto *node = context->newMem<OpItemNodeStruct>();
        INIT_NODE(node, context, parentNode, VTables::OpItemVTable);

        node->rightExprNode = nullptr;
        node->isFirstOp = false;

        Init::initSymbolToken(&node->opToken, context, node, op);
        return node;
    }
} // namespace