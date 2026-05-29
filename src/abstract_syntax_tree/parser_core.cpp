
#include <stdio.h>
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
#include <stdint.h>

#include "code_nodes.hpp"

namespace cshort
{
    ErrorInfo ErrorInfo::ErrorInfoList[errorListSize]; // errorIndex -> ErrorInfo
    bool ErrorInfo::errorInfoInitialized{false};
    struct InternalParsingData;

    
    // forward declaration of functions in parser_core.cpp to avoid circular dependency 
    CodeLine *VTableCall::callAppendToLine(void *node, CodeLine *currentCodeLine) {
        if (node == nullptr) {
            return currentCodeLine;
        }
        auto *nodeBase = Cast::upcast(node);
        // if the node has precedingLineBreakNode, append the precedingLineBreakNode before appending the node itself,
        // so that the line break will be before the node in the code line,
        // which is more intuitive and easier to handle when generating code later
        return nodeBase->vtable->appendToLine(nodeBase, currentCodeLine);
    }

    // Nested block comments are not supported, instead, we support named block comments which can be closed with the corresponding tag,
    // for example: /*[A] ... [A]*/.
    // It's more explicit and easier to use than nested block comments, and it can also avoid the problem of accidentally
    // closing the wrong block comment when there are multiple block comments.
    // It also allows block comments to be nested in a way, for example: /*[A] ... /*[B] ... [B]*/ ... [A]*/.
    static inline int detectBlockCommentEnd(int32_t i, ParseContext *context, int &tagLength, char *&tagText)
    {
        int textStartPos = i + 2;

        if (context->chars[i + 2] == '[') { // /*[hoge] ... [hoge]*/
            int nameStartPos = i + 3;
            // find out the tag name: hoge
            int endOfStartTagPos = ParseUtil::indexOf(context->chars, context->length, nameStartPos, ']');
            int lineEndPos = ParseUtil::indexOfBreakOrEnd(context->chars, context->length, nameStartPos);
            // the name of the named block comment must be in the same line with the start tag
            if (endOfStartTagPos > -1 && endOfStartTagPos < lineEndPos)
            {
                tagLength = endOfStartTagPos - nameStartPos;
                textStartPos = endOfStartTagPos + 1;
                tagText = context->memBuffer.newText(tagLength);
                TEXT_MEMCPY(tagText, context->chars + nameStartPos, tagLength);
                tagText[tagLength] = '\0';
            }
        }

        int searchEndPos = textStartPos;
        while (searchEndPos < context->length)
        {
            int endCommentPos = ParseUtil::indexOf2(context->chars, context->length, searchEndPos, '*', '/');
            if (endCommentPos == -1)
            {
                // the end of block comment not found, treat the rest of chars as comment
                return context->length;
            }

            searchEndPos = endCommentPos + 2; // continue to search for next block comment if the current found block comment end doesn't match the tag

            // [hoge]*/
            if (context->chars[endCommentPos - 1] == ']') {
                if (tagLength > 0
                    && context->chars[endCommentPos - tagLength - 2] == '['
                    && ParseUtil::matchWord(context->chars, context->length, tagText, tagLength, endCommentPos - tagLength - 1)
                ) {
                    return endCommentPos + 2;
                }
                if (tagLength == 0) {
                    // TODO: check '[' exists before ']' and allow non-named block comment to be closed with ]*/ if it doesn't have a tag,
                }

                continue; // if the block comment has tag, it must be closed with the same tag, so skip if the tag doesn't match
            }
            else {
                if (tagLength == 0) { // comments of /* needs to be closed with */ (not named block comment)
                    return endCommentPos + 2;
                }
                continue;
            }
        }

        return -1;
    }

