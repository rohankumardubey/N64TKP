#pragma once
#ifndef TKPEMU_N64_OPCODE_H
#define TKPEMU_N64_OPCODE_H
#include <cstdint>

namespace TKPEmu::N64::Devices {
    // This union describes how the inner bits of the 32 bit instructions are used
    // Source: VR4300 manual, 1.4.3
    union Instruction {
        struct {
            uint16_t immediate : 16; // immediate value
            uint8_t  rt        : 5;  // target (source/destination) register number
            uint8_t  rs        : 5;  // source register number (r0-r31)
            uint8_t  op        : 6;  // operation code

        } IType; // Immediate
        struct {
            uint8_t  target    : 26; // unconditional branch target address
            uint8_t  op        : 6;  // operation code
        } JType; // Jump
        struct {
            uint8_t  func      : 6;  // function field
            uint8_t  sa        : 5;  // shift amount
            uint8_t  rd        : 5;  // destination register number
            uint8_t  rt        : 5;  // target register number
            uint8_t  rs        : 5;  // source register number
            uint8_t  op        : 6;  // operation code
        } RType; // Register
        uint32_t Full;
    };
}
#endif