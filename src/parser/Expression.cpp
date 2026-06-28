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
        bool skipBinaryTokenizer = context->skipBinaryExpressionTokenizer;
        if (skipBinaryTokenizer) {
            context->skipBinaryExpressionTokenizer = false;
        }
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

        // after tokenizing a primary expression above, we will try to tokenize binary operation or
        // function call that follows the primary expression.

        auto *mostLeftToken = context->mostLeftToken;
        
        int extraPos;
        if (Search::IsTokenized(extraPos = Tokenizers::tokenizeFuncCall(argNode, context->chars[result], result, context))) {
           result = extraPos;
        }
        
        if (!skipBinaryTokenizer) {
            // binary operation: expression + expression.
            extraPos = Tokenizers::binaryOperationTokenizer(argNode, context->chars[result], result, context);
            if (Search::IsTokenized(extraPos)) {
                result = extraPos;
            }
        }

        context->mostLeftToken = mostLeftToken;
        return result;
    }
}