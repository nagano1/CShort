
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

#include "parser.hpp"
//#include "types.hpp"

namespace cshort
{
    ErrorInfo ErrorInfo::ErrorInfoList[errorListSize]; // errorIndex -> ErrorInfo
    struct InternalParsingData;


    
    void ParseContext::init() {
        memBuffer.init();
        memBufferForCodeLines.init();
        memBufferForTypeManager.init();

        appendLineMode = AppendLineMode::Normal;
        syntaxErrorInfo.hasError = false;
        syntaxErrorInfo.errorItem.errorIndex = ErrorIndex::no_syntax_error;
        syntaxErrorInfo.errorItem.errorId = 10000;

        semanticErrorInfo.hasError = false;
        semanticErrorInfo.count = 0;
        semanticErrorInfo.firstErrorItem = nullptr;
        semanticErrorInfo.lastErrorItem = nullptr;

//        typeManager = memBufferForTypeManager.newMem<TypeManager>(1);
//        typeManager->init(this);

        memBufferForError.init();
    }


    void ParseContext::addErrorWithNode(ErrorIndex errorCode, void* nodeArg) {
            printf("semantic error: %s\n", getErrorMessage(errorCode));
            auto *node = Cast::upcast(nodeArg);
            assert(node->vtable != nullptr);

            auto &errorInfo = this->semanticErrorInfo;
            errorInfo.count++;
            errorInfo.hasError = true;
            auto *mem = this->memBufferForError.newMem<SemanticErrorItem>(1);
            mem->node = node;
            mem->codeErrorItem.errorIndex = errorCode;
            mem->codeErrorItem.linePos1 = -1;
            mem->next = nullptr;
            if (errorInfo.firstErrorItem == nullptr) {
                errorInfo.firstErrorItem = mem;
            }

            if (errorInfo.lastErrorItem == nullptr) {
                errorInfo.lastErrorItem = mem;
            }
            else {
                errorInfo.lastErrorItem->next = mem;
                errorInfo.lastErrorItem = mem;
            }

            mem->codeErrorItem.errorId = getErrorCode(errorCode);
            const char* reason = getErrorMessage(errorCode);
            if (reason == nullptr) {
                reason = "";
            }
            int len = (int)strlen(reason);
            mem->codeErrorItem.reasonLength = len < MAX_REASON_LENGTH ? len : MAX_REASON_LENGTH;
            memcpy(mem->codeErrorItem.reason, reason, mem->codeErrorItem.reasonLength);
            mem->codeErrorItem.reason[mem->codeErrorItem.reasonLength] = '\0';
        }

        void ParseContext::dispose() {
            memBuffer.freeAll();
            memBufferForCodeLines.freeAll();
            memBufferForError.freeAll();
            //memBufferForTypeManager.freeAll();
        }


        int ParseContext::IncrementIndentDepth(ParseContext *context) {
            auto formerIndentDepth = currentIndentDepth;
            if (!isAfterOpenParenthesis) {
                currentIndentDepth++;
            }
            isAfterOpenParenthesis = false;
            return formerIndentDepth;
        }

        int ParseContext::IncrementIndentDepthForParenthesis(ParseContext *context) {
            auto formerIndentDepth = currentIndentDepth;
            isAfterOpenParenthesis = true;
            currentIndentDepth++;
            return formerIndentDepth;
        }

        void ParseContext::DecrementIndentDepth(ParseContext *context) {
            currentIndentDepth--;
        }

        
        
        void ParseContext::setError(ErrorIndex errorCode, st_int startPos) {
            setError2(errorCode, startPos, startPos);
        }

        int ParseContext::getNextLineIndentDepth() const {
            return currentIndentDepth + (incrementDepthOnNextLine ? 1 : 0);
        }

