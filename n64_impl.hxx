#pragma once
#ifndef TKP_N64_H
#define TKP_N64_H
#include <string>
#include "core/n64_cpu.hxx"

namespace TKPEmu::N64 {
    class N64 {
    public:
        bool LoadCartridge(std::string path);
        void Update();
        void Reset();
        Devices::CPU& GetCPU() {
            return cpu_;
        }
    private:
        Devices::CPU cpu_;
    };
}
#endif