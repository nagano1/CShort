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

#include "parser.hpp"


namespace cshort {

    // Assignment or variable declaration statement. It can be with or without type declaration.
    // e.g. a = 3; // assignment without type declaration
    //      int a = 3; // assignment with type declaration
    //      int a; // variable declaration without assignment
    // this struct is also used in method parameters, e.g. fn func(int a, string *b) { ... }.


    static int selfTextLength(AssignmentNodeStruct *)
    {
        return 0;
    }

    static void copySelfText(AssignmentNodeStruct *self, utf8byte *buf)
    {

    }


    static CodeLine *appendToCodeLine(AssignmentNodeStruct *self, CodeLine *currentCodeLine)
    {
        if (self->isExpressionFirstSyntax) {
            // In expression-first assignment, adding same tokens but the order is different.
            // for example:
            // 254 + 5123 - func()
            // =int varName
            int depth = self->context->currentIndentDepth;
            
            assert(self->hasTypeOrLet);
            assert(self->expressionNode);
            assert(self->equalSymbol.foundPos > -1);

            currentCodeLine = VTableCall::callAppendNodeToLine(self->expressionNode, currentCodeLine);

            self->context->currentIndentDepth = depth;

            currentCodeLine = TokenVTableCall::callAppendTokenToLine(&self->equalSymbol, currentCodeLine);
            currentCodeLine = VTableCall::callAppendNodeToLine(&self->typeOrLet, currentCodeLine);

            self->context->IncrementIndentDepth(self->context);
            currentCodeLine = TokenVTableCall::callAppendTokenToLine(&self->variableNameToken, currentCodeLine);

            self->context->currentIndentDepth = depth;

            return currentCodeLine;
        }
        else {

            // normal mode . e.g.
            // int varName = 254
            if (self->hasTypeOrLet) {
                currentCodeLine = VTableCall::callAppendNodeToLine(&self->typeOrLet, currentCodeLine);
            }

            int previousIndentDepth = self->context->currentIndentDepth;
            bool prevDepthIncrementMode = self->context->incrementDepthOnNextLine;
            
            currentCodeLine = TokenVTableCall::callAppendTokenToLine(&self->variableNameToken, currentCodeLine);

            if (self->equalSymbol.foundPos > -1) {
                self->context->incrementDepthOnNextLine = true;
                currentCodeLine = TokenVTableCall::callAppendTokenToLine(&self->equalSymbol, currentCodeLine);

                assert(self->expressionNode); // if equal symbol exists, expression node must exist, otherwise tokenizer throws syntax errors.
                self->context->incrementDepthOnNextLine = true;
                currentCodeLine = VTableCall::callAppendNodeToLine(self->expressionNode, currentCodeLine);
            }

            self->context->incrementDepthOnNextLine = prevDepthIncrementMode;
            self->context->currentIndentDepth = previousIndentDepth;
            return currentCodeLine;
        }
    }


    static constexpr const char assignTypeText[] = "<Assignment>";

    static int applyFuncToDescendants(AssignmentNodeStruct *node, ApplyFunc_params3)
    {
        if (parentIsFirst) {
            if (targetVTable == nullptr || node->vtable == targetVTable) {
                func(Cast::upcast(node), ApplyFunc_pass);
            }
        }

        if (node->expressionNode) {
            node->expressionNode->vtable->applyFuncToDescendants(node->expressionNode, ApplyFunc_pass2);
        }

        if (!parentIsFirst) {
            if (targetVTable == nullptr || node->vtable == targetVTable) {
                func(Cast::upcast(node), ApplyFunc_pass);
            }
        }

        return 0;
    }

    static node_vtable _assignVTable = CREATE_VTABLE(AssignmentNodeStruct,
                                                     selfTextLength,
                                                     copySelfText,
                                                     appendToCodeLine, applyFuncToDescendants,
                                                     assignTypeText,
                                                     NodeTypeId::Assignment);

    const struct node_vtable *VTables::AssignmentVTable = &_assignVTable;


