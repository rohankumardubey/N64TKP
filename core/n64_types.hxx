#pragma once
#ifndef TKP_N64_TYPES_H
#define TKP_N64_TYPES_H
#include <cstdint>
#include <limits>
#include <immintrin.h>
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
    using EndianType = decltype(std::endian::native);

    /**
        This class represents the ordering of an instruction cache line

        @see manual, 11.2.1
    */
    template <EndianType>
    union InstructionCacheLineBase;
    template<>
    union InstructionCacheLineBase<std::endian::little> {
        struct {
            uint32_t _1_PTag : 20; // Physical address tag
            uint32_t _1_V    : 1;  // Valid bit
            uint32_t _1_rest : 11;
            uint32_t _2      : 32;
            uint32_t _3      : 32;
            uint32_t _4      : 32;
            uint32_t _5      : 32;
            uint32_t _6      : 32;
            uint32_t _7      : 32;
            uint32_t _8      : 32;
        } Data;
        __m256 Full;
    };
    template<>
    union InstructionCacheLineBase<std::endian::big> {
        struct {
            uint32_t _8      : 32;
            uint32_t _7      : 32;
            uint32_t _6      : 32;
            uint32_t _5      : 32;
            uint32_t _4      : 32;
            uint32_t _3      : 32;
            uint32_t _2      : 32;
            uint32_t _1_rest : 11;
            uint32_t _1_V    : 1;  // Valid bit
            uint32_t _1_PTag : 20; // Physical address tag
        } Data;
        __m256 Full;
    };
    /**
        This union describes how the inner bits of the 32 bit instructions are used

        @see manual, 1.4.3
    */
    template <EndianType>
    union InstructionBase; // base case is purposefully not defined
    template<>
    union InstructionBase<std::endian::little> {
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
        uint32_t Full : 32;
    };
    template<>
    union InstructionBase<std::endian::big> {
        struct {
            uint32_t op        : 6;  // operation code
            uint32_t rs        : 5;  // source register number (r0-r31)
            uint32_t rt        : 5;  // target (source/destination) register number
            uint32_t immediate : 16; // immediate value
        } IType; // Immediate
        struct {
            uint32_t op        : 6;  // operation code
            uint32_t target    : 26; // unconditional branch target address
        } JType; // Jump
        struct {
            uint32_t op        : 6;  // operation code
            uint32_t rs        : 5;  // source register number
            uint32_t rt        : 5;  // target register number
            uint32_t rd        : 5;  // destination register number
            uint32_t sa        : 5;  // shift amount
            uint32_t func      : 6;  // function field
        } RType; // Register
        uint32_t Full : 32;
    };
    template<EndianType>
    union MemDataUnionDWBase;
    template<>
    union MemDataUnionDWBase <std::endian::little> {
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
    template<>
    union MemDataUnionDWBase <std::endian::big> {
        MemDataD  D;
        MemDataUD UD;
        struct {
            MemDataW     _1;
            MemDataW     _0;
        } W;
        struct {
            MemDataUW    _1;
            MemDataUW    _0;
        } UW;
        struct {
            MemDataH     _3;
            MemDataH     _2;
            MemDataH     _1;
            MemDataH     _0;
        } H;
        struct {
            MemDataUH    _3;
            MemDataUH    _2;
            MemDataUH    _1;
            MemDataUH    _0;
        } UH;
        struct {
            MemDataB     _7;
            MemDataB     _6;
            MemDataB     _5;
            MemDataB     _4;
            MemDataB     _3;
            MemDataB     _2;
            MemDataB     _1;
            MemDataB     _0;
        } B;
        struct {
            MemDataUB    _7;
            MemDataUB    _6;
            MemDataUB    _5;
            MemDataUB    _4;
            MemDataUB    _3;
            MemDataUB    _2;
            MemDataUB    _1;
            MemDataUB    _0;
        } UB;

        MemDataDouble DOUBLE;
        struct {
            MemDataFloat _1;
            MemDataFloat _0;
        } FLOAT;
    };
    using MemDataUnionDW = MemDataUnionDWBase<std::endian::native>;
    using Instruction = InstructionBase<std::endian::native>;
    using InstructionCacheLine = InstructionCacheLineBase<std::endian::native>;
    static_assert(sizeof(InstructionCacheLine) == 32, "InstructionCacheLine should be 32 bytes!");
    static_assert(sizeof(Instruction) == sizeof(uint32_t), "N64 instruction should be 4 bytes");
    static_assert(sizeof(MemDataDouble) == 8, "double data type is not 8 bytes!");
    static_assert(sizeof(MemDataUnionDW) == sizeof(MemDataD), "Size of MemDataUnionDW mismatch!");
    static_assert(std::numeric_limits<MemDataFloat>::is_iec559, "float data type is not ISO/IEC/IEEE 60559:2011 compliant!");
}
#endif