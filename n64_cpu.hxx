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
        class N64_TKPWrapper;
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
        MemDataUnionDW  fetched_rt;
        MemDataUnionDW  fetched_rs;
        size_t          fetched_rt_i;
    };
    struct EXDC_latch {
        WriteType       write_type;
        AccessType      access_type;
        uint64_t        data;
        uint8_t*        dest;
        uint32_t        vaddr;
        uint32_t        paddr;
        bool            cached;
        bool            sign_extend;
    };
    struct DCWB_latch {
        WriteType       write_type;
        AccessType      access_type;
        uint64_t        data;
        uint8_t*        dest;
        uint32_t        paddr;
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
        bool LoadCartridge(std::string path);
        bool LoadIPL(std::string path);
        bool IsEverythingLoaded() {
            return rom_loaded_ && ipl_loaded_;
        }
        void Reset();
    private:
        uint32_t  fetch_instruction_uncached(uint32_t paddr);
        uint32_t  fetch_instruction_cached  (uint32_t paddr);
        uint8_t*  redirect_paddress         (uint32_t paddr);
        uint8_t*  redirect_paddress_slow    (uint32_t paddr);
        void      map_direct_addresses();

        std::array<uint8_t, 0xFC00000> cart_rom_;
        bool rom_loaded_ = false;
        bool ipl_loaded_ = false;
        static std::vector<uint8_t> ipl_;
        std::array<uint8_t, 0x400000> rdram_ {};
        std::array<uint8_t, 0x400000> rdram_xpk_ {};
        std::array<uint8_t, 64> pif_ram_ {};
        std::array<uint8_t, 0x1000> rsp_imem_ {};
        std::array<uint8_t, 0x1000> rsp_dmem_ {};
        std::array<uint8_t, 0x100000> rdp_cmem_ {};
        std::array<uint8_t*, 0x1000> page_table_ {};
        uint8_t the_void_ = 0; // redirect unimplemented and useless addresses here

        // MIPS Interface
        uint32_t mi_mode_         = 0;
        uint32_t mi_mask_         = 0;

        // Peripheral Interface
        uint32_t pi_dram_addr_    = 0;
        uint32_t pi_cart_addr_    = 0;
        uint32_t pi_rd_len_       = 0;
        uint32_t pi_wr_len_       = 0;
        uint32_t pi_status_       = 0;
        uint32_t pi_bsd_dom1_lat_ = 0;
        uint32_t pi_bsd_dom1_pwd_ = 0;
        uint32_t pi_bsd_dom1_pgs_ = 0;
        uint32_t pi_bsd_dom1_rls_ = 0;
        uint32_t pi_bsd_dom2_lat_ = 0;
        uint32_t pi_bsd_dom2_pwd_ = 0;
        uint32_t pi_bsd_dom2_pgs_ = 0;
        uint32_t pi_bsd_dom2_rls_ = 0;

        // Audio Interface
        uint32_t ai_dram_addr_    = 0;
        uint32_t ai_length_       = 0;
        uint32_t ai_control_      = 0;
        uint32_t ai_status_       = 0;
        uint32_t ai_dacrate_       = 0;
        uint32_t ai_bitrate_       = 0;

        // RDRAM Interface
        uint32_t ri_mode_         = 0;
        uint32_t ri_config_       = 0;
        uint32_t ri_current_load_ = 0;
        uint32_t ri_select_       = 0;

        // Serial Interface
        uint32_t si_status_       = 0;

        Devices::RCP& rcp_;
        friend class CPU;
        friend class N64;
        friend class TKPEmu::Applications::N64_RomDisassembly;
    };
    template<auto MemberFunc>
    static void lut_wrapper(CPU* cpu) {
        // Props: calc84maniac
        // > making it into a template parameter lets the compiler avoid using an actual member function pointer at runtime
        (cpu->*MemberFunc)();
    }
    class CPU final {
    public:
        CPU(CPUBus& cpubus, RCP& rcp);
        void Reset();
    private:
        using PipelineStageRet  = void;
        using PipelineStageArgs = void;
        CPUBus& cpubus_;
        RCP& rcp_;
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
        uint64_t fcr0_, fcr31_;
        bool ldi_ = false;
        bool should_resize_ = false;
        unsigned text_format_ = 0;
        unsigned text_width_ = 0;
        unsigned text_height_ = 0;
        // Kernel mode addressing functions
        /**
            VR4300 manual, page 122: 

            There are virtual-to-physical address translations that occur outside of the TLB. For example, 
            addresses in the kseg0 and kseg1 spaces are unmapped translations. In these spaces the physical 
            address is derived by subtracting the base address of the space from the virtual address.
            @param addr virtual address, range 0x80000000-0x9fffffff
            @return physical address 
        */
        // inline uint32_t translate_kseg0(uint32_t vaddr) noexcept;
        /**
            VR4300 manual, page 122: 

            There are virtual-to-physical address translations that occur outside of the TLB. For example, 
            addresses in the kseg0 and kseg1 spaces are unmapped translations. In these spaces the physical 
            address is derived by subtracting the base address of the space from the virtual address.
            @param addr virtual address, range 0xa0000000-0xbfffffff
            @return physical address 
        */
        // inline uint32_t translate_kseg1(uint32_t vaddr) noexcept;
        /**
            
            @param addr virtual address, range 0x00000000-0x7fffffff
            @return physical address
        */
        // inline uint32_t translate_kuseg(uint32_t vaddr) noexcept;
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
        inline void invalidate_hwio(uint32_t addr, uint64_t& data);

        __always_inline PipelineStageRet IC(PipelineStageArgs);
        __always_inline PipelineStageRet RF(PipelineStageArgs);
        __always_inline PipelineStageRet EX(PipelineStageArgs);
        __always_inline PipelineStageRet DC(PipelineStageArgs);
        __always_inline PipelineStageRet WB(PipelineStageArgs);

        void SPECIAL(), REGIMM(), J(), JAL(), BEQ(), BNE(), BLEZ(), BGTZ(),
        ADDI(), ADDIU(), SLTI(), SLTIU(), ANDI(), ORI(), XORI(), LUI(),
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

        void r_BLTZ(), r_BGEZ(), r_BLTZL(), r_BGEZL(),
            r_TGEI(), r_TGEIU(), r_TLTI(), r_TLTIU(), r_TEQI(), r_TNEI(),
            r_BLTZAL(), r_BGEZAL(), r_BLTZALL(), r_BGEZALL();

        void f_ADD(), f_SUB(), f_MUL(), f_DIV(), f_SQRT(), f_ABS(), f_MOV(), f_NEG(),
            f_ROUNDL(), f_TRUNCL(), f_CEILL(), f_FLOORL(), f_ROUNDW(), f_TRUNCW(), f_CEILW(), f_FLOORW(),
            f_CVTS(), f_CVTD(), f_CVTW(), f_CVTL(), f_CF(), f_CUN(),
            f_CEQ(), f_CUEQ(), f_COLT(), f_CULT(), f_COLE(), f_CULE(),
            f_CSF(), f_CNGLE(), f_CSEQ(), f_CNGL(), f_CLT(), f_CNGE(), f_CLE(), f_CNGT();

        using func_ptr = void (*)(CPU*);
        constexpr static std::array<func_ptr, 64> InstructionTable = {
            &lut_wrapper<&CPU::SPECIAL>, &lut_wrapper<&CPU::REGIMM>, &lut_wrapper<&CPU::J>, &lut_wrapper<&CPU::JAL>, &lut_wrapper<&CPU::BEQ>, &lut_wrapper<&CPU::BNE>, &lut_wrapper<&CPU::BLEZ>, &lut_wrapper<&CPU::BGTZ>,
            &lut_wrapper<&CPU::ADDI>, &lut_wrapper<&CPU::ADDIU>, &lut_wrapper<&CPU::SLTI>, &lut_wrapper<&CPU::SLTIU>, &lut_wrapper<&CPU::ANDI>, &lut_wrapper<&CPU::ORI>, &lut_wrapper<&CPU::XORI>, &lut_wrapper<&CPU::LUI>,
            &lut_wrapper<&CPU::COP0>, &lut_wrapper<&CPU::COP1>, &lut_wrapper<&CPU::COP2>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::BEQL>, &lut_wrapper<&CPU::BNEL>, &lut_wrapper<&CPU::BLEZL>, &lut_wrapper<&CPU::BGTZL>,
            &lut_wrapper<&CPU::DADDI>, &lut_wrapper<&CPU::DADDIU>, &lut_wrapper<&CPU::LDL>, &lut_wrapper<&CPU::LDR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>,
            &lut_wrapper<&CPU::LB>, &lut_wrapper<&CPU::LH>, &lut_wrapper<&CPU::LWL>, &lut_wrapper<&CPU::LW>, &lut_wrapper<&CPU::LBU>, &lut_wrapper<&CPU::LHU>, &lut_wrapper<&CPU::LWR>, &lut_wrapper<&CPU::LWU>,
            &lut_wrapper<&CPU::SB>, &lut_wrapper<&CPU::SH>, &lut_wrapper<&CPU::SWL>, &lut_wrapper<&CPU::SW>, &lut_wrapper<&CPU::SDL>, &lut_wrapper<&CPU::SDR>, &lut_wrapper<&CPU::SWR>, &lut_wrapper<&CPU::CACHE>,
            &lut_wrapper<&CPU::LL>, &lut_wrapper<&CPU::LWC1>, &lut_wrapper<&CPU::LWC2>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::LLD>, &lut_wrapper<&CPU::LDC1>, &lut_wrapper<&CPU::LDC2>, &lut_wrapper<&CPU::LD>,
            &lut_wrapper<&CPU::SC>, &lut_wrapper<&CPU::SWC1>, &lut_wrapper<&CPU::SWC2>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::SCD>, &lut_wrapper<&CPU::SDC1>, &lut_wrapper<&CPU::SDC2>, &lut_wrapper<&CPU::SD>,
        };
        constexpr static std::array<func_ptr, 64> SpecialTable = {
            &lut_wrapper<&CPU::s_SLL>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::s_SRL>, &lut_wrapper<&CPU::s_SRA>, &lut_wrapper<&CPU::s_SLLV>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::s_SRLV>, &lut_wrapper<&CPU::s_SRAV>,
            &lut_wrapper<&CPU::s_JR>, &lut_wrapper<&CPU::s_JALR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::s_SYSCALL>, &lut_wrapper<&CPU::s_BREAK>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::s_SYNC>,
            &lut_wrapper<&CPU::s_MFHI>, &lut_wrapper<&CPU::s_MTHI>, &lut_wrapper<&CPU::s_MFLO>, &lut_wrapper<&CPU::s_MTLO>, &lut_wrapper<&CPU::s_DSLLV>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::s_DSRLV>, &lut_wrapper<&CPU::s_DSRAV>,
            &lut_wrapper<&CPU::s_MULT>, &lut_wrapper<&CPU::s_MULTU>, &lut_wrapper<&CPU::s_DIV>, &lut_wrapper<&CPU::s_DIVU>, &lut_wrapper<&CPU::s_DMULT>, &lut_wrapper<&CPU::s_DMULTU>, &lut_wrapper<&CPU::s_DDIV>, &lut_wrapper<&CPU::s_DDIVU>,
            &lut_wrapper<&CPU::s_ADD>, &lut_wrapper<&CPU::s_ADDU>, &lut_wrapper<&CPU::s_SUB>, &lut_wrapper<&CPU::s_SUBU>, &lut_wrapper<&CPU::s_AND>, &lut_wrapper<&CPU::s_OR>, &lut_wrapper<&CPU::s_XOR>, &lut_wrapper<&CPU::s_NOR>,
            &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::s_SLT>, &lut_wrapper<&CPU::s_SLTU>, &lut_wrapper<&CPU::s_DADD>, &lut_wrapper<&CPU::s_DADDU>, &lut_wrapper<&CPU::s_DSUB>, &lut_wrapper<&CPU::s_DSUBU>,
            &lut_wrapper<&CPU::s_TGE>, &lut_wrapper<&CPU::s_TGEU>, &lut_wrapper<&CPU::s_TLT>, &lut_wrapper<&CPU::s_TLTU>, &lut_wrapper<&CPU::s_TEQ>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::s_TNE>, &lut_wrapper<&CPU::ERROR>,
            &lut_wrapper<&CPU::s_DSLL>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::s_DSRL>, &lut_wrapper<&CPU::s_DSRA>, &lut_wrapper<&CPU::s_DSLL32>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::s_DSRL32>, &lut_wrapper<&CPU::s_DSRA32>,
        };
        constexpr static std::array<func_ptr, 64> FloatTable = {
            &lut_wrapper<&CPU::f_ADD>, &lut_wrapper<&CPU::f_SUB>, &lut_wrapper<&CPU::f_MUL>, &lut_wrapper<&CPU::f_DIV>, &lut_wrapper<&CPU::f_SQRT>, &lut_wrapper<&CPU::f_ABS>, &lut_wrapper<&CPU::f_MOV>, &lut_wrapper<&CPU::f_NEG>,
            &lut_wrapper<&CPU::f_ROUNDL>, &lut_wrapper<&CPU::f_TRUNCL>, &lut_wrapper<&CPU::f_CEILL>, &lut_wrapper<&CPU::f_FLOORL>, &lut_wrapper<&CPU::f_ROUNDW>, &lut_wrapper<&CPU::f_TRUNCW>, &lut_wrapper<&CPU::f_CEILW>, &lut_wrapper<&CPU::f_FLOORW>,
            &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>,
            &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>,
            &lut_wrapper<&CPU::f_CVTS>, &lut_wrapper<&CPU::f_CVTD>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::f_CVTW>, &lut_wrapper<&CPU::f_CVTL>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>,
            &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>,
            &lut_wrapper<&CPU::f_CF>, &lut_wrapper<&CPU::f_CUN>, &lut_wrapper<&CPU::f_CEQ>, &lut_wrapper<&CPU::f_CUEQ>, &lut_wrapper<&CPU::f_COLT>, &lut_wrapper<&CPU::f_CULT>, &lut_wrapper<&CPU::f_COLE>, &lut_wrapper<&CPU::f_CULE>,
            &lut_wrapper<&CPU::f_CSF>, &lut_wrapper<&CPU::f_CNGLE>, &lut_wrapper<&CPU::f_CSEQ>, &lut_wrapper<&CPU::f_CNGL>, &lut_wrapper<&CPU::f_CLT>, &lut_wrapper<&CPU::f_CNGE>, &lut_wrapper<&CPU::f_CLE>, &lut_wrapper<&CPU::f_CNGT>,
        };
        constexpr static std::array<func_ptr, 32> RegImmTable = {
            &lut_wrapper<&CPU::r_BLTZ>, &lut_wrapper<&CPU::r_BGEZ>, &lut_wrapper<&CPU::r_BLTZL>, &lut_wrapper<&CPU::r_BGEZL>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>,
            &lut_wrapper<&CPU::r_TGEI>, &lut_wrapper<&CPU::r_TGEIU>, &lut_wrapper<&CPU::r_TLTI>, &lut_wrapper<&CPU::r_TLTIU>, &lut_wrapper<&CPU::r_TEQI>,  &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::r_TNEI>,  &lut_wrapper<&CPU::ERROR>,
            &lut_wrapper<&CPU::r_BLTZAL>, &lut_wrapper<&CPU::r_BGEZAL>, &lut_wrapper<&CPU::r_BLTZALL>, &lut_wrapper<&CPU::r_BGEZALL>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>,
            &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>, &lut_wrapper<&CPU::ERROR>,
        };
        __always_inline void bypass_register();
        __always_inline void detect_ldi();
        /**
         * Called during EX stage, handles the logic execution of each instruction
         */
        void execute_instruction();
        void execute_cp0_instruction(const Instruction& instr);
        void update_pipeline();
        // Fills the pipeline with the first 5 instructions
        void fill_pipeline();

        void clear_registers();

        friend class TKPEmu::N64::N64_TKPWrapper;
        friend class TKPEmu::N64::N64;
        friend class TKPEmu::Applications::N64_RomDisassembly;
        friend class TKPEmu::N64::QA;
    };
}
#endif