    // -------------------- Implements Assignment Allocator --------------------- //
    AssignmentNodeStruct *Alloc::newAssignment(ParseContext *context, NodeBase *parentNode) {
        auto *assignment = context->newMem<AssignmentNodeStruct>();
        Init::initAssignment(context, parentNode,  assignment);
        return assignment;
    }

    void Init::initAssignment(ParseContext *context, NodeBase *parentNode, AssignmentNodeStruct *assignment) {
        INIT_NODE(assignment, context, parentNode, &_assignVTable);

        assignment->hasTypeOrLet = false;
        assignment->expressionNode = nullptr;
        assignment->stackOffset = 0;
        assignment->isExpressionFirstSyntax = false;


        Init::initIdentifierToken(&assignment->variableNameToken, context, assignment);
        Init::initSymbolToken(&assignment->equalSymbol, context, assignment, '=');
        Init::initTypeNode(&assignment->typeOrLet, context, assignment);
    }


    /// tokenizer for assignment statement without let keyword. e.g. a = 3
    static int tokenizeAssignmentLoop(TokenizerParams_argNode_ch_start_context) {
        auto *assignment = Cast::downcast<AssignmentNodeStruct *>(argNode);

        if (assignment->variableNameToken.foundPos == -1) {
            // if type is declared, it can be just declaration without assignment. e.g. int a 
            int result = Tokenizers::identifierTokenizer(Cast::upcastToken(&assignment->variableNameToken), ch, start, context);
            if (Search::IsTokenized(result)) {
                context->mostLeftToken = Cast::upcastToken(&assignment->variableNameToken);
                return result;
            }
            else {
                // no name found. not found as assignment statement.
                return Search::NOTFOUND;
            }
        }
        else if (assignment->equalSymbol.foundPos == -1) {
            if (ch == '=') {
                assignment->equalSymbol.foundPos = start;
                context->mostLeftToken = Cast::upcastToken(&assignment->equalSymbol);
                return start + 1;
            }
            else {
                if (assignment->hasTypeOrLet) {
                    // if has type declaration, it can be just declaration without assignment. e.g. int a
                    context->scanEnd = true;
                    return Search::DONE_WITH_PREVIOUS_POSITION;
                }
                else {
                    // no equal symbol found. not found as assignment statement.
                    return Search::NOTFOUND;
                }
            }
        }
        else { // already has name and equal symbol. now should be expression node.
            int result = Tokenizers::tokenizeExpression(Cast::upcast(assignment), ch, start, context);
            if (Search::IsTokenized(result)) {
                assignment->expressionNode = context->generatedPrimaryNode;
                context->scanEnd = true;
                return result;
            }
            else {
                // no expression node found after equal symbol. invalid syntax. stop scanning and report error.
                context->scanEnd = true;
                context->setError(ErrorIndex::syntax_error, start);
                return Search::NOTFOUND;
            }
        }

        return Search::NOTFOUND;
    }



    // b = 32
    int Tokenizers::assignmentWithoutTypeTokenizer(TokenizerParams_argNode_ch_start_context)
    {
        AssignmentNodeStruct *assignment;
        auto *parent = Cast::upcast(argNode);

        if (context->unusedAssignment == nullptr) {
            assignment = Alloc::newAssignment(context, parent);
        }
        else {
            assignment = context->unusedAssignment;
            Init::initAssignment(context, parent, assignment);
            context->unusedAssignment = nullptr;
        }

        int resultPos = Scanner::scanLoop(assignment, tokenizeAssignmentLoop, context, start);
        if (Search::IsTokenized(resultPos)) {
            assignment->hasTypeOrLet = false;
            assignment->typeOrLet.isLet = false;

            context->mostLeftToken = Cast::upcastToken(&assignment->variableNameToken);
            context->generatedPrimaryNode = Cast::upcast(assignment);

            return resultPos;
        }

        context->unusedAssignment = assignment;

        return Search::NOTFOUND;
    }


    static int tokenizeEqualSymbolAtLineStart(TokenizerParams_argNode_ch_start_context)
    {
        // = symbol must be appeared first on the next line
        if (ch == '=' && context->isAfterLineBreak) {
            SymbolTokenStruct *equalSymbol = Cast::downcast<SymbolTokenStruct *>(argNode);
            equalSymbol->foundPos = start;
            context->mostLeftToken = Cast::upcastToken(equalSymbol);
            return start + 1;
        }

        return Search::NOTFOUND;
    }
    

