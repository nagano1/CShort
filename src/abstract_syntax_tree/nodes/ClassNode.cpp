#include <cstdio>
#include <iostream>
#include <array>
#include <algorithm>


#include <cstdlib>
#include <cassert>
#include <cstdio>
#include <chrono>
#include <vector>

#include <cstdint>
#include <ctime>
#include <cstring>

#include "code_nodes.hpp"


namespace cshort {

    using InnerNodeStruct = struct
    {
        NODE_HEADER;
    };

    // --------------------- Defines Class VTable ---------------------- /

    static int selfTextLength(ClassNodeStruct *)
    {
        return 0;
    }

    static void copySelfText(ClassNodeStruct * classNode, utf8byte *buf) {
    }

    static CodeLine *appendToLine(ClassNodeStruct *classNode, CodeLine *currentCodeLine)
    {
        /*
        class className {
            [child nodes]
        }
        */
        auto *context = classNode->context;

        auto *firstLine = currentCodeLine;
        bool firstIncrementMode = context->incrementDepthOnNextLine;
        int firstDepth = context->currentIndentDepth;
        int thisClassBaseDepth = context->getNextLineIndentDepth();
        
        currentCodeLine = TokenVTableCall::callAppendTokenToLine(&classNode->classKeywordToken, currentCodeLine);

        auto formerParentDepth = context->currentIndentDepth;

        context->incrementDepthOnNextLine = true;
        // className
        currentCodeLine = TokenVTableCall::callAppendTokenToLine(&classNode->identifierToken, currentCodeLine);
        context->incrementDepthOnNextLine = false;
        context->currentIndentDepth = formerParentDepth;

        // {
        currentCodeLine  = TokenVTableCall::callAppendTokenToLine(&classNode->bodyStartSymbolToken, currentCodeLine);
        context->incrementDepthOnNextLine = true;

        {
            auto *child = classNode->firstChildNode;
            while (child) {
                currentCodeLine = VTableCall::callAppendNodeToLine(child, currentCodeLine);
                child = child->nextNode;
            }
        }

        auto *previousLine = currentCodeLine;
        // }
        currentCodeLine = TokenVTableCall::callAppendTokenToLine(&classNode->endBodySymbolToken, currentCodeLine);
        if (CodeLine::HasOnlyEndParentheses(currentCodeLine)) { // current line has only the end bracket.
            currentCodeLine->depth = thisClassBaseDepth;
        }

        if (currentCodeLine == firstLine) {
            // if the body is empty, the end bracket will be in the same line as the start bracket, in this case we should not change the indent depth for this line, and the indent depth should be the same as the parent node's indent depth.
            context->currentIndentDepth = firstDepth;
            context->incrementDepthOnNextLine = firstIncrementMode;
        }
        else {
            context->currentIndentDepth = thisClassBaseDepth;
        }

        context->currentIndentDepth = formerParentDepth;
        //context->incrementDepthOnNextLine = firstIncrementMode;

        return currentCodeLine;
    };


    static constexpr const char classTypeText[] = "<Class>";

    static int applyFuncToDescendants(ClassNodeStruct *Node, ApplyFunc_params3) {
        return 0;
    }

    /*
     * class A {
     *
     *
     * }
     */
    static node_vtable _classVTable = CREATE_VTABLE(ClassNodeStruct,
                                                          selfTextLength,
                                                          copySelfText,
                                                          appendToLine,
                                                          applyFuncToDescendants,
                                                          classTypeText
                                                          , NodeTypeId::Class);

    const struct node_vtable *VTables::ClassVTable = &_classVTable;


