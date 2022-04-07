#pragma once
#ifndef TKP_N64_CPU_H
#define TKP_N64_CPU_H
#include <cstdint>
#include <limits>
#include <array>
#include <vector>
#include <bit>
#include "n64_instruction.hxx"
#include "n64_types.hxx"
#define KB(x) (static_cast<size_t>(x << 10))
namespace TKPEmu::N64::Devices {
    // Bit hack to get signum of number (-1, 0 or 1)
    template <typename T> int sgn(T val) {
        return (T(0) < val) - (val < T(0));
    }
    constexpr static uint64_t OperationMasks[2] = {
        std::numeric_limits<uint32_t>::max(),
        std::numeric_limits<uint64_t>::max()
    };
    enum class OperatingMode {
        User,
        Supervisor,
        Kernel
    };
    class CPU {
    public:
        CPU();
    //private:
        using OpcodeFunctionPtr = void (CPU::*)();
        // To be used with OpcodeMasks (OpcodeMasks[mode64_])
        bool mode64_ = false;
        // The current instruction
        Instruction instr_;
        /// Registers
        // r0 is hardwired to 0, r31 is the link register
        std::array<MemDataUnionDW, 32> gpr_regs_;
        std::array<MemDataDouble, 32> fpr_regs_;
        std::array<MemDataUnionDW, 32> cp0_regs_;
        std::vector<MemDataUB> instr_cache_;
        std::vector<MemDataUB> data_cache_;
        MemDataUD pc_, hi_, lo_;
        MemDataBit llbit_;
        MemDataFloat fcr0_, fcr31_;

        void SPECIAL(), REGIMM(), J(), JAL(),
            BEQ(), BNE(), BLEZ(), BGTZ(),
            ADDI(), ADDIU(), SLTI(), SLTIU(),
            ANDI(), ORI(), XORI(), LUI(),
            CP0(), CP1(), CP2(), BEQL(), BNEL(),
            BLEZL(), BGTZL(), DADDI(), DADDIU(),
            LDL(), LDR(), LB(), LH(),
            LWL(), LW(), LBU(), LHU(),
            LWR(), LWU(), SB(), SH(),
            SWL(), SW(), SDL(), SDR(),
            SWR(), CACHE(), LL(), LLD() ,
            LWC1(), LWC2(),
            LDC1(), LDC2(), LD(), SC(), SWC1(),
            SWC2(), SCD(), SDC1(), SDC2(), SD();
        void BAD();

        inline MemDataUB m_load_b(MemAddr addr);
        inline MemDataUH m_load_h(MemAddr addr);
        inline MemDataUW m_load_w(MemAddr addr);
        inline MemDataUD m_load_d(MemAddr addr);

        inline void m_store_b(MemAddr addr, MemDataUB data);
        inline void m_store_h(MemAddr addr, MemDataUH data);
        inline void m_store_w(MemAddr addr, MemDataUW data);
        inline void m_store_d(MemAddr addr, MemDataUD data);

        uint32_t get_curr_ld_addr();
        // Maybe useless?
        // Function to read from CPUs internal 64-bit bus
        void internal_read(MemDataUD addr);
        // Function to write to CPUs internal 64-bit bus
        void internal_write(MemDataUD addr, MemDataUD data);
    };
        
}
#endif