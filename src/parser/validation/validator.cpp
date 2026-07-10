#define _CRT_SECURE_NO_WARNINGS

#include <cstdio>
#include <iostream>
#include <string>
#include <array>
#include <algorithm>
#include <cinttypes>

#include <cstdlib>
#include <cassert>
#include <cstdio>
#include <chrono>
#include <unordered_map>
#include <vector>

#include <cstdint>
#include <ctime>
#include <cstdint>

#include "script_runtime.hpp"
#include "types.hpp"

namespace cshort {

    //------------------------------------------------------------------------------------------
    //
    //                                       Validate Script
    //
    //------------------------------------------------------------------------------------------
    
    static GRPRegisterEnum findUnusedReg1(GRPRegisterEnum reg1) {
        if (reg1 != GRPRegisterEnum::eax) {
            return GRPRegisterEnum::eax;
        }
        else  {
            return GRPRegisterEnum::ebx;
        }
    }

    static GRPRegisterEnum findUnusedReg2(GRPRegisterEnum reg1, GRPRegisterEnum reg2) {
        if (reg1 != GRPRegisterEnum::eax && reg2 != GRPRegisterEnum::eax) {
            return GRPRegisterEnum::eax;
        }
        else if (reg1 != GRPRegisterEnum::ebx && reg2 != GRPRegisterEnum::ebx) {
            return GRPRegisterEnum::ebx;
        }
        else if (reg1 != GRPRegisterEnum::ecx && reg2 != GRPRegisterEnum::ecx) {
            return GRPRegisterEnum::ecx;
        }

        //assert(reg1 != GRPRegisterEnum::edx && reg2 != GRPRegisterEnum::edx);
        return GRPRegisterEnum::edx;
    }

    static void assignCalcRegToNode(NodeBase *node, const ParseContext *context)
    {
        int typeIndex = node->typeIndex;
        if (typeIndex == -1) {
            typeIndex = BuiltInTypeIndex::int64;
        }

        auto *typeManager = context->typeManager;
        auto *typeEntry = typeManager->getTypeEntryByIndex(typeIndex);
        int dataSize = typeEntry->getStackSizeForType();

        const GPRRegister *calcRegister = GetGPRRegisterByEnum(node->calcRegEnum, &context->scriptEngineContext->cpuRegister);
        if (calcRegister != nullptr) {
            st_byte* dataPointer = GetDataPointerFromGPRRegister(calcRegister, dataSize);
            node->calcReg = dataPointer;
        }
    }

    // assigns calculation registers to the left and right expression nodes of a binary operation node, as well as to the value node of a parentheses node, and to the expression node of an assignment or return statement node.
    // parent comes first

    static int applyFunc_assignCalcOpRegister(NodeBase *node, ApplyFunc_params2)
    {
        //auto *context = (ParseContext *)context;

        if (node->vtable == VTables::BinaryOperationVTable) {
            auto *binary = Cast::downcast<BinaryOperationNodeStruct *>(node);

            binary->leftExprNode->calcRegEnum = findUnusedReg1(binary->calcRegEnum);
            assignCalcRegToNode(binary->leftExprNode, context);

            binary->rightExprNode->calcRegEnum = findUnusedReg2(binary->calcRegEnum, binary->leftExprNode->calcRegEnum);
            assignCalcRegToNode(binary->rightExprNode, context);
        }

        if (node->vtable == VTables::ParenthesesVTable) {
            auto *parentheses = Cast::downcast<ParenthesesNodeStruct *>(node);

            assert(parentheses->valueNode != nullptr);
            parentheses->valueNode->calcRegEnum = parentheses->calcRegEnum;
            assignCalcRegToNode(parentheses->valueNode, context);
        }

        if (node->vtable == VTables::AssignmentVTable) {
            auto *assign = Cast::downcast<AssignmentNodeStruct *>(node);
            if (assign->expressionNode != nullptr) {
                assignCalcRegToNode(assign->expressionNode, context);
            }
        }

        if (node->vtable == VTables::ReturnStatementVTable) {
            auto *returnState = Cast::downcast<ReturnStatementNodeStruct *>(node);

            if (returnState->expressionNode) {
                assignCalcRegToNode(returnState->expressionNode, context);
            }
        }

        return 0;
    }


