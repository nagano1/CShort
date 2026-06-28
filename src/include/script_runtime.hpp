#pragma once

#include <stdlib.h>
//#include <array>
//
//#include <cstdlib>
//#include <cassert>
//#include <cstdio>
//#include <chrono>
//#include <unordered_map>
//
//#include <cstdint> // uint64_t, int_fast32_t
//#include <ctime>
//
//#include <string.h> // memcpy
//
#include "ParseUtil.hpp"
#include "common.hpp"
#include "parser.hpp"
#include "types.hpp"

namespace cshort
{
    struct _ScriptEnv;

    // general-purpose register	GPR
    enum class GRPRegisterEnum {
        eax, ebx, ecx, edx
    };

    // macros
    // pointer access to different parts of the register
    #define CS_RX(reg) (reg)->r_x
    #define CS_EX(reg) (reg)->e_x.e_x
    #define CS_X(reg) (reg)->e_x.ax.ax
    #define CS_H(reg) (reg)->e_x.ax.ahal.ah
    #define CS_L(reg) (reg)->e_x.ax.ahal.al

    // direct access to different parts of the register
    #define CS_RAX(reg) (reg).r_x
    #define CS_EAX(reg) (reg).e_x.e_x
    #define CS_AX(reg) (reg).e_x.ax.ax
    #define CS_AH(reg) (reg).e_x.ax.ahal.ah
    #define CS_AL(reg) (reg).e_x.ax.ahal.al

    // pointer access from CPUSim to different parts of the register
    #define RAX(cpu) CS_RAX((cpu)->rax)
    #define EAX(cpu) CS_EAX((cpu)->rax)
    #define AX(cpu) CS_AX((cpu)->rax)
    #define AH(cpu) CS_AH((cpu)->rax)
    #define AL(cpu) CS_AL((cpu)->rax)

    #define RBX(cpu) CS_RAX((cpu)->rbx)
    #define EBX(cpu) CS_EAX((cpu)->rbx)
    #define BX(cpu) CS_AX((cpu)->rbx)
    #define BH(cpu) CS_AH((cpu)->rbx)
    #define BL(cpu) CS_AL((cpu)->rbx)

    #define RCX(cpu) CS_RAX((cpu)->rcx)
    #define ECX(cpu) CS_EAX((cpu)->rcx)
    #define CX(cpu) CS_AX((cpu)->rcx)
    #define CH(cpu) CS_AH((cpu)->rcx)
    #define CL(cpu) CS_AL((cpu)->rcx)

    #define RDX(cpu) CS_RAX((cpu)->rdx)
    #define EDX(cpu) CS_EAX((cpu)->rdx)
    #define DX(cpu) CS_AX((cpu)->rdx)
    #define DH(cpu) CS_AH((cpu)->rdx)
    #define DL(cpu) CS_AL((cpu)->rdx)

    union GPRRegister {
        uint64_t r_x; // rax, rbx, rcx
        union {
            uint32_t e_x; // eax, ebx, ecx
            union {
                uint16_t ax; // ax, bx, cx
                struct {
                    uint8_t al; // al, bl, cl
                    uint8_t ah; // ah, bh, ch
                } ahal; // ah&al, bh&bl, ch&cl
            } ax;
        } e_x;
    };

    /*
          64bit RAX, RBX, RCX, RDX, RSI, RDI, RSP, RBP, R8~R15
          32bit EAX, EBX, ECX, EDX, ESI, EDI, ESP, EBP, R8D~R15D
          16bit AX, BX, CX, DX, SI, DI, SP, BP, R8W~R15W
    upper 8bit 	AH, BH, CH, DH,
    lower 8bit 	AL, BL, CL, DL, SIL, DIL, SPL, BPL, R8L~R15L
    */


    using CPUSim = struct _CPUSim {
        GPRRegister rax;
        GPRRegister rbx;
        GPRRegister rcx;
        GPRRegister rdx;
    };