    inline NodeBase* generateBlockCommentFragments(void *parentNode, ParseContext *context,
                                           const int32_t &i, int commentEndIndex, char* tagText, int tagLength) {
        auto *blockComment = Alloc::newBlockCommentNode(context, Cast::upcast(parentNode));
        blockComment->tagText = tagText;
        blockComment->tagTextLength = tagLength;

        // i is the position of '/'
        int currentIndex = i;
        BlockCommentFragmentStruct *lastNode = nullptr;
        LineBreakNodeStruct *lastBreakLine = nullptr;

        // split block comment into fragments by line break, and create LineBreakNodeStruct for each line break
        while (currentIndex <= commentEndIndex) {
            auto result = ParseUtil::indexOfBreakOrEndWithInfo(context->chars, context->length, currentIndex);
            int endIndex = result.index;
            bool hasLineBreak = result.hasLineBreak;

            if (commentEndIndex < endIndex) {
                endIndex = commentEndIndex;
            }

            if (endIndex > -1 && currentIndex <= endIndex) {
                auto *commentFragment = Alloc::newBlockCommentFragmentNode(context, Cast::upcast(parentNode));

                // link with previous line break node
                commentFragment->precedingLineBreakNode = lastBreakLine;

                // the length of comment fragment should not include the line break character, 
                // so use endIndex instead of currentIndex + commentLength
                int commentLength = hasLineBreak ? (endIndex - currentIndex) : (endIndex - currentIndex + 1);
                Init::assignText_SimpleTextNode(commentFragment, context, currentIndex, commentLength);

                if (hasLineBreak) {
                    // create a line break node for the line break after the comment fragment
                    LineBreakNodeStruct *newLineBreak = Alloc::newLineBreakNode(context, Cast::upcast(parentNode));
                    bool rn = context->chars[endIndex] == '\r' && context->chars[endIndex+1] == '\n';
                    if (rn) { // \r\n
                        newLineBreak->text[0] = '\r';
                        newLineBreak->text[1] = '\n';
                        newLineBreak->text[2] = '\0';
                        currentIndex = endIndex + 2;
                    }
                    else {
                        currentIndex = endIndex + 1;
                    }

                    lastBreakLine = newLineBreak;
                }
                else {
                    currentIndex = endIndex + 1;
                }

                if (lastNode != nullptr) {
                    lastNode->nextNode = Cast::upcast(commentFragment);
                }
                lastNode = commentFragment;
                if (blockComment->firstCommentFragment == nullptr) {
                    blockComment->firstCommentFragment = commentFragment;
                }
            }
            else {
                break;
            }
        }
        return Cast::upcast(blockComment);
    }


    struct InternalParsingData
    {
        int32_t returnPos = Search::NOTFOUND;

        int32_t whitespace_startpos = -1;

        // the first line break node before the code node generated by tokenizer.
        LineBreakNodeStruct *firstLineBreak = nullptr;
        // the last line break node, used for linking line break nodes in sequence when there are multiple line breaks before the next code node.
        LineBreakNodeStruct *lastLineBreak = nullptr;

        NodeBase *commentNode = nullptr; // LineCommentNodeStruct or BlockCommentNodeStruct

        void assignCommentNode(NodeBase* leftNode)
        {
            assert(leftNode != nullptr);

            if (commentNode != nullptr) {
                leftNode->precedingCommentNode = Cast::upcast(commentNode);
                commentNode = nullptr;
            }
        }

        void assignWhiteSpaces(NodeBase* commentNode, int endIndex)
        {
            if (whitespace_startpos != -1) {
                assert(whitespace_startpos < endIndex);
                // precedingSpaceCount allows only ascii whitespace. Japanese whitespaces are not allowed.
                commentNode->precedingSpaceCount = endIndex - whitespace_startpos;
                whitespace_startpos = -1;
            }
        }

        // attach line break node to the code node generated by tokenizer.
        void assignLineBreak(NodeBase* node)
        {
            if (this->firstLineBreak != nullptr) {
                node->precedingLineBreakNode = firstLineBreak;
                firstLineBreak = nullptr;
                lastLineBreak = nullptr;
            }
        }
    };
    


    static inline int tryDetectComments(void* parentNode, ParseContext* context, int32_t i, InternalParsingData* parsingData)
    {
        int commentEndIndex = -1;
        bool isLineComment = false;

        char *tagText = nullptr; // name of named block comment
        int tagLength = 0;

        if (i + 1 >= context->length) {
            return -1; // not enough chars for comment start tag
        }
        
        // line comment with "//"
        if ('/' == context->chars[i + 1]) {
            commentEndIndex = ParseUtil::indexOfBreakOrEnd(context->chars, context->length, i);
            assert(commentEndIndex > -1);
            isLineComment = true;

        } // block comment /* */
        else if ('*' == context->chars[i + 1]) {
            commentEndIndex = detectBlockCommentEnd(i, context, tagLength, tagText);
        }

        if (commentEndIndex == -1) {
            return -1; // not a comment
        }

        NodeBase *newCommentNode;
        if (isLineComment) {
            auto* comment = Alloc::newLineCommentNode(context, Cast::upcast(parentNode));
            Init::assignText_SimpleTextNode(comment, context, i, commentEndIndex - i);

            newCommentNode = Cast::upcast(comment);
        }
        else {
            newCommentNode = generateBlockCommentFragments(parentNode, context, i, commentEndIndex, tagText, tagLength);
        }

        parsingData->assignWhiteSpaces(newCommentNode, i);

        NodeBase* prevCommentNode = parsingData->commentNode;
        parsingData->commentNode = newCommentNode;
        if (prevCommentNode != nullptr) {
            newCommentNode->precedingCommentNode = prevCommentNode;
        }

        parsingData->assignLineBreak(newCommentNode);
        return commentEndIndex;
    }

    