        void ParseContext::setError2(ErrorIndex errorCode, st_int startPos, st_int startPos2) {
            if (syntaxErrorInfo.hasError) {
                return;
            }
            syntaxErrorInfo.hasError = true;
            auto &item = syntaxErrorInfo.errorItem;
            item.errorIndex = errorCode;
            item.errorId = getErrorCode(errorCode);
            item.charPosition = startPos;
            item.charPosition2 = startPos2;

            const char *msg = getErrorMessage(errorCode);
            if (msg != nullptr) {
                snprintf(item.reason, MAX_REASON_LENGTH, "%s", msg);
                item.reasonLength = static_cast<int>(strlen(item.reason));
            }
        }

        void ParseContext::setIndentError(ErrorIndex errorCode, st_int line1, st_int charPos1) {
            setError3(errorCode, line1, charPos1, line1, charPos1);
        }

        void ParseContext::setError3(ErrorIndex errorCode, st_int line1, st_int charPos1, st_int line2, st_int charPos2) {
            if (syntaxErrorInfo.hasError) {
                return;
            }
            syntaxErrorInfo.hasError = true;
            auto &item = syntaxErrorInfo.errorItem;
            item.errorIndex = errorCode;
            item.errorId = getErrorCode(errorCode);
            item.linePos1 = line1;
            item.charPos1 = charPos1;
            item.linePos2 = line2;
            item.charPos2 = charPos2;

            const char *msg = getErrorMessage(errorCode);
            if (msg != nullptr) {
                snprintf(item.reason, MAX_REASON_LENGTH, "%s", msg);
                item.reasonLength = static_cast<int>(strlen(item.reason));
            }
        }





    // Nested block comments are not supported, instead, we support named block comments which can be closed with the corresponding tag,
    // for example: /*<[A] ... [A]>*/.
    // It's more explicit and easier to use than nested block comments, and it can also avoid the problem of accidentally
    // closing the wrong block comment when there are multiple block comments.
    // It also allows block comments to be nested in a way, for example: /*<[A] ... /*<[B] ... [B]>*/ ... [A]>*/.
    static inline int detectBlockCommentEnd(int32_t i, ParseContext *context, int &tagLength, char *&tagText)
    {
        int textStartPos = i + 2;

        if (i + 2 >= context->length) {
            // not enough chars for block comment start tag
            return context->length;
        }
        if (context->chars[i + 2] == '<' && i + 3 < context->length && context->chars[i + 3] == '[') { // /*<[hoge] ... [hoge]>*/
            int nameStartPos = i + 4;
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


            // [hoge]>*/
            if (context->chars[endCommentPos - 2] == ']' && context->chars[endCommentPos - 1] == '>') {
                if (tagLength > 0
                    && (endCommentPos - tagLength - 3) >= 0 
                    && context->chars[endCommentPos - tagLength - 3] == '['
                    && ParseUtil::matchWord(context->chars, context->length, tagText, tagLength, endCommentPos - tagLength - 2)
                ) {
                    return endCommentPos + 2;
                }
                // if the block comment has tag, it must be closed with the same tag, so skip if the tag doesn't match
                // if the block comment doesn't have start tag,  */ can close it, but ]>*/ will not be treated as block comment end, so skip it as well
                continue;
            }
            else {
                if (tagLength == 0) { // comments of /* needs to be closed with */ (not named block comment)
                    return endCommentPos + 2;
                }
                continue;
            }
        }

        return context->length;
    }

