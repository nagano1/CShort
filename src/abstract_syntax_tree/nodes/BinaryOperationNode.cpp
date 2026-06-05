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

    /*
        343 + 23142
            - 1234132
            + 1234

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
        int formerParentDepth = self->context->parentDepth;

        if (self->leftExprNode) {
            // leftExpr
            currentCodeLine = VTableCall::callAppendNodeToLine(self->leftExprNode, currentCodeLine);
        }

        int formerArithmeticDepth = self->context->arithmeticBaseDepth;

        int diff = currentCodeLine->depth == self->context->parentDepth ? 0 : 1;

        int newDepth = self->context->arithmeticBaseDepth > -1 ?
                       self->context->arithmeticBaseDepth : formerParentDepth + diff;

        self->context->arithmeticBaseDepth = newDepth;
        self->context->parentDepth = newDepth;

        // operator +
        currentCodeLine = TokenVTableCall::callAppendTokenToLine(&self->opToken, currentCodeLine);


        if (self->rightExprNode) {
            // rightExpr
            currentCodeLine = VTableCall::callAppendNodeToLine(self->rightExprNode, currentCodeLine);
        }

        self->context->parentDepth = formerParentDepth;
        self->context->arithmeticBaseDepth = formerArithmeticDepth;

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

            auto *binaryOpNode = Alloc::newBinaryOperationNode(context, Cast::upcast(argNode), ch);
            binaryOpNode->opToken.foundPos = start;
            context->mostLeftToken = Cast::upcastToken(&binaryOpNode->opToken);
            context->generatedPrimaryNode = Cast::upcast(binaryOpNode);
            return start + 1;
        }

        return Search::NOTFOUND;
    }


    // tokenizer for binary operation, e.g. a + b
    // left expression is already tokenized. this tokenizer will try to tokenize operator and right expression.
    int Tokenizers::binaryOperationTokenizer(TokenizerParams_argNode_ch_start_context)
    {
        NodeBase *parent = Cast::upcast(argNode);
        assert(context->generatedPrimaryNode != nullptr);

        auto *leftExpressionNode = context->generatedPrimaryNode;
        int resultPos = Scanner::scanOnce(parent, inner_op_binaryOpTokenizer, context, start);

        if (Search::IsTokenized(resultPos)) {
            auto* binaryOpNode = Cast::downcast<BinaryOperationNodeStruct*>(context->generatedPrimaryNode);
            binaryOpNode->leftExprNode = leftExpressionNode;
            binaryOpNode->leftExprNode->parentNode = Cast::upcast(binaryOpNode);

            resultPos = Scanner::scanOnce(binaryOpNode, Tokenizers::tokenizeExpression, context, resultPos);
            if (Search::IsTokenized(resultPos)) {
                binaryOpNode->rightExprNode = context->generatedPrimaryNode;
                context->generatedPrimaryNode = Cast::upcast(binaryOpNode);
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

        Init::initSymbolToken(&node->opToken, context, node, op);

        return node;
    }
} // namespace