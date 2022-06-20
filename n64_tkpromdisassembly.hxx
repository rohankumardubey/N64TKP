#pragma once
#ifndef TKP_N64_TKPROMDIS_H
#define TKP_N64_TKPROMDIS_H
#include "../include/base_application.h"
#include "n64_tkpwrapper.hxx"
#include "n64_types.hxx"

namespace TKPEmu::Applications {
    enum class MemoryType {
        GPR_REGS,
        CP0_REGS,
        FPR_REGS,
        MEMORY,
        PIPELINE,
        FRAMEBUFFER,
    };
    struct MemoryView {
        std::string Name;
        MemoryType Type = MemoryType::MEMORY;
        uint32_t Size = 0;
        uint32_t StartAddress = 0;
        void* DataPtr = nullptr;
    };
    class N64_RomDisassembly : public IMApplication {
    public:
        N64_RomDisassembly(std::string menu_title, std::string window_title);
        ~N64_RomDisassembly();
    private:
        void fill_memory_views();
        void v_draw() override;
        void v_reset() override;
        std::vector<MemoryView> memory_views_ = {
            {"PIPELINE", MemoryType::PIPELINE, 0},
            {"GPR regs", MemoryType::GPR_REGS, 32}, {"CP0 regs", MemoryType::CP0_REGS, 32}, {"FPR regs", MemoryType::FPR_REGS, 32},
            {"PIF RAM", MemoryType::MEMORY, 64, 0x1FC0'07C0},
            {"RDRAM", MemoryType::MEMORY, 0x100'000, 0x0000'1000},
            {"sspec", MemoryType::MEMORY, 0x100'000, 0x1000'1000},
        };
    };
}
#endif