    inline TokenBase* generateBlockCommentFragments(void *parentNode, ParseContext *context,
                                           const int32_t &i, int commentEndIndex, char* tagText, int tagLength) {
        auto *blockComment = Alloc::newBlockCommentToken(context, parentNode);
        blockComment->tagText = tagText;
        blockComment->tagTextLength = tagLength;
        blockComment->foundPos = i;

        // i is the position of '/'
        int currentIndex = i;
        BlockCommentFragmentStruct *lastFragment = nullptr;
        LineBreakTokenStruct *lastBreakLine = nullptr;

        // split block comment into fragments by line break, and create LineBreakTokenStruct for each line break
        while (currentIndex < commentEndIndex) {
            auto result = ParseUtil::indexOfBreakOrEndWithInfo(context->chars, context->length, currentIndex);
            int endIndex = result.index;
            bool hasLineBreak = result.hasLineBreak;

            if (commentEndIndex < endIndex) {
                endIndex = commentEndIndex;
                hasLineBreak = false;
            }

            if (endIndex > -1 && currentIndex <= endIndex) {
                auto *commentFragment = Alloc::newBlockCommentFragmentToken(context, blockComment);

                // link with previous line break token
                commentFragment->precedingLineBreakToken = lastBreakLine;
                commentFragment->foundPos = currentIndex;

                // endIndex is exclusive for the fragment text (it points to a line break, '\0', or commentEndIndex)
                int commentLength = endIndex - currentIndex;
                if (commentLength == 0 && !hasLineBreak) {
                    // indexOfBreakOrEndWithInfo() can stop on an embedded '\0'; avoid an infinite loop.
                    break;
                }
                Init::assignText_SimpleTextToken(commentFragment, context, context->chars + currentIndex, commentLength);
                if (hasLineBreak) {
                    // create a line break token for the line break after the comment fragment
                    LineBreakTokenStruct *newLineBreak = Alloc::newLineBreakToken(context, blockComment);
                    newLineBreak->foundPos = endIndex;
                    bool rn = (endIndex + 1) < context->length && context->chars[endIndex] == '\r' && context->chars[endIndex + 1] == '\n';
                    if (rn) { // \r\n
                        newLineBreak->text[0] = '\r';
                        newLineBreak->text[1] = '\n';
                        newLineBreak->text[2] = '\0';
                        currentIndex = endIndex + 2;
                    }
                    else {
                        newLineBreak->text[0] = context->chars[endIndex];
                        newLineBreak->text[1] = '\0';
                        currentIndex = endIndex + 1;
                    }

                    lastBreakLine = newLineBreak;
                }
                else {
                    currentIndex = endIndex;
                }

                if (lastFragment != nullptr) {
                    lastFragment->nextToken = Cast::upcastToken(commentFragment);
                }
                lastFragment = commentFragment;
                if (blockComment->firstCommentFragment == nullptr) {
                    blockComment->firstCommentFragment = commentFragment;
                }
            }
            else {
                break;
            }
        }
        return Cast::upcastToken(blockComment);
    }


    struct InternalParsingData
    {
        int32_t returnPos = Search::NOTFOUND;

        int32_t whitespace_startpos = -1;

        // the first line break token before the token generated by tokenizer.
        LineBreakTokenStruct *firstLineBreak = nullptr;
        // the last line break token, used for linking line break tokens in sequence when there are multiple line breaks before the next token.
        LineBreakTokenStruct *lastLineBreak = nullptr;

        TokenBase *commentToken = nullptr; // LineCommentTokenStruct or BlockCommentTokenStruct

        void assignCommentToken(TokenBase* targetToken)
        {
            assert(targetToken != nullptr);

            if (commentToken != nullptr) {
                targetToken->precedingCommentToken = commentToken;
                commentToken = nullptr;
            }
        }

        void assignWhiteSpaces(TokenBase* commentToken, int endIndex)
        {
            if (whitespace_startpos != -1) {
                assert(whitespace_startpos < endIndex);
                // precedingSpaceCount allows only ascii whitespace. Japanese whitespaces are not allowed.
                commentToken->precedingSpaceCount = endIndex - whitespace_startpos;
                whitespace_startpos = -1;
            }
        }

