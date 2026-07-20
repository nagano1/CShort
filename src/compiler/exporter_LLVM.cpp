#include <cstdio>
#include <iostream>
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

#include "compiler.hpp"

namespace cshort {

    // ---------------------------------------------------------------------------
    //
    //                            Compiler using LLVM
    //
    // ---------------------------------------------------------------------------

    // Returns the LLVM IR type name for a given CShort typeIndex.
    static const char *llvmTypeName(int typeIndex) {
        if (typeIndex == BuiltInTypeIndex::int64) return "i64";
        if (typeIndex == BuiltInTypeIndex::int32) return "i32";
        return "i64";
    }

    // Writes the IR string into a context-allocated buffer and returns it.
    static char *irToText(const std::string &ir, ParseContext *context) {
        utf8byte *text = context->newText((int)ir.size());
        memcpy(text, ir.c_str(), ir.size());
        text[ir.size()] = '\0';
        return text;
    }

    // Safe fallback: a minimal function that returns 0.
    static char *emitFallback(ParseContext *context) {
        return irToText("define i64 @main() {\nentry:\n  ret i64 0\n}\n", context);
    }

    char *CompilerForLLVM::compile(DocumentStruct *document, ParseContext *context) {
        // Return safe fallback on parse/semantic errors
        if (context->syntaxErrorInfo.hasError || context->semanticErrorInfo.hasError) {
            return emitFallback(context);
        }

        FuncDefNodeStruct *mainFunc = document->mainFunc;
        if (mainFunc == nullptr) {
            return emitFallback(context);
        }

        // Only fn Main() is supported as the entry point
        const char *funcName = mainFunc->funcNameToken.name;
        if (funcName == nullptr || strcmp(funcName, "Main") != 0) {
            return emitFallback(context);
        }

        StringBuilder sb;
        sb.initWithInitialCapacity(1024);
        sb.appendWithAutoLength("define i64 @main() {\n");
        sb.appendWithAutoLength("entry:\n");

        // Track type of each named local variable (varName -> typeIndex)
        std::unordered_map<std::string, int> varTypeIndex;

        auto *statementNode = mainFunc->bodyNode.firstChildNode;
        bool emittedRet = false;
        char buf[256];

        while (statementNode != nullptr) {
            // Assignment: i64 a = 100
            if (statementNode->vtable == VTables::AssignmentVTable) {
                auto *assign = Cast::downcast<AssignmentNodeStruct *>(statementNode);

                if (assign->expressionNode == nullptr) {
                    statementNode = statementNode->nextNode;
                    continue;
                }

                int dstTypeIdx = assign->typeIndex;

                // Only integer types are supported
                if (dstTypeIdx != BuiltInTypeIndex::int64 && dstTypeIdx != BuiltInTypeIndex::int32) {
                    return emitFallback(context);
                }

                const char *varName = assign->variableNameToken.name;
                if (varName == nullptr) {
                    statementNode = statementNode->nextNode;
                    continue;
                }

                const char *dstType = llvmTypeName(dstTypeIdx);
                varTypeIndex[std::string(varName)] = dstTypeIdx;

                // alloca
                snprintf(buf, sizeof(buf), "  %%%s = alloca %s\n", varName, dstType);
                sb.append(buf);

                // store
                if (assign->expressionNode->vtable == VTables::NumberVTable) {
                    auto *numNode = Cast::downcast<NumberNodeStruct *>(assign->expressionNode);
                    snprintf(buf, sizeof(buf), "  store %s %" PRId64 ", %s* %%%s\n",
                             dstType, numNode->num, dstType, varName);
                    sb.append(buf);
                } else {
                    // Unsupported expression: store 0 as fallback value
                    snprintf(buf, sizeof(buf), "  store %s 0, %s* %%%s\n", dstType, dstType, varName);
                    sb.append(buf);
                }
            }
            // Return statement: return <expr>
            else if (statementNode->vtable == VTables::ReturnStatementVTable) {
                auto *retNode = Cast::downcast<ReturnStatementNodeStruct *>(statementNode);

                if (retNode->expressionNode == nullptr) {
                    sb.append("  ret i64 0\n");
                } else if (retNode->expressionNode->vtable == VTables::NumberVTable) {
                    // return 100
                    auto *numNode = Cast::downcast<NumberNodeStruct *>(retNode->expressionNode);
                    snprintf(buf, sizeof(buf), "  ret i64 %" PRId64 "\n", numNode->num);
                    sb.append(buf);
                } else if (retNode->expressionNode->vtable == VTables::IdentifiersAccessVTable) {
                    // return a
                    auto *identNode = Cast::downcast<IdentifiersAccessNodeStruct *>(retNode->expressionNode);
                    const char *varName = identNode->identifierToken.name;
                    auto it = varName ? varTypeIndex.find(std::string(varName)) : varTypeIndex.end();

                    if (varName != nullptr && it != varTypeIndex.end()) {
                        int srcTypeIdx = it->second;
                        const char *srcType = llvmTypeName(srcTypeIdx);

                        snprintf(buf, sizeof(buf), "  %%%s_load = load %s, %s* %%%s\n",
                                 varName, srcType, srcType, varName);
                        sb.append(buf);

                        if (srcTypeIdx == BuiltInTypeIndex::int32) {
                            // Widen i32 to i64 for the return value
                            snprintf(buf, sizeof(buf), "  %%ret_ext = sext i32 %%%s_load to i64\n", varName);
                            sb.append(buf);
                            sb.append("  ret i64 %ret_ext\n");
                        } else {
                            snprintf(buf, sizeof(buf), "  ret i64 %%%s_load\n", varName);
                            sb.append(buf);
                        }
                    } else {
                        sb.append("  ret i64 0\n");
                    }
                } else {
                    sb.append("  ret i64 0\n");
                }

                emittedRet = true;
            }

            statementNode = statementNode->nextNode;
        }

        if (!emittedRet) {
            sb.append("  ret i64 0\n");
        }

        sb.append("}\n");

        return irToText(sb.c_str(), context);
    }
}