    static void assignCalcOpRegister(ParseContext *context,
                                     FuncDefNodeStruct *func)
    {
        func->bodyNode.vtable->applyFuncToDescendants(Cast::upcast(&func->bodyNode),
                                                         context,
                                                         nullptr,
                                                         applyFunc_assignCalcOpRegister,
                                                         /*parent first*/true,
                                                         nullptr,
                                                         nullptr);
    }

    // ----------------------------------------------------------------
    //
    //                       Variable Block Chain
    //
    // ----------------------------------------------------------------

    struct VariableBlock {
        VoidHashMap *variableMap;
        VariableBlock *prev;
        void initializeVariableMap(ParseContext *context) {
            this->variableMap = context->memBufferForValidation.newMem<VoidHashMap>(1);
            this->variableMap->init(&context->memBufferForValidation);
        }

        void addAssignment(AssignmentNodeStruct *assign) {
            this->variableMap->put(assign->variableNameToken.name,
                                   assign->variableNameToken.nameLength, assign);
        }
    };

    // This structure represents a chain of variable blocks, where each block contains a mapping of variable names to their corresponding assignment nodes. It allows for efficient management of variable scopes and declarations within a function body.
    struct LocalVariableChain
    {
        VariableBlock *firstVariableBlock;
        VariableBlock *lastVariableBlock;

        AssignmentNodeStruct *findDeclareStatement(const char *name, int nameLength) {
            VariableBlock *block = this->lastVariableBlock;
            while (block != nullptr) {
                if (block->variableMap->hasKey(name, nameLength)) {
                    auto *assign = (AssignmentNodeStruct *) block->variableMap->get(name, nameLength);
                    return assign;
                }
                block = block->prev;
            }
            return nullptr;
        }

        VariableBlock *appendVariableBlock(ParseContext *context) {
            auto *newBlock = context->memBufferForValidation.newMem<VariableBlock>(1);
            newBlock->initializeVariableMap(context);
            newBlock->prev = this->lastVariableBlock;
            this->lastVariableBlock = newBlock;
            if (this->firstVariableBlock == nullptr) {
                this->firstVariableBlock = newBlock;
            }
            return newBlock;
        }

        void addToCurrentBlock(AssignmentNodeStruct *assign, ParseContext *context) {
            if (this->lastVariableBlock == nullptr) {
                this->appendVariableBlock(context);
            }

            if (this->lastVariableBlock != nullptr) {
                this->lastVariableBlock->addAssignment(assign);
            }
        }

        void deleteLastVariableBlock() {
            auto *lastVariableBlock = this->lastVariableBlock;
            if (lastVariableBlock != nullptr) {
                this->lastVariableBlock = lastVariableBlock->prev;
                if (this->lastVariableBlock == nullptr) { // if the last block is deleted, set firstVariableBlock to nullptr as well
                    assert(this->firstVariableBlock == lastVariableBlock);
                    this->firstVariableBlock = nullptr;
                }
            }
        }
    };

    //------------------------------------------------------------------------------------------
    //
    //                                       Validate Code
    //
    //------------------------------------------------------------------------------------------

    // validates an assignment node by checking type declarations, type compatibility, and other constraints.
    void validateAssignmentAndExpression(AssignmentNodeStruct *declarationStatement,
                                         AssignmentNodeStruct *assignment,
                                         //TypeEntry *declaredType,
                                         ParseContext *context)
    {
        //assert(declaredType != nullptr);
        assert(assignment->expressionNode != nullptr);

        int declaredTypeIndex = declarationStatement->typeIndex;
        if (!TypeManager::isValidTypeIndex(declaredTypeIndex)) {
            // Type selection should have already reported an error for this assignment.
            return;
        }
        TypeEntry *declaredType = context->typeManager->getTypeEntryByIndex(declaredTypeIndex);
        if (declaredType == nullptr) {
            // Type selection should have already reported an error for this assignment.
            return;
        }

        int childTypeIndex = assignment->expressionNode->typeIndex;
        if (declaredType->typeIndex != childTypeIndex) {
            if (childTypeIndex == BuiltInTypeIndex::null){
                if (!declarationStatement->typeOrLet.hasNullableMark) {
                    context->addErrorWithNode(ErrorIndex::assign_null_to_unnullable, declarationStatement);
                }
            }
            else {
                // check assignable
                auto *targetTypeEntry = context->typeManager->getTypeEntryByIndex(childTypeIndex);
                bool canAssign = declaredType->canAssignTypeImplicitly(context, targetTypeEntry);
                if (!canAssign) {
                    context->addErrorWithNode(ErrorIndex::type_is_not_assignable, assignment);
                }
            }
        }
    }

