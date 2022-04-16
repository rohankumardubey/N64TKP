#pragma once
#ifndef TKP_N64_CPU_H
#define TKP_N64_CPU_H
#include <cstdint>
#include <limits>
#include <array>
#include <queue>
#include <vector>
#include <bit>
#include <memory>
#include <optional>
#include "n64_types.hxx"
#include "n64_cpu_exceptions.hxx"

#define KB(x) (static_cast<size_t>(x << 10))
#define check_bit(x, y) ((x) & (1u << y))
constexpr uint32_t KSEG0_START = 0x8000'0000;
constexpr uint32_t KSEG0_END   = 0x9FFF'FFFF;
constexpr uint32_t KSEG1_START = 0xA000'0000;
constexpr uint32_t KSEG1_END   = 0xBFFF'FFFF;

namespace TKPEmu::N64::Devices {
    // Bit hack to get signum of number (-1, 0 or 1)
    template <typename T> int sgn(T val) {
        return (T(0) < val) - (val < T(0));
    }
    constexpr static uint64_t OperationMasks[2] = {
        std::numeric_limits<uint32_t>::max(),
        std::numeric_limits<uint64_t>::max()
    };
    // Only kernel mode is used for (most?) n64 licensed games
    enum class OperatingMode {
        User,
        Supervisor,
        Kernel
    };
    enum class PipelineStage {
        IC = 0, // Instruction cache fetch
        RF = 1, // Register fetch
        EX = 2, // Execution
        DC = 3, // Data cache fetch
        WB = 4, // Write back
        NOP // No operation
    };
    struct PipelineStorage {
        Instruction instruction;
        InstructionType type;
    };
    /**
        32-bit address bus 
        
        Source: https://n64brew.dev/wiki/Memory_map     
    */
    class CPUBus {
    public:
        bool LoadFromFile(std::string path);
    private:
        std::vector<uint8_t> rom_;
        friend class CPU;
    };
    class CPU {
    public:
        CPU();
        void Reset();
    private:
        using DirectMapRet = uint32_t;
        using DirectMapArgs = uint32_t;
        using TLBRet = uint32_t;
        using TLBArgs = uint32_t;
        using SelAddrSpace32Ret = uint32_t;
        using SelAddrSpace32Args = uint32_t;
        using PipelineStageRet  = void;
        using PipelineStageArgs = size_t;
        // To be used with OpcodeMasks (OpcodeMasks[mode64_])
        bool mode64_ = false;
        // The current instruction
        Instruction instr_;
        /// Registers
        // r0 is hardwired to 0, r31 is the link register
        std::array<MemDataUnionDW, 32> gpr_regs_;
        std::array<MemDataDouble, 32> fpr_regs_;
        std::array<MemDataUnionDW, 32> cp0_regs_;
        // CPU cache
        std::vector<MemDataUB> instr_cache_;
        std::vector<MemDataUB> data_cache_;
        // Special registers
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

        // Kernel mode addressing functions
        /**
            VR4300 manual, page 122: 

            There are virtual-to-physical address translations that occur outside of the TLB. For example, 
            addresses in the kseg0 and kseg1 spaces are unmapped translations. In these spaces the physical 
            address is derived by subtracting the base address of the space from the virtual address.
            @param addr virtual address, range 0x80000000-0x9fffffff
            @return physical address 
        */
        inline DirectMapRet translate_kseg0(DirectMapArgs addr) noexcept;
        /**
            VR4300 manual, page 122: 

            There are virtual-to-physical address translations that occur outside of the TLB. For example, 
            addresses in the kseg0 and kseg1 spaces are unmapped translations. In these spaces the physical 
            address is derived by subtracting the base address of the space from the virtual address.
            @param addr virtual address, range 0xa0000000-0xbfffffff
            @return physical address 
        */
        inline DirectMapRet translate_kseg1(DirectMapArgs addr) noexcept;
        /**
            
            @param addr virtual address, range 0x00000000-0x7fffffff
            @return physical address
        */
        inline TLBRet translate_kuseg(TLBArgs);

        SelAddrSpace32Ret select_addr_space32(SelAddrSpace32Args);

        std::array<std::queue<PipelineStage>, 5> pipeline_;
        std::array<PipelineStorage, 5>           pipeline_storage_;

        PipelineStageRet IC(PipelineStageArgs process_no);
        PipelineStageRet RF(PipelineStageArgs process_no);
        PipelineStageRet EX(PipelineStageArgs process_no);
        PipelineStageRet DC(PipelineStageArgs process_no) {};
        PipelineStageRet WB(PipelineStageArgs process_no) {};

        bool execute_instruction(PipelineStage stage, size_t process_no);
        void update_pipeline();
    };
}
#endif