    static inline const GPRRegister* GetGPRRegisterByEnum(GRPRegisterEnum regTypeEnum, const CPUSim* cpu)
    {
        if (regTypeEnum == GRPRegisterEnum::eax) {
            return &cpu->rax;
        }
        else if (regTypeEnum == GRPRegisterEnum::ebx) {
            return &cpu->rbx;
        }
        else if (regTypeEnum == GRPRegisterEnum::ecx) {
            return &cpu->rcx;
        }
        else if (regTypeEnum == GRPRegisterEnum::edx) {
            return  &cpu->rdx;
        }
        return nullptr;
    }

    static inline st_byte* GetDataPointerFromGPRRegister(const GPRRegister* gpr, int dataSize)
    {
        if (dataSize == 4) {
            return (st_byte*)&CS_EX(gpr);
        }
        else if (dataSize == 8) {
            return (st_byte*)&CS_RX(gpr);
        }

        return nullptr;
    }

    using BinaryOperationResult = struct _BinaryOperationResult {
        GRPRegisterEnum calcRegEnum;
        st_byte *calcReg;
        int typeIndex;
        bool typeAtHeap;
    };

    ///
    /// Simulation of stack memory for the script engine.
    ///
    using StackMemory = struct _StackMemory {
        int baseBytes; // 8 for 64bit, 4 for 32bit

        st_byte *stackChunk;
        int stackSize; // 2MB

        bool isOverflowed;

        // func(55, c:48) 0b101000...  for func(int a, int b = 32, int c = 8) // 32
        uint32_t argumentBits;

        bool useBigStructForReturnValue{false};

        st_byte *stackPointer; // esp, stack pointer, this points to the top of the stack
        st_byte *stackBasePointer; // ebp, stack base pointer. this points to the base of the current stack frame (function call)

        uint64_t returnValue; // EAX, Accumulator Register

        void init();
        void freeAll();
        void push(uint64_t data);
        uint64_t pop();
        void localVariables(int bytes); // assign variable on local
        void call();
        void ret();

        bool moveToStack(int offsetFromBase, int byteCount, st_byte* ptr);
        bool moveFromStack(int offsetFromBase, int byteCount, st_byte* ptr) const;

        void overflowed() {
            this->isOverflowed = true;
        }
    };


    // value base is used for storing values of variables, literals, and temporary results during expression evaluation.
    using ValueBase = struct _valueBase {
        int typeIndex;
        void* ptr;
        unsigned int size; // in byte
    };


