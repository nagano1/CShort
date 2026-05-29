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
        return 5;
    }

    static void copySelfText(ClassNodeStruct * classNode, utf8byte *buf) {
        TEXT_MEMCPY(buf, "class", 5); 
    }

    static CodeLine *appendToLine(ClassNodeStruct *classNode, CodeLine *currentCodeLine) {

        currentCodeLine = currentCodeLine->AddAttachedFormatNodes(classNode);
        currentCodeLine->appendNode(classNode);


        auto formerParentDepth = classNode->context->parentDepth;
        classNode->context->parentDepth += 1;
        currentCodeLine = VTableCall::callAppendToLine(&classNode->nameNode, currentCodeLine);
        classNode->context->parentDepth = formerParentDepth;

        currentCodeLine = VTableCall::callAppendToLine(&classNode->bodyStartNode, currentCodeLine);

        formerParentDepth = classNode->context->parentDepth;
        classNode->context->parentDepth += 1;

        {
            auto *child = classNode->firstChildNode;
            while (child) {
                currentCodeLine = VTableCall::callAppendToLine(child, currentCodeLine);
                child = child->nextNode;
            }
        }



        auto* prevCodeLine = currentCodeLine;
        currentCodeLine = VTableCall::callAppendToLine(&classNode->endBodyNode, currentCodeLine);

        if (prevCodeLine != currentCodeLine) {
            currentCodeLine->depth = formerParentDepth+1;
        }

        classNode->context->parentDepth = formerParentDepth;


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

        Init::initNameNode(&classNode->nameNode, context, classNode);

        classNode->startFound = false;

        Init::initSymbolNode(&classNode->bodyStartNode, context, classNode, '{');
        Init::initSymbolNode(&classNode->endBodyNode, context, classNode, '}');

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
        NodeBase *parent = argNode;
        auto *classNode = Cast::downcast<ClassNodeStruct *>(parent);

        if (!classNode->startFound) {
            if (ch == '{') {
                classNode->startFound = true;
                context->setCodeNode(&classNode->bodyStartNode);
                return start + 1;
            }
            else {
                context->setError(ErrorIndex::no_brace_for_class, classNode->foundPos);
            }
        }
        else if (ch == '}') {
            context->scanEnd = true;
            context->setCodeNode(&classNode->endBodyNode);
            return start + 1;
        }
        else {
            int result;
            if (Search::IsTokenized(result = Tokenizers::classTokenizer(parent, ch, start, context))) {
                auto *innerClassNode = Cast::downcast<ClassNodeStruct *>(parent);
                appendChildNode(innerClassNode, context->generatedPrimaryNode);
                return result;
            }

            context->scanEnd = true;
            context->setError2(ErrorIndex::no_brace_of_end_for_class, classNode->foundPos, start);
        }

        return Search::NOTFOUND;
    }




    int Tokenizers::classTokenizer(TokenizerParams_argNode_ch_start_context) {
        static constexpr const char class_chars[] = "class";
        static constexpr int size_of_class = sizeof(class_chars) - 1;

        NodeBase *parent = argNode;

        if ('c' == ch) {
            auto idx = ParseUtil::matchWordWithTerminatableEnd(context->chars, context->length, start, class_chars);
            if (idx > -1) {
                int currentPos = idx + size_of_class;
                int resultPos;

                // "class " came here
                auto *classNode = Alloc::newClassNode(context, parent);
                classNode->foundPos = start;

                {
                    resultPos = Scanner::scanOnce(&classNode->nameNode,
                                              Tokenizers::nameTokenizer,
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

                context->setCodeNode(classNode);
                return resultPos;
            }
        }
        return Search::NOTFOUND;
    }
}



