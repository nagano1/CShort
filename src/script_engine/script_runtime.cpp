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

namespace cshort {
    //------------------------------------------------------------------------------------------
    //
    //                                      evaluateExprNode
    //
    //------------------------------------------------------------------------------------------

    void ScriptEngineContext::evaluateExprNode(NodeBase *expressionNode)
    {
        assert(expressionNode != nullptr);
        assert(expressionNode->vtable != nullptr);

        if (expressionNode->vtable == VTables::IdentifiersAccessVTable) {
            auto* variableNode = Cast::downcast<IdentifiersAccessNodeStruct *>(expressionNode);
            TypeEntry *typeEntry = this->scriptEnv->getTypeEntryByIndex(variableNode->typeIndex);
            int dataSize = typeEntry->getStackSizeForType();
            this->stackMemory.moveFromStack(variableNode->stackOffset, dataSize, variableNode->calcReg);
            return;
        }
        
        if (expressionNode->vtable == VTables::ParenthesesVTable) {
            auto* parentheses = Cast::downcast<ParenthesesNodeStruct *>(expressionNode);
            evaluateExprNode(parentheses->valueNode);
            return;
        }

        // a + (b + c)
        if (expressionNode->vtable == VTables::BinaryOperationVTable) {
            auto* binaryNode = Cast::downcast<BinaryOperationNodeStruct *>(expressionNode);

            // evaluate right first, then left, to avoid overwriting registers
            this->evaluateExprNode(binaryNode->rightExprNode);
            uint64_t saved = *(uint64_t*)(binaryNode->rightExprNode->calcReg);
            this->evaluateExprNode(binaryNode->leftExprNode);
            *(uint64_t*)(binaryNode->rightExprNode->calcReg) = saved;

            auto *leftTypeEntry = this->scriptEnv->getTypeEntryByIndex(binaryNode->leftExprNode->typeIndex);
            leftTypeEntry->binary_operate(this, binaryNode, /**/false);

            return;
        }

        // To handle func call, like 3 + funcA(100)
        // 1. evaluate caller expression to get func entry
        // 2. evaluate arguments and save their values to registers
        // 3. call func and save return value to register
        /*
        if (expressionNode->vtable == VTables::FuncCallVTable) {
            auto *funcCall = Cast::downcast<FuncCallNodeStruct *>(expressionNode);
            this->evaluateExprNode(funcCall->callerExprNode);
        }
        */



        TypeEntry *typeEntry = nullptr;
        int typeIndex = expressionNode->typeIndex;
        if (typeIndex > 0) {
            typeEntry = this->scriptEnv->getTypeEntryByIndex(typeIndex);
        }

        if (typeEntry) {
            typeEntry->evaluateNode(this, expressionNode);
        }
    }


    //------------------------------------------------------------------------------------------
    //
    //                                       Script Engine
    //
    //------------------------------------------------------------------------------------------

    // Script engine steps:
    // 1. load scripts.
    // 2. validate scripts
    //    - set typeIndex for each expression node
    //    - check type error
    //    - assign stack offset for variables
    //    - assign registers for expressions
    //    - find main entry func
    // 3. run scripts (evaluate nodes)

    ScriptEnv *ScriptEnv::newScriptEnv()
    {
        auto *scriptEnv = (ScriptEnv*)malloc(sizeof(ScriptEnv));
        if (scriptEnv != nullptr) {
            auto *context = mallocForType<ScriptEngineContext>();

            scriptEnv->context = context;
            context->init(scriptEnv);

            scriptEnv->mainFunc = nullptr;
            scriptEnv->typeEntryList = nullptr;
            scriptEnv->typeEntryListNextIndex = 1;
            scriptEnv->typeEntryListCapacity = 0;

            _registerBuiltInTypes(scriptEnv);
        }
        return scriptEnv;
    }

    void ScriptEnv::deleteScriptEnv(ScriptEnv *scriptEnv)
    {
        scriptEnv->context->freeAll();
        free(scriptEnv->context);
        free(scriptEnv);
    }


    struct TypeAndExpression {
        TypeEntry *typeEntry;
        NodeBase *expressionNode;
    };

