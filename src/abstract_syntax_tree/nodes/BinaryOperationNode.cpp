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

namespace cshort
{
    // -----------------------------------------------------------------------------------
    //
    //                              BinaryOperationNodeStruct
    //
    // -----------------------------------------------------------------------------------
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
        assert(self->leftExprNode != nullptr);
        assert(self->rightExprNode != nullptr);

        currentCodeLine = VTableCall::callAppendNodeToLine(self->leftExprNode, currentCodeLine);
        self->context->incrementDepthOnNextLine = true;
        currentCodeLine = TokenVTableCall::callAppendTokenToLine(&self->opToken, currentCodeLine);
        currentCodeLine = VTableCall::callAppendNodeToLine(self->rightExprNode, currentCodeLine);

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


    
    
    // the next operator group has higher priority than the current operator group
    static BinaryOperationGroup GetNextOpGroup(BinaryOperationGroup currentGroup) {
        switch (currentGroup) {
            case BinaryOperationGroup::Add_Subtract: return BinaryOperationGroup::Multiply_Divide_Modulo;
            case BinaryOperationGroup::Multiply_Divide_Modulo: return BinaryOperationGroup::And_Or;
            case BinaryOperationGroup::And_Or: return BinaryOperationGroup::BitwiseAnd_BitwiseOr;
            default: return BinaryOperationGroup::None;
        }
        return BinaryOperationGroup::None;
    }


    struct BinaryOpInfo {
        BinaryOperationGroup operatingGroup;
        BinaryOperator binaryOp;
    };

    static BinaryOpInfo getBinaryOpInfo(utf8byte ch) {
        BinaryOpInfo opInfo;
        opInfo.operatingGroup = BinaryOperationGroup::None;

         if (ch == '+') {
            opInfo.binaryOp = BinaryOperator::Add;
            opInfo.operatingGroup = BinaryOperationGroup::Add_Subtract;
         }
         else if (ch == '-') {
            opInfo.binaryOp = BinaryOperator::Subtract;
            opInfo.operatingGroup = BinaryOperationGroup::Add_Subtract;
         }
         else if (ch == '*') {
            opInfo.binaryOp = BinaryOperator::Multiply;
            opInfo.operatingGroup = BinaryOperationGroup::Multiply_Divide_Modulo;
         }
         else if (ch == '/') {
            opInfo.binaryOp = BinaryOperator::Divide;
            opInfo.operatingGroup = BinaryOperationGroup::Multiply_Divide_Modulo;
         }
         else if (ch == '%') {
            opInfo.binaryOp = BinaryOperator::Modulo;
            opInfo.operatingGroup = BinaryOperationGroup::Multiply_Divide_Modulo;
         }
         else if (ch == '&') {
            opInfo.binaryOp = BinaryOperator::BitwiseAnd;
            opInfo.operatingGroup = BinaryOperationGroup::BitwiseAnd_BitwiseOr;
         }
         else if (ch == '|') {
            opInfo.binaryOp = BinaryOperator::BitwiseOr;
            opInfo.operatingGroup = BinaryOperationGroup::BitwiseAnd_BitwiseOr;
         }
         return  opInfo;
    }
    

static TempOpItem *CreateOpItem(ParseContext *context, NodeBase *parentNode, char op) {

    auto *node = context->newMem<TempOpItem>();

    INIT_NODE(node, context, parentNode, nullptr);


    node->rightExprNode = nullptr;

    node->nextOpNode = nullptr;

    node->opGroup = BinaryOperationGroup::None;

    node->binaryOp = BinaryOperator::Add; // overwritten for real operators


    Init::initSymbolToken(&node->opToken, context, node, op);

    return node;

}



