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
    int Tokenizers::tokenizeExpression(TokenizerParams_argNode_ch_start_context) {
        return boolTokenizer(TokenizerParams_pass);
        /*
        int result = numberTokenizer(TokenizerParams_pass);

        if (!Search::IsTokenized(result)) { result = boolTokenizer(TokenizerParams_pass); }
        if (!Search::IsTokenized(result)) { result = nullTokenizer(TokenizerParams_pass); }
        if (!Search::IsTokenized(result)) { result = parenthesesTokenizer(TokenizerParams_pass); }
        if (!Search::IsTokenized(result)) { result = variableTokenizer(TokenizerParams_pass); }
        if (!Search::IsTokenized(result)) { result = stringLiteralTokenizer(TokenizerParams_pass); }

        if (!Search::IsTokenized(result)) { return Search::NOTFOUND; }

        // call func expression: func()
        int extraPos;
        if (Search::IsTokenized(extraPos = Tokenizers::tokenizeFuncCall(argNode, context->chars[result],
                                                            result, context))) {
            result = extraPos;
        }

        //  binary operator expression: calc() + 421431
        if (Search::IsTokenized(extraPos = Tokenizers::binaryOperationTokenizer(argNode, context->chars[result],
                                                                    result, context))) {
            result = extraPos;
        }


        return result;
        */
    }
}