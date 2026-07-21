#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "compiler.hpp"

namespace cshort {

    // ---------------------------------------------------------------------------
    //
    //                            Compiler using Wasm (minimal)
    //
    // ---------------------------------------------------------------------------

    static const char *watTypeName(int typeIndex) {
        if (typeIndex == BuiltInTypeIndex::int32) return "i32";
        if (typeIndex == BuiltInTypeIndex::int64) return "i64";
        return "i64";
    }

    static bool isIntegerType(int typeIndex) {
        return typeIndex == BuiltInTypeIndex::int32 || typeIndex == BuiltInTypeIndex::int64;
    }

    // Safe fallback: minimal module exporting main that returns 0.
    static char *emitFallback(MemBuffer &context) {
        auto *consttext =
            "(module\n"
            "  (func $main (result i64)\n"
            "    i64.const 0\n"
            "    return)\n"
            "  (export \"main\" (func $main))\n"
            ")\n";
        int len = (int)strlen(consttext);
        utf8byte *text = context.newText(len);
        memcpy(text, consttext, len);
        text[len] = '\0';
        return text;
    }

    // Emits expression in stack-machine style WAT.
    // Returns false if unsupported syntax/type is found.
    static bool emitExprWAT(StringBuilder &sb,
                            NodeBase *expr,
                            VoidHashMap &varTypeIndex,
                            int expectedTypeIdx)
    {
        if (expr == nullptr) return false;
        if (!isIntegerType(expr->typeIndex)) return false;

        char buf[256];

        if (expr->vtable == VTables::NumberVTable) {
            auto *numNode = Cast::downcast(expr);
            const char *ty = watTypeName(expr->typeIndex);
            snprintf(buf, sizeof(buf), "    %s.const %" PRId64 "\n", ty, numNode->num);
            sb.append(buf);

            // minimal implicit widening where needed
            if (expr->typeIndex == BuiltInTypeIndex::int32 && expectedTypeIdx == BuiltInTypeIndex::int64) {
                sb.append("    i64.extend_i32_s\n");
            }
            return true;
        }

        if (expr->vtable == VTables::IdentifiersAccessVTable) {
            auto *ident = Cast::downcast(expr);
            const char *varName = ident->identifierToken.name;
            if (varName == nullptr) return false;

            void *item = varTypeIndex.get(varName, (int)strlen(varName));
            if (item == nullptr) return false;

            int srcTypeIdx = (int)(intptr_t)item - 1; // stored as type + 1
            if (!isIntegerType(srcTypeIdx)) return false;

            snprintf(buf, sizeof(buf), "    local.get $%s\n", varName);
            sb.append(buf);

            if (srcTypeIdx == BuiltInTypeIndex::int32 && expectedTypeIdx == BuiltInTypeIndex::int64) {
                sb.append("    i64.extend_i32_s\n");
            }
            return true;
        }

        if (expr->vtable == VTables::ParenthesesVTable) {
            auto *paren = Cast::downcast(expr);
            return emitExprWAT(sb, paren->valueNode, varTypeIndex, expectedTypeIdx);
        }

        if (expr->vtable == VTables::BinaryOperationVTable) {
            auto *bin = Cast::downcast(expr);

            if (!isIntegerType(bin->typeIndex)) return false;
            const int opTypeIdx = bin->typeIndex;

            // emit left/right as operation type
            if (!emitExprWAT(sb, bin->leftExprNode, varTypeIndex, opTypeIdx)) return false;
            if (!emitExprWAT(sb, bin->rightExprNode, varTypeIndex, opTypeIdx)) return false;

            const bool isI32 = (opTypeIdx == BuiltInTypeIndex::int32);

            switch (bin->binaryOp) {
                case BinaryOperator::Add:
                    sb.append(isI32 ? "    i32.add\n" : "    i64.add\n");
                    break;
                case BinaryOperator::Subtract:
                    sb.append(isI32 ? "    i32.sub\n" : "    i64.sub\n");
                    break;
                case BinaryOperator::Multiply:
                    sb.append(isI32 ? "    i32.mul\n" : "    i64.mul\n");
                    break;
                case BinaryOperator::Divide:
                    sb.append(isI32 ? "    i32.div_s\n" : "    i64.div_s\n");
                    break;
                default:
                    return false;
            }

            // widen final expression if caller expects i64
            if (opTypeIdx == BuiltInTypeIndex::int32 && expectedTypeIdx == BuiltInTypeIndex::int64) {
                sb.append("    i64.extend_i32_s\n");
            }
            return true;
        }

        return false;
    }

