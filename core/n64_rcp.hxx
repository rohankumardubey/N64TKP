#pragma once
#ifndef TKP_N64_RCP_H
#define TKP_N64_RCP_H
#include <array>
#include <cstdint>
#include <GL/glew.h>

namespace TKPEmu::N64 {
    class N64;
    namespace Devices {
        class CPUBus;
        class CPU;
    }
}

namespace TKPEmu::N64::Devices {
    class RCP {
    public:
        void Reset();
    private:
        int width_ = 320, height_ = 240;
        GLenum bitdepth_ = GL_UNSIGNED_BYTE;
		uint8_t* framebuffer_ptr_ = nullptr;
        // RSP internal registers
        uint32_t rsp_status_ = 0;
        uint32_t rsp_dma_busy_ = 0;
        uint32_t rsp_pc_ = 0;
        // Video Interface
        uint32_t vi_ctrl_ = 0;
        uint32_t vi_origin_ = 0;
        uint32_t vi_width_ = 0;
        uint32_t vi_v_intr_ = 0;
        uint32_t vi_v_current_ = 0;
        uint32_t vi_burst_ = 0;
        uint32_t vi_v_sync_ = 0;
        uint32_t vi_h_sync_ = 0;
        uint32_t vi_h_sync_leap_ = 0;
        uint32_t vi_h_video_ = 0;
        uint32_t vi_v_video_ = 0;
        uint32_t vi_v_burst_ = 0;
        uint32_t vi_x_scale_ = 0;
        uint32_t vi_y_scale_ = 0;
        uint32_t vi_test_addr_ = 0;
        uint32_t vi_staged_data_ = 0;
        // Called from cpubus when a relevant register is changed
        friend class TKPEmu::N64::N64;
        friend class TKPEmu::N64::Devices::CPUBus;
        friend class TKPEmu::N64::Devices::CPU;
    };
}
#endif