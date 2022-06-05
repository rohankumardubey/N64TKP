#include "n64_impl.hxx"

namespace TKPEmu::N64 {
    N64::N64(GLuint& text_format) : text_format_(text_format), cpubus_(rcp_), cpu_(cpubus_, rcp_, text_format) {

    }

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