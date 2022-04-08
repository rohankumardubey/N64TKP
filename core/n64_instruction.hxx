#pragma once
#ifndef TKP_N64_OPCODE_H
#define TKP_N64_OPCODE_H
#include <cstdint>
#include <bit>
namespace TKPEmu::N64::Devices {
    using EndianType = decltype(std::endian::native);
    // This union describes how the inner bits of the 32 bit instructions are used
    // Source: VR4300 manual, 1.4.3
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
    using Instruction = InstructionBase<std::endian::native>;
    static_assert(sizeof(Instruction) == sizeof(uint32_t), "N64 instruction should be 4 bytes");
}
#endif