    // This function validates an assignment node by checking type declarations, type compatibility,
    // and other constraints.
    static void validateDeclaredTypeAssignmentNode(AssignmentNodeStruct *assign,
                                                   void *topLevelNodeInBody,
                                                   ParseContext *context)
    {
        TypeEntry *declaredType = nullptr;
        if (assign->hasTypeDecl()) {
            declaredType = context->typeManager->getTypeEntryByName(
                assign->typeOrLet.nameNode.name,
                assign->typeOrLet.nameNode.nameLength
            );
            if (declaredType != nullptr) {
                assert(TypeManager::isValidTypeIndex(declaredType->typeIndex));
                assign->typeIndex = declaredType->typeIndex;
            }
            else {
                // error: no type found
                context->addErrorWithNode(ErrorIndex::type_not_found, &assign->typeOrLet);
            }
        }

        // check if the variable name is already defined in the current scope
        if (assign->hasTypeOrLet) {
            auto *currentStatement = Cast::downcast<NodeBase*>(topLevelNodeInBody);
            auto *bodyNode = Cast::downcast<FuncBodyNodeStruct *>(currentStatement->parentNode);
            auto *existingAssignment = bodyNode->localVariableChain->findDeclareStatement(
                assign->variableNameToken.name,
                assign->variableNameToken.nameLength
            );

            if (existingAssignment != nullptr) {
                // error: variable already defined in the current scope
                context->addErrorWithNode(ErrorIndex::variable_name_duplicated, assign);
            }
        }

        if (assign->expressionNode != nullptr) { // int b = 8, let b = 8
            int childTypeIndex = assign->expressionNode->typeIndex;
            if (!TypeManager::isValidTypeIndex(childTypeIndex)) {
                // Child node should have already reported a semantic error (e.g., unknown identifier).
                return;
            }

            if (assign->typeOrLet.isLet) { // let b = 8
                assign->typeIndex = childTypeIndex; // just assign the type index
            }
            else { // int b = 8
                if (declaredType != nullptr) {
                    validateAssignmentAndExpression(assign, assign, context);
                }
            }
        }
        else { // no value: int b, let b
            if (assign->typeOrLet.hasImmutableMark) {
                // #int a
                context->addErrorWithNode(ErrorIndex::cant_put_immutable_mark_for_non_value_assignment, &assign->typeOrLet);
            }

            if (assign->typeOrLet.isLet) { // let b
                context->addErrorWithNode(ErrorIndex::let_without_value, assign);
            }
        }
    }

    // b = 4
    static void validateOnlyAssignmentNode(AssignmentNodeStruct *assign,
                                           void *topLevelNodeInBody,
                                           ParseContext *context
    ) {
        auto *currentStatement = Cast::downcast<NodeBase*>(topLevelNodeInBody);
        auto *bodyNode = Cast::downcast<FuncBodyNodeStruct *>(currentStatement->parentNode);
        assert(bodyNode->vtable == VTables::FuncBodyVTable);
         if (assign->expressionNode == nullptr || !TypeManager::isValidTypeIndex(assign->expressionNode->typeIndex)) {
             // Type selection should have already reported an error for this expression.
             return;
         }


        auto *varDeclarationStatement = bodyNode->localVariableChain->findDeclareStatement(
            assign->variableNameToken.name,
            assign->variableNameToken.nameLength);

        if (varDeclarationStatement == nullptr) {
            // error: no decl found
            context->addErrorWithNode(ErrorIndex::no_variable_defined, assign);
            return;
        }

        assign->stackOffset = varDeclarationStatement->stackOffset;

        if (varDeclarationStatement->hasTypeOrLet && varDeclarationStatement->typeOrLet.hasImmutableMark) {
            context->addErrorWithNode(ErrorIndex::assign_to_immutable, assign);
        }

        validateAssignmentAndExpression(varDeclarationStatement, assign, context);
    }