    char *CompilerForWasm::compile(DocumentStruct *document, MemBuffer &memBufferForText) {
        FuncDefNodeStruct *mainFunc = document->mainFunc;
        if (mainFunc == nullptr) {
            return emitFallback(memBufferForText);
        }

        // Only fn Main() is supported as the entry point
        const char *funcName = mainFunc->funcNameToken.name;
        if (funcName == nullptr || strcmp(funcName, "Main") != 0) {
            return emitFallback(memBufferForText);
        }

        StringBuilder sb;
        sb.initWithInitialCapacity(1024);
        sb.appendWithAutoLength("(module\n");
        sb.appendWithAutoLength("  (func $main (result i64)\n");

        // Track type of each local variable (varName -> typeIndex+1)
        MemBuffer varTypeIndexMemBuffer;
        varTypeIndexMemBuffer.init();
        VoidHashMap varTypeIndex;
        varTypeIndex.init(&varTypeIndexMemBuffer);

        // Emit locals first by scanning assignments
        for (auto *n = mainFunc->bodyNode.firstChildNode; n != nullptr; n = n->nextNode) {
            if (n->vtable != VTables::AssignmentVTable) continue;
            auto *assign = Cast::downcast(n);
            const int dstTypeIdx = assign->typeIndex;
            if (!isIntegerType(dstTypeIdx)) {
                varTypeIndexMemBuffer.freeAll();
                sb.freeAll();
                return emitFallback(memBufferForText);
            }

            const char *varName = assign->variableNameToken.name;
            if (varName == nullptr) continue;

            // first definition wins for local declaration
            if (varTypeIndex.get(varName, (int)strlen(varName)) == nullptr) {
                varTypeIndex.put(varName, (int)strlen(varName), (void *)(intptr_t)(dstTypeIdx + 1));
                char lbuf[256];
                snprintf(lbuf, sizeof(lbuf), "    (local $%s %s)\n", varName, watTypeName(dstTypeIdx));
                sb.append(lbuf);
            }
        }

        bool emittedRet = false;

        for (auto *statementNode = mainFunc->bodyNode.firstChildNode;
             statementNode != nullptr;
             statementNode = statementNode->nextNode)
        {
            // Assignment: i64 a = expr
            if (statementNode->vtable == VTables::AssignmentVTable) {
                auto *assign = Cast::downcast(statementNode);

                if (assign->expressionNode == nullptr) continue;
                const int dstTypeIdx = assign->typeIndex;
                if (!isIntegerType(dstTypeIdx)) {
                    varTypeIndexMemBuffer.freeAll();
                    sb.freeAll();
                    return emitFallback(memBufferForText);
                }

                const char *varName = assign->variableNameToken.name;
                if (varName == nullptr) continue;

                // ensure variable exists in map
                if (varTypeIndex.get(varName, (int)strlen(varName)) == nullptr) {
                    varTypeIndex.put(varName, (int)strlen(varName), (void *)(intptr_t)(dstTypeIdx + 1));
                }

                if (!emitExprWAT(sb, assign->expressionNode, varTypeIndex, dstTypeIdx)) {
                    varTypeIndexMemBuffer.freeAll();
                    sb.freeAll();
                    return emitFallback(memBufferForText);
                }

                char buf[256];
                snprintf(buf, sizeof(buf), "    local.set $%s\n", varName);
                sb.append(buf);
            }
            // Return statement: return expr
            else if (statementNode->vtable == VTables::ReturnStatementVTable) {
                auto *retNode = Cast::downcast(statementNode);

                if (retNode->expressionNode == nullptr) {
                    sb.append("    i64.const 0\n");
                    sb.append("    return\n");
                } else {
                    if (!emitExprWAT(sb, retNode->expressionNode, varTypeIndex, BuiltInTypeIndex::int64)) {
                        varTypeIndexMemBuffer.freeAll();
                        sb.freeAll();
                        return emitFallback(memBufferForText);
                    }
                    sb.append("    return\n");
                }

                emittedRet = true;
            }
            else {
                // unsupported statement kind in minimal exporter
                varTypeIndexMemBuffer.freeAll();
                sb.freeAll();
                return emitFallback(memBufferForText);
            }
        }

        if (!emittedRet) {
            sb.append("    i64.const 0\n");
            sb.append("    return\n");
        }

        sb.appendWithAutoLength("  )\n");
        sb.appendWithAutoLength("  (export \"main\" (func $main))\n");
        sb.appendWithAutoLength(")\n");

        varTypeIndexMemBuffer.freeAll();

        utf8byte *text = memBufferForText.newText((int)sb.length());
        memcpy(text, sb.str, sb.length());
        text[sb.length()] = '\0';
        sb.freeAll();
        return text;
    }
}