#pragma once
#ifndef TKP_N64_TKPROMDIS_H
#define TKP_N64_TKPROMDIS_H
#include "../include/base_application.h"

namespace TKPEmu::Applications {
    
    class N64_RomDisassembly : public IMApplication {
    public:
        N64_RomDisassembly(std::string menu_title, std::string window_title);
        ~N64_RomDisassembly();
    private:
        void v_draw() override;
    };
}
#endif