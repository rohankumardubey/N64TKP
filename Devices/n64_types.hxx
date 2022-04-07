#ifndef TKP_N64_TYPES_H
#define TKP_N64_TYPES_H
#include <cstdint>
namespace TKPEmu::N64 {
    using MemAddr = uint64_t;
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
    // If your system is big endian, MemDataUnionDWBase<std::endian::big> is used
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
}
#endif