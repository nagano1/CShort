#include <cstring>
#include <cstdint>
#include <cstdlib>

#include "compiler.hpp"

namespace cshort {

    // ---------------------------------------------------------------------------
    //
    //                            Compiler using Wasm
    //
    // ---------------------------------------------------------------------------

    // --- LEB128 helpers -------------------------------------------------------

    static void emit_leb128_u(BinaryDataBuilder &b, uint64_t val)
    {
        do {
            uint8_t byte = (uint8_t)(val & 0x7F);
            val >>= 7;
            if (val != 0) byte |= 0x80;
            b.append_byte(byte);
        } while (val != 0);
    }

    static void emit_leb128_s(BinaryDataBuilder &b, int64_t val)
    {
        bool more = true;
        while (more) {
            uint8_t byte = (uint8_t)(val & 0x7F);
            val >>= 7;
            if ((val == 0 && (byte & 0x40) == 0) || (val == -1 && (byte & 0x40) != 0)) {
                more = false;
            } else {
                byte |= 0x80;
            }
            b.append_byte(byte);
        }
    }

    // Emit a Wasm section: id + LEB128(content.size) + content bytes.
    static void emit_section(BinaryDataBuilder &out, uint8_t id, BinaryDataBuilder &content)
    {
        out.append_byte(id);
        emit_leb128_u(out, (uint64_t)content.size);
        out.append_bytes(content.data, content.size);
    }

    // --- Local variable table (name → index, parallel arrays) ----------------

    #define WASM_MAX_LOCALS 64

    static int find_local(const char * const *names, int count, const char *name)
    {
        if (name == nullptr) return -1;
        for (int i = 0; i < count; i++) {
            if (names[i] != nullptr && strcmp(names[i], name) == 0) return i;
        }
        return -1;
    }

    // --- Fallback module: () -> i64 { return 0 } ------------------------------

    static size_t emit_wasm_fallback(MemBuffer &memBuffer, uint8_t **outData)
    {
        static const uint8_t fallback[] = {
            // magic + version
            0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00,
            // type section (id=1, size=5): () -> i64
            0x01, 0x05, 0x01, 0x60, 0x00, 0x01, 0x7E,
            // function section (id=3, size=2): type[0]
            0x03, 0x02, 0x01, 0x00,
            // export section (id=7, size=8): "main" = func 0
            0x07, 0x08, 0x01, 0x04, 0x6D, 0x61, 0x69, 0x6E, 0x00, 0x00,
            // code section (id=10, size=6): 1 func body [local_count=0, i64.const 0, end]
            0x0A, 0x06, 0x01, 0x04, 0x00, 0x42, 0x00, 0x0B
        };
        size_t len = sizeof(fallback);
        uint8_t *buf = (uint8_t *)memBuffer.newText((unsigned int)len);
        memcpy(buf, fallback, len);
        *outData = buf;
        return len;
    }

    // --- Expression code generation -------------------------------------------

    // Forward declaration (mutual recursion between emit_expr and emit_expr_coerce).
    static void emit_expr(BinaryDataBuilder &b, NodeBase *expr,
                          const char * const *localNames,
                          const int *localTypes,
                          int localCount);

    // Emit an expression and then widen i32→i64 if the required type is i64
    // but the expression produced i32.
    static void emit_expr_coerce(BinaryDataBuilder &b, NodeBase *expr,
                                 int requiredTypeIdx,
                                 const char * const *localNames,
                                 const int *localTypes,
                                 int localCount)
    {
        emit_expr(b, expr, localNames, localTypes, localCount);
        if (requiredTypeIdx == BuiltInTypeIndex::int64 &&
            expr->typeIndex  == BuiltInTypeIndex::int32)
        {
            b.append_byte(0xAC); // i64.extend_i32_s
        }
    }