    // -------------------- Implements ClassNode Allocator --------------------- //
    ClassNodeStruct *Alloc::newClassNode(ParseContext *context, NodeBase *parentNode) {
        auto *classNode = context->newMem<ClassNodeStruct>();

        INIT_NODE(classNode, context, parentNode, &_classVTable);
        classNode->lastChildNode = nullptr;
        classNode->firstChildNode = nullptr;
        classNode->childCount = 0;

        Init::initSimpleTextToken(&classNode->classKeywordToken, context, classNode, /*"class",*/ 5);
        memcpy(classNode->classKeywordToken.text, "class", 5);

        Init::initIdentifierToken(&classNode->identifierToken, context, classNode);

        classNode->startFound = false;

        Init::initSymbolToken(&classNode->bodyStartSymbolToken, context, classNode, '{');
        Init::initSymbolToken(&classNode->endBodySymbolToken, context, classNode, '}');
        classNode->endBodySymbolToken.isEndFlag = true;

        return classNode;
    }


    // --------------------- Implements ClassNode Parser ----------------------

    static void appendChildNode(ClassNodeStruct *classNode, NodeBase *node) {
        if (classNode->firstChildNode == nullptr) {
            classNode->firstChildNode = node;
        }
        if (classNode->lastChildNode != nullptr) {
            classNode->lastChildNode->nextNode = node;
        }
        classNode->lastChildNode = node;
        classNode->childCount++;
    }

    static int inner_classBodyTokenizer(TokenizerParams_argNode_ch_start_context) {
        NodeBase *parent = reinterpret_cast<NodeBase *>(argNode);
        auto *classNode = Cast::downcast<ClassNodeStruct *>(parent);

        if (!classNode->startFound) {
            if (ch == '{') {
                classNode->startFound = true;
                classNode->bodyStartSymbolToken.foundPos = start;
                context->mostLeftToken = Cast::upcastToken(&classNode->bodyStartSymbolToken);
                return start + 1;
            }
            else {
                context->setError(ErrorIndex::no_brace_for_class, classNode->classKeywordToken.foundPos);
            }
        }
        else if (ch == '}') {
            context->scanEnd = true;
            classNode->endBodySymbolToken.foundPos = start;
            context->mostLeftToken = Cast::upcastToken(&classNode->endBodySymbolToken);
            return start + 1;
        }
        else {
            int result;
            if (Search::IsTokenized(result = Tokenizers::classTokenizer(parent, ch, start, context))) {
                appendChildNode(classNode, context->generatedPrimaryNode);
                return result;
            }
            else if (Search::IsTokenized(result = Tokenizers::fnTokenizer(parent, ch, start, context))) {
                appendChildNode(classNode, context->generatedPrimaryNode);
                return result;
            }

            context->scanEnd = true;
            if (ch == '\0') {
                context->setError(ErrorIndex::no_brace_of_end_for_class, classNode->classKeywordToken.foundPos);
            } else {
                context->setError(ErrorIndex::syntax_error, start);
            }
        }

        return Search::NOTFOUND;
    }




    int Tokenizers::classTokenizer(TokenizerParams_argNode_ch_start_context) {
        static constexpr const char class_chars[] = "class";
        static constexpr int size_of_class = sizeof(class_chars) - 1;

        NodeBase *parent = reinterpret_cast<NodeBase *>(argNode);

        if ('c' == ch) {
            if (ParseUtil::matchWordWithTerminatableEnd(context->chars, context->length, start, class_chars)) {
                int currentPos = start + size_of_class;
                int resultPos;

                // "class " matched
                auto *classNode = Alloc::newClassNode(context, parent);
                classNode->classKeywordToken.foundPos = start;

                {
                    resultPos = Scanner::scanOnce(&classNode->identifierToken,
                                              Tokenizers::identifierTokenizer,
                                              context, currentPos);

                    if (!Search::IsTokenized(resultPos)) {
                        // the class should have a class name
                        context->setError(ErrorIndex::invalid_class_name, start);
                        return Search::NOTFOUND;
                    }
                }


                // Parse body
                currentPos = resultPos;
                if (!Search::IsTokenized(resultPos = Scanner::scanLoop(classNode, inner_classBodyTokenizer,
                                                     context, currentPos))) {
                    return Search::NOTFOUND;
                }

                context->mostLeftToken = Cast::upcastToken( &classNode->classKeywordToken);
                context->generatedPrimaryNode = Cast::upcast(classNode);
                return resultPos;
            }
        }
        return Search::NOTFOUND;
    }
}



