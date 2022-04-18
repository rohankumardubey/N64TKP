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
#include <any>
#include "n64_types.hxx"
#include "n64_cpu_exceptions.hxx"

// TODO: Move these to cmake
#define SKIP64BITCHECK 1
#define SKIPEXCEPTIONS 0
#define KB(x) (static_cast<size_t>(x << 10))
#define check_bit(x, y) ((x) & (1u << y))
constexpr uint32_t KSEG0_START = 0x8000'0000;
constexpr uint32_t KSEG0_END   = 0x9FFF'FFFF;
constexpr uint32_t KSEG1_START = 0xA000'0000;
constexpr uint32_t KSEG1_END   = 0xBFFF'FFFF;
namespace TKPEmu {
    namespace N64 {
        class N64;
    }
    namespace Applications {
        class N64_RomDisassembly;
    }
}
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
        NOP     // No operation
    };
    // Storage for each pipeline instruction
    // Does the job of pipeline latches
    struct PipelineStorage {
        Instruction instruction;
        InstructionType instr_type;
        AccessType access_type;
        WriteType write_type;
        std::any write_loc = nullptr;
        std::any data;
        MemDataUD paddr;
        MemDataUD vaddr;
        bool cached = false;
    };
    /**
        32-bit address bus 
        
        @see https://n64brew.dev/wiki/Memory_map     
    */
    class CPUBus {
    public:
        bool LoadFromFile(std::string path);
    private:
        MemDataUW  fetch_instruction_uncached(MemDataUW paddr);
        MemDataUW  fetch_instruction_cached  (MemDataUW paddr);
        MemDataUW& redirect_paddress (MemDataUW paddr);
        std::vector<uint8_t> rom_;
        friend class CPU;
    };
    class CPU {
    public:
        CPU();
        void Reset();
    private:
        using SelAddrSpace32Ret = uint32_t;
        using SelAddrSpace32Args = uint32_t;
        using PipelineStageRet  = void;
        using PipelineStageArgs = size_t;
        CPUBus cpubus_;
        std::array<std::queue<PipelineStage>, 5> pipeline_;
        std::array<PipelineStorage, 5>           pipeline_storage_;
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
        // Kernel mode addressing functions
        /**
            VR4300 manual, page 122: 

            There are virtual-to-physical address translations that occur outside of the TLB. For example, 
            addresses in the kseg0 and kseg1 spaces are unmapped translations. In these spaces the physical 
            address is derived by subtracting the base address of the space from the virtual address.
            @param addr virtual address, range 0x80000000-0x9fffffff
            @return physical address 
        */
        inline MemDataUW translate_kseg0(MemDataUD vaddr) noexcept;
        /**
            VR4300 manual, page 122: 

            There are virtual-to-physical address translations that occur outside of the TLB. For example, 
            addresses in the kseg0 and kseg1 spaces are unmapped translations. In these spaces the physical 
            address is derived by subtracting the base address of the space from the virtual address.
            @param addr virtual address, range 0xa0000000-0xbfffffff
            @return physical address 
        */
        inline MemDataUW translate_kseg1(MemDataUD vaddr) noexcept;
        /**
            
            @param addr virtual address, range 0x00000000-0x7fffffff
            @return physical address
        */
        inline MemDataUW translate_kuseg(MemDataUD vaddr) noexcept;
        SelAddrSpace32Ret select_addr_space32(SelAddrSpace32Args);

        /**
         * Load and store instruction common functions
         * 
         * @see manual 16.2
         */

        /**
         * "Uses TLB to search a physical address from a virtual address. If
         * TLB does not have the requested contents of conversion, this
         * function fails, and TLB non-coincidence exception occurs."
         * 
         * For non TLB address space, the address is just fetched from cache or
         * memory.
         */
        inline MemDataUD AddressTranslation();
        /**
         * Searches the cache and main memory to search for the contents
         * of the specified data length stored in a specified physical address.
         * If the specified data length is less than a word, the contents of a
         * data position taking the endian mode and reverse endian mode of
         * the processor into consideration are loaded. The low-order 3 bits
         * and access type field of the address determine the data position in
         * a data word. The data is loaded to the cache if the cache is
         * enabled. 
         */
        inline MemDataUD LoadMemory();
        /**
         * Searches the cache, write buffer, and main memory to store the
         * contents of a specified data length to a specified physical address.
         * If the specified data length is less than a word, the contents of a
         * data position taking the endian mode and reverse endian mode of
         * the processor into consideration are stored. The low-order 3 bits
         * and access type field of the address determine the data position in
         * a data word.
         * 
         * @param cached whether to store in cache
         * @param type access type - see AccessType enum
         * @param data data to store
         * @param paddr physical address
         * @param vaddr virtual address (unneeded?)
         */
        inline void store_memory(bool cached, AccessType type, MemDataUD data, MemDataUW paddr, MemDataUD vaddr = 0);

        PipelineStageRet IC(PipelineStageArgs process_no);
        PipelineStageRet RF(PipelineStageArgs process_no);
        PipelineStageRet EX(PipelineStageArgs process_no);
        PipelineStageRet DC(PipelineStageArgs process_no);
        PipelineStageRet WB(PipelineStageArgs process_no);

        inline bool execute_instruction(PipelineStage stage, size_t process_no);
        inline void register_write(size_t process_no);
        void update_pipeline();

        friend class TKPEmu::N64::N64;
        // TKPEmu specific friends (applications)
        friend class TKPEmu::Applications::N64_RomDisassembly;
    };
}
#endif