#include <cstdio>
#include <cstring>
#include <cinttypes>
#include <cassert>

#include "compiler.hpp"

namespace cshort {

    // ---------------------------------------------------------------------------
    //
    //                            Compiler using Wasm  (WAT text format)
    //
    // Supported subset (intentionally minimal):
    //   - fn Main() only
    //   - statements : assignment (i32/i64) + return
    //   - expressions: integer literal, identifier access,
    //                  binary op (+,-,*,/), parentheses
    //   - i32 -> i64 widening where needed (i64.extend_i32_s)
    //   - unsupported node/type -> safe fallback module
    // ---------------------------------------------------------------------------

    // ---- helpers ---------------------------------------------------------------

    static const char *watTypeName(int typeIndex) {
        if (typeIndex == BuiltInTypeIndex::int64) return "i64";
        if (typeIndex == BuiltInTypeIndex::int32) return "i32";
        return nullptr; // unsupported
    }

    // Fallback: minimal WAT module that exports main returning 0.
    static char *emitWasmFallback(MemBuffer &mem) {
        static const char *text =
            "(module\n"
            "  (func $main (result i64)\n"
            "    i64.const 0\n"
            "  )\n"
            "  (export \"main\" (func $main))\n"
            ")\n";
        int len = (int)strlen(text);
        utf8byte *buf = mem.newText(len);
        memcpy(buf, text, len);
        buf[len] = '\0';
        return buf;
    }

    // ---- expression emitter ----------------------------------------------------
    // Returns false when an unsupported node is encountered (caller must fallback).
    // 'resultTypeIdx' is the type the parent context wants from this expression;
    // used to decide whether to emit a widening instruction after the value.

    struct WasmEmitCtx {
        StringBuilder    &sb;
        VoidHashMap      &varTypeIndex;  // varName -> (typeIndex + 1) as void*
        char              buf[256];
        bool              failed;
    };

    // Forward declaration.
    static bool emitExprWat(WasmEmitCtx &ctx, NodeBase *expr, int wantTypeIdx);

    // Emit a widening instruction when srcType is i32 and wantType is i64.
    static void emitWiden(WasmEmitCtx &ctx, int srcTypeIdx, int wantTypeIdx) {
        if (srcTypeIdx == BuiltInTypeIndex::int32 && wantTypeIdx == BuiltInTypeIndex::int64) {
            ctx.sb.appendWithAutoLength("    i64.extend_i32_s\n");
        }
    }

    // returns false if unsupported node is encountered.
    static bool emitExprWat(WasmEmitCtx &ctx, NodeBase *expr, int wantTypeIdx) {
        if (expr == nullptr || ctx.failed) {
            ctx.failed = true;
            return false;
        }

        // ---- integer literal: push i32.const / i64.const ----
        if (expr->vtable == VTables::NumberVTable) {
            auto *numNode = Cast::downcast<NumberNodeStruct *>(expr);
            int typeIdx = expr->typeIndex;
            const char *wat = watTypeName(typeIdx);
            if (wat == nullptr) { ctx.failed = true; return false; }
            snprintf(ctx.buf, sizeof(ctx.buf), "    %s.const %" PRId64 "\n", wat, numNode->num);
            ctx.sb.append(ctx.buf);
            emitWiden(ctx, typeIdx, wantTypeIdx);
            return true;
        }

        // ---- identifier access: local.get $name ----
        if (expr->vtable == VTables::IdentifiersAccessVTable) {
            auto *identNode = Cast::downcast<IdentifiersAccessNodeStruct *>(expr);
            const char *varName = identNode->identifierToken.name;
            if (varName == nullptr) { ctx.failed = true; return false; }

            void *item = ctx.varTypeIndex.get(varName, (int)strlen(varName));
            if (item == nullptr) { ctx.failed = true; return false; }
            int srcTypeIdx = (int)(intptr_t)item - 1;

            snprintf(ctx.buf, sizeof(ctx.buf), "    local.get $%s\n", varName);
            ctx.sb.append(ctx.buf);
            emitWiden(ctx, srcTypeIdx, wantTypeIdx);
            return true;
        }

        // ---- parentheses: transparent passthrough ----
        if (expr->vtable == VTables::ParenthesesVTable) {
            auto *paren = Cast::downcast<ParenthesesNodeStruct *>(expr);
            return emitExprWat(ctx, paren->valueNode, wantTypeIdx);
        }

        // ---- binary operation: left op right ----
        if (expr->vtable == VTables::BinaryOperationVTable) {
            auto *binNode = Cast::downcast<BinaryOperationNodeStruct *>(expr);

            // The result type of the binary op is the expression's own typeIndex.
            int resultTypeIdx = expr->typeIndex;
            if (watTypeName(resultTypeIdx) == nullptr) { ctx.failed = true; return false; }

            // For each operand: emit the value, then widen to resultTypeIdx if needed.
            if (!emitExprWat(ctx, binNode->leftExprNode,  resultTypeIdx)) return false;
            if (!emitExprWat(ctx, binNode->rightExprNode, resultTypeIdx)) return false;

            // Emit the operator instruction using the result type.
            const char *ty = watTypeName(resultTypeIdx);
            const char *opName = nullptr;
            switch (binNode->binaryOp) {
                case BinaryOperator::Add:      opName = "add";   break;
                case BinaryOperator::Subtract: opName = "sub";   break;
                case BinaryOperator::Multiply: opName = "mul";   break;
                case BinaryOperator::Divide:   opName = "div_s"; break;
                default:                        ctx.failed = true; return false;
            }

            snprintf(ctx.buf, sizeof(ctx.buf), "    %s.%s\n", ty, opName);
            ctx.sb.append(ctx.buf);

            // Widen the result to whatever the parent wants.
            emitWiden(ctx, resultTypeIdx, wantTypeIdx);
            return true;
        }

        // Unsupported node type.
        ctx.failed = true;
        return false;
    }

