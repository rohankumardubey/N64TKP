#ifndef TKP_N64_ADDRESSES_H
#define TKP_N64_ADDRESSES_H
#include <cstdint>
#define addr constexpr uint32_t

// Video Interface
addr VI_CTRL_REG        = 0x0440'0000;
addr VI_ORIGIN_REG      = 0x0440'0004;
addr VI_WIDTH_REG       = 0x0440'0008;
addr VI_V_INTR_REG      = 0x0440'000C;
addr VI_V_CURRENT_REG   = 0x0440'0010;
addr VI_BURST_REG       = 0x0440'0014;
addr VI_V_SYNC_REG      = 0x0440'0018;
addr VI_H_SYNC_REG      = 0x0440'001C;
addr VI_H_SYNC_LEAP_REG = 0x0440'0020;
addr VI_H_VIDEO_REG     = 0x0440'0024;
addr VI_V_VIDEO_REG     = 0x0440'0028;
addr VI_V_BURST_REG     = 0x0440'002C;
addr VI_X_SCALE_REG     = 0x0440'0030;
addr VI_Y_SCALE_REG     = 0x0440'0034;
addr VI_TEST_ADDR_REG   = 0x0440'0038;
addr VI_STAGED_DATA_REG = 0x0440'003C;

// Peripheral Interface
addr PI_DRAM_ADDR_REG    = 0x0460'0000;
addr PI_CART_ADDR_REG    = 0x0460'0004;
addr PI_RD_LEN_REG       = 0x0460'0008;
addr PI_WR_LEN_REG       = 0x0460'000C;
addr PI_STATUS_REG       = 0x0460'0010;
addr PI_BSD_DOM1_LAT_REG = 0x0460'0014;
addr PI_BSD_DOM1_PWD_REG = 0x0460'0018;
addr PI_BSD_DOM1_PGS_REG = 0x0460'001C;
addr PI_BSD_DOM1_RLS_REG = 0x0460'0020;
addr PI_BSD_DOM2_LAT_REG = 0x0460'0024;
addr PI_BSD_DOM2_PWD_REG = 0x0460'0028;
addr PI_BSD_DOM2_PGS_REG = 0x0460'002C;
addr PI_BSD_DOM2_RLS_REG = 0x0460'0030;

#undef addr
#endif