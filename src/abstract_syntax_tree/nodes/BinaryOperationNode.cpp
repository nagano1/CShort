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

    //    +--------------------------+
    //    |  Binary Operation        |
    //    +--------------------------+

    static CodeLine *binaryop_appendToLine(BinaryOperationNodeStruct *self, CodeLine *currentCodeLine)
    {
        int formerParentDepth = self->context->parentDepth;

        if (self->leftExprNode) {
            // leftExpr
            currentCodeLine = VTableCall::callAppendToLine(self->leftExprNode, currentCodeLine);
        }

        int formerArithmeticDepth = self->context->arithmeticBaseDepth;

        int diff = currentCodeLine->depth == self->context->parentDepth ? 0 : 1;

        int newDepth = self->context->arithmeticBaseDepth > -1 ?
                       self->context->arithmeticBaseDepth : formerParentDepth + diff;

        self->context->arithmeticBaseDepth = newDepth;
        self->context->parentDepth = newDepth;

        // operator +
        currentCodeLine = VTableCall::callAppendToLine(&self->opNode, currentCodeLine);


        if (self->rightExprNode) {
            // rightExpr
            currentCodeLine = VTableCall::callAppendToLine(self->rightExprNode, currentCodeLine);
        }

        self->context->parentDepth = formerParentDepth;
        self->context->arithmeticBaseDepth = formerArithmeticDepth;

        return currentCodeLine;
    }

    // virtual node does not have self text. underlying nodes will be appended to code line.
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

        if (node->leftExprNode) {
            node->leftExprNode->vtable->applyFuncToDescendants(node->leftExprNode,
                                                           ApplyFunc_pass2);
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


    static constexpr const char binaryop_NodeTypeText[] = "<binary op>";

    static node_vtable binaryop_VTable = CREATE_VTABLE(BinaryOperationNodeStruct ,
                                                             binaryop_selfTextLength,
                                                             copySelfText_binaryOp,
                                                             binaryop_appendToLine,
                                                       BinaryOperationNodeStruct_applyFuncToDescendants,
                                                             binaryop_NodeTypeText,
                                                                NodeTypeId::BinaryOperation);

    const node_vtable *VTables::BinaryOperationVTable = &binaryop_VTable;



    static int inner_op_binaryOpTokenizer(TokenizerParams_argNode_ch_start_context) {

        if (ch == '+' || ch == '*' || ch == '-' || ch == '/' || ch == '%'
            || ch == '&' || ch == '|') {

            auto *binaryOpNode = Alloc::newBinaryOperationNode(context, argNode, ch);

            context->mostLeftNode = Cast::upcast(&binaryOpNode->opNode);
            context->generatedPrimaryNode = Cast::upcast(binaryOpNode);
            return start + 1;
        }

        return Search::NOTFOUND;
    }


    // tokenizer for binary operation, e.g. a + b
    // left expression is already tokenized. this tokenizer will try to tokenize operator and right expression.
    int Tokenizers::binaryOperationTokenizer(TokenizerParams_argNode_ch_start_context)
    {
        NodeBase *parent = argNode;
        assert(context->generatedPrimaryNode != nullptr);

        auto *leftExpressionNode = context->generatedPrimaryNode;
        auto *leftNode = context->mostLeftNode;

        int resultPos = Scanner::scanOnce(parent, inner_op_binaryOpTokenizer, context, start);
        context->mostLeftNode = leftNode;

        if (Search::IsTokenized(resultPos)) {
            auto* binaryOpNode = Cast::downcast<BinaryOperationNodeStruct*>(context->generatedPrimaryNode);
            binaryOpNode->leftExprNode = leftExpressionNode;
            binaryOpNode->leftExprNode->parentNode = Cast::upcast(binaryOpNode);

            resultPos = Scanner::scanOnce(binaryOpNode, Tokenizers::tokenizeExpression, context, resultPos);
            if (Search::IsTokenized(resultPos)) {
                binaryOpNode->rightExprNode = context->generatedPrimaryNode;
                context->generatedPrimaryNode = Cast::upcast(binaryOpNode);
                context->mostLeftNode = leftNode;
                return resultPos;
            }
        }
        return Search::NOTFOUND;
    }


    BinaryOperationNodeStruct *Alloc::newBinaryOperationNode(ParseContext *context, NodeBase *parentNode, char op)
    {
        auto *node = context->newMem<BinaryOperationNodeStruct>();
        INIT_NODE(node, context, parentNode, VTables::BinaryOperationVTable);

        node->leftExprNode = nullptr;
        node->rightExprNode = nullptr;

        Init::initSymbolNode(&node->opNode, context, node, op);

        return node;
    }








} // namespace
