#ifndef TKP_N64_ADDRESSES_H
#define TKP_N64_ADDRESSES_H
#include <cstdint>
#define addr constexpr uint32_t

// RSP internal registers
addr RSP_DMA_SPADDR      = 0x0404'0000;
addr RSP_DMA_RAMADDR     = 0x0404'0004;
addr RSP_DMA_RDLEN       = 0x0404'0008;
addr RSP_DMA_WRLEN       = 0x0404'000C;
addr RSP_STATUS          = 0x0404'0010;
addr RSP_DMA_FULL        = 0x0404'0014;
addr RSP_DMA_BUSY        = 0x0404'0018;
addr RSP_SEMAPHORE       = 0x0404'001C;
addr RSP_PC              = 0x0408'0000;

// MIPS Interface
addr MI_MODE             = 0x0430'0000;
addr MI_INTERRUPT        = 0x0430'0008;
addr MI_MASK             = 0x0430'000C;

// Video Interface
addr VI_CTRL         = 0x0440'0000;
addr VI_ORIGIN       = 0x0440'0004;
addr VI_WIDTH        = 0x0440'0008;
addr VI_V_INTR       = 0x0440'000C;
addr VI_V_CURRENT    = 0x0440'0010;
addr VI_BURST        = 0x0440'0014;
addr VI_V_SYNC       = 0x0440'0018;
addr VI_H_SYNC       = 0x0440'001C;
addr VI_H_SYNC_LEAP  = 0x0440'0020;
addr VI_H_VIDEO      = 0x0440'0024;
addr VI_V_VIDEO      = 0x0440'0028;
addr VI_V_BURST      = 0x0440'002C;
addr VI_X_SCALE      = 0x0440'0030;
addr VI_Y_SCALE      = 0x0440'0034;
addr VI_TEST_ADDR    = 0x0440'0038;
addr VI_STAGED_DATA  = 0x0440'003C;

// Audio Interface
addr AI_DRAM_ADDR    = 0x0450'0000;
addr AI_LEN          = 0x0450'0004;
addr AI_CONTROL      = 0x0450'0008;
addr AI_STATUS       = 0x0450'000C;
addr AI_DACRATE      = 0x0450'0010;
addr AI_BITRATE      = 0x0450'0014;

// Peripheral Interface
addr PI_DRAM_ADDR    = 0x0460'0000;
addr PI_CART_ADDR    = 0x0460'0004;
addr PI_RD_LEN       = 0x0460'0008;
addr PI_WR_LEN       = 0x0460'000C;
addr PI_STATUS       = 0x0460'0010;
addr PI_BSD_DOM1_LAT = 0x0460'0014;
addr PI_BSD_DOM1_PWD = 0x0460'0018;
addr PI_BSD_DOM1_PGS = 0x0460'001C;
addr PI_BSD_DOM1_RLS = 0x0460'0020;
addr PI_BSD_DOM2_LAT = 0x0460'0024;
addr PI_BSD_DOM2_PWD = 0x0460'0028;
addr PI_BSD_DOM2_PGS = 0x0460'002C;
addr PI_BSD_DOM2_RLS = 0x0460'0030;

// RDRAM Interface
addr RI_MODE             = 0x0470'0000;
addr RI_CONFIG           = 0x0470'0004;
addr RI_CURRENT_LOAD     = 0x0470'0008;
addr RI_SELECT           = 0x0470'000C;

// Serial Interface
addr SI_DRAM_ADDR        = 0x0480'0000;
addr SI_PIF_AD_RD64B     = 0x0480'0004;
addr SI_PIF_AD_WR4B      = 0x0480'0008;
addr SI_PIF_AD_WR64B     = 0x0480'0010;
addr SI_PIF_AD_RD4B      = 0x0480'0014;
addr SI_STATUS           = 0x0480'0018;

// PIF RAM
addr PIF_COMMAND         = 0x1FC0'07FC;

#undef addr
#endif