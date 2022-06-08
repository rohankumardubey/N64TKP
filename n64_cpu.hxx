#pragma once
#ifndef TKP_N64_CPU_H
#define TKP_N64_CPU_H
#include <cstdint>
#include <limits>
#include <array>
#include <queue>
#include <vector>
#include <memory>
#include "n64_types.hxx"
#include "n64_cpu_exceptions.hxx"
#include "n64_rcp.hxx"
#include <GL/glew.h>

// TODO: Move these to cmake
#define SKIP64BITCHECK 1
#define SKIPEXCEPTIONS 1
#define DONTDEBUGSTUFF 0
#define KB(x) (static_cast<size_t>(x << 10))
#define check_bit(x, y) ((x) & (1u << y))

constexpr uint32_t KSEG0_START = 0x8000'0000;
constexpr uint32_t KSEG0_END   = 0x9FFF'FFFF;
constexpr uint32_t KSEG1_START = 0xA000'0000;
constexpr uint32_t KSEG1_END   = 0xBFFF'FFFF;

constexpr auto CP0_COUNT = 9;
constexpr auto CP0_COMPARE = 11;

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
    struct ICRF_latch {
        Instruction     instruction;
    };
    struct RFEX_latch {
        Instruction     instruction;
        uint8_t instruction_type;
        uint8_t instruction_target;
        MemDataUnionDW  fetched_rt;
        MemDataUnionDW  fetched_rs;
        size_t fetched_rt_i = -1;
        size_t fetched_rs_i = -1;
    };
    struct EXDC_latch {
        WriteType       write_type = WriteType::NONE;
        AccessType      access_type = AccessType::NONE;
        uint64_t        data;
        uint8_t*        dest = nullptr;
        uint32_t        vaddr;
        uint32_t        paddr;
        bool            cached = false;
        size_t fetched_rt_i = -1;
        size_t fetched_rs_i = -1;
    };
    struct DCWB_latch {
        WriteType       write_type = WriteType::NONE;
        AccessType      access_type = AccessType::NONE;
        uint64_t        data;
        uint8_t*        dest = nullptr;
        uint32_t        paddr;
        uint8_t         dest_reg = 0;
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
        CPUBus(Devices::RCP& rcp);
        bool LoadFromFile(std::string path);
        void Reset();
    private:
        uint32_t  fetch_instruction_uncached(uint32_t paddr);
        uint32_t  fetch_instruction_cached  (uint32_t paddr);
        uint8_t*  redirect_paddress         (uint32_t paddr);
        uint8_t*  redirect_paddress_slow    (uint32_t paddr);
        void      map_direct_addresses();

        std::array<uint8_t, 0xFC00000> cart_rom_;
        std::array<uint8_t, 0x400000> rdram_ {};
        std::array<uint8_t, 0x400000> rdram_xpk_ {};
        std::array<uint8_t, 64> pif_ram_ {};
        std::array<uint8_t*, 0x1000> page_table_ {};

        // Peripheral Interface
        uint32_t pi_dram_addr_ = 0;
        uint32_t pi_cart_addr_ = 0;
        uint32_t pi_rd_len_ = 0;
        uint32_t pi_wr_len_ = 0;
        uint32_t pi_status_ = 0;
        uint32_t pi_bsd_dom1_lat_ = 0;
        uint32_t pi_bsd_dom1_pwd_ = 0;
        uint32_t pi_bsd_dom1_pgs_ = 0;
        uint32_t pi_bsd_dom1_rls_ = 0;
        uint32_t pi_bsd_dom2_lat_ = 0;
        uint32_t pi_bsd_dom2_pwd_ = 0;
        uint32_t pi_bsd_dom2_pgs_ = 0;
        uint32_t pi_bsd_dom2_rls_ = 0;

        Devices::RCP& rcp_;
        friend class CPU;
        friend class TKPEmu::Applications::N64_RomDisassembly;
    };
    class CPU final {
    public:
        CPU(CPUBus& cpubus, RCP& rcp, GLuint& text_format);
        void Reset();
    private:
        using PipelineStageRet  = void;
        using PipelineStageArgs = void;
        CPUBus& cpubus_;
        RCP& rcp_;
        uint8_t pipeline_ = 0b00001; // the 5 stages
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
        uint64_t instructions_ran_ = 0;

        GLuint& text_format_;
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
        inline TranslatedAddress translate_vaddr(uint32_t vaddr);
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
         * @param paddr physical address to read from
         * @param data the result is stored here
         * @param size size to load
         */
        __always_inline void load_memory(bool cached, uint32_t paddr, uint64_t& data, int size);
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
         * @param paddr physical address to write to
         * @param data data to store
         * @param size size to store
         */
        __always_inline void store_memory(bool cached, uint32_t paddr, uint64_t& data, int size);
        __always_inline void store_register(uint8_t* dest, uint64_t data, int size);
        /**
         * Executes memory mapped register related stuff
         */
        inline void invalidate_hwio(uint32_t addr, uint64_t data);

        __always_inline PipelineStageRet IC(PipelineStageArgs);
        __always_inline PipelineStageRet RF(PipelineStageArgs);
        __always_inline PipelineStageRet EX(PipelineStageArgs);
        __always_inline PipelineStageRet DC(PipelineStageArgs);
        __always_inline PipelineStageRet WB(PipelineStageArgs);

        void NOP();

        void SPECIAL(), REGIMM(), J(), JAL(), BEQ(), BNE(), BLEZ(), BGTZ(),
        ADDI(), ADDIU(), SLTI(), STLIU(), ANDI(), ORI(), XORI(), LUI(),
        COP0(), COP1(), COP2(), BEQL(), BNEL(), BLEZL(), BGTZL(),
        DADDI(), DADDIU(), LDL(), LDR(), ERROR(),
        LB(), LH(), LWL(), LW(), LBU(), LHU(), LWR(), LWU(),
        SB(), SH(), SWL(), SW(), SDL(), SDR(), SWR(), CACHE(),
        LL(), LWC1(), LWC2(), LLD(), LDC1(), LDC2(), LD(),
        SC(), SWC1(), SWC2(), SCD(), SDC1(), SDC2(), SD();

        void s_SLL(), s_SRL(), s_SRA(), s_SLLV(), s_SRLV(), s_SRAV(),
        s_JR(), s_JALR(), s_SYSCALL(), s_BREAK(), s_SYNC(),
        s_MFHI(), s_MTHI(), s_MFLO(), s_MTLO(), s_DSLLV(), s_DSRLV(), s_DSRAV(),
        s_MULT(), s_MULTU(), s_DIV(), s_DIVU(), s_DMULT(), s_DMULTU(), s_DDIV(), s_DDIVU(),
        s_ADD(), s_ADDU(), s_SUB(), s_SUBU(), s_AND(), s_OR(), s_XOR(), s_NOR(),
        s_SLT(), s_SLTU(), s_DADD(), s_DADDU(), s_DSUB(), s_DSUBU(),
        s_TGE(), s_TGEU(), s_TLT(), s_TLTU(), s_TEQ(), s_TNE(),
        s_DSLL(), s_DSRL(), s_DSRA(), s_DSLL32(), s_DSRL32(), s_DSRA32();
        using func_ptr = void (CPU::*)();
        constexpr static std::array<func_ptr, 64> InstructionTable = {
            &CPU::SPECIAL, &CPU::REGIMM, &CPU::J, &CPU::JAL, &CPU::BEQ, &CPU::BNE, &CPU::BLEZ, &CPU::BGTZ,
            &CPU::ADDI, &CPU::ADDIU, &CPU::SLTI, &CPU::STLIU, &CPU::ANDI, &CPU::ORI, &CPU::XORI, &CPU::LUI,
            &CPU::COP0, &CPU::COP1, &CPU::COP2, &CPU::ERROR, &CPU::BEQL, &CPU::BNEL, &CPU::BLEZL, &CPU::BGTZL,
            &CPU::DADDI, &CPU::DADDIU, &CPU::LDL, &CPU::LDR, &CPU::ERROR, &CPU::ERROR, &CPU::ERROR, &CPU::ERROR,
            &CPU::LB, &CPU::LH, &CPU::LWL, &CPU::LW, &CPU::LBU, &CPU::LHU, &CPU::LWR, &CPU::LWU,
            &CPU::SB, &CPU::SH, &CPU::SWL, &CPU::SW, &CPU::SDL, &CPU::SDR, &CPU::SWR, &CPU::CACHE,
            &CPU::LL, &CPU::LWC1, &CPU::LWC2, &CPU::ERROR, &CPU::LLD, &CPU::LDC1, &CPU::LDC2, &CPU::LD,
            &CPU::SC, &CPU::SWC1, &CPU::SWC2, &CPU::ERROR, &CPU::SCD, &CPU::SDC1, &CPU::SDC2, &CPU::SD,
        };
        constexpr static std::array<func_ptr, 64> SpecialTable = {
            &CPU::s_SLL, &CPU::ERROR, &CPU::s_SRL, &CPU::s_SRA, &CPU::s_SLLV, &CPU::ERROR, &CPU::s_SRLV, &CPU::s_SRAV,
            &CPU::s_JR, &CPU::s_JALR, &CPU::ERROR, &CPU::ERROR, &CPU::s_SYSCALL, &CPU::s_BREAK, &CPU::ERROR, &CPU::s_SYNC,
            &CPU::s_MFHI, &CPU::s_MTHI, &CPU::s_MFLO, &CPU::s_MTLO, &CPU::s_DSLLV, &CPU::ERROR, &CPU::s_DSRLV, &CPU::s_DSRAV,
            &CPU::s_MULT, &CPU::s_MULTU, &CPU::s_DIV, &CPU::s_DIVU, &CPU::s_DMULT, &CPU::s_DMULTU, &CPU::s_DDIV, &CPU::s_DDIVU,
            &CPU::s_ADD, &CPU::s_ADDU, &CPU::s_SUB, &CPU::s_SUBU, &CPU::s_AND, &CPU::s_OR, &CPU::s_XOR, &CPU::s_NOR,
            &CPU::ERROR, &CPU::ERROR, &CPU::s_SLT, &CPU::s_SLTU, &CPU::s_DADD, &CPU::s_DADDU, &CPU::s_DSUB, &CPU::s_DSUBU,
            &CPU::s_TGE, &CPU::s_TGEU, &CPU::s_TLT, &CPU::s_TLTU, &CPU::s_TEQ, &CPU::ERROR, &CPU::s_TNE, &CPU::ERROR,
            &CPU::s_DSLL, &CPU::ERROR, &CPU::s_DSRL, &CPU::s_DSRA, &CPU::s_DSLL32, &CPU::ERROR, &CPU::s_DSRL32, &CPU::s_DSRA32,
        };
        constexpr static func_ptr NopTable = &CPU::NOP;
        constexpr static std::array<const func_ptr*, 3> TableTable = {
            &NopTable, InstructionTable.data(), SpecialTable.data()
        };
        __always_inline void bypass_register();
        /**
         * Called during EX stage, handles the logic execution of each instruction
         */
        void execute_instruction();
        void execute_cp0_instruction(const Instruction& instr);
        void update_pipeline();
        // Fills the pipeline with the first 5 instructions
        void fill_pipeline();

        void clear_registers();

        friend class TKPEmu::N64::N64;
        friend class TKPEmu::Applications::N64_RomDisassembly;
        friend class TKPEmu::N64::QA;
    };
}
#endif