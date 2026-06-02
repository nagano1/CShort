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
        // There's no "ExpressionNodeStruct" defined.ExpressionNode is just a general term for nodes that can be used as expressions, including LiteralValueNode, NumberValueNode, VariableNode, FuncCallNode, BinaryOperationNode, etc. we can just use NodeBase or define an interface for expression nodes if needed.

    int Tokenizers::fixedLiteralNodeTokenizer(TokenizerParams_argNode_ch_start_context)
    {
        bool isTrue = false;
        bool isFalse = false;
        bool isNull = false;

        int result;
        if (Search::IsTokenized(result = Tokenizers::tokenizeWord(TokenizerParams_pass,
                                                                  Alloc::newConstLiteralToken,'t', "true"))) {
            isTrue = true;
        }
        else if (Search::IsTokenized(result = Tokenizers::tokenizeWord(TokenizerParams_pass,
                                                                  Alloc::newConstLiteralToken, 'f', "false"))) {
            isFalse = true;
        }
        else if (Search::IsTokenized(result = Tokenizers::tokenizeWord(TokenizerParams_pass,
                                                                  Alloc::newConstLiteralToken, 'n', "null"))) {
            isNull = true;
        }
        else {
            return Search::NOTFOUND;
        }

        auto newLiteralValueNode = Alloc::newLiteralValueNode(context, Cast::downcast<NodeBase*>(argNode));

        newLiteralValueNode->isTrue = isTrue;
        newLiteralValueNode->isFalse = isFalse;
        newLiteralValueNode->isNull = isNull;

        newLiteralValueNode->textToken = Cast::downcast<ConstLiteralTokenStruct*>(context->mostLeftToken);
        newLiteralValueNode->textToken->parentNode = Cast::upcast(newLiteralValueNode);
        context->generatedPrimaryNode = Cast::upcast(newLiteralValueNode);
        return result;
    }


    int Tokenizers::tokenizeExpression(TokenizerParams_argNode_ch_start_context) {
        int result = Tokenizers::numberNodeTokenizer(TokenizerParams_pass);
        if (!Search::IsTokenized(result)) {
            result = Tokenizers::fixedLiteralNodeTokenizer(TokenizerParams_pass);
        }

        //if (!Search::IsTokenized(result)) { result = parenthesesTokenizer(TokenizerParams_pass); }
        //if (!Search::IsTokenized(result)) { result = variableTokenizer(TokenizerParams_pass); }
        //if (!Search::IsTokenized(result)) { result = stringLiteralTokenizer(TokenizerParams_pass); }

        //if (!Search::IsTokenized(result)) { return Search::NOTFOUND; }

        // call func expression: func()
        //int extraPos;
        //if (Search::IsTokenized(extraPos = Tokenizers::tokenizeFuncCall(argNode, context->chars[result],
                                                           //result, context))) {
            //result = extraPos;
        //}

        //  binary operator expression: calc() + 421431
        //if (Search::IsTokenized(extraPos = Tokenizers::binaryOperationTokenizer(argNode, context->chars[result],
                                                                  //result, context))) {
            //result = extraPos;
        //}
        return result;
    }


    static CodeLine *appendToLine(LiteralValueNodeStruct *self, CodeLine *currentCodeLine)
    {
        currentCodeLine = TokenVTableCall::callAppendTokenToLine(self->textToken, currentCodeLine);
        return currentCodeLine;
    }

    static void copySelfText(LiteralValueNodeStruct *self, utf8byte *buf)
    {
        TokenVTableCall::copySelfText(self->textToken, buf);
    }

    static int selfTextLength(LiteralValueNodeStruct *self)
    {
        return self->textToken->textLength;
    }



    static int applyFuncToDescendants(LiteralValueNodeStruct *node, ApplyFunc_params3)
    {
        if (targetVTable == nullptr || node->vtable == targetVTable) {
            func(Cast::upcast(node), ApplyFunc_pass);
        }

        return 0;
    }

    static constexpr const char literalTypeText[] = "<Literal>";

    static node_vtable _literalVTable = CREATE_VTABLE(LiteralValueNodeStruct, selfTextLength,
                                                         copySelfText,
                                                         appendToLine,
                                                         applyFuncToDescendants,
                                                         literalTypeText,
                                                         NodeTypeId::FixedLiteral);
    const node_vtable *VTables::FixedLiteralVTable = &_literalVTable;

    LiteralValueNodeStruct *Alloc::newLiteralValueNode(ParseContext *context, NodeBase *parentNode) {
        auto *node = context->newMem<LiteralValueNodeStruct>();
        INIT_NODE(node, context, parentNode, VTables::FixedLiteralVTable);

        node->isTrue = false;
        node->isFalse = false;
        node->isNull = false;
        node->textToken = nullptr; // it will be assigned in tokenizer after the token is generated

        return node;
    }
}