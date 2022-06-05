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

    void N64::clear_screen() {
        // if (rcp_.screen_color_data_) {
        //     for (std::size_t i = 0; i < 320 * 240 * 4; i++) {
        //         if ((i & 0b11) == 0b11)
        //             rcp_.screen_color_data_[i] = 1.0f;
        //     }
        // }
    }
}