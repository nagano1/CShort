#pragma once

#include <stdlib.h>

#include "ParseUtil.hpp"
#include "common.hpp"
#include "code_nodes.hpp"

namespace cshort
{
    enum class PrimitiveCalcRegisterEnum {
        eax, ebx, ecx, edx
    };

    // macros for CalcRegister
    #define __RX(reg) (reg)->r_x
    #define __EX(reg) (reg)->e_x.e_x
    #define __X(reg) (reg)->e_x.ax.ax
    #define __H(reg) (reg)->e_x.ax.ahal.ah
    #define __L(reg) (reg)->e_x.ax.ahal.al

    #define __RAX(reg) (reg).r_x
    #define __EAX(reg) (reg).e_x.e_x
    #define __AX(reg) (reg).e_x.ax.ax
    #define __AH(reg) (reg).e_x.ax.ahal.ah
    #define __AL(reg) (reg).e_x.ax.ahal.al

    // macros for CPU Register
    #define RAX(cpu) __RAX((cpu)->rax)
    #define EAX(cpu) __EAX((cpu)->rax)
    #define AX(cpu) __AX((cpu)->rax)
    #define AH(cpu) __AH((cpu)->rax)
    #define AL(cpu) __AL((cpu)->rax)

    #define RBX(cpu) __RAX((cpu)->rbx)
    #define EBX(cpu) __EAX((cpu)->rbx)
    #define BX(cpu) __AX((cpu)->rbx)
    #define BH(cpu) __AH((cpu)->rbx)
    #define BL(cpu) __AL((cpu)->rbx)

    #define RCX(cpu) __RAX((cpu)->rcx)
    #define ECX(cpu) __EAX((cpu)->rcx)
    #define CX(cpu) __AX((cpu)->rcx)
    #define CH(cpu) __AH((cpu)->rcx)
    #define CL(cpu) __AL((cpu)->rcx)

    #define RDX(cpu) __RAX((cpu)->rdx)
    #define EDX(cpu) __EAX((cpu)->rdx)
    #define DX(cpu) __AX((cpu)->rdx)
    #define DH(cpu) __AH((cpu)->rdx)
    #define DL(cpu) __AL((cpu)->rdx)

    union CalcRegister {
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


    using CPURegister = struct _CPURegister {
        CalcRegister rax;
        CalcRegister rbx;
        CalcRegister rcx;
        CalcRegister rdx;

        void add(int num) {
            auto *reg = this;
            RAX(reg) = RAX(reg) + num;
        }
    };


    static inline const CalcRegister* CalcEnumToCalcRegister(PrimitiveCalcRegisterEnum regTypeEnum, const CPURegister* cpu)
    {
        if (regTypeEnum == PrimitiveCalcRegisterEnum::eax) {
            return &cpu->rax;
        }
        else if (regTypeEnum == PrimitiveCalcRegisterEnum::ebx) {
            return &cpu->rbx;
        }
        else if (regTypeEnum == PrimitiveCalcRegisterEnum::ecx) {
            return &cpu->rcx;
        }
        else if (regTypeEnum == PrimitiveCalcRegisterEnum::edx) {
            return  &cpu->rdx;
        }
        return nullptr;
    }

    static inline st_byte* GetDataPointerFromCalcRegister(const CalcRegister* calcRegister, int dataSize)
    {
        if (dataSize == 4) {
            return (st_byte*)&__EX(calcRegister);
        }
        else if (dataSize == 8) {
            return (st_byte*)&__RX(calcRegister);
        }

        return nullptr;
    }

    using StackMemory = struct _StackMemory {
        int baseBytes; // 8 for 64bit, 4 for 32bit; 8 is used for alignment of local variables, return address, base pointer, and arguments

        st_byte *chunk;
        int stackSize; // 2MB

        bool isOverflowed;

        st_byte *stackPointer; // esp, stack pointer
        st_byte *stackBasePointer; // ebp, stack base pointer

        uint64_t returnValue; // EAX, Accumulator Register

        // methods
        void init();
        void freeAll();
        void push(uint64_t bytes);
        void localVariables(int bytes); // assign variable on local
        uint64_t pop();
        void call();
        void ret();

        void overflowed() {
            this->isOverflowed = true;
        }

        void moveTo(int offsetFromBase, int byteCount, st_byte* ptr);
        void moveFrom(int offsetFromBase, int byteCount, st_byte* ptr) const;
    };
}