    // ---- main compile entry point ----------------------------------------------

    char *CompilerForWasm::compile(DocumentStruct *document, MemBuffer &memBufferForText) {
        FuncDefNodeStruct *mainFunc = document->mainFunc;
        if (mainFunc == nullptr) {
            return emitWasmFallback(memBufferForText);
        }

        const char *funcName = mainFunc->funcNameToken.name;
        if (funcName == nullptr || strcmp(funcName, "Main") != 0) {
            return emitWasmFallback(memBufferForText);
        }

        // ---- pass 1: collect local variable declarations (assignments) ---------
        // We need their names and types to emit (local $name type) before the body.

        MemBuffer varTypeIndexMem;
        varTypeIndexMem.init();
        VoidHashMap varTypeIndex;
        varTypeIndex.init(&varTypeIndexMem);

        // We also record the order of variable names so locals are declared in
        // source order (purely cosmetic, but deterministic).
        // Use a small fixed-size slab via MemBuffer; at most a few hundred locals.
        static const int MAX_LOCALS = 256;
        const char *localNames[MAX_LOCALS];
        int         localTypes[MAX_LOCALS];
        int         localCount = 0;

        bool unsupported = false;
        auto *statementNode = mainFunc->bodyNode.firstChildNode;
        while (statementNode != nullptr && !unsupported) {
            if (statementNode->vtable == VTables::AssignmentVTable) {
                auto *assign = Cast::downcast<AssignmentNodeStruct *>(statementNode);
                int dstTypeIdx = assign->typeIndex;
                if (dstTypeIdx != BuiltInTypeIndex::int64 && dstTypeIdx != BuiltInTypeIndex::int32) {
                    unsupported = true;
                    break;
                }
                const char *varName = assign->variableNameToken.name;
                if (varName != nullptr) {
                    varTypeIndex.put(varName, (int)strlen(varName), (void *)(intptr_t)(dstTypeIdx + 1));
                    if (localCount < MAX_LOCALS) {
                        localNames[localCount] = varName;
                        localTypes[localCount] = dstTypeIdx;
                        localCount++;
                    }
                }
            }
            statementNode = statementNode->nextNode;
        }

        if (unsupported) {
            varTypeIndexMem.freeAll();
            return emitWasmFallback(memBufferForText);
        }

        // ---- pass 2: emit WAT --------------------------------------------------

        StringBuilder sb;
        sb.initWithInitialCapacity(1024);
        char buf[256];

        sb.appendWithAutoLength("(module\n");
        sb.appendWithAutoLength("  (func $main (result i64)\n");

        // Emit local declarations.
        for (int i = 0; i < localCount; i++) {
            snprintf(buf, sizeof(buf), "    (local $%s %s)\n",
                     localNames[i], watTypeName(localTypes[i]));
            sb.append(buf);
        }

        WasmEmitCtx ctx{sb, varTypeIndex, {}, false};

        // Emit statement bodies.
        bool emittedReturn = false;
        statementNode = mainFunc->bodyNode.firstChildNode;
        while (statementNode != nullptr && !ctx.failed) {

            // ---- assignment: evaluate RHS, widen if needed, local.set ----------
            if (statementNode->vtable == VTables::AssignmentVTable) {
                auto *assign = Cast::downcast<AssignmentNodeStruct *>(statementNode);
                if (assign->expressionNode == nullptr) { // declaration only : int a
                    statementNode = statementNode->nextNode;
                    continue;
                }
                const char *varName = assign->variableNameToken.name;
                int dstTypeIdx = assign->typeIndex;

                // Emit RHS, requesting the destination type (handles widening).
                if (!emitExprWat(ctx, assign->expressionNode, dstTypeIdx)) break;

                snprintf(buf, sizeof(buf), "    local.set $%s\n", varName);
                sb.append(buf);
            }

            // ---- return: evaluate expr, widen to i64, return -------------------
            else if (statementNode->vtable == VTables::ReturnStatementVTable) {
                auto *retNode = Cast::downcast<ReturnStatementNodeStruct *>(statementNode);
                if (retNode->expressionNode == nullptr) {
                    sb.appendWithAutoLength("    i64.const 0\n");
                } else {
                    // Always produce i64 at the top of the stack (func result is i64).
                    if (!emitExprWat(ctx, retNode->expressionNode, BuiltInTypeIndex::int64)) break;
                }
                sb.appendWithAutoLength("    return\n");
                emittedReturn = true;
            }

            statementNode = statementNode->nextNode;
        }

        if (ctx.failed) {
            varTypeIndexMem.freeAll();
            sb.freeAll();
            return emitWasmFallback(memBufferForText);
        }

        if (!emittedReturn) {
            sb.appendWithAutoLength("    i64.const 0\n");
        }

        sb.appendWithAutoLength("  )\n");
        sb.appendWithAutoLength("  (export \"main\" (func $main))\n");
        sb.appendWithAutoLength(")\n");

        varTypeIndexMem.freeAll();

        utf8byte *text = memBufferForText.newText((int)sb.length());
        memcpy(text, sb.str, sb.length());
        text[sb.length()] = '\0';
        sb.freeAll();
        return text;
    }
}
