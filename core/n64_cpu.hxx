#pragma once
#ifndef TKP_N64_CPU_H
#define TKP_N64_CPU_H
#include <cstdint>
#include <limits>
#include <array>
#include <vector>
#include <bit>
#include <memory>
#include "n64_types.hxx"
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
    class CPU {
    public:
        CPU();
    private:
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

        using DirectMapret = uint32_t;
        using DirectMapargs = uint32_t;
        using TLBret = uint32_t;
        using TLBargs = uint32_t;
        using SelAddrSpace32ret = void;
        using SelAddrSpace32args = uint32_t;

        // Kernel mode addressing functions
        /**
            VR4300 manual, page 122: 

            There are virtual-to-physical address translations that occur outside of the TLB. For example, 
            addresses in the kseg0 and kseg1 spaces are unmapped translations. In these spaces the physical 
            address is derived by subtracting the base address of the space from the virtual address.
            @param addr virtual address, range 0x80000000-0x9fffffff
            @return physical address 
        */
        inline DirectMapret translate_kseg0(DirectMapargs addr) {
            return addr - KSEG0_START;
        }
        /**
            VR4300 manual, page 122: 

            There are virtual-to-physical address translations that occur outside of the TLB. For example, 
            addresses in the kseg0 and kseg1 spaces are unmapped translations. In these spaces the physical 
            address is derived by subtracting the base address of the space from the virtual address.
            @param addr virtual address, range 0xa0000000-0xbfffffff
            @return physical address 
        */
        inline DirectMapret translate_kseg1(DirectMapargs addr) {
            return addr - KSEG1_START;
        }
        /**
            
            @param addr virtual address, range 0x00000000-0x7fffffff
            @return physical address
        */
        inline TLBret translate_kuseg(TLBargs) {

        }

        SelAddrSpace32ret select_addr_space32(SelAddrSpace32args);


        /*
            Pipeline
        */
        // In case we need to change return/argument type in the future
        using ICFret  = void;    using ICFargs  = void;
        using ITLBret = void;    using ITLBargs = void;
        using ITCret  = void;    using ITCargs  = void;
        using RFRret  = void;    using RFRargs  = void;
        using IDECret = void;    using IDECargs = void;
        using IVAret  = void;    using IVAargs  = void;
        using BCMPret = void;    using BCMPargs = void;
        using ALUret  = void;    using ALUargs  = void;
        using DVAret  = void;    using DVAargs  = void;
        using DCRret  = void;    using DCRargs  = void;
        using DTLBret = void;    using DTLBargs = void;
        using LAret   = void;    using LAargs   = void;
        using DTCret  = void;    using DTCargs  = void;
        using DCWret  = void;    using DCWargs  = void;
        using RFWret  = void;    using RFWargs  = void;

        
        
        /**
            Instruction cache fetch.
            Used to address the page/bank of the instruction cache.
            Note: In ICF, virtual address is used like this:

            xx  xxxxxxxxxxxxxx
            |1| |_____ 2 ____|

            1: selected IC bank (0-3)
            2: these bits address the selected IC bank

        */
        ICFret  ICF(ICFargs);
        

        // Instruction micro-TLB read
        ITLBret ITLB(ITLBargs);
        // Instruction cache tag check
        ITCret  ITC(ITCargs);
        // Register file read
        RFRret  RFR(RFRargs);
        // Instruction decode
        IDECret IDEC(IDECargs);
        // Instruction virtual address calculation
        IVAret  IVA(IVAargs);
        // Branch compare
        BCMPret BCMP(BCMPargs);
        // Arithmetic logic operation
        ALUret  ALU(ALUargs);
        // Data virtual address calculation
        DVAret  DVA(DVAargs);
        // Data cache read
        DCRret  DCR(DCRargs);
        // Data joint-TLB read
        DTLBret DTLB(DTLBargs);
        // Load data alignment
        LAret   LA(LAargs);
        // Data cache tag check
        DTCret  DTC(DTCargs);
        // Data cache write
        DCWret  DCW(DCWargs);
        // Register file write
        RFWret  RFW(RFWargs);
    };
        
}
#endif