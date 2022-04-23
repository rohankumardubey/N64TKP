#include "n64_tkpromdisassembly.hxx"
#include "../include/emulator_disassembler.hxx"
#include <sstream>
#include <iostream>

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
                    case MemoryType::PIPELINE: {
                        auto& n64cpu = n64_ptr->GetCPU();
                        ImGui::BeginChild("pipeline view", ImVec2(0, 300));
                        static std::array<std::string, 6> chars {"IC", "RF", "EX", "DC", "WB", "  "};
                        ImGui::Text("Stages:");
                        ImGui::Separator();
                        for (size_t i = 0; i < n64cpu.pipeline_.size(); i++) {
                            int cur_num = static_cast<int>(n64cpu.pipeline_[i]);
                            std::string cur_stage = chars[cur_num];
                            auto cur_instr = n64cpu.pipeline_cur_instr_[i];
                            N64::Instruction instr; instr.Full = cur_instr;
                            std::string dis_string = TKPEmu::GeneralDisassembler::GetOpcodeName(EmuType::N64, cur_instr);
                            ImGui::Text("%s", cur_stage.c_str());
                            ImGui::SameLine();
                            std::stringstream ss;
                            ss << std::setw(7) << std::setfill(' ') << dis_string;
                            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(115, 120, 46, 255));
                            ImGui::Text("%s", ss.str().c_str());
                            ImGui::PopStyleColor();
                            // Draw arguments
                            if (instr.IType.op != 0) {
                                if (cur_instr != N64::EMPTY_INSTRUCTION) [[likely]] {
                                    ImGui::SameLine(0, 0);
                                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(160, 30, 40, 255));
                                    ImGui::Text(" $r%02d", instr.IType.rt);
                                    ImGui::PopStyleColor();
                                    ImGui::SameLine(0, 0);
                                    ImGui::TextUnformatted(", ");
                                    ImGui::SameLine(0, 0);
                                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(160, 30, 40, 255));
                                    ImGui::PushItemWidth(20);
                                    ImGui::Text("$r%02d", instr.IType.rs);
                                    ImGui::PopItemWidth();
                                    ImGui::PopStyleColor();
                                    ImGui::SameLine(0, 0);
                                    ImGui::TextUnformatted(", ");
                                    ImGui::SameLine(0, 0);
                                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(66, 135, 179, 255));
                                    ImGui::Text("0x%04x", instr.IType.immediate);
                                    ImGui::PopStyleColor();
                                }
                            } else {
                                if (cur_instr != 0)[[likely]] {
                                    ImGui::SameLine(0, 0);
                                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(160, 30, 40, 255));
                                    ImGui::Text(" $r%02d", instr.IType.rs);
                                    ImGui::PopStyleColor();
                                    ImGui::SameLine(0, 0);
                                    ImGui::TextUnformatted(", ");
                                    ImGui::SameLine(0, 0);
                                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(160, 30, 40, 255));
                                    ImGui::Text("$r%02d", instr.IType.rs);
                                    ImGui::PopStyleColor();
                                    ImGui::SameLine(0, 0);
                                    ImGui::TextUnformatted(", ");
                                    ImGui::SameLine(0, 0);
                                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(66, 135, 179, 255));
                                    ImGui::Text("0x%02x  ", instr.RType.sa);
                                    ImGui::PopStyleColor();
                                } else {
                                    // NOP doesn't have arguments
                                    ImGui::SameLine(0, 0);
                                    ImGui::TextUnformatted("                   ");
                                }
                            }
                            if (cur_instr != N64::EMPTY_INSTRUCTION) {
                                ImGui::SameLine(0, 0);
                                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(80, 80, 80, 255));
                                ImGui::Text("  ; %08x", instr.Full);
                                ImGui::PopStyleColor();
                            }
                        }
                        ImGui::EndChild();
                        ImGui::BeginChild("pipeline rest", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));
                        ImGui::Separator();
                        ImGui::Text("PC: %08x", static_cast<uint32_t>(n64cpu.pc_));
                        ImGui::EndChild();
                        break;
                    }
                    case MemoryType::GPR_REGS: {
                        for (uint32_t i = 0; i < mem.Size; ++i) {
                            auto data = (static_cast<N64::MemDataUnionDW*>(mem.DataPtr) + i);
                            ImGui::Text("r%02u:", i);
                            ImGui::SameLine();
                            if (data->UD == 0) {
                                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(80, 80, 80, 255));
                            }
                            ImGui::Text("%016lx", data->UD);
                            if (ImGui::IsItemHovered()) {
                                ImGui::BeginTooltip();
                                ImGui::Text("Signed: %ld", data->D);
                                ImGui::Text("Unsigned: %lu", data->UD);
                                ImGui::EndTooltip();
                            }
                            if (data->UD == 0) {
                                ImGui::PopStyleColor();
                            }
                            if (i % 3 != 2) {
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
                                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(80, 80, 80, 255));
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
            static int steps = 1;
            ImGui::PushItemWidth(300);
            ImGui::InputInt("##steps", &steps);
            ImGui::PopItemWidth();
            ImGui::SameLine();
            if (ImGui::Button("update")) {
                for (int i = 0; i < steps; i++) {
                    n64_ptr->update();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("reset")) {
                n64_ptr->Reset();
            }
            ImGui::EndGroup();
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
                    case MemoryType::PIPELINE: {
                        mem.DataPtr = n64_ptr;
                        break;
                    }
                }
            }
        }
    }
}