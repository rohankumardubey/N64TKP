#pragma once
#ifndef TKP_N64_COMMON_TYPES_H
#define TKP_N64_COMMON_TYPES_H
#include <cstdint>
#include <limits>
#include <immintrin.h>
#include <array>
#include <string>
#include <bit>
#ifndef __cpp_lib_endian
static_assert(false && "std::endian not found");
#endif

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
        NONE
    };
    enum class WriteType {
        REGISTER,     // for writing to register on EX
        LATEREGISTER, // for writing to register on WB
        MMU,          // for writing to mmu
        NONE,         // don't write anything
    };
    constexpr static uint32_t CPzOPERATION_BIT = 0b00000010'00000000'00000000'00000000;
    constexpr static uint32_t CPz_TO_GPR = 0b00010;
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