    static TypeAndExpression executeFunc(ScriptEnv* env, FuncDefNodeStruct* mainFunc)
    {
        int stackSize = mainFunc->stackSize;
        int baseBytesMinusOne = env->context->stackMemory.baseBytes - 1;
        const int alignedStackSize = (stackSize + baseBytesMinusOne) & ~baseBytesMinusOne; // Align to base bytes

        env->context->stackMemory.call(); // simulate call by pushing return address
        env->context->stackMemory.localVariables(alignedStackSize); // allocate space for local variables

        auto* statementNode = mainFunc->bodyNode.firstChildNode;
        while (statementNode != nullptr)
        {
            // call func: funcA(100)
            if (statementNode->vtable == VTables::FuncCallVTable) {
                auto* funcCall = Cast::downcast<FuncCallNodeStruct*>(statementNode);
                FuncCallArgItemStruct *arg = funcCall->firstArgumentItem;
                while (arg != nullptr) {
                    printf("arg = <%s>\n", arg->exprNode->vtable->typeChars);
                    env->context->evaluateExprNode(arg->exprNode);
                    if (arg->nextNode == nullptr) {
                        break;
                    }
                    arg = Cast::downcast<FuncCallArgItemStruct *>(arg->nextNode);
                }
            }

            // assignment: int a = 3
            if (statementNode->vtable == VTables::AssignmentVTable) {
                auto* assignStatement = Cast::downcast<AssignmentNodeStruct *>(statementNode);
                if (assignStatement->expressionNode != nullptr) {
                    env->context->evaluateExprNode(assignStatement->expressionNode);
                    auto *typeEntry = env->getTypeEntryByIndex(assignStatement->typeIndex);

                    // 
                    auto dataSize = typeEntry->getStackSizeForType();
                    env->context->stackMemory.moveToStack(assignStatement->stackOffset, dataSize,
                                                     assignStatement->expressionNode->calcReg);
                }
            }

            // return 3
            if (statementNode->vtable == VTables::ReturnStatementVTable) {
                auto* returnNode = Cast::downcast<ReturnStatementNodeStruct*>(statementNode);
                env->context->evaluateExprNode(returnNode->expressionNode);
                auto* typeEntry = env->getTypeEntryByIndex(returnNode->expressionNode->typeIndex);

                env->context->stackMemory.ret(); // simulate return by popping return address
                return TypeAndExpression{typeEntry, returnNode->expressionNode};
            }

            statementNode = statementNode->nextNode;
        }

        env->context->stackMemory.ret();
        return TypeAndExpression{nullptr, nullptr};
    }


    _ScriptEnv* ScriptEnv::loadScript(char* script, int byteLength)
    {
        ScriptEnv* env = ScriptEnv::newScriptEnv();
        setupBuiltInTypeSelectors(env);

        auto* document = Alloc::newDocument(DocumentType::CodeDocument);
        DocumentUtils::parseText(document, script, byteLength);
        env->document = document;

        return env;
    }



    int ScriptEnv::runScript()
    {
        assert(this->document->context->syntaxErrorInfo.hasError == false);
        assert(this->context->semanticErrorInfo.hasError == false);

        int ret = 0;
        auto *mainFunc = this->mainFunc;
        if (mainFunc) {
            // printf("main found <%s()>\n", mainFunc2->nameNode.name);
            TypeAndExpression typeAndExpression = executeFunc(this, mainFunc);
            TypeEntry *typeEntry = typeAndExpression.typeEntry;
            if (typeEntry != nullptr) {
                if (typeEntry->getStackSizeForType() == 8) {
                    int64_t v = *(int64_t*)typeAndExpression.expressionNode->calcReg;
                    ret = (int32_t)v; // return of main entry func is always int32, so cast to int32
                }
                else {
                    ret = *(int32_t*)typeAndExpression.expressionNode->calcReg;
                }
            }
        }

        auto *rootNode = this->document->firstRootNode;
        while (rootNode != nullptr) {
            if (rootNode->vtable == VTables::ClassVTable) {
                // class
            }
            else if (rootNode->vtable == VTables::FuncDefVTable) {
                // fn
            }

            rootNode = rootNode->nextNode;
        }

        Alloc::deleteDocument(this->document);
        ScriptEnv::deleteScriptEnv(this);

        return ret;
    }


    int ScriptEnv::startScriptInternal(char* script, int scriptLength)
    {
        // Load the script
        ScriptEnv *env = ScriptEnv::loadScript(script, scriptLength);

        // Return if there's a syntax error
        if (env->document->context->syntaxErrorInfo.hasError) {
            return env->document->context->syntaxErrorInfo.errorItem.errorId;
        }

        // Validate types, values and finding Main(entry) function
        env->validateScript();
        if (env->context->semanticErrorInfo.hasError) {
            env->context->setErrorPositions();
            return env->context->semanticErrorInfo.firstErrorItem->codeErrorItem.errorId;
        }

        // Run script
        return env->runScript();
    }
}