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
    //    | Parentheses value        |
    //    +--------------------------+
    static CodeLine *parentheses_appendToLine(ParenthesesNodeStruct *self, CodeLine *currentCodeLine)
    {
        printf("depth = %d, incrementmode = %d\n", self->context->currentIndentDepth, self->context->incrementDepthOnNextLine);
        IndentRuleApplier indentRuleApplier = IndentRuleApplier::CreateForExpression(self->context, currentCodeLine);
        printf("baseDepth = %d\n", indentRuleApplier.GetBaseDepth());
        // (
        currentCodeLine = TokenVTableCall::callAppendTokenToLine(&self->openNode, currentCodeLine);
        indentRuleApplier.StartBracket(currentCodeLine);
        //self->context->incrementDepthOnNextLine = true;

        auto *openCodeLine = currentCodeLine;
        int formerDepth = currentCodeLine->depth;

        if (self->valueNode) {
            int formerParentDepth = self->context->currentIndentDepth;
            int formerArithmeticDepth = self->context->arithmeticBaseDepth;

            self->context->arithmeticBaseDepth = -1;

            currentCodeLine = VTableCall::callAppendNodeToLine(self->valueNode, currentCodeLine);

            self->context->arithmeticBaseDepth = formerArithmeticDepth;
            //self->context->currentIndentDepth = formerParentDepth;
        }


        // )
        currentCodeLine = TokenVTableCall::callAppendTokenToLine(&self->closeNode, currentCodeLine);
        indentRuleApplier.FinishAfterEndBracket(currentCodeLine);
/*
        // if there is no non-bracket entity in this line, it means this line is only for brackets, we can set the depth of this line to the former depth before parentheses, which can make the code more tidy when generating code later.
        if (currentCodeLine != openCodeLine) {
            bool hasNonBracketEntity = false;
            auto *token = currentCodeLine->firstToken;
            while (token) {
                if (token->vtable == VTables::SymbolTokenVTable) {
                    auto *symbol = Cast::downcast<SymbolTokenStruct *>(token);
                    bool end = symbol->symbol[0] == ')' || symbol->symbol[0] == '}';
                    if (!end) {
                        hasNonBracketEntity = true;
                        break;
                    }
                } else if (token->vtable != VTables::LineBreakVTable
                           && token->vtable != VTables::LineCommentVTable
                           && token->vtable != VTables::BlockCommentFragmentVTable
                           && token->vtable != VTables::BlockCommentVTable
                           && !NodeUtil::isEndOfFileToken(token)) {
                    hasNonBracketEntity = true;
                    break;
                }
                token = token->nextToken;
            }
            if (!hasNonBracketEntity) {
                currentCodeLine->depth = formerDepth;
            }
        }
*/
        return currentCodeLine;
    }

    static void copySelfText_ParenthesesNode(ParenthesesNodeStruct *self, utf8byte *buf)
    {
    }

    static int parentheses_selfTextLength(ParenthesesNodeStruct *self)
    {
        return 0;
    }


    static constexpr const char parenthesesNodeTypeText[] = "<parentheses>";

    static int tokenizeExpressionForParenthesesInternalLoop(TokenizerParams_argNode_ch_start_context) {
        auto *parenthesesNode = Cast::downcast<ParenthesesNodeStruct *>(argNode);

        if (ch == ')') {
            parenthesesNode->closeNode.foundPos = start;
            context->mostLeftToken = Cast::upcastToken(&parenthesesNode->closeNode);
            context->scanEnd = true;
            return start + 1;
        }
        else {
            if (parenthesesNode->valueNode != nullptr) {
                context->setError(ErrorIndex::expect_end_parenthesis, context->lastTokenizedPos);
            }
            else {
                int result = Tokenizers::tokenizeExpression(Cast::upcast(parenthesesNode), TokenizerParams_pass_3);
                if (Search::IsTokenized(result)) {
                    parenthesesNode->valueNode = context->generatedPrimaryNode;
                    return result;
                } 
                else {
                    context->setError(ErrorIndex::expect_end_parenthesis_for_fn_params,
                                      context->lastTokenizedPos);
                }
            }
        }
        return Search::NOTFOUND;
    }


    

    int Tokenizers::parenthesesTokenizer(TokenizerParams_argNode_ch_start_context)
    {
        if ('(' == ch) {
            auto *parenthesesNode = Alloc::newParenthesesNode(context, Cast::upcast(argNode));
            parenthesesNode->openNode.foundPos = start;
            int currentPos = start + 1;
            bool prevBinaryMode = context->skipBinaryExpressionTokenizer;
            context->skipBinaryExpressionTokenizer = false;
            int resultPos =  Scanner::scanLoop(parenthesesNode, tokenizeExpressionForParenthesesInternalLoop, context, currentPos);
            context->skipBinaryExpressionTokenizer = prevBinaryMode;
            if (Search::IsTokenized(resultPos)) {
                context->generatedPrimaryNode = Cast::upcast(parenthesesNode);
                context->mostLeftToken = Cast::upcastToken(&parenthesesNode->openNode);
                return resultPos;
            }
        }

        return Search::NOTFOUND;
    }

    static int parentheses_applyFuncToDescendants(ParenthesesNodeStruct *node, ApplyFunc_params3)
    {
        if (parentIsFirst) {
            if (targetVTable == nullptr || node->vtable == targetVTable) {
                func(Cast::upcast(node), ApplyFunc_pass);
            }
        }
        if (node->valueNode) {
            node->valueNode->vtable->applyFuncToDescendants(node->valueNode, ApplyFunc_pass2);
        }

        if (!parentIsFirst) {
            if (targetVTable == nullptr || node->vtable == targetVTable) {
                func(Cast::upcast(node), ApplyFunc_pass);
            }
        }

        return 0;
    }

    static node_vtable _parenthesesVTable = CREATE_VTABLE(ParenthesesNodeStruct,
                                                          parentheses_selfTextLength,
                                                          copySelfText_ParenthesesNode,
                                                          parentheses_appendToLine,
                                                          parentheses_applyFuncToDescendants,
                                                          parenthesesNodeTypeText,
                                                          NodeTypeId::Parentheses);

    const node_vtable *VTables::ParenthesesVTable = &_parenthesesVTable;

    ParenthesesNodeStruct *Alloc::newParenthesesNode(ParseContext *context, NodeBase *parentNode)
    {
        auto *node = context->newMem<ParenthesesNodeStruct>();
        INIT_NODE(node, context, parentNode, VTables::ParenthesesVTable);
        node->valueNode = nullptr;

        Init::initSymbolToken(&node->openNode, context, node, '(');
        Init::initSymbolToken(&node->closeNode, context, node, ')');
        node->closeNode.isEndFlag = true;
        return node;
    }
}