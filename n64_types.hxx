#pragma once
#ifndef TKP_N64_COMMON_TYPES_H
#define TKP_N64_COMMON_TYPES_H
#include <cstdint>
#include <limits>
#include <immintrin.h>
#include <array>

namespace TKPEmu::N64 {
    constexpr uint32_t EMPTY_INSTRUCTION = 0xFFFFFFFF;
    // Note: manual here refers to vr4300 manual
    union TLBEntry32 {
        struct {
            uint32_t _0_unused1 : 1;
            uint32_t _0_V       : 1;
            uint32_t _0_D       : 1;
            uint32_t _0_C       : 3;
            uint32_t _0_PFN     : 20;
            uint32_t _0_unused2 : 6;
            uint32_t _1_unused1 : 1;
            uint32_t _1_V       : 1;
            uint32_t _1_D       : 1;
            uint32_t _1_C       : 3;
            uint32_t _1_PFN     : 20;
            uint32_t _1_unused2 : 6;
            uint32_t _2_ASID    : 8;
            uint32_t _2_unused1 : 8;
            uint32_t _2_G       : 8;
            uint32_t _2_VPN2    : 8;
            uint32_t _3_unused1 : 13;
            uint32_t _3_MASK    : 12;
            uint32_t _3_unused2 : 7;
            
        } Data;
        __m128 Full;
    };
    /**
        This class represents the ordering of an instruction cache line

        @see manual 11.2.1
    */
    union InstructionCacheLineBase {
        struct {
            uint32_t _0_PTag : 20; // Physical address tag
            uint32_t _0_V    : 1;  // Valid bit
            uint32_t _0_rest : 11;
            uint32_t _1      : 32;
            uint32_t _2      : 32;
            uint32_t _3      : 32;
            uint32_t _4      : 32;
            uint32_t _5      : 32;
            uint32_t _6      : 32;
            uint32_t _7      : 32;
        } Data;
        __m256 Full;
    };
    /**
        This union describes how the inner bits of the 32 bit instructions are used

        @see manual 1.4.3
    */
    union InstructionBase {
        struct {
            uint32_t immediate : 16; // immediate value
            uint32_t rt        : 5;  // target (source/destination) register number
            uint32_t rs        : 5;  // source register number (r0-r31)
            uint32_t op        : 6;  // operation code
        } IType; // Immediate
        struct {
            uint32_t target    : 26; // unconditional branch target address
            uint32_t op        : 6;  // operation code
        } JType; // Jump
        struct {
            uint32_t func      : 6;  // function field
            uint32_t sa        : 5;  // shift amount
            uint32_t rd        : 5;  // destination register number
            uint32_t rt        : 5;  // target register number
            uint32_t rs        : 5;  // source register number
            uint32_t op        : 6;  // operation code
        } RType; // Register
        uint32_t Full;
    };
    union MemDataUnionDWBase {
        int64_t  D;
        uint64_t UD;
        struct {
            int32_t     _0;
            int32_t     _1;
        } W;
        struct {
            uint32_t    _0;
            uint32_t    _1;
        } UW;
        struct {
            int16_t     _0;
            int16_t     _1;
            int16_t     _2;
            int16_t     _3;
        } H;
        struct {
            uint16_t    _0;
            uint16_t    _1;
            uint16_t    _2;
            uint16_t    _3;
        } UH;
        struct {
            int8_t     _0;
            int8_t     _1;
            int8_t     _2;
            int8_t     _3;
            int8_t     _4;
            int8_t     _5;
            int8_t     _6;
            int8_t     _7;
        } B;
        struct {
            uint8_t    _0;
            uint8_t    _1;
            uint8_t    _2;
            uint8_t    _3;
            uint8_t    _4;
            uint8_t    _5;
            uint8_t    _6;
            uint8_t    _7;
        } UB;

