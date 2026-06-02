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
    //    | NumberValueNode          |
    //    +--------------------------+

    static CodeLine *appendToLine_NumberNode(NumberNodeStruct *self, CodeLine *currentCodeLine)
    {
        assert(self->originalNumberTextToken.text != nullptr);
        return TokenVTableCall::callAppendTokenToLine(&self->originalNumberTextToken, currentCodeLine);
    }

    static void copySelfText_NumberNode(NumberNodeStruct *self, utf8byte *buf)
    {
    }

    static int selfTextLength_NumberNode(NumberNodeStruct *self)
    {
        return 0;
    }

    inline int64_t ConvertStringToInt64(const char *s, int length) {
        // have to check over flow
        return atoll(s); // can't use strtoll beacause of wasm conversion
    }
    
    static constexpr const char numberNodeTypeText[] = "<number>";

    int Tokenizers::numberNodeTokenizer(TokenizerParams_argNode_ch_start_context)
    {
        bool hasNegative = false;
        int numberStart;
        int charCount;

        if (context->chars[start] == '-') {
            hasNegative = true;
            numberStart = start + 1;
            charCount = 1;
        } else {
            numberStart = start;
            charCount = 0;
        }

        int digitCount = 0;
        for (int_fast32_t i = numberStart; i < context->length; i++) {
            if (!ParseUtil::isNumberLetter(context->chars[i])) {
                break;
            }

            charCount++;
            digitCount++;
        }

        bool hasDigit = hasNegative ? charCount > 1 : charCount > 0;
        if (!hasDigit) {
            return Search::NOTFOUND;
        }

        // Supported suffixes: L for long (64-bit), no suffix for int (32-bit). Suffixes are case-sensitive.
        bool hasSuffixLetter = start + charCount < context->length && context->chars[start + charCount] == 'L';
        if (hasSuffixLetter) {
            charCount++;
        }

        int end = start + charCount;
        if (end < context->length && !ParseUtil::isTerminatableChar(context->chars[end])) {
            return Search::NOTFOUND; // invalid character after number literal
        }

        // create Number node and token
        auto *numberNode = Alloc::newNumberNode(context, Cast::upcast(argNode));
        numberNode->originalNumberTextToken.foundPos = start;
        numberNode->num = ConvertStringToInt64(context->chars + start, (hasNegative ? 1 : 0) + digitCount);
        numberNode->unit = hasSuffixLetter ? 64 : 32;
        Init::assignText_SimpleTextToken(&numberNode->originalNumberTextToken, context, context->chars + start, charCount);

        context->generatedPrimaryNode = Cast::upcast(numberNode);
        context->mostLeftToken = Cast::upcastToken(&numberNode->originalNumberTextToken);
        return start + charCount;
    }


    static int NumberNodeStruct_applyFuncToDescendants(NumberNodeStruct *node, ApplyFunc_params3)
    {
        if (targetVTable == nullptr || node->vtable == targetVTable) {
            func(Cast::upcast(node), ApplyFunc_pass);
        }

        return 0;
    }


    static node_vtable _numberVTable_ = CREATE_VTABLE(NumberNodeStruct,
                                                      selfTextLength_NumberNode,
                                                      copySelfText_NumberNode,
                                                      appendToLine_NumberNode,
                                                      NumberNodeStruct_applyFuncToDescendants,
                                                      numberNodeTypeText,
                                                      NodeTypeId::Number);

    const node_vtable *VTables::NumberVTable = &_numberVTable_;


    NumberNodeStruct *Alloc::newNumberNode(ParseContext *context, NodeBase *parentNode)
    {
        auto *node = context->newMem<NumberNodeStruct>();
        INIT_NODE(node, context, parentNode, VTables::NumberVTable);

        INIT_TOKEN(&node->originalNumberTextToken, context, node, VTables::SimpleTextVTable);

        node->num = 0;
        node->unit = 0;

        return node;
    }
}