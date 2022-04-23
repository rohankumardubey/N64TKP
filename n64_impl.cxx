#include "n64_impl.hxx"

namespace TKPEmu::N64 {
    bool N64::LoadCartridge(std::string path) {
        return cpu_.cpubus_.LoadFromFile(path);
    }
    
    void N64::Update() {
        cpu_.update_pipeline();
    }
    
    void N64::Reset() {
        cpu_.Reset();
    }
}