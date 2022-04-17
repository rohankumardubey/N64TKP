#include "n64_tkpromdisassembly.hxx"
#include <sstream>

namespace TKPEmu::Applications {
    N64_RomDisassembly::N64_RomDisassembly(std::string menu_title, std::string window_title) 
            : IMApplication(menu_title, window_title)
    {
        
    }
    N64_RomDisassembly::~N64_RomDisassembly() {

    }
    void N64_RomDisassembly::v_draw() {
        auto* n64_ptr = static_cast<N64::N64_TKPWrapper*>(emulator_.get());
        for (int i = 0; i < 32; i++) {
            std::stringstream ss;
            ss << "r" << i << ": " << std::hex << n64_ptr->GetCPU().gpr_regs_[i].UW._0;
            ImGui::TextUnformatted(ss.str().c_str());
        }
        if (ImGui::Button("update")) {
            n64_ptr->update();
        }
    }
}