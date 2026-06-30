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
        /*
        int typeIndex = node->typeIndex;
        if (typeIndex == -1) {
            typeIndex = BuiltInTypeIndex::int64;
        }

        auto *typeManager = context->typeManager;
        auto *typeEntry = typeManager->getTypeEntryByIndex(typeIndex);
        int dataSize = typeEntry->getStackSizeForType();

        const GPRRegister *calcRegister = GetGPRRegisterByEnum(node->calcRegEnum, &context->cpuRegister);
        if (calcRegister != nullptr) {
            st_byte* dataPointer = GetDataPointerFromGPRRegister(calcRegister, dataSize);
            node->calcReg = dataPointer;
        }
        */
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

    static void validateDeclaredTypeAssignmentNode(AssignmentNodeStruct *assign,
                                                   void *topLevelNodeInBody,
                                                   ParseContext *context)
    {
        TypeEntry *declaredType = nullptr;
        if (assign->hasTypeDecl()) {
            declaredType = (TypeEntry *) context->typeManager->typeNameMap->get(
                assign->typeOrLet.nameNode.name,
                assign->typeOrLet.nameNode.nameLength
            );
            if (declaredType != nullptr) {
                assert(declaredType->typeIndex > 0);
                assign->typeIndex = declaredType->typeIndex;
            }
            else {
                // error: no type found
                context->addErrorWithNode(ErrorIndex::type_not_found, &assign->typeOrLet);
            }
        }

        if (assign->expressionNode) { // int b = 8, let b = 8
            assert(assign->expressionNode->typeIndex > 0);
            int childTypeIndex = assign->expressionNode->typeIndex;

            if (assign->typeOrLet.isLet) { // let b = 8
                assign->typeIndex = childTypeIndex;

                TypeEntry *typeEntry = context->typeManager->getTypeEntryByIndex(assign->typeIndex);
                assign->typeIndex = childTypeIndex;
            }
            else { // int b = 8
                if (declaredType != nullptr) {
                    if (declaredType->typeIndex != childTypeIndex) {
                        if (childTypeIndex == BuiltInTypeIndex::null){
                            if (!assign->typeOrLet.hasNullableMark) {
                                context->addErrorWithNode(ErrorIndex::assign_null_to_unnullable, assign);
                            }
                        }
                        else {
                            // check assignable
                            auto *targetTypeEntry = context->typeManager->getTypeEntryByIndex(assign->expressionNode->typeIndex);
                            bool canAssign = declaredType->canAssignTypeImplicitly(context, targetTypeEntry);
                            if (!canAssign) {
                                context->addErrorWithNode(ErrorIndex::type_is_not_assignable, assign);
                            }
                        }
                    }
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

    static void validateOnlyAssignmentNode(AssignmentNodeStruct *assign,
                                           void *topLevelNodeInBody,
                                           ParseContext *context
    ) {
        auto *currentStatement = Cast::downcast<NodeBase*>(topLevelNodeInBody);
        auto *bodyNode = Cast::downcast<FuncBodyNodeStruct *>(currentStatement->parentNode);
        assert(bodyNode->vtable == VTables::FuncBodyVTable);
        assert(assign->expressionNode);

        int childTypeIndex = assign->expressionNode->typeIndex;
        assign->typeIndex = childTypeIndex;

        auto *child = bodyNode->firstChildNode;
        bool hit = false;
        while (child) {
            if (child == currentStatement) {
                break;
            }
            if (child->vtable == VTables::AssignmentVTable) {
                auto *declAssign = Cast::downcast<AssignmentNodeStruct *>(child);
                if (declAssign->hasTypeOrLet) {
                    if (ParseUtil::equals(assign->variableNameToken.name, assign->variableNameToken.nameLength,
                                            declAssign->variableNameToken.name, declAssign->variableNameToken.nameLength)) {
                        if (declAssign->typeOrLet.hasImmutableMark) {
                            context->addErrorWithNode(ErrorIndex::assign_to_immutable, assign);
                        }

                        assign->stackOffset = declAssign->stackOffset;
                        hit = true;
                        if (childTypeIndex != declAssign->typeIndex) {
                            if (childTypeIndex > 0) {
                                auto *targetTypeEntry = context->typeManager->getTypeEntryByIndex(childTypeIndex);
                                bool canAssign = targetTypeEntry->canAssignTypeImplicitly(context, targetTypeEntry);
                                if (!canAssign) {
                                    // error
                                    context->addErrorWithNode(ErrorIndex::type_is_not_assignable, assign);
                                }
                            }
                        }

                        // if (assign->expressionNode->typeAtHeap != declAssign->typeAtHeap) {
                        //     // error
                        //     context->addErrorWithNode(ErrorIndex::type_is_not_assignable, assign);
                        // }
                    }
                }
            }
            child = child->nextNode;
        }

        if (!hit) {
            // error: no decl found
            context->addErrorWithNode(ErrorIndex::no_variable_defined, assign);
        }
    }


    // This function validates an assignment node by checking type declarations, type compatibility, and other constraints.
    static void validateAssignmentNode(NodeBase *node, void *topLevelNodeInBody, ParseContext *context) {
        auto *assign = Cast::downcast<AssignmentNodeStruct *>(node);

        if (assign->hasTypeOrLet) { // int b = 8, let b = 8, b = 4
            validateDeclaredTypeAssignmentNode(assign, topLevelNodeInBody, context);
        }
        else { // b = 4
            validateOnlyAssignmentNode(assign, topLevelNodeInBody, context);
        }
    }

    // children come first
    static int callTypeSelectorOnExpressions(NodeBase *node, ApplyFunc_params2)
    {

        if (node->vtable == VTables::BinaryOperationVTable) {
            auto *binary = Cast::downcast<BinaryOperationNodeStruct *>(node);

            int leftTypeIndex = binary->leftExprNode->typeIndex;
            int rightTypeIndex = binary->rightExprNode->typeIndex;
             assert(leftTypeIndex > -1 && rightTypeIndex > -1);

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
            assert(binaryType > -1);
            binary->typeIndex = binaryType;
        }
        else if (node->vtable == VTables::ParenthesesVTable) {
            auto *parentheses = Cast::downcast<ParenthesesNodeStruct *>(node);
            if (parentheses->valueNode != nullptr) {
                assert(parentheses->valueNode != nullptr && parentheses->valueNode->typeIndex > -1);
                parentheses->typeIndex = parentheses->valueNode->typeIndex;
            }
            else {
                parentheses->typeIndex = (int)TypeIndexEnum::Empty; // empty parentheses (syntax error)
            }
        }
        else if (node->vtable == VTables::ReturnStatementVTable) {
            auto *returnState = Cast::downcast<ReturnStatementNodeStruct*>(node);
            if (returnState->expressionNode != nullptr) {
                assert(returnState->expressionNode->typeIndex > -1);
                returnState->typeIndex = returnState->expressionNode->typeIndex;
            }
            else {
                returnState->typeIndex = (int)TypeIndexEnum::Empty; // return for void function
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

            auto *child = bodyNode->firstChildNode;
            while (child) {
                if (child == currentStatement) {
                    break;
                }
                if (child->vtable == VTables::AssignmentVTable) {
                    auto *declAssign = Cast::downcast<AssignmentNodeStruct *>(child);
                    if (declAssign->hasTypeOrLet) {
                        if (ParseUtil::equals(varibleName.name, varibleName.nameLength,
                                             declAssign->variableNameToken.name, declAssign->variableNameToken.nameLength)) {
                            vari->stackOffset = declAssign->stackOffset;
                            vari->typeIndex = declAssign->typeIndex;
                        }
                    }
                }
                child = child->nextNode;
            }
        }
        else {
            // for other nodes, like number, string literal, etc.
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


    /// <summary>
    /// Sets typeIndex and Assigns stackOffset
    /// </summary>
    static void callTypeSelectorsOnExpressions2(ParseContext *context, FuncDefNodeStruct *func)
    {
        func->localVariableMap = func->context->memBuffer.newMem<VoidHashMap>(1);

        auto *statement = func->bodyNode.firstChildNode;
        int currentStackOffset = 0;
        while (statement) {
            statement->vtable->applyFuncToDescendants(
                    statement,
                    context,
                    nullptr,
                    callTypeSelectorOnExpressions,
                    /* children first */false,
                    (void *) statement,
                    nullptr);

            // int a = 3
            if (statement->vtable == VTables::AssignmentVTable) {
                auto* assign = Cast::downcast<AssignmentNodeStruct*>(statement);
                if (assign->hasTypeOrLet) { // assign stack offset for variable
                    if (assign->typeIndex == -1) {
                        // can't assign stack offset further if type not found 
                        return;
                    }
                    TypeEntry *typeEntry = context->typeManager->getTypeEntryByIndex(assign->typeIndex);
                    currentStackOffset -= typeEntry->getStackSizeForType();
                    assign->stackOffset = currentStackOffset;
                }
            }

            statement = statement->nextNode;
        }

        func->stackSize = -currentStackOffset;
    }


    void TypeManager::validateFuncDef(FuncDefNodeStruct *func)
    {
        int errorCount = this->context->semanticErrorInfo.count;

        // set typeIndex to all expressions and assignments
        callTypeSelectorsOnExpressions2(context, func);

        if (errorCount == this->context->semanticErrorInfo.count) {
            //assignCalcOpRegister(context, func);
        }

    }

    void TypeManager::validateScript(DocumentStruct *document)
    {
        assert(document->context->syntaxErrorInfo.hasError == false);

        // search all funcs
        auto *rootNode = document->firstRootNode;
        while (rootNode != nullptr) {
            if (rootNode->vtable == VTables::FuncDefVTable) {
                // fn
                auto *fnNode = Cast::downcast<FuncDefNodeStruct*>(rootNode);
                this->validateFuncDef(fnNode);
            }
            rootNode = rootNode->nextNode;
        }

        auto *mainFunc = findMainFunc(document);
        if (mainFunc == nullptr) {
            // error: entry func not found
        }
    }

}