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
    // FuncDefNodeStruct represents a function declaration. It contains the function name, parameters, and body.
    // Declaration syntax of function looks like this:
    //     fn funcName(int arg1, string *arg2, ...) {
    //        [body]
    //     }

    static constexpr const char fn_chars[] = "fn";
    static constexpr const char fn_first_char = fn_chars[0];
    static constexpr unsigned int size_of_fn = sizeof(fn_chars) - 1;


    // -----------------------------------------------------------------------------------
    //
    //                                    FuncBodyNode
    //
    // -----------------------------------------------------------------------------------
    static int selfTextLength_FuncBody(FuncBodyNodeStruct* d) {
        return 0;
    }

    static void copySelfText_FuncBody(FuncBodyNodeStruct *self, utf8byte *buf) {
    }

    /*
    class A {
        fn est() {
            let a = 3
            let b = 5
        }
    }
    
    */
    static CodeLine *appendToLine_FuncBodyNode(FuncBodyNodeStruct *self, CodeLine *currentCodeLine)
    {
        auto *context = self->context;
        IndentRuleApplier indentRuleApplier = IndentRuleApplier::CreateWithBase(context, currentCodeLine);

        // {
        currentCodeLine = TokenVTableCall::callAppendTokenToLine(&self->bodyStartNode, currentCodeLine);
        indentRuleApplier.StartBracket(currentCodeLine);

        // [child nodes]
        auto *child = self->firstChildNode;
        while (child) {
            currentCodeLine = VTableCall::callAppendNodeToLine(child, currentCodeLine);
            child = child->nextNode;
        }

        // }
        currentCodeLine = TokenVTableCall::callAppendTokenToLine(&self->endBodyNode, currentCodeLine);
        indentRuleApplier.FinishAfterEndBracket(currentCodeLine);
        //printf("B depth = %d\n", currentCodeLine->depth);
        return currentCodeLine;
    }


    static constexpr const char bodyTypeText[] = "<body>";


    static int BodyNodeStruct_applyFuncToDescendants(FuncBodyNodeStruct *node, ApplyFunc_params3)
    {
        if (parentIsFirst) {
            if (targetVTable == nullptr || node->vtable == targetVTable) {
                func(Cast::upcast(node), ApplyFunc_pass);
            }
        }

        auto *child = node->firstChildNode;
        while (child) {
            child->vtable->applyFuncToDescendants(
                    Cast::upcast(child), ApplyFunc_pass2);
            child = child->nextNode;
        }

        if (!parentIsFirst) {
            if (targetVTable == nullptr || node->vtable == targetVTable) {
                func(Cast::upcast(node), ApplyFunc_pass);
            }
        }
        return 0;
    }


    static node_vtable _bodyVTable = CREATE_VTABLE(FuncBodyNodeStruct,
                                                   selfTextLength_FuncBody,
                                                   copySelfText_FuncBody,
                                                   appendToLine_FuncBodyNode,
                                                   BodyNodeStruct_applyFuncToDescendants,
                                                   bodyTypeText, NodeTypeId::Body);

    const struct node_vtable *VTables::FuncBodyVTable = &_bodyVTable;

    void Init::initFuncBodyNode(FuncBodyNodeStruct *node, ParseContext *context, void *parentNode) {
        INIT_NODE(node, context, parentNode, VTables::FuncBodyVTable);

        node->lastChildNode = nullptr;
        node->firstChildNode = nullptr;
        node->childCount = 0;
        node->startFound = false;
        node->firstStatementFound = false;

        Init::initSymbolToken(&node->bodyStartNode, context, node, '{');
        Init::initSymbolToken(&node->endBodyNode, context, node, '}');
        node->endBodyNode.isEndFlag = true;
    }


    static void appendChildNode(FuncBodyNodeStruct *body, NodeBase *node) {
        if (body->firstChildNode == nullptr) {
            body->firstChildNode = node;
        }
        if (body->lastChildNode != nullptr) {
            body->lastChildNode->nextNode = node;
        }
        body->lastChildNode = node;
        body->childCount++;
    }

    static int inner_bodyTokenizerLoop(TokenizerParams_argNode_ch_start_context)
    {
        auto *body = Cast::downcast<FuncBodyNodeStruct *>(argNode);
        if (ch == '}') {
            body->endBodyNode.foundPos = start;
            context->scanEnd = true;
            context->mostLeftToken = Cast::upcastToken(&body->endBodyNode);
            return start + 1;
        }

        
        if (!body->firstStatementFound || context->isAfterLineBreak) {
            body->firstStatementFound = true;
            int nextPos;
            // value as a statement
            if (Search::IsTokenized(nextPos = Tokenizers::returnStatementTokenizer(TokenizerParams_pass))) {
                appendChildNode(body, context->generatedPrimaryNode);
                return nextPos;
            }
            else if (Search::IsTokenized(nextPos = Tokenizers::assignStatementTokenizer(TokenizerParams_pass))) {
                appendChildNode(body, context->generatedPrimaryNode);
                return nextPos;
            }
            else if (Search::IsTokenized(nextPos = Tokenizers::assignStatementWithoutTypeTokenizer(TokenizerParams_pass))) {
                appendChildNode(body, context->generatedPrimaryNode);
                return nextPos;
            }
            else if (Search::IsTokenized(nextPos = Tokenizers::tokenizeExpressionFirstAssignStatement(TokenizerParams_pass))) {
                appendChildNode(body, context->generatedPrimaryNode);
                return nextPos;
            }
        } else {
            context->setError(ErrorIndex::should_break_line, start);
        }

        context->setError(ErrorIndex::syntax_error2, start);
        context->scanEnd = true;
        return Search::NOTFOUND;
    }

    int Tokenizers::bodyTokenizer(TokenizerParams_argNode_ch_start_context) {
        auto *bodyNode = Cast::downcast<FuncBodyNodeStruct *>(argNode);


        if (ch == '{') {
            int result = Scanner::scanLoop(bodyNode, inner_bodyTokenizerLoop, context, start + 1);
            if (Search::IsTokenized(result)) {
                bodyNode->bodyStartNode.foundPos = start;
                context->mostLeftToken = Cast::upcastToken(&bodyNode->bodyStartNode);
                return result;
            }
        }
        else {
            context->setError(ErrorIndex::expect_bracket_for_fn_body, context->lastTokenizedPos);
        }
        return Search::NOTFOUND;
    };







    // ----------------------------------------------------------------------------------------
    //
    //                               FuncParameterItemStruct
    //
    // ----------------------------------------------------------------------------------------
    enum FuncParamParsePhase {
        EXPECT_Type = 0,
        EXPECT_COMMA2 = 3
    };

    static CodeLine *appendToLine_FuncParameterItemStruct(FuncParameterItemStruct *self, CodeLine *currentCodeLine) {

        if (self->assignStatementNodeStruct != nullptr) {
            currentCodeLine = VTableCall::callAppendNodeToLine(self->assignStatementNodeStruct, currentCodeLine);
        }

        if (self->hasComma) {
            currentCodeLine = TokenVTableCall::callAppendTokenToLine(&self->followingComma, currentCodeLine);
        }

        return currentCodeLine;
    }

    // --------------------- Implements ClassNode Parser ----------------------
    static void appendChildParameterNode(FuncDefNodeStruct *fnNode, FuncParameterItemStruct *node) {
        if (fnNode->firstChildParameterNode == nullptr) {
            fnNode->firstChildParameterNode = node;
        }
        if (fnNode->lastChildParameterNode != nullptr) {
            fnNode->lastChildParameterNode->nextNode = Cast::upcast(node);
        }
        fnNode->lastChildParameterNode = node;
        fnNode->parameterChildCount++;
    }


    static inline int parseNextValue(TokenizerParams_argNode_ch_start_context, FuncDefNodeStruct* funcNode)
    {
        NodeBase *parent = Cast::upcast(argNode);
        auto *nextParam = Alloc::newFuncParameterItem(context, parent);
        int result;
        if (Search::IsTokenized(result = Tokenizers::assignStatementTokenizer(Cast::upcast(nextParam), ch, start, context))) {
            nextParam->assignStatementNodeStruct = Cast::downcast<AssignStatementNodeStruct *>(context->generatedPrimaryNode);
            appendChildParameterNode(funcNode, nextParam);

            funcNode->parameterParsePhase = FuncParamParsePhase::EXPECT_COMMA2;
            return result;
        }
        return Search::NOTFOUND;
    }

    // This loop is responsible for parsing function parameters. It will keep parsing until it finds the closing parenthesis of the parameter list.
    static int internal_parameterListTokenizerLoop(TokenizerParams_argNode_ch_start_context) {
        NodeBase *parent = Cast::upcast(argNode);
        auto *funcNode = Cast::downcast<FuncDefNodeStruct *>(parent);

        if (ch == ')') {
            context->scanEnd = true;
            funcNode->parameterEndNode.foundPos = start;
            context->mostLeftToken = Cast::upcastToken(&funcNode->parameterEndNode);
            return start + 1;
        }

        if (funcNode->parameterParsePhase == FuncParamParsePhase::EXPECT_Type) {
            return parseNextValue(TokenizerParams_pass, funcNode);
        }

        auto *currentKeyValueItem = funcNode->lastChildParameterNode;

        if (funcNode->parameterParsePhase == FuncParamParsePhase::EXPECT_COMMA2) {
            if (ch == ',') { // try to find ',' which leads to next key-value
                currentKeyValueItem->hasComma = true;
                currentKeyValueItem->followingComma.foundPos = start;
                context->mostLeftToken = Cast::upcastToken(&currentKeyValueItem->followingComma);
                funcNode->parameterParsePhase = FuncParamParsePhase::EXPECT_Type;
                return start + 1;
            }
            else if (context->isAfterLineBreak) {
                // comma is not needed after a line break
                return parseNextValue(TokenizerParams_pass, funcNode);
            }
            return Search::NOTFOUND;
        }

        return Search::NOTFOUND;
    }

    static void copySelfText_FuncParameterItemStruct(FuncParameterItemStruct *self, utf8byte *buf) {
        return;
    }

    static int selfTextLength_FuncParameterItemStruct(FuncParameterItemStruct *) {
        return 0;
    }

    static int FuncParameterItemStruct_applyFuncToDescendants(
            FuncParameterItemStruct *node, ApplyFunc_params3)
    {
        if (targetVTable == nullptr || node->vtable == targetVTable) {
            func(Cast::upcast(node), ApplyFunc_pass);
        }
        return 0;
    }



    static node_vtable _funcParameterItemVTable = CREATE_VTABLE(FuncParameterItemStruct,
                                                                selfTextLength_FuncParameterItemStruct,
                                                                copySelfText_FuncParameterItemStruct,
                                                                appendToLine_FuncParameterItemStruct,
                                                                FuncParameterItemStruct_applyFuncToDescendants,
                                                                "<FuncParameterItem>",
                                                                NodeTypeId::FuncParameter);

    const struct node_vtable *VTables::FuncParameterVTable = &_funcParameterItemVTable;


    FuncParameterItemStruct *Alloc::newFuncParameterItem(ParseContext *context, NodeBase *parentNode) {
        auto *funcParameterItem = context->newMem<FuncParameterItemStruct>();

        INIT_NODE(funcParameterItem, context, parentNode, &_funcParameterItemVTable);

        Init::initSymbolToken(&funcParameterItem->followingComma, context, funcParameterItem, ',');

        funcParameterItem->hasComma = false;
        funcParameterItem->nextNode = nullptr;
        funcParameterItem->assignStatementNodeStruct = nullptr;

        return funcParameterItem;
    }







    //=======================================================================================
    //
    //                                    FuncDefNodeStruct
    //
    //=======================================================================================
    static int selfTextLength(FuncDefNodeStruct *) {
        return 0;
    }

    static void copySelfText(FuncDefNodeStruct *self, utf8byte *buf)
    {
    }

    // fn funcName ( [parameters] ) funcBody
    static CodeLine *appendToLine(FuncDefNodeStruct *self, CodeLine *currentCodeLine)
    {
        ParseContext *context = self->context;
        CodeLine *firstLine = currentCodeLine;
        context->baseCodeLine = currentCodeLine; // referred by body node
        context->baseindentDepth = context->currentIndentDepth;
        context->baseIncrementMode = context->incrementDepthOnNextLine;

        IndentRuleApplier indentRuleApplier = IndentRuleApplier::Create(context, currentCodeLine);

        // fn
        currentCodeLine = TokenVTableCall::callAppendTokenToLine (&self->fnKeywordToken, currentCodeLine);

        // funcName
        self->context->incrementDepthOnNextLine = false;
        currentCodeLine = TokenVTableCall::callAppendTokenToLine(&self->funcNameToken, currentCodeLine);

        // (
        currentCodeLine = TokenVTableCall::callAppendTokenToLine(&self->parameterStartNode, currentCodeLine);
        indentRuleApplier.StartBracket(currentCodeLine);

        // [parameters]
        auto *item = self->firstChildParameterNode;
        while (item != nullptr) {
            currentCodeLine = VTableCall::callAppendNodeToLine(item, currentCodeLine);
            item = Cast::downcast<FuncParameterItemStruct *>(item->nextNode);
        }

        // )
        currentCodeLine = TokenVTableCall::callAppendTokenToLine(&self->parameterEndNode, currentCodeLine);
        indentRuleApplier.FinishAfterEndBracket(currentCodeLine);   

        // funcBody
        return VTableCall::callAppendNodeToLine(&self->bodyNode, currentCodeLine);
    }


    static int FuncNodeStruct_applyFuncToDescendants(FuncDefNodeStruct *node, ApplyFunc_params3)
    {
        if (parentIsFirst) {
            if (targetVTable == nullptr || node->vtable == targetVTable) {
                func(Cast::upcast(node), ApplyFunc_pass);
            }
        }
        node->bodyNode.vtable->applyFuncToDescendants(
                reinterpret_cast<NodeBase *>(&node->bodyNode),
                ApplyFunc_pass2);

        if (!parentIsFirst) {
            if (targetVTable == nullptr || node->vtable == targetVTable) {
                func(Cast::upcast(node), ApplyFunc_pass);
            }
        }
        return 0;
    }


    static constexpr const char fnTypeText[] = "<fn>";

    static node_vtable _fnVTable = CREATE_VTABLE(FuncDefNodeStruct,
                                                 selfTextLength,
                                                 copySelfText,
                                                 appendToLine,
                                                 FuncNodeStruct_applyFuncToDescendants,
                                                 fnTypeText,
                                                 NodeTypeId::Func);

    const struct node_vtable *VTables::FuncDefVTable = &_fnVTable;

    FuncDefNodeStruct* Alloc::newFuncNode(ParseContext *context, NodeBase *parentNode)
    {
        auto *funcNode = context->newMem<FuncDefNodeStruct>();

        INIT_NODE(funcNode, context, parentNode, &_fnVTable);

        funcNode->parameterParsePhase = FuncParamParsePhase::EXPECT_Type;
        funcNode->lastChildParameterNode = nullptr;
        funcNode->firstChildParameterNode = nullptr;

        Init::initSimpleTextToken(&funcNode->fnKeywordToken, context, funcNode, size_of_fn);
        Init::assignText_SimpleTextToken(&funcNode->fnKeywordToken, context, fn_chars, size_of_fn);

        Init::initIdentifierToken(&funcNode->funcNameToken, context, funcNode);

        Init::initSymbolToken(&funcNode->parameterStartNode, context, funcNode, '(');
        Init::initSymbolToken(&funcNode->parameterEndNode, context, funcNode, ')');
        funcNode->parameterEndNode.isEndFlag = true;

        Init::initFuncBodyNode(&funcNode->bodyNode, context, funcNode);

        return funcNode;
    }



    // tokenizer for function declaration, function name is already parsed by the caller,
    // this tokenizer is responsible for parsing function parameters and function body.
    static int inner_fnParamsAndBodyTokenizer(TokenizerParams_argNode_ch_start_context) {
        auto *fnNode = Cast::downcast<FuncDefNodeStruct *>(argNode);

        if (fnNode->parameterStartNode.foundPos == -1) {
            if (ch == '(') {
                fnNode->parameterStartNode.foundPos = start;
                int nextPos =  start + 1;
                // parse parameters
                int result = Scanner::scanLoop(fnNode, internal_parameterListTokenizerLoop, context, nextPos);
                if (Search::IsTokenized(result)) {
                    // parse body
                    int result2 = Scanner::scanOnce(Cast::upcast(&fnNode->bodyNode), Tokenizers::bodyTokenizer, context, result);
                    if (Search::IsTokenized(result2)) {
                        context->mostLeftToken = Cast::upcastToken(&fnNode->parameterStartNode);
                        return result2;
                    }
                }
            }
            else {
                context->setError(ErrorIndex::expect_parenthesis_for_fn_params, context->lastTokenizedPos);
            }
        }
        else {
            context->setError(ErrorIndex::expect_parenthesis_for_fn_params, context->lastTokenizedPos);
        }
        return Search::NOTFOUND;
    }


    int Tokenizers::fnTokenizer(TokenizerParams_argNode_ch_start_context) {
        if (fn_first_char != ch) {
            return Search::NOTFOUND;
        }

        // fn
        bool matched = ParseUtil::matchWordWithTerminatableEnd(context->chars, context->length, start, fn_chars);
        if (!matched) {
            return Search::NOTFOUND;
        }


        int currentPos = start + size_of_fn;
        int resultPos = -1;

        auto *parent  = Cast::upcast(argNode);
        // now after "fn "
        auto *fnNode = Alloc::newFuncNode(context, parent);
        fnNode->fnKeywordToken.foundPos = start;

        resultPos = Scanner::scanOnce(&fnNode->funcNameToken, Tokenizers::identifierTokenizer, context, currentPos);
        if (!Search::IsTokenized(resultPos)) {
            // fn should have a function name
            context->setError(ErrorIndex::invalid_fn_name, start);
            context->generatedPrimaryNode = Cast::upcast(fnNode);
            return currentPos;
        }

        // Parse body
        currentPos = resultPos;
        resultPos = Scanner::scanOnce(fnNode, inner_fnParamsAndBodyTokenizer, context, currentPos);
        if (Search::IsTokenized(resultPos)) {
            context->generatedPrimaryNode = Cast::upcast(fnNode);
            context->mostLeftToken = Cast::upcastToken(&fnNode->fnKeywordToken);

            return resultPos;
        }
        else {
            context->setError(ErrorIndex::syntax_error, context->lastTokenizedPos);
            context->generatedPrimaryNode = Cast::upcast(fnNode);
            return currentPos;
        }
    }
}