        double DOUBLE;
        struct {
            float _0;
            float _1;
        } FLOAT;
    };
    const static std::array<std::string, 64> OperationCodes = {
        "special", "regimm", "j", "jal", "beq", "bne", "blez", "bgtz",
        "addi", "addiu", "slti", "stliu", "andi", "ori", "xori", "lui",
        "cop0", "cop1", "cop2", "23err", "beql", "bnel", "blezl", "bgtzl",
        "daddi", "daddiu", "ldl", "ldr", "34err", "35err", "36err", "37err",
        "lb", "lh", "lwl", "lw", "lbu", "lhu", "lwr", "lwu",
        "sb", "sh", "swl", "sw", "sdl", "sdr", "swr", "cache",
        "ll", "lwc1", "lwc2", "63err", "lld", "ldc1", "ldc2", "ld",
        "sc", "swc1", "swc2", "73err", "scd", "sdc1", "sdc2", "sd",
    };
    const static std::array<std::string, 64> SpecialCodes = {
        "sll", "s2err", "srl", "sra", "sllv", "s5err", "srlv", "srav",
        "jr", "jalr", "s12err", "s13err", "syscall", "break", "s16err", "sync",
        "mfhi", "mthi", "mflo", "mtlo", "dsllv", "s25err", "dsrlv", "dsrav",
        "mult", "multu", "div", "divu", "dmult", "dmultu", "ddiv", "ddivu",
        "add", "addu", "sub", "subu", "and", "or", "xor", "nor",
        "s50err", "s51err", "slt", "sltu", "dadd", "daddu", "dsub", "dsubu",
        "tge", "tgeu", "tlt", "tltu", "teq", "s65err", "tne", "s67err",
        "dsll", "s71err", "dsrl", "dsra", "dsll32", "s75err", "dsrl32", "dsra32",
    };
    enum class InstructionType {
        SPECIAL, REGIMM, J, JAL, BEQ, BNE, BLEZ, BGTZ,
        ADDI, ADDIU, SLTI, STLIU, ANDI, ORI, XORI, LUI,
        COP0, COP1, COP2, BEQL, BNEL, BLEZL, BGTZL,
        DADDI, DADDIU, LDL, LDR,
        LB, LH, LWL, LW, LBU, LHU, LWR, LWU,
        SB, SH, SWL, SW, SDL, SDR, SWR, CACHE,
        LL, LWC1, LWC2, _63ERR, LLD, LDC1, LDC2, LD,
        SC, SWC1, SWC2, _73ERR, SCD, SDC1, SDC2, SD,
        
        s_SLL, s_SRL, s_SRA, s_SLLV, s_SRLV, s_SRAV,
        s_JR, s_JALR, s_SYSCALL, s_BREAK, s_SYNC,
        s_MFHI, s_MTHI, s_MFLO, s_MTLO, s_DSLLV, s_DSRLV, s_DSRAV,
        s_MULT, s_MULTU, s_DIV, s_DIVU, s_DMULT, s_DMULTU, s_DDIV, s_DDIVU,
        s_ADD, s_ADDU, s_SUB, s_SUBU, s_AND, s_OR, s_XOR, s_NOR,
        s_SLT, s_SLTU, s_DADD, s_DADDU, s_DSUB, s_DSUBU,
        s_TGE, s_TGEU, s_TLT, s_TLTU, s_TEQ, s_TNE,
        s_DSLL, s_DSRL, s_DSRA, s_DSLL32, s_DSRL32, s_DSRA32,

