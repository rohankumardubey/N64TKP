#ifndef TKPEMU_N64_TYPES_H
#define TKPEMU_N64_TYPES_H
#include <cstdint>
namespace TKPEmu::N64 {
    // This union format was copied by Project64, very helpful
    union Word {
        int32_t  W;
        uint32_t UW;
        int16_t  HW[2];
        uint16_t UHW[2];
        int8_t   B[4];
        uint8_t  UB[4];

        float    F;
    };
    union DoubleWord {
        int64_t  DW;
        uint64_t UDW;
        int32_t  W[2];
        uint32_t UW[2];
        int16_t  HW[4];
        uint16_t UHW[4];
        int8_t   B[8];
        uint8_t  UB[8];

        double   D;
        float    F[2];
    };
}
#endif