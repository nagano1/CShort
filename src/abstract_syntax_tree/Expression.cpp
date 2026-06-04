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
        /// There's no "ExpressionNodeStruct" defined. Expression node is just a general term for nodes that can be used as expressions, including LiteralValueNode, NumberValueNode, IdentifiersAccessNode, FuncCallNode, BinaryOperationNode, etc. we can just use NodeBase or define an interface for expression nodes if needed.

        
    int Tokenizers::tokenizeExpression(TokenizerParams_argNode_ch_start_context) {
        int result;
        if (Search::IsTokenized(result = Tokenizers::fixedLiteralNodeTokenizer(TokenizerParams_pass))) {
        }
        else if (Search::IsTokenized(result = Tokenizers::numberNodeTokenizer(TokenizerParams_pass))) {
        }
        else if (Search::IsTokenized(result = Tokenizers::parenthesesTokenizer(TokenizerParams_pass))) {
        }
        else if (Search::IsTokenized(result = Tokenizers::identifiersAccessTokenizer(TokenizerParams_pass))) {
        }
        else {
            return Search::NOTFOUND;
        }

        // call func expression: func()
        ch = context->chars[result];
        int extraPos;
        if (Search::IsTokenized(extraPos = Tokenizers::tokenizeFuncCall(argNode, ch, result, context))) {
           result = extraPos;
        }
        else if (Search::IsTokenized(extraPos = Tokenizers::binaryOperationTokenizer(argNode, ch, result, context))) {
           result = extraPos;
        }

        return result;
    }
}