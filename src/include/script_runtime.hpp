#pragma once

#include <stdlib.h>
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

    // get the pointer to the data in the register based on the data size (1, 2, 4, or 8 bytes)
    static inline st_byte* GetDataPointerFromGPRRegister(const GPRRegister* gpr, int dataSize)
    {
        if (dataSize == 1) {
            return (st_byte*)&CS_H(gpr);
        }
        else if (dataSize == 2) {
            return (st_byte*)&CS_X(gpr);
        }
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


    // ----------------------------------------------------------------------------
    //
    //                              Script Engine 
    //
    // ----------------------------------------------------------------------------

    using ScriptEngineContext = struct _scriptEngineContext {
        CPUSim cpuRegister;
        //DocumentStruct *document;

        MemBuffer memBufferForValueBase; // for value base
        MemBuffer memBufferForHeap; // for value

        StackMemory stackMemory;

        void evaluateExprNode(NodeBase* expressionNode);

        TypedValue *newTypedValueForHeap();
        TypedValue *generateTypedValue(int type, int size, void *ptr);

        ParseContext *parseContext;

        void init(ParseContext *context);

        // allocating methods for objects in script running, 
        // which will be freed all together after script execution finishes
        void* mallocHeapObject(int bytes) {
            return this->memBufferForHeap.mallocHeapEntry(bytes);
        }

        void freeHeapObject(void *ptr) {
            this->memBufferForHeap.freeHeapEntry(ptr);
        }

        void freeAll()
        {
            this->memBufferForHeap.freeAll();
            this->memBufferForValueBase.freeAll();
            this->stackMemory.freeAll();
        }
    };




    struct ScriptRunner {
        static int runScriptWithLength(const char* script, int byteLength);

        template<std::size_t SIZE>
        static int runScript(const char(&text)[SIZE])
        {
            return runScriptWithLength((char*)text, SIZE - 1);
        }
    };
}