#include <iostream>
#include "n64_impl.hxx"

namespace TKPEmu::N64 {
    N64::N64() :
        cpubus_(rcp_), 
        cpu_(cpubus_, rcp_)
    {
        Reset();
    }

    bool N64::LoadCartridge(std::string path) {
        return cpu_.cpubus_.LoadCartridge(path);
    }

    bool N64::LoadIPL(std::string path) {
        if (!path.empty()) {
            return cpu_.cpubus_.LoadIPL(path);
        }
        return false;
    }

    void N64::Update() {
        cpu_.update_pipeline();
    }

    void N64::Reset() {
        cpu_.Reset();
        rcp_.Reset();
    }
}