#pragma once
#ifndef TKP_N64_H
#define TKP_N64_H
#include <string>
#include "n64_cpu.hxx"
#include "n64_rcp.hxx"

namespace TKPEmu::N64 {
    class N64_TKPWrapper;
    class N64 {
    public:
        N64();
        bool LoadCartridge(std::string path);
        bool LoadIPL(std::string path);
        void Update();
        void Reset();
        void* GetColorData() {
            return rcp_.framebuffer_ptr_;
        }
        int GetWidth() {
            return rcp_.width_;
        }
        int GetHeight() {
            return rcp_.height_;
        }
        int GetBitdepth() {
            return rcp_.bitdepth_;
        }
    private:
        Devices::RCP rcp_;
        Devices::CPUBus cpubus_;
        Devices::CPU cpu_;
        friend class N64_TKPWrapper;
    };
}
#endif