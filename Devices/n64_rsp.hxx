#pragma once
#ifndef TKP_N64_RSP_H
#define TKP_N64_RSP_H
#include <cstdint>
#include <array>

namespace TKPEmu::N64::Devices {
    // Reality Signal Processor
    // Apparently processes vertices, like a vertex shader, and passes them to RDP
    class RSP {
    public:
        RSP();
    private:
        ScalarUnit scalar_unit_;
        VectorUnit vector_unit_;
        SystemControl system_control_;
    };
    // Scalar unit, a MIPS R400-based CPU
    class ScalarUnit {
        
    };
    // A co-processor that performs vector operations
    class VectorUnit {

    private:
        // The vector unit contains 32 128-bit registers
        // Each register is 128 bits to operate 8 16-bit vectors
        using VReg = std::array<uint16_t, 8>;
        std::array<VReg, 32> registers_;
    };
    // A co-processor that provides DMA func. and controls RDP
    class SystemControl {

    };
}
#endif