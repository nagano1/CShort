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
    //    | NumberValueNodd          |
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
        int digitCount = 0;
        int charCount;

        if (context->chars[start] == '-') {
            hasNegative = true;
            numberStart = start + 1;
            charCount = 1;
        } else {
            numberStart = start;
            charCount = 0;
        }

        for (int_fast32_t i = numberStart; i < context->length; i++) {
            if (!ParseUtil::isNumberLetter(context->chars[i])) {
                break;
            }

            charCount++;
            digitCount++;
        }

        NodeBase *parent = Cast::upcast(argNode);
        bool hasDigit = hasNegative ? charCount > 1 : charCount > 0;
        if (!hasDigit) {
            return Search::NOTFOUND;
        }

        bool hasSuffix = start + charCount < context->length && context->chars[start + charCount] == 'L';
        if (hasSuffix) {
            charCount++;
        }

        int end = start + charCount;
        if (end < context->length && !ParseUtil::isTerminatableChar(context->chars[end])) {
            return Search::NOTFOUND; // invalid suffix character for numbers
        }

        auto *numberNode = Alloc::newNumberNode(context, parent);

        context->generatedPrimaryNode = Cast::upcast(numberNode);
        Init::assignText_SimpleTextToken(&numberNode->originalNumberTextToken, context, context->chars + start, charCount);

        numberNode->num = ConvertStringToInt64(context->chars + start, digitCount + (hasNegative ? 1 : 0));

        numberNode->unit = hasSuffix ? 64 : 32;

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