    using ScriptEngineContext = struct _scriptEngineContext {
        _ScriptEnv* scriptEnv;
        CPUSim cpuRegister;

        SemanticErrorInfo semanticErrorInfo;

        MemBuffer memBuffer; // for TypeEntry, variable->value map

        MemBuffer memBufferForValueBase; // for value base
        MemBuffer memBufferForHeap; // for value
        MemBuffer memBufferForError; // for value

        VoidHashMap *variableMap2;
        VoidHashMap *typeNameMap;
        StackMemory stackMemory;

        void evaluateExprNode(NodeBase* expressionNode);

        ValueBase *newValueForHeap();
        ValueBase *genValueBase(int type, int size, void *ptr);

        void init(_ScriptEnv *scriptEnv);

        // allocating methods for objects in script running, which will be freed all together after script execution finishes, this is more efficient than malloc/free for each object, and also easier to manage memory in the script engine.
        void* mallocHeapObject(int bytes) {
            return this->memBufferForHeap.mallocHeapEntry(bytes);
        }

        void freeHeapObject(void *ptr) {
            this->memBufferForHeap.freeHeapEntry(ptr);
        }

        void freeAll()
        {
            this->memBufferForHeap.freeAllHeapEntries();
            this->memBufferForHeap.freeAll();

            this->memBufferForValueBase.freeAll();
            this->memBufferForError.freeAll();
            this->memBuffer.freeAll();
            this->stackMemory.freeAll();
        }

        void setErrorPositions();

        void addErrorWithNode(ErrorIndex errorCode, void* nodeArg) {
            printf("semantic error: %s\n", getErrorMessage(errorCode));
            auto *node = Cast::upcast(nodeArg);
            assert(node->vtable != nullptr);

            auto &errorInfo = this->semanticErrorInfo;
            errorInfo.count++;
            errorInfo.hasError = true;
            auto *mem = this->memBufferForError.newMem<SemanticErrorItem>(1);
            mem->node = node;
            mem->codeErrorItem.errorIndex = errorCode;
            mem->codeErrorItem.linePos1 = -1;
            mem->next = nullptr;
            if (errorInfo.firstErrorItem == nullptr) {
                errorInfo.firstErrorItem = mem;
            }

            if (errorInfo.lastErrorItem == nullptr) {
                errorInfo.lastErrorItem = mem;
            }
            else {
                errorInfo.lastErrorItem->next = mem;
                errorInfo.lastErrorItem = mem;
            }

            mem->codeErrorItem.errorId = getErrorCode(errorCode);
            const char* reason = getErrorMessage(errorCode);
            if (reason == nullptr) {
                reason = "";
            }
            int len = (int)strlen(reason);
            mem->codeErrorItem.reasonLength = len < MAX_REASON_LENGTH ? len : MAX_REASON_LENGTH;
            memcpy(mem->codeErrorItem.reason, reason, mem->codeErrorItem.reasonLength);
            mem->codeErrorItem.reason[mem->codeErrorItem.reasonLength] = '\0';
        }
    };




    using ScriptEnv = struct _ScriptEnv {

        DocumentStruct* document;
        FuncDefNodeStruct* mainFunc;
        TypeEntry **typeEntryList;
        int typeEntryListCapacity;
        // next index to insert new type entry, which is also the count of type entries in the list
        int typeEntryListNextIndex;

        TypeEntry* getTypeEntryByIndex(int typeIndex) {
            assert(typeIndex >= 0 && typeIndex < this->typeEntryListNextIndex);
            return this->typeEntryList[typeIndex];
        }

        ScriptEngineContext *context;


        int typeFromNode(NodeBase *expressionNode);

        static void deleteScriptEnv(_ScriptEnv *doc);
        static _ScriptEnv *newScriptEnv();
        //TypeEntry *newTypeEntry() const;

        static int startScriptInternal(char* script, int byteLength);

        template<std::size_t SIZE>
        static int startScript(const char(&text)[SIZE])
        {
            return startScriptInternal((char*)text, SIZE - 1);
        }

        static _ScriptEnv* loadScript(char* script, int byteLength);
        void validateScript();
        void validateFuncDef(FuncDefNodeStruct* funcDefNode);
        int runScript();

        void registerTypeEntry(TypeEntry* typeEntry);

        void addTypeAliasEntity(TypeEntry* typeEntry, char *f3 , int length);
        template<std::size_t SIZE>
        void addTypeAlias(TypeEntry* typeEntry, const char(&f3)[SIZE]) {
            this->addTypeAliasEntity(typeEntry , (char*)f3, SIZE-1);
        }
    };


    /*
     *
    // 11111111 11111111 11111111 11111111
    static constexpr unsigned char BYTE_BIT_COUNTS[256]{
        0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
        4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
    };

    // EXPECT_EQ(4, GetSetBitsCount(0b1111));

    int GetSetBitsCount(uint32_t n)
    {
        auto counts = BYTE_BIT_COUNTS;
        return n <= 0xff ? counts[n]
            : n <= 0xffff ? counts[n & 0xff] + counts[n >> 8]
            : n <= 0xffffff ? counts[n & 0xff] + counts[(n >> 8) & 0xff] + counts[(n >> 16) & 0xff]
            : counts[n & 0xff] + counts[(n >> 8) & 0xff] + counts[(n >> 16) & 0xff] + counts[(n >> 24) & 0xff];
    }
     *
     */

}