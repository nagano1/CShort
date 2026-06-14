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

    // Assignment or variable declaration statement. It can be with or without type declaration.
    // e.g. a = 3; // assignment without type declaration
    //      int a = 3; // assignment with type declaration
    //      int a; // variable declaration without assignment
    // this struct is also used in method parameters, e.g. fn func(int a, string *b) { ... }.


    static int selfTextLength(AssignStatementNodeStruct *)
    {
        return 0;
    }

    static void copySelfText(AssignStatementNodeStruct *self, utf8byte *buf)
    {

    }


    static CodeLine *appendToCodeLine(AssignStatementNodeStruct *self, CodeLine *currentCodeLine)
    {
        if (self->isMultiLineAssign) {
            // In multi-line assign, adding same tokens, but the order is different.
            // for example:
            // 254
            // =int varName
            int depth = self->context->currentIndentDepth;
            
            assert(self->hasTypeDecl);
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

            // In single-line assign, the order is like normal assignment statement. e.g.
            // int varName = 254
            if (self->hasTypeDecl) {
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


    static constexpr const char assignTypeText[] = "<AssignStatement>";

    static int applyFuncToDescendants(AssignStatementNodeStruct *node, ApplyFunc_params3)
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

    static node_vtable _assignVTable = CREATE_VTABLE(AssignStatementNodeStruct,
                                                     selfTextLength,
                                                     copySelfText,
                                                     appendToCodeLine, applyFuncToDescendants,
                                                     assignTypeText,
                                                     NodeTypeId::AssignStatement);

    const struct node_vtable *VTables::AssignStatementVTable = &_assignVTable;


    // -------------------- Implements AssignStatement Allocator --------------------- //
    AssignStatementNodeStruct *Alloc::newAssignStatement(ParseContext *context, NodeBase *parentNode) {
        auto *assignStatement = context->newMem<AssignStatementNodeStruct>();
        Init::initAssignStatement(context, parentNode,  assignStatement);
        return assignStatement;
    }

    void Init::initAssignStatement(ParseContext *context, NodeBase *parentNode, AssignStatementNodeStruct *assignStatement) {
        INIT_NODE(assignStatement, context, parentNode, &_assignVTable);

        assignStatement->hasTypeDecl = false;
        assignStatement->expressionNode = nullptr;
        assignStatement->stackOffset = 0;
        assignStatement->isMultiLineAssign = false;


        Init::initIdentifierToken(&assignStatement->variableNameToken, context, assignStatement);
        Init::initSymbolToken(&assignStatement->equalSymbol, context, assignStatement, '=');
        Init::initTypeNode(&assignStatement->typeOrLet, context, assignStatement);
    }


    /// tokenizer for assignment statement without let keyword. e.g. a = 3
    static int tokenizeAssignStatementLoop(TokenizerParams_argNode_ch_start_context) {
        auto *assignment = Cast::downcast<AssignStatementNodeStruct *>(argNode);

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
                if (assignment->hasTypeDecl) {
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
    int Tokenizers::assignStatementWithoutTypeTokenizer(TokenizerParams_argNode_ch_start_context)
    {
        AssignStatementNodeStruct *assignment;
        auto *parent = Cast::upcast(argNode);

        if (context->unusedAssignment == nullptr) {
            assignment = Alloc::newAssignStatement(context, parent);
        }
        else {
            assignment = context->unusedAssignment;
            Init::initAssignStatement(context, parent, assignment);
            context->unusedAssignment = nullptr;
        }

        int resultPos = Scanner::scanLoop(assignment, tokenizeAssignStatementLoop, context, start);
        if (Search::IsTokenized(resultPos)) {
            assignment->hasTypeDecl = false;
            assignment->typeOrLet.isLet = false;

            context->mostLeftToken = Cast::upcastToken(&assignment->variableNameToken);
            context->generatedPrimaryNode = Cast::upcast(assignment);

            return resultPos;
        }

        context->unusedAssignment = assignment;

        return Search::NOTFOUND;
    }


    static int firstEqualLetterTokenizer(TokenizerParams_argNode_ch_start_context)
    {
        // = symbol must be appeared first on the next line
        if (ch == '=' && context->isAfterLineBreak) {
            SymbolTokenStruct *equalSymbol = Cast::downcast<SymbolTokenStruct *>(argNode);
            Init::initSymbolToken(equalSymbol, context, nullptr, '=');
            equalSymbol->foundPos = start;
            context->mostLeftToken = Cast::upcastToken(equalSymbol);
            return start + 1;
        }

        return Search::NOTFOUND;
    }
    

    // This language supports multiple line syntax for assignment to decrease line length
    // ```
    // 1423 + 442 + func()
    // =let longVariableName
    // ```
    // this is equal to ```let longVariableName = 1423 + 442 + func()```
    int Tokenizers::tokenizeMultipleLineAssignStatement(TokenizerParams_argNode_ch_start_context)
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
        int resultPos = Scanner::scanOnce(&equalSymbol, firstEqualLetterTokenizer, context, nextPos);
        if (!Search::IsTokenized(resultPos)) {
            return Search::NOTFOUND;
        }

        AssignStatementNodeStruct *assignment;
        NodeBase *expressionNode = context->generatedPrimaryNode;

        if (context->unusedAssignment == nullptr) {
            assignment = Alloc::newAssignStatement(context, parent);
        }
        else {
            assignment = context->unusedAssignment;
            Init::initAssignStatement(context, parent, assignment);
            context->unusedAssignment = nullptr;
        }


        assignment->equalSymbol = equalSymbol;
        assignment->equalSymbol.parentNode = Cast::upcast(assignment);
        assignment->isMultiLineAssign = true;
        assignment->expressionNode = expressionNode;

        // Type name must be right after = symbol, no line break or comment allowed in between
        int res = Tokenizers::typeTokenizer(&assignment->typeOrLet, ch, resultPos, context);
        if (!Search::IsTokenized(res)) {
            assignment->isMultiLineAssign = false;
            context->unusedAssignment = assignment;
            return Search::NOTFOUND;
        }

        // spaces and comments are allowed between type declaration and variable name,
        // so we need to use scanOnce until we find the variable name token.
        int resultPos2 = Scanner::scanOnce(&assignment->variableNameToken, Tokenizers::identifierTokenizer, context, res);
        if (Search::IsTokenized(resultPos2)) {
            assignment->hasTypeDecl = true;
            context->mostLeftToken = mostLeftToken;
            context->generatedPrimaryNode = Cast::upcast(assignment);

            return resultPos2;
        }

        // if no variable name found, it can be just type declaration without assignment, e.g.
        assignment->isMultiLineAssign = false;
        context->unusedAssignment = assignment;
        return Search::NOTFOUND;
    }
    // let a = 3
    // int m = 5
    // int a
    // ?string *str = null
    int Tokenizers::assignStatementTokenizer(TokenizerParams_argNode_ch_start_context)
    {
        NodeBase *parent = Cast::upcast(argNode);
        AssignStatementNodeStruct *assignStatement;
        if (context->unusedAssignment == nullptr) {
            assignStatement = Alloc::newAssignStatement(context, parent);
        }
        else {
            assignStatement = context->unusedAssignment;
            Init::initAssignStatement(context, parent, assignStatement);
            context->unusedAssignment = nullptr;
        }

        int result = Tokenizers::typeTokenizer(Cast::upcast(&assignStatement->typeOrLet), ch, start, context);
        if (Search::IsTokenized(result)) {
            assignStatement->hasTypeDecl = true;

            int resultPos;
            if (Search::IsTokenized(resultPos = Scanner::scanLoop(assignStatement, tokenizeAssignStatementLoop, context, result)))
            {
                context->mostLeftToken = Cast::upcastToken(&assignStatement->typeOrLet.typeTextToken);
                context->generatedPrimaryNode = Cast::upcast(assignStatement);

                return resultPos;
            }
        }

        context->unusedAssignment = assignStatement;
        return Search::NOTFOUND;
    }
}