    static int binaryOpTokenizerInternal(TokenizerParams_argNode_ch_start_context)
    {
        BinaryOpInfo opInfo = getBinaryOpInfo(ch);
        if (opInfo.operatingGroup == BinaryOperationGroup::None) {
            return Search::NOTFOUND;
        }

        // try to tokenize right expression: + 300, * 3214, etc...
        context->skipBinaryExpressionTokenizer = true; // to avoid recursive call to binary operation tokenizer when tokenizing right expression
        int resultPos = Scanner::scanOnce(argNode, Tokenizers::tokenizeExpression, context, start + 1);
        if (Search::IsTokenized(resultPos)) {
            auto *newOpNode = CreateOpItem(context, Cast::upcast(argNode), ch);
            newOpNode->opToken.foundPos = start;
            newOpNode->binaryOp = opInfo.binaryOp;
            newOpNode->opGroup = opInfo.operatingGroup;
            newOpNode->rightExprNode = context->generatedPrimaryNode;
            newOpNode->rightExprNode->parentNode =  Cast::upcast(newOpNode);

            context->mostLeftToken = Cast::upcastToken(&newOpNode->opToken);
            context->generatedPrimaryNode = Cast::upcast(newOpNode);

            return resultPos;
        }
        else {
            context->setError(ErrorIndex::expected_expression_after_operator, start);
            return Search::NOTFOUND;
        }
    }


    using OpItemTempNodeStruct = struct _OpItemTempNodeStruct{
        NODE_HEADER;

        NodeBase *rightExprNode;
        SymbolTokenStruct opToken; // +, -, *, /, %, etc..
        BinaryOperationGroup opGroup;
        BinaryOperator binaryOp; // specific operator like Add, Subtract, etc.. used for code generation and type checking.

        struct _OpItemTempNodeStruct *nextOpNode;
    };


    static NodeBase* buildBinaryOperationNodeTree(
        TempOpItem *firstOpItem,
        ParseContext *context,
        NodeBase *parent,
        BinaryOperationGroup opGroup
    );

    // create binary operation node for the given left op item and right op item
    static BinaryOperationNodeStruct* createBinaryOperationNode(ParseContext *context,
                                                                NodeBase *parent,
                                                                TempOpItem *startOpItem,
                                                                BinaryOperationGroup opGroup,
                                                                TempOpItem *rightSideOpItem_first,
                                                                BinaryOperationNodeStruct* previousBinaryOpNode = nullptr)
    {
        // 124 * 2 + 1234 * 10 * 12 / 5 - 3142 / 23 - 5

        BinaryOperationNodeStruct *newBinaryOpNode = Alloc::newBinaryOperationNode(context, parent);

        // assign left expression
        {
            if (previousBinaryOpNode != nullptr) {
                newBinaryOpNode->leftExprNode = Cast::upcast(previousBinaryOpNode);
                previousBinaryOpNode->parentNode = Cast::upcast(newBinaryOpNode);
            }
            else { // the first binary operation node for this operator group
                NodeBase *leftBinaryOpNode = buildBinaryOperationNodeTree(startOpItem, context, Cast::upcast(newBinaryOpNode), GetNextOpGroup(opGroup));
        newBinaryOpNode->opToken.parentNode = Cast::upcast(newBinaryOpNode);
                newBinaryOpNode->leftExprNode = Cast::upcast(leftBinaryOpNode);
            }
        }

        // assign right expression
        {
            NodeBase *rightBinaryOpNode = buildBinaryOperationNodeTree(rightSideOpItem_first, context, Cast::upcast(newBinaryOpNode), GetNextOpGroup(opGroup));
            newBinaryOpNode->rightExprNode = Cast::upcast(rightBinaryOpNode);
        }

        newBinaryOpNode->opToken = rightSideOpItem_first->opToken;
        newBinaryOpNode->binaryOp = rightSideOpItem_first->binaryOp;
        newBinaryOpNode->opGroup = rightSideOpItem_first->opGroup;
        return newBinaryOpNode;
    }