    static void emit_expr(BinaryDataBuilder &b, NodeBase *expr,
                          const char * const *localNames,
                          const int *localTypes,
                          int localCount)
    {
        if (expr->vtable == VTables::NumberVTable) {
            auto *numNode = Cast::downcast<NumberNodeStruct *>(expr);
            // unit==64 → i64 literal; otherwise → i32 literal.
            if (numNode->unit == 64 || numNode->typeIndex == BuiltInTypeIndex::int64) {
                b.append_byte(0x42); // i64.const
                emit_leb128_s(b, numNode->num);
            } else {
                b.append_byte(0x41); // i32.const
                emit_leb128_s(b, numNode->num);
            }
        } else if (expr->vtable == VTables::IdentifiersAccessVTable) {
            auto *identNode = Cast::downcast<IdentifiersAccessNodeStruct *>(expr);
            int idx = find_local(localNames, localCount, identNode->identifierToken.name);
            b.append_byte(0x20); // local.get
            emit_leb128_u(b, (uint64_t)(idx >= 0 ? idx : 0));
        } else if (expr->vtable == VTables::BinaryOperationVTable) {
            auto *binNode = Cast::downcast<BinaryOperationNodeStruct *>(expr);
            int resultType = binNode->typeIndex;
            if (resultType != BuiltInTypeIndex::int32 && resultType != BuiltInTypeIndex::int64) {
                resultType = BuiltInTypeIndex::int64; // safe default
            }
            // Emit operands, widening i32→i64 if the operation is i64.
            emit_expr_coerce(b, binNode->leftExprNode,  resultType, localNames, localTypes, localCount);
            emit_expr_coerce(b, binNode->rightExprNode, resultType, localNames, localTypes, localCount);
            bool is64 = (resultType == BuiltInTypeIndex::int64);
            switch (binNode->binaryOp) {
                case BinaryOperator::Add:      b.append_byte(is64 ? 0x7C : 0x6A); break;
                case BinaryOperator::Subtract: b.append_byte(is64 ? 0x7D : 0x6B); break;
                case BinaryOperator::Multiply: b.append_byte(is64 ? 0x7E : 0x6C); break;
                case BinaryOperator::Divide:   b.append_byte(is64 ? 0x7F : 0x6D); break;
                default:
                    // Unsupported operator: push 0 as a safe fallback.
                    b.append_byte(0x42);
                    emit_leb128_s(b, 0);
                    break;
            }
        } else if (expr->vtable == VTables::ParenthesesVTable) {
            auto *parenNode = Cast::downcast<ParenthesesNodeStruct *>(expr);
            emit_expr(b, parenNode->valueNode, localNames, localTypes, localCount);
        } else {
            // Unknown expression: emit i64.const 0 as fallback.
            b.append_byte(0x42);
            emit_leb128_s(b, 0);
        }
    }

    // --- Public API -----------------------------------------------------------

    char *CompilerForWasm::compile(DocumentStruct * /*document*/,
                                   MemBuffer       & /*memBufferForText*/)
    {
        return nullptr;
    }

