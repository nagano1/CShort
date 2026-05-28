#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>
#include <string>
#include <array>
#include <algorithm>


#include <cstdlib>
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <ctime>
#include <stdio.h>

#include "code_nodes.hpp"

namespace cshort {

    // --------------------- Defines Document VTable ----------------------

    static int selfTextLength(DocumentStruct *self)
    {
        return 5;
    }

    static void copySelfText(DocumentStruct *self, utf8byte *buf)
    {
    }

    static CodeLine *appendToLine(DocumentStruct *self, CodeLine *currentCodeLine)
    {
        auto *child = self->firstRootNode;
        while (child) {
            currentCodeLine = VTableCall::callAppendToLine(child, currentCodeLine);
            child = child->nextNode;
        }
        return currentCodeLine;
    }

    static int applyFuncToDescendants(DocumentStruct *Node, ApplyFunc_params3)
    {
        return 0;
    }


    static constexpr const char DocumentTypeText[] = "<Document>";

    static node_vtable DocumentVTable_ = CREATE_VTABLE(DocumentStruct, selfTextLength, copySelfText,
                                                       appendToLine, applyFuncToDescendants,
                                                       DocumentTypeText, NodeTypeId::Document);

    const node_vtable *VTables::DocumentVTable = &DocumentVTable_;



    // --------------------- Implements Document functions ----------------------
    DocumentStruct *Alloc::newDocument(DocumentType docType) {

        auto *context = mallocForType<ParseContext>();
        auto *doc = mallocForType<DocumentStruct>();


        INIT_NODE(doc, context, nullptr, VTables::DocumentVTable);
        INIT_NODE(&doc->endOfFile, context, Cast::upcast(doc), VTables::EndOfFileVTable);

        doc->documentType = docType;
        doc->firstRootNode = nullptr;
        doc->lastRootNode = nullptr;

        doc->firstCodeLine = nullptr;
        doc->nodeCount = 0;

        context->init();
        return doc;
    }

    static inline void deleteLineNodes(CodeLine *line) {
        assert(line != nullptr);
        if (line) {
            if (line->nextLine) {
                deleteLineNodes(line->nextLine);
            }
            free(line);
        }
    }

    void Alloc::deleteDocument(DocumentStruct *doc) {
        doc->context->dispose();
        free(doc->context);
        free(doc);
    }


    utf8byte *DocumentUtils::getTextFromNode(NodeBase *node) {
        int len = VTableCall::selfTextLength(node);
        int spaceCount = node->precedingSpaceCount;
        auto *text = node->context->newText(len + spaceCount);

        for (int i = 0; i < spaceCount; i++) {
            text[i] = ' ';
        }

        if (len > 0) {
            VTableCall::copySelfText(node, text + spaceCount);
        }

        text[len + spaceCount] = '\0';
        return text;
    }