        NOP,
        ERROR
    };
    /**
     * AccessType table for L&S common funcs
     * 
     * @see manual Table 16-3
     */
    enum AccessType {
        UBYTE       = 1,
        BYTE        = 1,
        UHALFWORD   = 2,
        HALFWORD    = 2,
        UWORD       = 4,
        WORD        = 4,
        UDOUBLEWORD = 8,
        DOUBLEWORD  = 8,
        UDOUBLEWORD_DIRECT = 8,
        NONE
    };
    enum class WriteType {
        REGISTER,     // for writing to register on EX
        LATEREGISTER, // for writing to register on WB
        MMU,          // for writing to mmu
        NONE,         // don't write anything
    };
    constexpr static std::array<InstructionType, 64> InstructionTypeTable = {
        InstructionType::SPECIAL, InstructionType::REGIMM, InstructionType::J, InstructionType::JAL, InstructionType::BEQ, InstructionType::BNE, InstructionType::BLEZ, InstructionType::BGTZ,
        InstructionType::ADDI, InstructionType::ADDIU, InstructionType::SLTI, InstructionType::STLIU, InstructionType::ANDI, InstructionType::ORI, InstructionType::XORI, InstructionType::LUI,
        InstructionType::COP0, InstructionType::COP1, InstructionType::COP2, InstructionType::ERROR, InstructionType::BEQL, InstructionType::BNEL, InstructionType::BLEZL, InstructionType::BGTZL,
        InstructionType::DADDI, InstructionType::DADDIU, InstructionType::LDL, InstructionType::LDR, InstructionType::ERROR, InstructionType::ERROR, InstructionType::ERROR, InstructionType::ERROR,
        InstructionType::LB, InstructionType::LH, InstructionType::LWL, InstructionType::LW, InstructionType::LBU, InstructionType::LHU, InstructionType::LWR, InstructionType::LWU,
        InstructionType::SB, InstructionType::SH, InstructionType::SWL, InstructionType::SW, InstructionType::SDL, InstructionType::SDR, InstructionType::SWR, InstructionType::CACHE,
        InstructionType::LL, InstructionType::LWC1, InstructionType::LWC2, InstructionType::ERROR, InstructionType::LLD, InstructionType::LDC1, InstructionType::LDC2, InstructionType::LD,
        InstructionType::SC, InstructionType::SWC1, InstructionType::SWC2, InstructionType::ERROR, InstructionType::SCD, InstructionType::SDC1, InstructionType::SDC2, InstructionType::SD,
    };
    constexpr static std::array<InstructionType, 64> SpecialInstructionTypeTable = {
        InstructionType::s_SLL, InstructionType::ERROR, InstructionType::s_SRL, InstructionType::s_SRA, InstructionType::s_SLLV, InstructionType::ERROR, InstructionType::s_SRLV, InstructionType::s_SRAV,
        InstructionType::s_JR, InstructionType::s_JALR, InstructionType::ERROR, InstructionType::ERROR, InstructionType::s_SYSCALL, InstructionType::s_BREAK, InstructionType::ERROR, InstructionType::s_SYNC,
        InstructionType::s_MFHI, InstructionType::s_MTHI, InstructionType::s_MFLO, InstructionType::s_MTLO, InstructionType::s_DSLLV, InstructionType::ERROR, InstructionType::s_DSRLV, InstructionType::s_DSRAV,
        InstructionType::s_MULT, InstructionType::s_MULTU, InstructionType::s_DIV, InstructionType::s_DIVU, InstructionType::s_DMULT, InstructionType::s_DMULTU, InstructionType::s_DDIV, InstructionType::s_DDIVU,
        InstructionType::s_ADD, InstructionType::s_ADDU, InstructionType::s_SUB, InstructionType::s_SUBU, InstructionType::s_AND, InstructionType::s_OR, InstructionType::s_XOR, InstructionType::s_NOR,
        InstructionType::ERROR, InstructionType::ERROR, InstructionType::s_SLT, InstructionType::s_SLTU, InstructionType::s_DADD, InstructionType::s_DADDU, InstructionType::s_DSUB, InstructionType::s_DSUBU,
        InstructionType::s_TGE, InstructionType::s_TGEU, InstructionType::s_TLT, InstructionType::s_TLTU, InstructionType::s_TEQ, InstructionType::ERROR, InstructionType::s_TNE, InstructionType::ERROR,
        InstructionType::s_DSLL, InstructionType::ERROR, InstructionType::s_DSRL, InstructionType::s_DSRA, InstructionType::s_DSLL32, InstructionType::ERROR, InstructionType::s_DSRL32, InstructionType::s_DSRA32,
    };
    using MemDataUnionDW = MemDataUnionDWBase;
    using Instruction = InstructionBase;
    using InstructionCacheLine = InstructionCacheLineBase;
    using GPRRegister = MemDataUnionDW;
    static_assert(std::endian::native == std::endian::little, "This emulator does not work on big endian systems!");
    static_assert(sizeof(InstructionCacheLine) == 32, "InstructionCacheLine should be 32 bytes!");
    static_assert(sizeof(Instruction) == sizeof(uint32_t), "N64 instruction should be 4 bytes!");
    static_assert(sizeof(double) == 8, "double data type is not 8 bytes!");
    static_assert(sizeof(MemDataUnionDW) == sizeof(int64_t), "Size of MemDataUnionDW mismatch!");
    static_assert(std::numeric_limits<float>::is_iec559, "float data type is not ISO/IEC/IEEE 60559:2011 compliant!");
}
#endif