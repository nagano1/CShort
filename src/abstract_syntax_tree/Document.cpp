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
        context->lastTokenizedPos = 0;

        context->remainedLineBreakNode = nullptr;
        context->remainedCommentNode = nullptr;

        context->remainedSpaceCount = 0;
        context->baseIndent = 4;
        context->parentDepth = -1;
        context->arithmeticBaseDepth = -1;
        context->isAfterLineBreak = false;

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

            callAllLineEvent(docStruct, docStruct->firstCodeLine, context);
        }
    }
}