    size_t CompilerForWasm::compileToBytes(DocumentStruct *document,
                                           MemBuffer      &memBufferForText,
                                           uint8_t       **outData)
    {
        FuncDefNodeStruct *mainFunc = document->mainFunc;
        if (mainFunc == nullptr) {
            return emit_wasm_fallback(memBufferForText, outData);
        }

        const char *funcName = mainFunc->funcNameToken.name;
        if (funcName == nullptr || strcmp(funcName, "Main") != 0) {
            return emit_wasm_fallback(memBufferForText, outData);
        }

        // Pre-pass: collect local variables in declaration order.
        const char *localNames[WASM_MAX_LOCALS];
        int         localTypes[WASM_MAX_LOCALS];
        int         localCount = 0;

        auto *stmtNode = mainFunc->bodyNode.firstChildNode;
        while (stmtNode != nullptr) {
            if (stmtNode->vtable == VTables::AssignmentVTable) {
                auto *assign = Cast::downcast<AssignmentNodeStruct *>(stmtNode);
                int typeIdx = assign->typeIndex;
                if (typeIdx != BuiltInTypeIndex::int32 && typeIdx != BuiltInTypeIndex::int64) {
                    return emit_wasm_fallback(memBufferForText, outData);
                }
                if (localCount < WASM_MAX_LOCALS) {
                    localNames[localCount] = assign->variableNameToken.name;
                    localTypes[localCount] = typeIdx;
                    localCount++;
                }
            }
            stmtNode = stmtNode->nextNode;
        }

        // --- Build Wasm binary ------------------------------------------------
        BinaryDataBuilder module;
        module.init(512);

        // Magic + version
        static const uint8_t header[] = {
            0x00, 0x61, 0x73, 0x6D,   // magic
            0x01, 0x00, 0x00, 0x00    // version 1
        };
        module.append_bytes(header, 8);

        // Type section (id=1): one functype () -> i64
        {
            BinaryDataBuilder sec;
            sec.init(16);
            emit_leb128_u(sec, 1);    // 1 type entry
            sec.append_byte(0x60);    // functype tag
            sec.append_byte(0x00);    // 0 params
            sec.append_byte(0x01);    // 1 result
            sec.append_byte(0x7E);    // i64
            emit_section(module, 1, sec);
            sec.freeAll();
        }

        // Function section (id=3): func 0 → type 0
        {
            BinaryDataBuilder sec;
            sec.init(8);
            emit_leb128_u(sec, 1);    // 1 function
            emit_leb128_u(sec, 0);    // type index 0
            emit_section(module, 3, sec);
            sec.freeAll();
        }

        // Export section (id=7): "main" = func 0
        {
            BinaryDataBuilder sec;
            sec.init(16);
            emit_leb128_u(sec, 1);    // 1 export
            sec.append_byte(0x04);    // name length = 4
            static const uint8_t mainName[] = {'m', 'a', 'i', 'n'};
            sec.append_bytes(mainName, 4);
            sec.append_byte(0x00);    // export kind: func
            emit_leb128_u(sec, 0);    // func index 0
            emit_section(module, 7, sec);
            sec.freeAll();
        }

        // Code section (id=10): one function body
        {
            BinaryDataBuilder body;
            body.init(256);

            // Local variable declarations (one group per local, each of count 1)
            emit_leb128_u(body, (uint64_t)localCount);
            for (int i = 0; i < localCount; i++) {
                emit_leb128_u(body, 1);  // count = 1
                body.append_byte(localTypes[i] == BuiltInTypeIndex::int32 ? 0x7F : 0x7E);
            }

            // Emit statement instructions
            bool emittedRet = false;
            stmtNode = mainFunc->bodyNode.firstChildNode;
            while (stmtNode != nullptr) {
                if (stmtNode->vtable == VTables::AssignmentVTable) {
                    auto *assign = Cast::downcast<AssignmentNodeStruct *>(stmtNode);
                    if (assign->expressionNode != nullptr) {
                        int idx = find_local(localNames, localCount,
                                             assign->variableNameToken.name);
                        emit_expr_coerce(body, assign->expressionNode, assign->typeIndex,
                                         localNames, localTypes, localCount);
                        body.append_byte(0x21); // local.set
                        emit_leb128_u(body, (uint64_t)(idx >= 0 ? idx : 0));
                    }
                } else if (stmtNode->vtable == VTables::ReturnStatementVTable) {
                    auto *retNode = Cast::downcast<ReturnStatementNodeStruct *>(stmtNode);
                    if (retNode->expressionNode != nullptr) {
                        // Always widen to i64 for the function result.
                        emit_expr_coerce(body, retNode->expressionNode,
                                         BuiltInTypeIndex::int64,
                                         localNames, localTypes, localCount);
                    } else {
                        body.append_byte(0x42); // i64.const 0
                        emit_leb128_s(body, 0);
                    }
                    body.append_byte(0x0F); // return
                    emittedRet = true;
                }
                stmtNode = stmtNode->nextNode;
            }

            if (!emittedRet) {
                body.append_byte(0x42); // i64.const 0
                emit_leb128_s(body, 0);
            }
            body.append_byte(0x0B); // end

            // Wrap into code section content: count=1 + body_size + body bytes
            BinaryDataBuilder sec;
            sec.init(body.size + 16);
            emit_leb128_u(sec, 1);                          // 1 function body
            emit_leb128_u(sec, (uint64_t)body.size);        // body byte count
            sec.append_bytes(body.data, body.size);
            body.freeAll();

            emit_section(module, 0x0A, sec);
            sec.freeAll();
        }

        // Name section (custom section id=0): function and local variable names
        // for debugger support (Wasm spec §binary-namesec).
        {
            static const uint8_t nameStr[] = {'n', 'a', 'm', 'e'}; // custom section id string
            static const uint8_t mainStr[] = {'m', 'a', 'i', 'n'}; // function name

            // Subsection 1 – function names: func 0 = "main"
            BinaryDataBuilder fnNameBody;
            fnNameBody.init(16);
            emit_leb128_u(fnNameBody, 1);          // 1 entry
            emit_leb128_u(fnNameBody, 0);          // func index 0
            emit_leb128_u(fnNameBody, 4);          // length of "main"
            fnNameBody.append_bytes(mainStr, 4);

            BinaryDataBuilder nameSec;
            nameSec.init(256);

            // Custom section name must be exactly "name" (0x6E 0x61 0x6D 0x65)
            emit_leb128_u(nameSec, 4);
            nameSec.append_bytes(nameStr, 4);

            // Emit function-names subsection (id=1)
            nameSec.append_byte(0x01);
            emit_leb128_u(nameSec, (uint64_t)fnNameBody.size);
            nameSec.append_bytes(fnNameBody.data, fnNameBody.size);
            fnNameBody.freeAll();

            // Emit local-names subsection (id=2) only when there are named locals
            if (localCount > 0) {
                BinaryDataBuilder localNameBody;
                localNameBody.init(64 + (size_t)localCount * 32);
                emit_leb128_u(localNameBody, 1);   // 1 function entry (func 0)
                emit_leb128_u(localNameBody, 0);   // func index 0
                emit_leb128_u(localNameBody, (uint64_t)localCount);
                for (int i = 0; i < localCount; i++) {
                    emit_leb128_u(localNameBody, (uint64_t)i);
                    const char *n = localNames[i] != nullptr ? localNames[i] : "";
                    size_t nlen = strlen(n);
                    emit_leb128_u(localNameBody, (uint64_t)nlen);
                    localNameBody.append_bytes((const uint8_t *)n, nlen);
                }
                nameSec.append_byte(0x02);
                emit_leb128_u(nameSec, (uint64_t)localNameBody.size);
                nameSec.append_bytes(localNameBody.data, localNameBody.size);
                localNameBody.freeAll();
            }

            emit_section(module, 0x00, nameSec);
            nameSec.freeAll();
        }

        // Copy the finished module into the MemBuffer and return.
        size_t totalSize = module.size;
        uint8_t *buf = (uint8_t *)memBufferForText.newText((unsigned int)totalSize);
        memcpy(buf, module.data, totalSize);
        module.freeAll();

        *outData = buf;
        return totalSize;
    }

}