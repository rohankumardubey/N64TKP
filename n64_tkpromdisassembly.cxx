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
        static int selected = 0;
        {
            ImGui::BeginChild("left pane", ImVec2(150, 0), true);
            int i = 0;
            for (auto& mem : memory_views_) {
                if (ImGui::Selectable(mem.Name.c_str(), selected == i)) {
                    selected = i;
                }
                ++i;
            }
            ImGui::EndChild();
        }
        ImGui::SameLine();
        {
            ImGui::BeginGroup();
            ImGui::BeginChild("right pane", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));
            auto& mem = memory_views_[selected];
            if (mem.DataPtr) {
                switch(mem.Type) {
                    case MemoryType::GPR_REGS: {
                        for (uint32_t i = 0; i < mem.Size; ++i) {
                            auto data = (static_cast<N64::MemDataUnionDW*>(mem.DataPtr) + i)->UW._0;
                            ImGui::Text("r%02u:", i);
                            ImGui::SameLine();
                            if (data == 0) {
                                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(180,180,180,255));
                            }
                            ImGui::Text("%08x", data);
                            if (data == 0) {
                                ImGui::PopStyleColor();
                            }
                            if (i % 4 != 3) {
                                ImGui::SameLine();
                            }
                        }
                        break;
                    }
                    case MemoryType::FPR_REGS: {
                        ImGui::Text("TODO");
                        break;
                    }
                    case MemoryType::MEMORY: {
                        for (uint32_t i = 0; i < mem.Size; ++i) {
                            auto data = *(static_cast<uint8_t*>(mem.DataPtr) + i);
                            if (data == 0) {
                                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(180,180,180,255));
                            }
                            ImGui::Text("%02x", data);
                            if (data == 0) {
                                ImGui::PopStyleColor();
                            }
                            if (i % 16 != 15) {
                                ImGui::SameLine();
                            }
                        }
                    }
                }
            } else {
                fill_memory_views();
            }
            ImGui::EndChild();
            ImGui::EndGroup();
        }
        if (ImGui::Button("update")) {
            n64_ptr->update();
        }
    }
    void N64_RomDisassembly::v_reset() {
        for (auto& mem : memory_views_) {
            mem.DataPtr = nullptr;
        }
    }
    void N64_RomDisassembly::fill_memory_views() {
        if (emulator_) {
            auto* n64_ptr = static_cast<N64::N64_TKPWrapper*>(emulator_.get());
            for (auto& mem : memory_views_) {
                switch(mem.Type) {
                    case MemoryType::GPR_REGS: {
                        mem.DataPtr = n64_ptr->GetCPU().gpr_regs_.data();
                        break;
                    }
                    case MemoryType::FPR_REGS: {
                        mem.DataPtr = n64_ptr->GetCPU().fpr_regs_.data();
                        break;
                    }
                    case MemoryType::MEMORY: {
                        mem.DataPtr = n64_ptr->GetCPU().cpubus_.redirect_paddress(mem.StartAddress);
                        break;
                    }
                }
            }
        }
    }
}