    static inline int createLineBreakNode(void* parentNode, ParseContext* context, 
                                          int32_t& position, utf8byte ch, InternalParsingData* parsingData)
    {
        auto* newLineBreak = Alloc::newLineBreakNode(context, Cast::upcast(parentNode));

        if (parsingData->firstLineBreak == nullptr) { // the first line break
            parsingData->lastLineBreak = parsingData->firstLineBreak = newLineBreak;
        }
        else {
            assert(parsingData->lastLineBreak != nullptr);
            parsingData->lastLineBreak->nextLineBreakNode = newLineBreak;
            parsingData->lastLineBreak = newLineBreak;
        }

        parsingData->assignWhiteSpaces(Cast::upcast(newLineBreak), position);
        parsingData->assignCommentNode(Cast::upcast(newLineBreak));

        bool rn = ch == '\r' && context->chars[position + 1] == '\n';
        int result;
        if (rn) { // \r\n
            newLineBreak->text[0] = '\r';
            newLineBreak->text[1] = '\n';
            newLineBreak->text[2] = '\0';
            result = position + 2;
        }
        else {
            result = position + 1;
        }
        return result;
    }

    int detectSpaceEndIndex(int32_t i, ParseContext *context)
    {
        int spaceEndIndex = i + 1;
        for (; spaceEndIndex < context->length; spaceEndIndex++)
        {
            if (!ParseUtil::isSpace(context->chars[spaceEndIndex]))
            {
                break;
            }
        }
        return spaceEndIndex;
    }


    // scan with the given tokenizer.
    // this function handles spaces, line breaks and comments, so tokenizers can focus on scanning their syntax.
    // if loopMode is true, it will continue to scan after a token is found until scanEnd is set to true by tokenizer
    static InternalParsingData scanWithTokenizer(void *parentNode, TokenizerFunction tokenizer,
                                                 ParseContext *context, int start, bool loopMode) {
        utf8byte ch;
        InternalParsingData parsingData;
        context->isAfterLineBreak = false;
        int lastTokenizedPos = context->lastTokenizedPos;

        for (int32_t i = start; i <= context->length;) { // iterate until the end of chars
            ch = (i < context->length) ? context->chars[i] : 0;

            if (ch == '/') {
                int endPos = tryDetectComments(parentNode, context, i, &parsingData);
                if (endPos > -1) {
                    i = endPos;
                    continue;
                }
            }
            else if (ParseUtil::isBreakLine(ch)) {
                i = createLineBreakNode(parentNode, context, i, ch, &parsingData);
                context->isAfterLineBreak = true;
                continue;
            }
            else if (ParseUtil::isSpace(ch)) {
                parsingData.whitespace_startpos = i;
                i = detectSpaceEndIndex(i, context);
                continue;
            }


            int result = tokenizer(Cast::upcast(parentNode), ch, i, context);
            parsingData.returnPos = result;

            if (context->syntaxErrorInfo.hasError) {
                parsingData.returnPos = Search::NOTFOUND;
                return parsingData;
            }

            if (result > -1) {
                context->isAfterLineBreak = false;
                context->lastTokenizedPos = result;

                assert(context->mostLeftNode != nullptr);
                parsingData.assignWhiteSpaces(context->mostLeftNode, i);
                parsingData.assignCommentNode(context->mostLeftNode);
                parsingData.assignLineBreak(context->mostLeftNode);

                if (loopMode && !context->scanEnd) {
                    i = result;
                    continue;
                }
            }
            break;
        }

        if (parsingData.returnPos == Search::NOTFOUND) {
            context->lastTokenizedPos = lastTokenizedPos; // reset lastTokenizedPos if not found
        }
        context->scanEnd = false; // reset scanEnd for the next scan
        return parsingData;
    }

        

    // scan once with the given tokenizer, it will return when a token is found or the end of chars is reached
    int Scanner::scanOnce(void *parentNode, TokenizerFunction tokenizer,ParseContext *context,  int start) {
        return scanWithTokenizer(parentNode, tokenizer, context, start, false).returnPos;
    }

    // scan until scanEnd==true, tokenizer is responsible for setting scanEnd to true when it wants to stop scanning
    int Scanner::scanLoop(void *parentNode, TokenizerFunction tokenizer, ParseContext *context, int start) {
        return scanWithTokenizer(parentNode, tokenizer, context, start, true).returnPos;
    }

    int Scanner::scanRoot(void *parentNode, TokenizerFunction tokenizer, ParseContext *context) {
        InternalParsingData parsingData = scanWithTokenizer(parentNode, tokenizer, context, 0, /* loopMode */ true);

        context->remainedLineBreakNode = parsingData.firstLineBreak;
        context->remainedCommentNode = parsingData.commentNode;
        if (parsingData.whitespace_startpos > -1 && parsingData.whitespace_startpos < context->length) {
            context->remainedSpaceCount = context->length - parsingData.whitespace_startpos;
        }

        return parsingData.returnPos;
    }
}
