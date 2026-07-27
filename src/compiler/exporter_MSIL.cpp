#include <cstdio>
#include <cstring>
#include <cinttypes>
#include <cstdlib>
#include <cassert>

#include "compiler.hpp"

namespace cshort {

    // ---------------------------------------------------------------------------
    //
    //                            Compiler using MSIL
    //
    // ---------------------------------------------------------------------------

    // Returns the MSIL type name for a given CShort typeIndex.
    static const char *msilTypeName(int typeIndex) {
        if (typeIndex == BuiltInTypeIndex::int32) return "int32";
        if (typeIndex == BuiltInTypeIndex::int64) return "int64";
        return "int64";
    }

    static const char *msilFallbackText =
        ".assembly extern mscorlib {}\n"
        ".assembly CShort {}\n"
        ".module CShort.exe\n"
        "\n"
        ".class public auto ansi sealed Program extends [mscorlib]System.Object\n"
        "{\n"
        "  .method public static int64 Main() cil managed\n"
        "  {\n"
        "    .entrypoint\n"
        "    .maxstack 8\n"
        "    ldc.i8 0\n"
        "    ret\n"
        "  }\n"
        "}\n";

    // Safe fallback: a minimal IL that returns 0.
    static char *emitFallback(MemBuffer &context) {
        int len = (int)strlen(msilFallbackText);
        utf8byte *text = context.newText(len);
        memcpy(text, msilFallbackText, len);
        text[len] = '\0';
        return text;
    }

    // Example output
    /*
    .assembly extern mscorlib {}
    .assembly CShort {}
    .module CShort.exe

    .class public auto ansi sealed Program extends [mscorlib]System.Object
    {
        .method public static int64 Main() cil managed
        {
            .entrypoint
            .maxstack 8
            .locals init (
            [0] int32 b
            )
            ldc.i4 42
            stloc.s b
            ldloc.s b
            conv.i8
            ret
        }
    }
    */

