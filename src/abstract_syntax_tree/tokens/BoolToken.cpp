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

    /*
        +--------------------------+
        |                          |
        |                          |
        |      nullTokenizer       |
        |                          |
        |                          |
        +--------------------------+
    */

    // int Tokenizers::nullTokenizer(TokenizerParams_argNode_ch_start_context) {
    //     static constexpr const char null_chars[] = "null";
    //     return Tokenizers::tokenizeWord(TokenizerParams_pass, Alloc::newNullNode, 'n', null_chars);
    // }




    /*
        +----------------------------------------------------+
        |                          
        |      BoolNode            
        |                          
        +----------------------------------------------------+
    */

    static CodeLine *appendToLine_BoolNode(BoolTokenStruct *self, CodeLine *currentCodeLine) {
        return currentCodeLine->AddAttachedFormatTokens(self)->appendToken(self);
    }

    static void copySelfText_BoolNode(BoolTokenStruct *self, utf8byte *buf) {
        TEXT_MEMCPY(buf, self->text, self->textLength);
    }

    static int selfTextLength_BoolNode(BoolTokenStruct *self) {
        return self->textLength;
    }


    int Tokenizers::boolTokenizer(TokenizerParams_argNode_ch_start_context)
    {
        int result = Tokenizers::tokenizeWord(TokenizerParams_pass, Alloc::newBoolNode,'t', "true");
        bool trueFound = Search::IsTokenized(result);
        if (!trueFound) {
            result = Tokenizers::tokenizeWord(TokenizerParams_pass, Alloc::newBoolNode, 'f', "false");
            if (!Search::IsTokenized(result)) {
                return Search::NOTFOUND;
            }
        }

        auto *boolNode = Cast::downcast<BoolTokenStruct*>(context->generatedPrimaryNode);
        boolNode->foundPos = start;
        boolNode->boolValue = trueFound;
        return result;
    }

    static int BoolTokenStruct_applyFuncToDescendants(BoolTokenStruct *token, TokenApplyFunc_params3) {
        if (targetVTable == nullptr || token->vtable == targetVTable) {
            func(Cast::upcastToken(token), ApplyFunc_pass);
        }

        return 0;
    }



    static constexpr const char boolNodeTypeText[] = "<bool>";
    static token_vtable _boolVTable = CREATE_TOKEN_VTABLE(BoolTokenStruct,
                                                   selfTextLength_BoolNode,
                                                   copySelfText_BoolNode, appendToLine_BoolNode,
                                                   BoolTokenStruct_applyFuncToDescendants,
                                                   boolNodeTypeText, TokenTypeId::Bool);

    const token_vtable *VTables::BoolVTable = &_boolVTable;

    BoolTokenStruct* Alloc::newBoolNode(ParseContext *context, NodeBase *parentNode) {
        auto *node = context->newMem<BoolTokenStruct>();
        INIT_TOKEN(node, context, parentNode, VTables::BoolVTable);
        node->text = nullptr;
        node->textLength = 0;
        node->boolValue = false;
        return node;
    }













    //    +--------------------------+
    //    | Number                   |
    //    +--------------------------+

    // static CodeLine *appendToLine_NumberNode(NumberNodeStruct *self, CodeLine *currentCodeLine)
    // {
    //     assert(self->text != nullptr);

    //     currentCodeLine = currentCodeLine->AddAttachedFormatNodes(self);
    //     currentCodeLine->appendNode(self);

    //     return currentCodeLine;
    // }

    // static void copySelfText_NumberNode(NumberNodeStruct *self, utf8byte *buf)
    // {
    //     TEXT_MEMCPY(buf, self->text, self->textLength);
    // }

    // static int selfTextLength_NumberNode(NumberNodeStruct *self)
    // {
    //     return self->textLength;
    // }

    // inline int64_t ConvertStringToInt64(const char *s, int length) {
    //     // have to check over flow
    //     return atoll(s); // can't use strtoll beacause of wasm conversion
    // }
    
    // static constexpr const char numberNodeTypeText[] = "<number>";

    // int Tokenizers::numberTokenizer(TokenizerParams_argNode_ch_start_context)
    // {
    //     bool hasNegative = false;
        
    //     int numberStart;
    //     int charCount;

    //     if (context->chars[start] == '-') {
    //         hasNegative = true;
    //         numberStart = start + 1;
    //         charCount = 1;
    //     } else {
    //         numberStart = start;
    //         charCount = 0;
    //     }

    //     for (int_fast32_t i = numberStart; i < context->length; i++) {
    //         if (!ParseUtil::isNumberLetter(context->chars[i])) {
    //             break;
    //         }

    //         charCount++;
    //     }

    //     if (hasNegative ? charCount > 1 : charCount > 0) {

    //         auto *numberNode = Alloc::newNumberNode(context, argNode);

    //         context->setCodeNode(numberNode);
    //         numberNode->text = context->memBuffer.newMem<char>(charCount + 1/* \0 */ + 1/*L*/);
    //         numberNode->textLength = charCount;

    //         TEXT_MEMCPY(numberNode->text, context->chars + start, charCount);
    //         numberNode->text[charCount] = '\0';

    //         numberNode->num = ConvertStringToInt64(numberNode->text, charCount);

    //         if ('L' == context->chars[start + charCount]) {
    //             numberNode->textLength++;
    //             numberNode->unit = 64;
    //             numberNode->text[charCount] = 'L';
    //             charCount++;
    //             numberNode->text[charCount] = '\0';
    //         } else if (!ParseUtil::isTerminatableChar(context->chars[start + charCount])) {
    //             // invalid suffix character for numbers
    //         } else {
    //             numberNode->num = (int32_t)numberNode->num;
    //         }

    //         return start + charCount;
    //     }

    //     return Search::NOTFOUND;
    // }


    // static int NumberNodeStruct_applyFuncToDescendants(NumberNodeStruct *node, ApplyFunc_params3)
    // {
    //     if (targetVTable == nullptr || node->vtable == targetVTable) {
    //         func(Cast::upcast(node), ApplyFunc_pass);
    //     }

    //     return 0;
    // }


    // static node_vtable _numberVTable_ = CREATE_VTABLE(NumberNodeStruct, selfTextLength_NumberNode,
    //                                                         copySelfText_NumberNode,
    //                                                         appendToLine_NumberNode,
    //                                                   NumberNodeStruct_applyFuncToDescendants,
    //                                                         numberNodeTypeText,
    //                                                         NodeTypeId::Number);

    // const node_vtable *VTables::NumberVTable = &_numberVTable_;



    // NumberNodeStruct *Alloc::newNumberNode(ParseContext *context, NodeBase *parentNode)
    // {
    //     auto *node = context->newMem<NumberNodeStruct>();
    //     INIT_NODE(node, context, parentNode, VTables::NumberVTable);
    //     node->text = nullptr;
    //     node->textLength = 0;

    //     return node;
    // }








    // //    +--------------------------+
    // //    | Parentheses value        |
    // //    +--------------------------+
    // static CodeLine *parentheses_appendToLine(ParenthesesNodeStruct *self, CodeLine *currentCodeLine)
    // {
    //     // (
    //     currentCodeLine = currentCodeLine->AddAttachedFormatNodes(self)->appendNode(self);

    //     auto *openCodeLine = currentCodeLine;
    //     int formerDepth = currentCodeLine->depth;

    //     if (self->valueNode) {
    //         int formerParentDepth = self->context->parentDepth;
    //         int formerArithmeticDepth = self->context->arithmeticBaseDepth;

    //         self->context->arithmeticBaseDepth = -1;

    //         int diff = currentCodeLine->depth == self->context->parentDepth ? 0 : 1;
    //         self->context->parentDepth += diff;
    //         currentCodeLine = VTableCall::callAppendToLine(self->valueNode, currentCodeLine);

    //         self->context->arithmeticBaseDepth = formerArithmeticDepth;
    //         self->context->parentDepth = formerParentDepth;
    //     }


    //     // )
    //     currentCodeLine = VTableCall::callAppendToLine(&self->closeNode, currentCodeLine);

    //     if (currentCodeLine != openCodeLine) {
    //         bool hasNonBracketEntity = false;
    //         auto *node = currentCodeLine->firstNode;
    //         while (node) {
    //             if (node->vtable == VTables::SymbolVTable) {
    //                 auto *symbol = Cast::downcast<SymbolStruct *>(node);
    //                 bool end = symbol->symbol[0] == ')' || symbol->symbol[0] == '}';
    //                 if (!end) {
    //                     hasNonBracketEntity = true;
    //                     break;
    //                 }
    //             } else if (node->vtable != VTables::LineBreakVTable
    //                        && node->vtable != VTables::LineCommentVTable
    //                        && node->vtable != VTables::BlockCommentFragmentVTable
    //                        && node->vtable != VTables::BlockCommentVTable
    //                        && node->vtable != VTables::EndOfFileVTable
    //                     ) {
    //                 hasNonBracketEntity = true;
    //                 break;
    //             }
    //             node = node->nextNode;
    //         }
    //         if (!hasNonBracketEntity) {
    //             currentCodeLine->depth = formerDepth;
    //         }
    //     }

    //     return currentCodeLine;
    // }

    // static void copySelfText_ParenthesesNode(ParenthesesNodeStruct *self, utf8byte *buf)
    // {
    //     buf[0] = '(';
    // }

    // static int parentheses_selfTextLength(ParenthesesNodeStruct *self)
    // {
    //     return 1;
    // }


    // static constexpr const char parenthesesNodeTypeText[] = "<parentheses>";

    // static int tokenizeExpressionForParenthesesInternalLoop(TokenizerParams_argNode_ch_start_context) {
    //     auto *parenthesesNode = Cast::downcast<ParenthesesNodeStruct *>(argNode);

    //     if (ch == ')') {
    //         parenthesesNode->closeNode.foundPos = start;
    //         context->setCodeNode(&parenthesesNode->closeNode);
    //         context->scanEnd = true;
    //         return start + 1;
    //     }
    //     else {
    //         if (parenthesesNode->valueNode != nullptr && parenthesesNode->valueNode->foundPos > -1) {
    //             context->setError(ErrorIndex::expect_end_parenthesis, context->lastTokenizedPos);
    //         }
    //         else {
    //             int result = Tokenizers::tokenizeExpression(Cast::upcast(parenthesesNode), TokenizerParams_pass_3);
    //             if (Search::IsTokenized(result)) {
    //                 parenthesesNode->valueNode = context->generatedPrimaryNode;
    //                 parenthesesNode->valueNode->foundPos = start;

    //                 return result;
    //             } 
    //             else {
    //                 context->setError(ErrorIndex::expect_end_parenthesis_for_fn_params,
    //                                   context->lastTokenizedPos);
    //             }
    //         }
    //     }
    //     return Search::NOTFOUND;
    // }


    // int Tokenizers::parenthesesTokenizer(TokenizerParams_argNode_ch_start_context)
    // {
    //     if ('(' == ch) {
    //         auto *parenthesesNode = Alloc::newParenthesesNode(context, argNode);
    //         int currentPos = start + 1;
    //         int resultPos =  Scanner::scanLoop(parenthesesNode, tokenizeExpressionForParenthesesInternalLoop, context, currentPos);
    //         if (Search::IsTokenized(resultPos)) {
    //             context->setCodeNode(parenthesesNode);
    //             return resultPos;
    //         }
    //     }

    //     return Search::NOTFOUND;
    // }

    // static int parentheses_applyFuncToDescendants(ParenthesesNodeStruct *node, ApplyFunc_params3)
    // {
    //     if (parentIsFirst) {
    //         if (targetVTable == nullptr || node->vtable == targetVTable) {
    //             func(Cast::upcast(node), ApplyFunc_pass);
    //         }
    //     }
    //     if (node->valueNode) {
    //         node->valueNode->vtable->applyFuncToDescendants(node->valueNode, ApplyFunc_pass2);
    //     }

    //     if (!parentIsFirst) {
    //         if (targetVTable == nullptr || node->vtable == targetVTable) {
    //             func(Cast::upcast(node), ApplyFunc_pass);
    //         }
    //     }

    //     return 0;
    // }

    // static node_vtable _parenthesesVTable = CREATE_VTABLE(ParenthesesNodeStruct,
    //                                                       parentheses_selfTextLength,
    //                                                       copySelfText_ParenthesesNode,
    //                                                       parentheses_appendToLine,
    //                                                       parentheses_applyFuncToDescendants,
    //                                                       parenthesesNodeTypeText,
    //                                                       NodeTypeId::Parentheses);

    // const node_vtable *VTables::ParenthesesVTable = &_parenthesesVTable;

    // ParenthesesNodeStruct *Alloc::newParenthesesNode(ParseContext *context, NodeBase *parentNode)
    // {
    //     auto *node = context->newMem<ParenthesesNodeStruct>();
    //     INIT_NODE(node, context, parentNode, VTables::ParenthesesVTable);
    //     node->valueNode = nullptr;

    //     Init::initSymbolNode(&node->closeNode, context, node, ')');
    //     return node;
    // }
}