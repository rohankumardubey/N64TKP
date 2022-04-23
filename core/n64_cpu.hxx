#pragma once
#ifndef TKP_N64_CPU_H
#define TKP_N64_CPU_H
#include <cstdint>
#include <limits>
#include <array>
#include <queue>
#include <vector>
#include <memory>
#include <any>
#include <deque>
#include "common/n64_types.hxx"
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
        class QA;
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
        NOP = 5,    // No operation
    };
    struct ICRF_latch {
        Instruction     instruction;
    };
    struct RFEX_latch {
        Instruction     instruction;
        InstructionType instruction_type;
        MemDataUnionDW  fetched_rt;
        MemDataUnionDW  fetched_rs;
    };
    struct EXDC_latch {
        WriteType       write_type = WriteType::NONE;
        AccessType      access_type = AccessType::NONE;
        std::any        data;
        uint32_t        dest;
        uint32_t        vaddr;
        uint64_t*       dest_direct = nullptr;
        bool            cached;
    };
    struct DCWB_latch {
        WriteType       write_type = WriteType::NONE;
        AccessType      access_type = AccessType::NONE;
        std::any        data;
        uint32_t        dest;
        bool            cached;
    };
    struct TranslatedAddress {
        uint32_t paddr;
        bool cached;
    };
    /**
        32-bit address bus 
        
        @see https://n64brew.dev/wiki/Memory_map     
    */
    class CPUBus {
    public:
        CPUBus();
        bool LoadFromFile(std::string path);
        void Reset();
    private:
        uint32_t  fetch_instruction_uncached(uint32_t paddr);
        uint32_t  fetch_instruction_cached  (uint32_t paddr);
        uint8_t*  redirect_paddress (uint32_t paddr);
        void      map_direct_addresses();
        std::array<uint8_t, 0xFC00000> cart_rom_;
        std::array<uint8_t, 0x400000> rdram_ {};
        std::array<uint8_t, 0x400000> rdram_xpk_ {};
        std::array<uint8_t, 64> pif_ram_ {};
        std::array<uint8_t, 0x100> temp_addresses_{};
        std::array<uint8_t*, 0x1000> page_table_ {};
        friend class CPU;
        friend class TKPEmu::Applications::N64_RomDisassembly;
    };
    class CPU {
    public:
        CPU();
        void Reset();
    private:
        using PipelineStageRet  = void;
        using PipelineStageArgs = void;
        CPUBus cpubus_;
        std::deque<PipelineStage> pipeline_;
        std::deque<uint32_t> pipeline_cur_instr_ {};
        ICRF_latch icrf_latch_ {};
        RFEX_latch rfex_latch_ {};
        EXDC_latch exdc_latch_ {};
        DCWB_latch dcwb_latch_ {};
        
        OperatingMode opmode_ = OperatingMode::Kernel;
        // To be used with OpcodeMasks (OpcodeMasks[mode64_])
        bool mode64_ = false;
        /// Registers
        // r0 is hardwired to 0, r31 is the link register
        std::array<MemDataUnionDW, 32> gpr_regs_;
        std::array<double, 32> fpr_regs_;
        std::array<MemDataUnionDW, 32> cp0_regs_;
        // CPU cache
        std::vector<uint8_t> instr_cache_;
        std::vector<uint8_t> data_cache_;
        // Special registers
        uint64_t pc_, hi_, lo_;
        bool llbit_;
        float fcr0_, fcr31_;
        // Kernel mode addressing functions
        /**
            VR4300 manual, page 122: 

            There are virtual-to-physical address translations that occur outside of the TLB. For example, 
            addresses in the kseg0 and kseg1 spaces are unmapped translations. In these spaces the physical 
            address is derived by subtracting the base address of the space from the virtual address.
            @param addr virtual address, range 0x80000000-0x9fffffff
            @return physical address 
        */
        inline uint32_t translate_kseg0(uint32_t vaddr) noexcept;
        /**
            VR4300 manual, page 122: 

            There are virtual-to-physical address translations that occur outside of the TLB. For example, 
            addresses in the kseg0 and kseg1 spaces are unmapped translations. In these spaces the physical 
            address is derived by subtracting the base address of the space from the virtual address.
            @param addr virtual address, range 0xa0000000-0xbfffffff
            @return physical address 
        */
        inline uint32_t translate_kseg1(uint32_t vaddr) noexcept;
        /**
            
            @param addr virtual address, range 0x00000000-0x7fffffff
            @return physical address
        */
        inline uint32_t translate_kuseg(uint32_t vaddr) noexcept;
        TranslatedAddress translate_vaddr(uint32_t vaddr);
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
        inline uint64_t AddressTranslation();
        /**
         * Searches the cache and main memory to search for the contents
         * of the specified data length stored in a specified physical address.
         * If the specified data length is less than a word, the contents of a
         * data position taking the endian mode and reverse endian mode of
         * the processor into consideration are loaded. The low-order 3 bits
         * and access type field of the address determine the data position in
         * a data word. The data is loaded to the cache if the cache is
         * enabled. 
         * 
         * @param cached whether to store in cache
         * @param type access type - see AccessType enum
         * @param data the result is stored here
         * @param paddr physical address to read from
         */
        inline void load_memory(bool cached, AccessType type, std::any& data_any, uint32_t paddr);
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
         * @param paddr physical address to write to
         */
        inline void store_memory(bool cached, AccessType type, std::any data_any, uint32_t paddr);
        void store_register(AccessType access_type, uint32_t dest, uint64_t* dest_direct, std::any data);

        PipelineStageRet IC(PipelineStageArgs);
        PipelineStageRet RF(PipelineStageArgs);
        PipelineStageRet EX(PipelineStageArgs);
        PipelineStageRet DC(PipelineStageArgs);
        PipelineStageRet WB(PipelineStageArgs);

        inline bool execute_stage(PipelineStage stage);
        /**
         * Called during EX stage, handles the logic execution of each instruction
         */
        void execute_instruction();
        void update_pipeline();

        void clear_registers();

        friend class TKPEmu::N64::N64;
        friend class TKPEmu::Applications::N64_RomDisassembly;
        friend class TKPEmu::N64::QA;
    };
}
#endif