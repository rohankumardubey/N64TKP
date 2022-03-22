#pragma once
#ifndef TKPEMU_N64_CPU_H
#define TKPEMU_N64_CPU_H
#include <cstdint>
#include <limits>
#include <array>
#include "n64_instruction.h"

namespace TKPEmu::N64::Devices {
    using Byte = uint8_t;
    using HalfWord = uint16_t;
    using Word = uint32_t;
    using DoubleWord = uint64_t;
    // General purpose registers. They occupy 32 or 64 bits based on the
    // operation mode
    using GPR = DoubleWord;
    // Floating point registers, they occupy 64 bits
    using FPR = double;
    // Bit hack to get signum of number (-1, 0 or 1)
    template <typename T> int sgn(T val) {
        return (T(0) < val) - (val < T(0));
    }
    constexpr static uint64_t OperationMasks[2] = {
        std::numeric_limits<uint32_t>::max(),
        std::numeric_limits<uint64_t>::max()
    };
    // This union allows you to access the 9th 'G'pu bit or the isolated global 8 'B'its
    union RAMByte {
        struct {
            bool gpu_bit : 1;
            uint8_t data : 8;
        } sliced;
        uint16_t full;
    };
    class CPU {
    public:
    private:
        using OpcodeFunctionPtr = void (CPU::*)();
        // To be used with OpcodeMasks (OpcodeMasks[mode64_])
        bool mode64_ = false;

        /// Registers
        // r0 is hardwired to 0, r31 is the link register
        std::array<GPR, 32> gpr_regs_;
        std::array<FPR, 32> fpr_regs_;
        DoubleWord pc_, hi_, lo_;
        bool llbit_;
        float fcr0_, fcr31_;
        void SPECIAL(), REGIMM(), J(), JAL(),
            BEQ(), BNE(), BLEZ(), BGTZ(),
            ADDI(), ADDIU(), SLTI(), SLTIU(),
            ANDI(), ORI(), XORI(), LUI(),
            CP0(), CP1(), BEQL(), BNEL(),
            BLEZL(), BGTZL(), DADDI(), DADDIU(),
            LDL(), LDR(), LB(), LH(),
            LWL(), LW(), LBU(), LHU(),
            LWR(), LWU(), SB(), SH(),
            SWL(), SW(), SDL(), SDR(),
            SWR(), CACHE(), LL(), LWC1(),
            LDC1(), LD(), SC(), SWC1(),
            SDC1(), SDC2(), SD();
        const std::array<OpcodeFunctionPtr, 64> opcodes_ = {
            &CPU::SPECIAL, &CPU::REGIMM, &CPU::J, &CPU::JAL,
            &CPU::BEQ, &CPU::BNE, &CPU::BLEZ, &CPU::BGTZ,
            &CPU::ADDI, &CPU::ADDIU, &CPU::SLTI, &CPU::SLTIU,
            &CPU::ANDI, &CPU::ORI, &CPU::XORI, &CPU::LUI,
            &CPU::CP0, &CPU::CP1, &CPU::BEQL, &CPU::BNEL,
            &CPU::BLEZL, &CPU::BGTZL, &CPU::DADDI, &CPU::DADDIU,
            &CPU::LDL, &CPU::LDR, &CPU::LB, &CPU::LH,
            &CPU::LWL, &CPU::LW, &CPU::LBU, &CPU::LHU,
            &CPU::LWR, &CPU::LWU, &CPU::SB, &CPU::SH,
            &CPU::SWL, &CPU::SW, &CPU::SDL, &CPU::SDR,
            &CPU::SWR, &CPU::CACHE, &CPU::LL, &CPU::LWC1,
            &CPU::LDC1, &CPU::LD, &CPU::SC, &CPU::SWC1,
            &CPU::SDC1, &CPU::SDC2, &CPU::SD
        };

        // Functions to get or set a GPR that deal with r0
        inline GPR& get_gpr(int index);
        inline GPR& set_gpr(int index, GPR data);
        // Function to read from CPUs internal 64-bit bus
        void internal_read(DoubleWord addr);
        // Function to write to CPUs internal 64-bit bus
        void internal_write(DoubleWord addr, DoubleWord data);
    };
        
}
#endif