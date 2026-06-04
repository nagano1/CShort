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
    int Tokenizers::fixedLiteralNodeTokenizer(TokenizerParams_argNode_ch_start_context)
    {
        bool isTrue = false;
        bool isFalse = false;
        bool isNull = false;
        bool isStringLiteral = false;

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
        else if (Search::IsTokenized(result = Tokenizers::stringLiteralTokenizer(TokenizerParams_pass))) {
            // string literal is also a kind of fixed literal,
            // we can use the same node for them, and distinguish them by a flag in the node.
            // this can simplify the AST and make it easier for code generation,
            // because string literals also need to store the original text for code generation,
            // and we can just use the textToken field
            // in LiteralValueNodeStruct to store the string literal token.
            isStringLiteral = true;
            
        }
        else {
            return Search::NOTFOUND;
        }


        auto newLiteralValueNode = Alloc::newLiteralValueNode(context, Cast::downcast<NodeBase*>(argNode));

        newLiteralValueNode->isTrue = isTrue;
        newLiteralValueNode->isFalse = isFalse;
        newLiteralValueNode->isNull = isNull;
        newLiteralValueNode->isStringLiteral = isStringLiteral;
        context->generatedPrimaryNode = Cast::upcast(newLiteralValueNode);

        if (isStringLiteral) {
            newLiteralValueNode->stringLiteralToken = Cast::downcast<StringLiteralTokenStruct*>(context->mostLeftToken);
            newLiteralValueNode->stringLiteralToken->parentNode = Cast::upcast(newLiteralValueNode);
        }
        else {
            newLiteralValueNode->textToken = Cast::downcast<ConstLiteralTokenStruct*>(context->mostLeftToken);
            newLiteralValueNode->textToken->parentNode = Cast::upcast(newLiteralValueNode);
        }
        
        return result;
    }



    static TokenBase *getLiteralToken(LiteralValueNodeStruct *self) {
        if (self->isStringLiteral) {
            assert(self->stringLiteralToken != nullptr);
            return Cast::upcastToken(self->stringLiteralToken);
        }
        else {
            assert(self->textToken != nullptr);
            return Cast::upcastToken(self->textToken);
        }
    }

    static CodeLine *appendToLine(LiteralValueNodeStruct *self, CodeLine *currentCodeLine)
    {
        TokenVTableCall::callAppendTokenToLine(getLiteralToken(self), currentCodeLine);
        return currentCodeLine;
    }


    static void copySelfText(LiteralValueNodeStruct *self, utf8byte *buf)
    {

    }

    static int selfTextLength(LiteralValueNodeStruct *self)
    {
        return 0;
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
        node->stringLiteralToken = nullptr;

        return node;
    }
}