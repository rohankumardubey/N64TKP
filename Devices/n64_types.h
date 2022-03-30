#ifndef TKPEMU_N64_TYPES_H
#define TKPEMU_N64_TYPES_H
#include <cstdint>
namespace TKPEmu::N64 {
    using MemAddr = uint64_t;
    using MemData_UB = uint8_t;
    using MemData_UH = uint16_t;
    using MemData_UW = uint32_t;
    using MemData_UD = uint64_t;
    using MemData_B = int8_t;
    using MemData_H = int16_t;
    using MemData_W = int32_t;
    using MemData_D = int64_t;
    using MemData_Float = float;
    using MemData_Double = double;
    // This union format idea was copied by Project64
    union MemData_UnionW {
        MemData_W  W;
        MemData_UW UW;
        MemData_H  H[2];
        MemData_UH UH[2];
        MemData_B  B[4];
        MemData_UB UB[4];

        MemData_Float FLOAT;
    };
    union MemData_UnionDW {
        MemData_D  D;
        MemData_UD UD;
        MemData_W  W[2];
        MemData_UW UW[2];
        MemData_H  H[4];
        MemData_UH UH[4];
        MemData_B  B[8];
        MemData_UB UB[8];

        MemData_Double DOUBLE;
        MemData_Float  FLOAT[2];
    };
}
#endif