    // This language supports expression-first syntax for assignment to decrease line length
    // ```
    // 1423 + 442 + func()
    // =let longVariableName
    // ```
    // this is equal to ```let longVariableName = 1423 + 442 + func()```
    int Tokenizers::tokenizeExpressionFirstAssignment(TokenizerParams_argNode_ch_start_context)
    {
        auto *parent = Cast::upcast(argNode);

        TokenBase *mostLeftToken;
        // Tokinize expression first
        int nextPos = 0;
        if (!Search::IsTokenized(nextPos = Tokenizers::tokenizeExpression(TokenizerParams_pass)))
        {
            return Search::NOTFOUND;
        }

        mostLeftToken = context->mostLeftToken;

        // = symbol must be appeared first on the next line
        SymbolTokenStruct equalSymbol;
        Init::initSymbolToken(&equalSymbol, context, nullptr, '=');
        nextPos = Scanner::scanOnce(&equalSymbol, tokenizeEqualSymbolAtLineStart, context, nextPos);
        if (!Search::IsTokenized(nextPos)) {
            return Search::NOTFOUND;
        }

        AssignmentNodeStruct *assignment;
        NodeBase *expressionNode = context->generatedPrimaryNode;

        if (context->unusedAssignment == nullptr) {
            assignment = Alloc::newAssignment(context, parent);
        }
        else {
            assignment = context->unusedAssignment;
            Init::initAssignment(context, parent, assignment);
            context->unusedAssignment = nullptr;
        }


        assignment->equalSymbol = equalSymbol;
        assignment->equalSymbol.parentNode = Cast::upcast(assignment);
        assignment->isExpressionFirstSyntax = true;
        assignment->expressionNode = expressionNode;

        // Type name must be right after = symbol without spaces, no line break or comment allowed in between
        nextPos = Tokenizers::typeTokenizer(&assignment->typeOrLet, ch, nextPos, context);
        if (!Search::IsTokenized(nextPos)) {
            context->unusedAssignment = assignment;
            return Search::NOTFOUND;
        }

        assignment->hasTypeOrLet = true;

        // spaces and comments are allowed between type declaration and variable name,
        // so we need to use scanOnce until we find the variable name token.
        nextPos = Scanner::scanOnce(&assignment->variableNameToken, Tokenizers::identifierTokenizer, context, nextPos);
        if (Search::IsTokenized(nextPos)) {
            context->mostLeftToken = mostLeftToken;
            context->generatedPrimaryNode = Cast::upcast(assignment);
            return nextPos;
        }

        // if no variable name found, it can be just type declaration without assignment, e.g.
        assignment->isExpressionFirstSyntax = false;
        context->unusedAssignment = assignment;
        return Search::NOTFOUND;
    }
    // let a = 3
    // int m = 5
    // int a
    // ?string str = null
    int Tokenizers::tokenizeAssignment(TokenizerParams_argNode_ch_start_context)
    {
        NodeBase *parent = Cast::upcast(argNode);
        AssignmentNodeStruct *assignment;
        if (context->unusedAssignment == nullptr) {
            assignment = Alloc::newAssignment(context, parent);
        }
        else {
            assignment = context->unusedAssignment;
            Init::initAssignment(context, parent, assignment);
            context->unusedAssignment = nullptr;
        }

        int result = Tokenizers::typeTokenizer(Cast::upcast(&assignment->typeOrLet), ch, start, context);
        if (Search::IsTokenized(result)) {
            assignment->hasTypeOrLet = true;

            int resultPos;
            if (Search::IsTokenized(resultPos = Scanner::scanLoop(assignment, tokenizeAssignmentLoop, context, result)))
            {
                context->mostLeftToken = Cast::upcastToken(&assignment->typeOrLet.typeTextToken);
                context->generatedPrimaryNode = Cast::upcast(assignment);

                return resultPos;
            }
        }

        context->unusedAssignment = assignment;
        return Search::NOTFOUND;
    }
}