    // output type text of all nodes in the tree, for debugging, example:
    // <LineBreak>
    // <Class>        class<Name> TestCass<LineBreak>
    // <Symbol>        {<LineBreak>
    // <fn>            fn<Name> func<Symbol>(<Symbol>)<LineBreak>
    // <body>            {<LineBreak>
    // <Type>                let<Name> aw<Symbol> =<number> 242<LineBreak>
    // <bool>                true<LineBreak>
    // <NULL>                null<LineBreak>
    // <Variable>                printf<Symbol>(<FuncArgument><number>214<Symbol>)<LineBreak>
    // <Symbol>            }<LineBreak>
    // <Symbol>        }<LineBreak>
    // <EndOfFile>
    utf8byte *DocumentUtils::getTypeTextFromTree(DocumentStruct *doc) {
        // get size of chars
        int totalBytes = 0;
        {
            auto *line = doc->firstCodeLine;
            while (line) {
                auto *node = line->firstNode;
                while (node) {
                    if (node->precedingSpaceCount > 0) {
                        totalBytes += node->precedingSpaceCount;
                    }
                    int len = VTableCall::typeTextLength(node) + VTableCall::selfTextLength(node);
                    totalBytes += len;
                    node = node->nextNodeInLine;
                }

                line = line->nextLine;
            }
        }

        if (totalBytes == 0) {
            return nullptr;
        }

        // malloc and copy text
        auto *outputText = (char *) malloc(sizeof(char) * totalBytes + 1);

        auto *line = doc->firstCodeLine;
        size_t currentOffset = 0;
        while (line) {
            auto *node = line->firstNode;
            while (node) {
                auto *typeText = VTableCall::typeText(node);
                size_t typeTextLen = VTableCall::typeTextLength(node);
                memcpy(outputText + currentOffset, typeText, typeTextLen);
                currentOffset += typeTextLen;

                if (node->precedingSpaceCount > 0) {
                    memset(outputText + currentOffset, ' ', node->precedingSpaceCount);
                    currentOffset += node->precedingSpaceCount;
                }

                int textLen = VTableCall::selfTextLength(node);
                if (textLen > 0) {
                    VTableCall::copySelfText(node, outputText + currentOffset);
                }
                currentOffset += textLen;

                node = node->nextNodeInLine;
            }

            line = line->nextLine;
        }

        outputText[totalBytes] = '\0';

        return outputText;
    }
/*
    static int getTokenTypeId(NodeBase *node, int i)
    {
        auto *targetNode = node;
        if (targetNode->vtable == VTables::SimpleTextVTable) {
            targetNode = targetNode->parentNode;
        }

        if (targetNode->vtable == VTables::VariableVTable) {
            if (targetNode->parentNode->vtable == VTables::CallFuncVTable) {
                targetNode = targetNode->parentNode;
            }
        }

        if (targetNode->vtable == VTables::BlockCommentFragmentVTable
            || targetNode->vtable == VTables::LineCommentVTable) {
            return (int) TokenTypeIds::commentId;
        } else if (targetNode->vtable == VTables::BoolVTable) {
            return -1;
        } else if (targetNode->vtable == VTables::NullVTable) {
            return -1;
        } else if (targetNode->vtable == VTables::NumberVTable) {
            return (int) TokenTypeIds::numberId;
        } else if (targetNode->vtable == VTables::StringLiteralVTable) {
            return (int) TokenTypeIds::stringId;
        } else if (targetNode->vtable == VTables::CallFuncVTable) {
            return (int) TokenTypeIds::functionId;
        } else if (targetNode->vtable == VTables::VariableVTable) {
            return (int) TokenTypeIds::variableId;
        } else if (targetNode->vtable == VTables::SimpleTextVTable) {
            return -1;//(int)TokenTypeIds::keywordId;
        } else if (targetNode->vtable == VTables::ClassVTable) {
            return -1;//(int)TokenTypeIds::myclass;
        }  else if (targetNode->vtable == VTables::FnVTable) {
            return -1;
        } else if (targetNode->vtable == VTables::SymbolVTable) {
            return (int) TokenTypeIds::keywordId;
        } else if (targetNode->vtable == VTables::AssignStatementVTable) {
            auto *assign = Cast::downcast<AssignStatementNodeStruct *>(targetNode);
            if (assign->typeOrLet.hasImmutableMark || assign->typeOrLet.hasNullableMark) {
                if (i == 0) {
                    return (int) TokenTypeIds::numberId;
                }
            }

            return -1;//(int) TokenTypeIds::keywordId;
        } else if (targetNode->vtable == VTables::NameVTable) {
            if (targetNode->parentNode->vtable == VTables::FnVTable) {
                return (int) TokenTypeIds::functionId;
            } else if (targetNode->parentNode->vtable == VTables::ClassVTable) {
                return (int) TokenTypeIds::classId;
            } else {
                return (int) TokenTypeIds::variableId;
            }
        }
        else if (targetNode->vtable == VTables::TypeVTable) {
            auto* typeNode = Cast::downcast<TypeNodeStruct*>(targetNode);

            if (typeNode->hasImmutableMark || typeNode->hasNullableMark) {
                if (i == 0) {
                    return (int)TokenTypeIds::commentId;
                }
            }

            if (typeNode->parentNode->vtable == VTables::AssignStatementVTable) {
                //return (int)TokenTypeIds::keywordId;
            }
            if (typeNode->isLet) {
                return (int)TokenTypeIds::keywordId;
            }
            return (int)TokenTypeIds::numberId;
        }
        return -1;
    }

    static void
    splitCharsIfYouWant(NodeBase *node, int *len0, int *utf16Len0, int *len1, int *utf16Len1) {
        auto *chs = VTableCall::selfText(node);
        int len = VTableCall::selfTextLength(node);
        *utf16Len0 = ParseUtil::utf16_length(chs, len);

        auto *targetNode = node;
        if (targetNode->vtable == VTables::SimpleTextVTable) {
            targetNode = targetNode->parentNode;
        }

        if (targetNode->vtable == VTables::AssignStatementVTable) {
            auto *assign = Cast::downcast<AssignStatementNodeStruct *>(targetNode);
            if (assign->typeOrLet.hasImmutableMark || assign->typeOrLet.hasNullableMark) {
                *utf16Len1 = *utf16Len0 - 1;
                *len0 = 1;
                *utf16Len0 = 1;
                *len1 = len - 1;
                return;
            }
        }
        if (targetNode->vtable == VTables::TypeVTable) {
            auto* typeNode = Cast::downcast<TypeNodeStruct*>(targetNode);
            if (typeNode->hasImmutableMark || typeNode->hasNullableMark) {
                *utf16Len1 = *utf16Len0 - 1;
                *len0 = 1;
                *utf16Len0 = 1;
                *len1 = len - 1;
                return;
            }
        }



        if (ParseUtil::hasCharBeforeLineBreak(chs, len, 0)) {
            *len0 = len;
        }
    }

    static inline int addSemanticTokens(NodeBase *node, char *text, int currentLineNo, bool *first,
                                        int *prevSetLine, int *prevSetStart, int *charPos) {
        int len0 = 0, len1 = 0, utf16Len0 = 0, utf16Len1 = 0;
        splitCharsIfYouWant(node, &len0, &utf16Len0, &len1, &utf16Len1);

        int writeBytes = 0;

        for (int i = 0; i <= 1; i++) {
            int len = i == 0 ? len0 : len1;
            int utf16len = i == 0 ? utf16Len0 : utf16Len1;
            if (len > 0) {
                int tokenTypeId = getTokenTypeId(node, i);
                if (tokenTypeId > -1) {
                    // { line: 2, startChar:  5, length: 3, tokenType: 0, tokenModifiers: 3 },
                    const char *tokenModifiersFlag = "0";
                    int wlen = sprintf(text + writeBytes,
                                       "%s%d,%d,%d,%d,%s",
                                       *first ? "" : ",",
                                       currentLineNo - *prevSetLine,
                                       *charPos - *prevSetStart,
                                       utf16len,
                                       tokenTypeId,
                                       tokenModifiersFlag
                    );

                    if (wlen > 0) {
                        *prevSetLine = currentLineNo;
                        *prevSetStart = *charPos;

                        if (*first) {
                            *first = false;
                        }
                        writeBytes += wlen;
                    }
                }
                *charPos += utf16len;
            }
        }
        return writeBytes;
    }


// { line: 2, startChar:  5, length: 3, tokenType: 0, tokenModifiers: 3 },
    static int getSemanticTokensLength(DocumentStruct *doc, char *text, int line0, int line1)
    {
        int totalByteCount = 0;
        {
            static char buff[255];

            auto *line = doc->firstCodeLine;
            int currentLineNo = 0;
            bool first = true;
            int prevLine = 0;
            while (line) {
                bool insideRange = true;
                if (line0 != -1) {
                    if (currentLineNo < line0 || line1 < currentLineNo) {
                        insideRange = false;
                    }
                }

                if (insideRange) {
                    auto *node = line->firstNode;

                    int charPos = 0;
                    int prevStart = 0;
                    while (node) {
                        charPos += node->precedingSpaceCount;

                        char *dst = text != nullptr ? text + totalByteCount : buff;
                        int writeBytes = addSemanticTokens(node, dst, currentLineNo, &first,
                                                           &prevLine,
                                                           &prevStart, &charPos);
                        totalByteCount += writeBytes;

                        node = node->nextNodeInLine;
                    }
                }
                line = line->nextLine;
                currentLineNo++;
            }
        }

        return totalByteCount;
    }


    utf8byte *DocumentUtils::getSemanticTokensTextFromTree(DocumentStruct *doc, int *len, int line0,
                                                           int line1) {
        int totalCount = getSemanticTokensLength(doc, nullptr, line0, line1);

        *len = totalCount;
        auto *text = (char *) malloc(sizeof(char) * totalCount + 1);
        getSemanticTokensLength(doc, text, line0, line1);
        text[totalCount] = '\0';
        return text;
    }
*/
    utf8byte *DocumentUtils::getTextFromTree(DocumentStruct *doc)
    {
        // get size of chars
        int totalCount = 0;
        {
            auto *line = doc->firstCodeLine;
            while (line) {
                auto *node = line->firstNode;
                while (node) {
                    if (node->precedingSpaceCount > 0) {
                        totalCount += node->precedingSpaceCount;
                    }
                    int len = VTableCall::selfTextLength(node);
                    totalCount += len;
                    node = node->nextNodeInLine;
                }

                line = line->nextLine;
            }
        }

        // malloc and copy text
        auto *text = (char *) malloc(sizeof(char) * totalCount + 1);
        text[totalCount] = '\0';
        if (totalCount > 0) {
            CodeLine *line = doc->firstCodeLine;
            size_t currentOffset = 0;
            while (line) {
                auto *node = line->firstNode;
                while (node) {
                    if (node->precedingSpaceCount > 0) {
                        memset(text + currentOffset, ' ', node->precedingSpaceCount);
                        currentOffset += node->precedingSpaceCount;
                    }

                    size_t len = VTableCall::selfTextLength(node);
                    VTableCall::copySelfText(node, text + currentOffset);

                    currentOffset += len;
                    node = node->nextNodeInLine;
                }

                line = line->nextLine;
            }
        }

        return text;
    }