    // This function validates an assignment node by checking type declarations, type compatibility, and other constraints.
    static void validateAssignmentNode(NodeBase *node, void *topLevelNodeInBody, ParseContext *context) {
        auto *assign = Cast::downcast<AssignmentNodeStruct *>(node);

        if (assign->hasTypeOrLet) { // int b = 8, let b = 8
            validateDeclaredTypeAssignmentNode(assign, topLevelNodeInBody, context);
        }
        else { // b = 4
            validateOnlyAssignmentNode(assign, topLevelNodeInBody, context);
        }
    }

    // This function is called for each expression node in the AST to determine its type and perform necessary validations.
    // children come first
    static int callTypeSelectorOnExpressions(NodeBase *node, ApplyFunc_params2)
    {
        if (node->vtable == VTables::BinaryOperationVTable) {
            auto *binary = Cast::downcast<BinaryOperationNodeStruct *>(node);

            int leftTypeIndex = binary->leftExprNode->typeIndex;
            int rightTypeIndex = binary->rightExprNode->typeIndex;
             if (!TypeManager::isValidTypeIndex(leftTypeIndex) || !TypeManager::isValidTypeIndex(rightTypeIndex)) {
                 // A child node should have already emitted a semantic error (e.g., unknown identifier).
                 return 0;
             }

            auto *baseTypeEntry = context->typeManager->getTypeEntryByIndex(leftTypeIndex);
            auto *targetTypeEntry = context->typeManager->getTypeEntryByIndex(rightTypeIndex);
            if (baseTypeEntry->typeIndex == BuiltInTypeIndex::null) {
                auto *temp = targetTypeEntry;
                targetTypeEntry = baseTypeEntry;
                baseTypeEntry = temp;
            }

            if (baseTypeEntry->typeIndex == BuiltInTypeIndex::null) {
                context->addErrorWithNode(ErrorIndex::invalid_operator_for_type, binary);
                return 0;
            }

            int binaryType = baseTypeEntry->binary_operate(context, binary);
             if (!TypeManager::isValidTypeIndex(binaryType)) { // invalid operator for the type
                 context->addErrorWithNode(ErrorIndex::internal_error, binary);
                 binary->typeIndex = (int)TypeIndexConst::NotAssigned;
                 return 0;
             }
            binary->typeIndex = binaryType;
        }
        else if (node->vtable == VTables::ParenthesesVTable) {
            auto *parentheses = Cast::downcast<ParenthesesNodeStruct *>(node);
            if (parentheses->valueNode != nullptr) {
                 if (TypeManager::isValidTypeIndex(parentheses->valueNode->typeIndex)) {
                     parentheses->typeIndex = parentheses->valueNode->typeIndex;
                 }
                 else {
                     // Child node should have already emitted a semantic error.
                     parentheses->typeIndex = (int)TypeIndexConst::NotAssigned;
                 }
            }
            else {
                parentheses->typeIndex = (int)TypeIndexConst::Empty; // empty parentheses (syntax error)
            }
        }
        else if (node->vtable == VTables::ReturnStatementVTable) {
            auto *returnState = Cast::downcast<ReturnStatementNodeStruct*>(node);
            if (returnState->expressionNode != nullptr) {
                if (!TypeManager::isValidTypeIndex(returnState->expressionNode->typeIndex)) {
                    // Expression already reported a semantic error.
                    return 0;
                }
                returnState->typeIndex = returnState->expressionNode->typeIndex;
            }
            else {
                returnState->typeIndex = (int)TypeIndexConst::Empty; // return for void function
            }
        }
        else if (node->vtable == VTables::AssignmentVTable) {
            validateAssignmentNode(node, topLevelNodeInBody, context);
        }
        else if (node->vtable == VTables::IdentifiersAccessVTable) {
            auto *currentStatement = Cast::downcast<NodeBase*>(topLevelNodeInBody);
            auto *bodyNode = Cast::downcast<FuncBodyNodeStruct *>(currentStatement->parentNode);
            assert(bodyNode->vtable == VTables::FuncBodyVTable);

            auto *vari = Cast::downcast<IdentifiersAccessNodeStruct*>(node);
            auto varibleName = vari->identifierToken;//.name;

            auto *targetAssignment = bodyNode->localVariableChain->findDeclareStatement(
                varibleName.name,
                varibleName.nameLength);
            if (targetAssignment != nullptr) {
                vari->stackOffset = targetAssignment->stackOffset;
                vari->typeIndex = targetAssignment->typeIndex;
            }
            else {
                context->addErrorWithNode(ErrorIndex::no_variable_defined, vari);
            }
        }
        else {
            // for other node types, assign typeIndex based on the node's type using the TypeManager.
            // e.g., for literals, function calls, etc.
            node->typeIndex = context->typeManager->typeFromNode(node);
        }

        return 0; // varDefFound ? 1 : 0;
    }


