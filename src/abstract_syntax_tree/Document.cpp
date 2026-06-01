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

    static int selfTextLength(DocumentStruct *)
    {
        return 0;
    }

    static void copySelfText(DocumentStruct *self, utf8byte *buf)
    {
    }

    static CodeLine *appendToLine(DocumentStruct *self, CodeLine *currentCodeLine)
    {
        auto *child = self->firstRootNode;
        while (child) {
            currentCodeLine = VTableCall::callAppendNodeToLine(child, currentCodeLine);
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
        context->init();


        INIT_NODE(doc, context, nullptr, VTables::DocumentVTable);
        INIT_NODE(&doc->endOfFile, context, Cast::upcast(doc), VTables::EndOfFileVTable);
        Init::initSimpleTextToken(&doc->endOfFile.eofToken, context, Cast::upcast(&doc->endOfFile), 0);

        doc->documentType = docType;
        doc->firstRootNode = nullptr;
        doc->lastRootNode = nullptr;

        doc->firstCodeLine = nullptr;
        doc->nodeCount = 0;

        return doc;
    }

    // CodeLine instances are arena-allocated via ParseContext::memBufferForCodeLines; freeing is handled by memBufferForCodeLines.freeAll().

    void Alloc::deleteDocument(DocumentStruct *doc) {
        doc->context->dispose();
        free(doc->context);
        free(doc);
    }

    utf8byte *DocumentUtils::getTextFromTree(DocumentStruct *doc)
    {
        // get size of chars
        size_t totalCount = 0;
        {
            auto *line = doc->firstCodeLine;
            while (line) {
                auto *token = line->firstToken;
                while (token) {
                    if (token->precedingSpaceCount > 0) {
                        totalCount += token->precedingSpaceCount;
                    }
                    int len = TokenVTableCall::selfTextLength(token);
                    totalCount += len;
                    token = token->nextTokenInLine;
                }

                line = line->nextLine;
            }
        }

        // malloc and copy text
        auto *text = (char *) malloc(totalCount + 1);
        text[totalCount] = '\0';
        if (totalCount > 0) {
            CodeLine *line = doc->firstCodeLine;
            size_t currentOffset = 0;
            while (line) {
                auto *token = line->firstToken;
                while (token) {
                    if (token->precedingSpaceCount > 0) {
                        memset(text + currentOffset, ' ', token->precedingSpaceCount);
                        currentOffset += token->precedingSpaceCount;
                    }

                    size_t len = TokenVTableCall::selfTextLength(token);
                    TokenVTableCall::copySelfText(token, text + currentOffset);

                    currentOffset += len;
                    token = token->nextTokenInLine;
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
        } else if (Search::IsTokenized(result = Tokenizers::fnTokenizer(TokenizerParams_pass))) {
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

        VTableCall::callAppendNodeToLine(docStruct, docStruct->firstCodeLine);
    }

    void DocumentUtils::parseText(DocumentStruct *docStruct, const utf8byte *text, int length)
    {
        assert(docStruct->context != nullptr);

        auto *context = docStruct->context;
        // Reset previous parse state (parseText can be called multiple times on the same document)
        docStruct->firstRootNode = nullptr;
        docStruct->lastRootNode = nullptr;
        docStruct->nodeCount = 0;

        // Ensure the embedded EOF node doesn't retain links from a previous parse.
        INIT_NODE(&docStruct->endOfFile, context, docStruct, VTables::EndOfFileVTable);

        docStruct->firstCodeLine = nullptr;
        docStruct->lineCount = 0;
        // Clear previous CodeLine arena to avoid stale pointers when parsing fails.
        context->memBufferForCodeLines.freeAll();
        context->memBufferForCodeLines.init();
        // Clear previous token/node arena (this invalidates pointers from the previous parse).
        context->memBuffer.freeAll();
        context->memBuffer.init();
        // Re-init persistent EOF token so its internal pointers don't refer to freed arena memory.
        Init::initSimpleTextToken(&docStruct->endOfFile.eofToken, context, Cast::upcast(&docStruct->endOfFile), 0);






        context->syntaxErrorInfo.hasError = false;
        context->syntaxErrorInfo.errorItem.errorIndex = ErrorIndex::no_syntax_error;
        context->syntaxErrorInfo.errorItem.errorId = 10000;
        context->syntaxErrorInfo.errorItem.charPosition = -1;
        context->syntaxErrorInfo.errorItem.charPosition2 = -1;
        context->chars = text;
        context->start = 0;
        context->scanEnd = false;
        context->length = length;
        context->mostLeftToken = nullptr;
        context->generatedPrimaryNode = nullptr;
        context->lastTokenizedPos = 0;
        context->remainedLineBreakToken = nullptr;
        context->remainedCommentToken = nullptr;

        context->remainedSpaceCount = 0;
        context->baseIndent = 4;
        context->parentDepth = -1;
        context->arithmeticBaseDepth = -1;
        context->isAfterLineBreak = false;

        context->unusedClassNode = nullptr;
        context->unusedAssignment = nullptr;


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
            docStruct->endOfFile.eofToken.precedingSpaceCount = context->remainedSpaceCount;
            docStruct->endOfFile.eofToken.precedingLineBreakToken = context->remainedLineBreakToken;
            docStruct->endOfFile.eofToken.precedingCommentToken = context->remainedCommentToken;

            DocumentUtils::regenerateCodeLines(docStruct);

            callAllLineEvent(docStruct, docStruct->firstCodeLine, context);

        }
    }
}
