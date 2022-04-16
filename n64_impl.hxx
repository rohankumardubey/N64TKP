#pragma once
#ifndef TKP_N64_H
#define TKP_N64_H
#include <string>
#include "core/n64_cpu.hxx"

namespace TKPEmu::N64 {
    class N64 {
    public:
        bool LoadCartridge(std::string path);
    private:
        Devices::CPU cpu_;
        Devices::CPUBus cpubus_;
    };
}
#endif