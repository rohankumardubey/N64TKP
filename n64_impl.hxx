#pragma once
#ifndef TKP_N64_H
#define TKP_N64_H
#include <string>
#include <GL/glew.h>
#include "n64_cpu.hxx"
#include "n64_rcp.hxx"

namespace TKPEmu {
    namespace Applications {
        class N64_RomDisassembly;
    }
    namespace N64 {
        class N64_TKPWrapper;
    }
}

namespace TKPEmu::N64 {
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
    private:
        Devices::RCP rcp_;
        Devices::CPUBus cpubus_;
        Devices::CPU cpu_;
		friend class TKPEmu::N64::N64_TKPWrapper;
        friend class TKPEmu::Applications::N64_RomDisassembly;
    };
}
#endif