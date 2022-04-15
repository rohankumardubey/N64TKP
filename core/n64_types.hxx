#pragma once
#ifndef TKP_N64_TYPES_H
#define TKP_N64_TYPES_H
#include <cstdint>
#include <limits>
#include <immintrin.h>
#include <array>

namespace TKPEmu::N64 {
    // Note: manual here refers to vr4300 manual
    using MemAddr = uint64_t;
    using MemDataBit = bool;
    using MemDataUB = uint8_t;
    using MemDataUH = uint16_t;
    using MemDataUW = uint32_t;
    using MemDataUD = uint64_t;
    using MemDataB = int8_t;
    using MemDataH = int16_t;
    using MemDataW = int32_t;
    using MemDataD = int64_t;
    using MemDataFloat = float;
    using MemDataDouble = double;
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

        @see manual, 11.2.1
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

        @see manual, 1.4.3
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
        MemDataD  D;
        MemDataUD UD;
        struct {
            MemDataW     _0;
            MemDataW     _1;
        } W;
        struct {
            MemDataUW    _0;
            MemDataUW    _1;
        } UW;
        struct {
            MemDataH     _0;
            MemDataH     _1;
            MemDataH     _2;
            MemDataH     _3;
        } H;
        struct {
            MemDataUH    _0;
            MemDataUH    _1;
            MemDataUH    _2;
            MemDataUH    _3;
        } UH;
        struct {
            MemDataB     _0;
            MemDataB     _1;
            MemDataB     _2;
            MemDataB     _3;
            MemDataB     _4;
            MemDataB     _5;
            MemDataB     _6;
            MemDataB     _7;
        } B;
        struct {
            MemDataUB    _0;
            MemDataUB    _1;
            MemDataUB    _2;
            MemDataUB    _3;
            MemDataUB    _4;
            MemDataUB    _5;
            MemDataUB    _6;
            MemDataUB    _7;
        } UB;

        MemDataDouble DOUBLE;
        struct {
            MemDataFloat _0;
            MemDataFloat _1;
        } FLOAT;
    };

    const static std::array<std::string, 64> OperationCodes = {
        "SPECIAL", "REGIMM ", "J      ", "JAL    ", "BEQ    ", "BNE    ", "BLEZ   ", "BGTZ   ",
        "ADDI   ", "ADDIU  ", "SLTI   ", "STLIU  ", "ANDI   ", "ORI    ", "XORI   ", "LUI    ",
        "COP0   ", "COP1   ", "COP2   ", "23ERR  ", "BEQL   ", "BNEL   ", "BLEZL  ", "BGTZL  ",
        "DADDI  ", "DADDIU ", "LDL    ", "LDR    ", "34ERR  ", "35ERR  ", "36ERR  ", "37ERR  ",
        "LB     ", "LH     ", "LWL    ", "LW     ", "LBU    ", "LHU    ", "LWR    ", "LWU    ",
        "SB     ", "SH     ", "SWL    ", "SW     ", "SDL    ", "SDR    ", "SWR    ", "CACHE  ",
        "LL     ", "LWC1   ", "LWC2   ", "63ERR  ", "LLD    ", "LDC1   ", "LDC2   ", "LD     ",
        "SC     ", "SWC1   ", "SWC2   ", "73ERR  ", "SCD    ", "SDC1   ", "SDC2   ", "SD     ",
    };

    const static std::array<std::string, 64> SpecialCodes = {
        "SLL    ", "S2ERR  ", "SRL    ", "SRA    ", "SLLV   ", "S5ERR  ", "SRLV   ", "SRAV   ",
        "JR     ", "JALR   ", "S12ERR ", "S13ERR ", "SYSCALL", "BREAK  ", "S16ERR ", "SYNC   ",
        "MFHI   ", "MTHI   ", "MFLO   ", "MTLO   ", "DSLLV  ", "S25ERR ", "DSRLV  ", "DSRAV  ",
        "MULT   ", "MULTU  ", "DIV    ", "DIVU   ", "DMULT  ", "DMULTU ", "DDIV   ", "DDIVU  ",
        "ADD    ", "ADDU   ", "SUB    ", "SUBU   ", "AND    ", "OR     ", "XOR    ", "NOR    ",
        "S50ERR ", "S51ERR ", "SLT    ", "SLTU   ", "DADD   ", "DADDU  ", "DSUB   ", "DSUBU  ",
        "TGE    ", "TGEU   ", "TLT    ", "TLTU   ", "TEQ    ", "S65ERR ", "TNE    ", "S67ERR ",
        "DSLL   ", "S71ERR ", "DSRL   ", "DSRA   ", "DSLL32 ", "S75ERR ", "DSRL32 ", "DSRA32 ",
    };
    
    using MemDataUnionDW = MemDataUnionDWBase;
    using Instruction = InstructionBase;
    using InstructionCacheLine = InstructionCacheLineBase;
    static_assert(std::endian::native == std::endian::little, "This emulator does not work on big endian systems!");
    static_assert(sizeof(InstructionCacheLine) == 32, "InstructionCacheLine should be 32 bytes!");
    static_assert(sizeof(Instruction) == sizeof(uint32_t), "N64 instruction should be 4 bytes!");
    static_assert(sizeof(MemDataDouble) == 8, "double data type is not 8 bytes!");
    static_assert(sizeof(MemDataUnionDW) == sizeof(MemDataD), "Size of MemDataUnionDW mismatch!");
    static_assert(std::numeric_limits<MemDataFloat>::is_iec559, "float data type is not ISO/IEC/IEEE 60559:2011 compliant!");
}
#endif