    char *CompilerForMSIL::compile(DocumentStruct *document, MemBuffer &memBufferForText) {
        FuncDefNodeStruct *mainFunc = document->mainFunc;
        if (mainFunc == nullptr) {
            return emitFallback(memBufferForText);
        }

        // Only fn Main() is supported as the entry point
        const char *funcName = mainFunc->funcNameToken.name;
        if (funcName == nullptr || strcmp(funcName, "Main") != 0) {
            return emitFallback(memBufferForText);
        }

        // Track type of each named local variable (varName -> typeIndex)
        MemBuffer varTypeIndexMemBuffer;
        varTypeIndexMemBuffer.init();
        Int32HashMap varTypeIndex;
        varTypeIndex.init(&varTypeIndexMemBuffer);

        // First pass: collect all local variable declarations for .locals init
        StringBuilder sbLocals;
        sbLocals.initWithInitialCapacity(256);
        int localCount = 0;
        char buf[256];

        auto *stmtNode = mainFunc->bodyNode.firstChildNode;
        while (stmtNode != nullptr) {
            if (stmtNode->vtable == VTables::AssignmentVTable) {
                auto *assign = Cast::downcast<AssignmentNodeStruct *>(stmtNode);

                int dstTypeIdx = assign->typeIndex;
                if (dstTypeIdx != BuiltInTypeIndex::int64 && dstTypeIdx != BuiltInTypeIndex::int32) {
                    varTypeIndexMemBuffer.freeAll();
                    sbLocals.freeAll();
                    return emitFallback(memBufferForText);
                }

                const char *varName = assign->variableNameToken.name;
                if (varName == nullptr) {
                    stmtNode = stmtNode->nextNode;
                    continue;
                }

                varTypeIndex.put(varName, (int)strlen(varName), dstTypeIdx + 1); // store typeIndex + 1 to avoid nullptr ambiguity

                const char *typeName = msilTypeName(dstTypeIdx);
                if (localCount == 0) {
                    snprintf(buf, sizeof(buf), "      [%d] %s %s", localCount, typeName, varName);
                } else {
                    snprintf(buf, sizeof(buf), ",\n      [%d] %s %s", localCount, typeName, varName);
                }
                sbLocals.append(buf);
                localCount++;
            }
            stmtNode = stmtNode->nextNode;
        }

        // Build the full output
        StringBuilder sb;
        sb.initWithInitialCapacity(1024);

        sb.appendWithAutoLength(".assembly extern mscorlib {}\n");
        sb.appendWithAutoLength(".assembly CShort {}\n");
        sb.appendWithAutoLength(".module CShort.exe\n");
        sb.appendWithAutoLength("\n");
        sb.appendWithAutoLength(".class public auto ansi sealed Program extends [mscorlib]System.Object\n");
        sb.appendWithAutoLength("{\n");
        sb.appendWithAutoLength("  .method public static int64 Main() cil managed\n");
        sb.appendWithAutoLength("  {\n");
        sb.appendWithAutoLength("    .entrypoint\n");
        sb.appendWithAutoLength("    .maxstack 8\n");

        if (localCount > 0) {
            sb.appendWithAutoLength("    .locals init (\n");
            sb.append(sbLocals.str);
            sb.appendWithAutoLength("\n    )\n");
        }

        // Second pass: emit instructions
        NodeBase *nextNode = mainFunc->bodyNode.firstChildNode;
        bool emittedRet = false;

        while (nextNode != nullptr) {
            NodeBase *currentNode = nextNode;
            nextNode = currentNode->nextNode;
            
            // Assignment: i64 a = 100
            if (currentNode->vtable == VTables::AssignmentVTable) {
                auto *assign = Cast::downcast<AssignmentNodeStruct *>(currentNode);

                if (assign->expressionNode == nullptr) {
                    continue;
                }

                int dstTypeIdx = assign->typeIndex;
                const char *varName = assign->variableNameToken.name;
                if (varName == nullptr) {
                    continue;
                }

                if (assign->expressionNode->vtable == VTables::NumberVTable) {
                    auto *numNode = Cast::downcast<NumberNodeStruct *>(assign->expressionNode);
                    bool isInt32 = dstTypeIdx == BuiltInTypeIndex::int32;
                    
                    snprintf(buf, sizeof(buf), isInt32 ? "    ldc.i4 %": "    ldc.i8 %" PRId64 "\n", numNode->num);
                    sb.append(buf);
                }
                else {
                    // Unsupported expression: push 0 as fallback
                    if (dstTypeIdx == BuiltInTypeIndex::int32) {
                        sb.appendWithAutoLength("    ldc.i4 0\n");
                    } else {
                        sb.appendWithAutoLength("    ldc.i8 0\n");
                    }
                }

                snprintf(buf, sizeof(buf), "    stloc.s %s\n", varName);
                sb.append(buf);
            }
            // Return statement: return <expr>
            else if (currentNode->vtable == VTables::ReturnStatementVTable) {
                auto *retNode = Cast::downcast<ReturnStatementNodeStruct *>(currentNode);

                if (retNode->expressionNode == nullptr) {
                    sb.appendWithAutoLength("    ldc.i8 0\n");
                }
                else if (retNode->expressionNode->vtable == VTables::NumberVTable) {
                    // return 100
                    auto *numNode = Cast::downcast<NumberNodeStruct *>(retNode->expressionNode);
                    snprintf(buf, sizeof(buf), "    ldc.i8 %" PRId64 "\n", numNode->num);
                    sb.append(buf);
                }
                else if (retNode->expressionNode->vtable == VTables::IdentifiersAccessVTable) {
                    // return a
                    auto *identNode = Cast::downcast<IdentifiersAccessNodeStruct *>(retNode->expressionNode);
                    const char *varName = identNode->identifierToken.name;

int32_t item;
                    if (varName != nullptr && (item = varTypeIndex.get(varName, (int)strlen(varName))) != 0) {
                        int srcTypeIdx = item - 1; // stored value is typeIndex + 1
                        snprintf(buf, sizeof(buf), "    ldloc.s %s\n", varName);
                        sb.append(buf);

                        if (srcTypeIdx == BuiltInTypeIndex::int32) {
                            // Widen i32 to i64 for the return value
                            sb.appendWithAutoLength("    conv.i8\n");
                        }
                    } else {
                        sb.appendWithAutoLength("    ldc.i8 0\n");
                    }
                } else {
                    sb.appendWithAutoLength("    ldc.i8 0\n");
                }

                sb.appendWithAutoLength("    ret\n");
                emittedRet = true;
            }
        }

        if (!emittedRet) {
            sb.appendWithAutoLength("    ldc.i8 0\n");
            sb.appendWithAutoLength("    ret\n");
        }

        sb.appendWithAutoLength("  }\n");
        sb.appendWithAutoLength("}\n");

        sbLocals.freeAll();
        varTypeIndexMemBuffer.freeAll();

        utf8byte *text = memBufferForText.newText((int)sb.length());
        memcpy(text, sb.str, sb.length());
        text[sb.length()] = '\0';
        sb.freeAll();
        return text;
    }
}