    // build binary operation node tree based on operator priority
    static NodeBase* buildBinaryOperationNodeTree(TempOpItem *firstOpItem,
                                                                   ParseContext *context,
                                                                   NodeBase *parent,
                                                                   BinaryOperationGroup opGroup
    ) {
        if (firstOpItem->nextOpNode == nullptr) {
            // only one expression, no operator, we can directly return the expression node without building binary operation node
            firstOpItem->rightExprNode->parentNode = parent;
            return firstOpItem->rightExprNode;
        }

        // 124 * 2 + 1234 * 10 * 12 / 5 - 3142 / 23 - 5
        // 10 * 10 - 10
        TempOpItem *rightSideOpItem_first = nullptr;
        BinaryOperationNodeStruct* currentBinaryOperationNode = nullptr; // left side binary operation node built in the previous loop

        TempOpItem *prevOpItem = firstOpItem;
        TempOpItem *currentOpItem = firstOpItem->nextOpNode;

        // split op items into groups based on operator priority
        while (currentOpItem != nullptr)
        {
            if (currentOpItem->opGroup == opGroup) {
                prevOpItem->nextOpNode = nullptr; // break the link between left op item and right op item

                if (rightSideOpItem_first != nullptr) {
                    currentBinaryOperationNode = createBinaryOperationNode(context, parent, firstOpItem, opGroup, rightSideOpItem_first, currentBinaryOperationNode);
                }
                rightSideOpItem_first = currentOpItem;
            }
            prevOpItem = currentOpItem;
            currentOpItem = currentOpItem->nextOpNode;
        }

        NodeBase *resultNode = nullptr;

        if (rightSideOpItem_first != nullptr) { // the last pair of op items remaining
            resultNode = Cast::upcast(createBinaryOperationNode(context, parent, firstOpItem, opGroup, rightSideOpItem_first, currentBinaryOperationNode));
        }
        else {
            // no operator found for this group, we need to build binary operation node for the next group
            BinaryOperationGroup nextOpGroup = GetNextOpGroup(opGroup);
            assert( nextOpGroup != BinaryOperationGroup::None); // we should have processed all operator groups before reaching here
            if (nextOpGroup == BinaryOperationGroup::None) {
                context->setError(ErrorIndex::internal_error, firstOpItem->opToken.foundPos);
                return nullptr;
            }
            resultNode = buildBinaryOperationNodeTree(firstOpItem, context, parent, nextOpGroup);
        }

        return resultNode;
    }



    // tokenizer for binary operation, e.g. a + b
    // left expression is already tokenized. this tokenizer will try to tokenize operator and right expression.
    int Tokenizers::binaryOperationTokenizer(TokenizerParams_argNode_ch_start_context)
    {
        NodeBase *parent = Cast::upcast(argNode);

        assert(context->generatedPrimaryNode != nullptr);
        NodeBase *firstExpressionNode = context->generatedPrimaryNode;

        TempOpItem *firstOpItem = nullptr;
        TempOpItem *secondOpItem = nullptr;
        TempOpItem *currentOpItem = nullptr;
        
        // first, pre-parse expressions and operators without considering operator priority.
        // + 320 - 1123 * 5
        while (start < context->length)
        {
            int resultPos = Scanner::scanOnce(parent, binaryOpTokenizerInternal, context, start);
            if (!Search::IsTokenized(resultPos)) {
                break;
            }

            TempOpItem *foundOpItem = Cast::downcast<TempOpItem*>(context->generatedPrimaryNode);
            
            if (secondOpItem == nullptr) { // first time to tokenize a binary operation
                firstOpItem = CreateOpItem(context, parent, '_');
                firstOpItem->rightExprNode = firstExpressionNode;

                currentOpItem = firstOpItem;
                secondOpItem = foundOpItem;
            }
        
            currentOpItem->nextOpNode = foundOpItem;
            currentOpItem = foundOpItem;

            assert(start < resultPos);
            start = resultPos;
        }

        if (secondOpItem == nullptr) {
            return Search::NOTFOUND;
        }

        // second, build BinaryOperationNode Tree based on operator priority.
        NodeBase *rootBinaryNode = buildBinaryOperationNodeTree(firstOpItem, context, parent,
                      LowestBinaryOperationGroup);

        context->generatedPrimaryNode = rootBinaryNode;
        context->mostLeftToken = Cast::upcastToken(&secondOpItem->opToken);
        return start;
    }


    BinaryOperationNodeStruct *Alloc::newBinaryOperationNode(ParseContext *context, NodeBase *parentNode)
    {
        auto *node = context->newMem<BinaryOperationNodeStruct>();
        INIT_NODE(node, context, parentNode, VTables::BinaryOperationVTable);

        node->leftExprNode = nullptr;
        node->rightExprNode = nullptr;
        // node->opToken is copied from op item when building binary operation node tree
        return node;
    }
} // namespace