    static void appendRootNode(DocumentStruct *doc, NodeBase *node)
    {
        if (doc->firstRootNode == nullptr) {
            doc->firstRootNode = node;
        }
        if (doc->lastRootNode != nullptr) {
            doc->lastRootNode->nextNode = node;
        }
        doc->lastRootNode = node;
        doc->nodeCount++;
    }


    static int tokenizeDocumentRootLoop(TokenizerParams_argNode_ch_start_context)
    {
        auto *doc = Cast::downcast<DocumentStruct *>(argNode);
        int result;

        if (Search::IsTokenized(result = Tokenizers::classTokenizer(TokenizerParams_pass))) {
            appendRootNode(doc, context->generatedPrimaryNode);
            return result;
        }
        else if (Search::IsTokenized(result = Tokenizers::fnTokenizer(TokenizerParams_pass))) {
            appendRootNode(doc, context->generatedPrimaryNode);
            return result;
        }

        if (ch != '\0') {
            context->setError(ErrorIndex::syntax_error, start);
        }

        if (context->syntaxErrorInfo.hasError) {
            //throw 3;
        }

        return Search::NOTFOUND;
    }

    static void callAllLineEvent(DocumentStruct *docStruct, CodeLine *line, ParseContext *context) {
        CodeLine *prev = nullptr;
        int lineCount = 0;
        while (line) {
            lineCount++;
            prev = line;
            line = line->nextLine;
        }

        docStruct->lineCount = lineCount;
    }

