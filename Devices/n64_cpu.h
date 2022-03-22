#pragma once
#ifndef TKPEMU_N64_CPU_H
#define TKPEMU_N64_CPU_H
#include <cstdint>
#include <limits>
#include <array>

namespace TKPEmu::N64::Devices {
    // Bit hack to get signum of number (-1, 0 or 1)
    template <typename T> int sgn(T val) {
        return (T(0) < val) - (val < T(0));
    }
    using Word = uint32_t;
    using DoubleWord = uint64_t;
    // General purpose registers. They occupy 32 or 64 bits based on the
    // operation mode
    using GPR = DoubleWord;
    // Floating point registers, they occupy 64 bits
    using FPR = double;
    // This union describes how the inner bits of the 32 bit instructions are used
    // Source: VR4300 manual, 1.4.3
    union Instruction {
        struct {
            uint8_t  op        : 6;
            uint8_t  rs        : 5;
            uint8_t  rt        : 5;
            uint16_t immediate : 16;
        } IType;
        struct {
            uint8_t  op        : 6;
            uint8_t  target    : 26;
        } JType;
        struct {
            uint8_t  op        : 6;
            uint8_t  rs        : 5;
            uint8_t  rt        : 5;
            uint8_t  rd        : 5;
            uint8_t  sa        : 5;
            uint8_t  func      : 6;
        } RType;
        uint32_t Full;
    };
    constexpr static uint64_t OpcodeMasks[2] = {
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
        // To be used with OpcodeMasks (OpcodeMasks[mode64_])
        bool mode64_ = false;

        /// Registers
        // r0 is hardwired to 0, r31 is the link register
        std::array<GPR, 32> gpr_regs_;
        std::array<FPR, 32> fpr_regs_;
        DoubleWord pc_, hi_, lo_;
        bool llbit_;
        float fcr0_, fcr31_;

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