        // attach line break token to the token generated by tokenizer.
        void assignLineBreak(TokenBase* token)
        {
            if (this->firstLineBreak != nullptr) {
                token->precedingLineBreakToken = firstLineBreak;
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

        TokenBase *newCommentToken;
        if (isLineComment) {
            auto* lineComment = Alloc::newLineCommentToken(context, parentNode);
            lineComment->foundPos = i;
            Init::assignText_SimpleTextToken(lineComment, context, context->chars +  i, commentEndIndex - i);

            newCommentToken = Cast::upcastToken(lineComment);
        }
        else {
            newCommentToken = generateBlockCommentFragments(parentNode, context, i, commentEndIndex, tagText, tagLength);
        }

        parsingData->assignWhiteSpaces(newCommentToken, i);

        TokenBase* prevCommentToken = parsingData->commentToken;
        parsingData->commentToken = newCommentToken;
        if (prevCommentToken != nullptr) {
            newCommentToken->precedingCommentToken = prevCommentToken;
        }

        parsingData->assignLineBreak(newCommentToken);
        return commentEndIndex;
    }

    

    static inline int createLineBreakToken(void* parentNode, ParseContext* context, 
                                          int32_t& position, utf8byte ch, InternalParsingData* parsingData)
    {
        auto* newLineBreak = Alloc::newLineBreakToken(context, parentNode);
        newLineBreak->foundPos = position;

        if (parsingData->firstLineBreak == nullptr) { // the first line break
            parsingData->lastLineBreak = parsingData->firstLineBreak = newLineBreak;
        }
        else {
            assert(parsingData->lastLineBreak != nullptr);
            parsingData->lastLineBreak->nextLineBreak = newLineBreak;
            parsingData->lastLineBreak = newLineBreak;
        }

        parsingData->assignWhiteSpaces(Cast::upcastToken(newLineBreak), position);
        parsingData->assignCommentToken(Cast::upcastToken(newLineBreak));

        bool rn = ch == '\r' && (position + 1) < context->length && context->chars[position + 1] == '\n';
        int result;
        if (rn) { // \r\n
            newLineBreak->text[0] = '\r';
            newLineBreak->text[1] = '\n';
            newLineBreak->text[2] = '\0';
            result = position + 2;
        }
        else {
            newLineBreak->text[0] = ch;
            newLineBreak->text[1] = '\0';
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
        context->mostLeftToken = nullptr;

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
                i = createLineBreakToken(parentNode, context, i, ch, &parsingData);
                context->isAfterLineBreak = true;
                continue;
            }
            else if (ParseUtil::isSpace(ch)) {
                parsingData.whitespace_startpos = i;
                i = detectSpaceEndIndex(i, context);
                continue;
            }

            int result = tokenizer(parentNode, ch, i, context);
            if (result != Search::DONE_WITH_PREVIOUS_POSITION) {
                parsingData.returnPos = result;
            }
            if (context->syntaxErrorInfo.hasError) {
                parsingData.returnPos = Search::NOTFOUND;
                return parsingData;
            }

            if (result > -1) {
                context->isAfterLineBreak = false;
                context->lastTokenizedPos = result;

                assert(context->mostLeftToken != nullptr);
                parsingData.assignWhiteSpaces(context->mostLeftToken, i);
                parsingData.assignCommentToken(context->mostLeftToken);
                parsingData.assignLineBreak(context->mostLeftToken);

                if (loopMode && !context->scanEnd) {
                    i = result;
                    continue;
                }
            }
            break;
        }

        if (parsingData.returnPos == Search::NOTFOUND && context->lastTokenizedPos == lastTokenizedPos) {
            context->lastTokenizedPos = lastTokenizedPos; // reset lastTokenizedPos only if nothing was tokenized in this scan
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

        context->remainedLineBreakToken = parsingData.firstLineBreak;
        context->remainedCommentToken = parsingData.commentToken;
        if (parsingData.whitespace_startpos > -1 && parsingData.whitespace_startpos < context->length) {
            context->remainedSpaceCount = context->length - parsingData.whitespace_startpos;
        }

        return parsingData.returnPos;
    }
}