    void DocumentUtils::regenerateCodeLines(DocumentStruct *docStruct)
    {
        auto *context = docStruct->context;
        context->appendLineMode = AppendLineMode::Normal;
        context->memBufferForCodeLines.freeAll();
        context->memBufferForCodeLines.init();
        docStruct->firstCodeLine = context->newCodeLine();
        docStruct->firstCodeLine->init(context);

        VTableCall::callAppendToLine(docStruct, docStruct->firstCodeLine);
    }

    void DocumentUtils::parseText(DocumentStruct *docStruct, const utf8byte *text, int length)
    {
        assert(docStruct->context != nullptr);

        auto *context = docStruct->context;
        context->syntaxErrorInfo.hasError = false;
        context->syntaxErrorInfo.errorItem.errorIndex = ErrorIndex::no_syntax_error;
        context->syntaxErrorInfo.errorItem.errorId = 10000;
        context->syntaxErrorInfo.errorItem.charPosition = -1;
        context->syntaxErrorInfo.errorItem.charPosition2 = -1;
        context->chars = const_cast<utf8byte *>(text);
        context->start = 0;
        context->scanEnd = false;
        context->length = length;
        context->mostLeftNode = nullptr;
        context->generatedPrimaryNode = nullptr;

        context->remainedLineBreakNode = nullptr;
        context->remainedCommentNode = nullptr;

        context->remainedSpaceCount = 0;
        context->baseIndent = 4;
        context->parentDepth = -1;
        context->arithmeticBaseDepth = -1;
        context->isAfterLineBreak = false;

        context->unusedAssignment = nullptr;
        context->unusedClassNode = nullptr;


        if (docStruct->documentType == DocumentType::CodeDocument) {
            Scanner::scanRoot(docStruct, tokenizeDocumentRootLoop, context);
        }
        
        if (!context->syntaxErrorInfo.hasError) {
            if (docStruct->lastRootNode) {
                docStruct->lastRootNode->nextNode = Cast::upcast(&docStruct->endOfFile);
            }
            else {
                docStruct->firstRootNode = Cast::upcast(&docStruct->endOfFile);
            }
            docStruct->lastRootNode = Cast::upcast(&docStruct->endOfFile);
            docStruct->lastRootNode->precedingSpaceCount = context->remainedSpaceCount;
            docStruct->lastRootNode->precedingLineBreakNode = context->remainedLineBreakNode;
            docStruct->lastRootNode->precedingCommentNode = context->remainedCommentNode;

            DocumentUtils::regenerateCodeLines(docStruct);

            DocumentUtils::assignIndents(docStruct);
            DocumentUtils::checkIndentSyntaxErrors(docStruct);

            DocumentUtils::calcStackSize(docStruct);

            callAllLineEvent(docStruct, docStruct->firstCodeLine, context);
        }
    }
}