    static FuncDefNodeStruct* findMainFunc(DocumentStruct* document)
    {
        auto* rootNode = document->firstRootNode;
        while (rootNode != nullptr) {
            if (rootNode->vtable == VTables::FuncDefVTable) {
                // fn
                auto* fnNode = Cast::downcast<FuncDefNodeStruct*>(rootNode);
                auto* nameNode = &fnNode->funcNameToken;
                if (ParseUtil::equals(nameNode->name, nameNode->nameLength, "Main", 4))
                {
                    return fnNode;
                }
            }
            rootNode = rootNode->nextNode;
        }
        return nullptr;
    }



    // This function executes type selection for the body of a function definition,
    // determining the types of expressions and assignments within the function body.
    // fun A () {
    //     let a = 3
    //     int k = 5
    //     return (a + k)
    //}
    // validation steps:
    // 1. Create a local variable chain for the function body to manage variable scopes.
    // 2. Iterate through each statement in the function body.
    // 3. For each statement, apply type selection to all expressions within the statement.
    // 4. If the statement is an assignment, add it to the local variable chain
    // 5. Calculate the stack offset for each variable based on its type size.

    void Validator::validateFuncDef(FuncDefNodeStruct *func)
    {
        // set typeIndex to all expressions and assignments
        func->bodyNode.localVariableChain = func->context->memBuffer.newMem<LocalVariableChain>(1);

        auto *statement = func->bodyNode.firstChildNode;
        int currentStackOffset = 0;
        while (statement != nullptr) {
            // call type selector for all expressions in the statement
            statement->vtable->applyFuncToDescendants(
                    statement,
                    func->context,
                    nullptr,
                    callTypeSelectorOnExpressions,
                    false, // children first 
                    (void *) statement,
                    nullptr);

            if (statement->vtable == VTables::AssignmentVTable) {
                // add variable declaration(assignment node) to local variable chain
                auto *assign = Cast::downcast<AssignmentNodeStruct *>(statement);
                if (assign->hasTypeOrLet)
                {
                    func->bodyNode.localVariableChain->addToCurrentBlock(assign, func->context);

                    if (TypeManager::isValidTypeIndex(assign->typeIndex)) { // typeIndex is not valid if the type is not found
                        assign->stackOffset = currentStackOffset;
                        TypeEntry *typeEntry = func->context->typeManager->getTypeEntryByIndex(assign->typeIndex);
                        currentStackOffset += typeEntry->getStackSizeForType();
                    }
                }
            }

            statement = statement->nextNode;
        }

        func->stackSize = currentStackOffset;
    }

    void Validator::validateScript(DocumentStruct *document)
    {
        assert(document->context->syntaxErrorInfo.hasError == false);

        // Ensure vtable type selectors are registered before typeFromNode() is used.
        document->context->typeManager->initializeBuiltinTypeSelectors();

        // search all funcs
        auto *rootNode = document->firstRootNode;
        while (rootNode != nullptr) {
            if (rootNode->vtable == VTables::FuncDefVTable) {
                // fn
                auto *fnNode = Cast::downcast<FuncDefNodeStruct*>(rootNode);
                Validator::validateFuncDef(fnNode);
            }
            rootNode = rootNode->nextNode;
        }

        auto *mainFunc = findMainFunc(document);
        if (mainFunc == nullptr) {
            // error: entry func not found
            document->context->addErrorWithNode(ErrorIndex::main_func_not_found, document);
        }
    }
}