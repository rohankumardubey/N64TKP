#include <fstream>
#include <sstream>
#include <iostream>
#include "n64_cpu.hxx"
#include "n64_addresses.hxx"
#include "../include/error_factory.hxx"

namespace TKPEmu::N64::Devices {
    CPUBus::CPUBus(Devices::RCP& rcp) : rcp_(rcp) {
        map_direct_addresses();
    }

    bool CPUBus::LoadCartridge(std::string path) {
        std::ifstream ifs(path, std::ios::in | std::ios::binary);
        if (ifs.is_open()) {
            ifs.unsetf(std::ios::skipws);
            ifs.seekg(0, std::ios::end);
            std::streampos size = ifs.tellg();
            ifs.seekg(0, std::ios::beg);
            ifs.read(reinterpret_cast<char*>(cart_rom_.data()), size);
            std::cout << "first byte: " << std::hex << (uint16_t)cart_rom_[0] << std::endl;
            std::cout << "2first byte: " << std::hex << (uint16_t)*redirect_paddress(0x1000'0000) << std::endl;
            Reset();
        } else {
            return false;
        }
        return true;
    }

    bool CPUBus::LoadIPL(std::string path) {
        std::ifstream ifs(path, std::ios::in | std::ios::binary);
        if (ifs.is_open()) {
            ifs.unsetf(std::ios::skipws);
            ifs.seekg(0, std::ios::end);
            std::streampos size = ifs.tellg();
            ifs.seekg(0, std::ios::beg);
            ipl_.resize(size);
            ifs.read(reinterpret_cast<char*>(ipl_.data()), size);
            Reset();
        } else {
            return false;
        }
        return true;
    }

    void CPUBus::Reset() {
        rdram_.fill(0);
        rdram_xpk_.fill(0);
        pif_ram_.fill(0);
    }
    
    uint32_t CPUBus::fetch_instruction_uncached(uint32_t paddr) {
        // Loads in big endian
        // should only work for .z64 files
        uint8_t* ptr = redirect_paddress(paddr);
        uint32_t ret = __builtin_bswap32(*reinterpret_cast<uint32_t*>(ptr));
        return ret;
    }

    uint8_t* CPUBus::redirect_paddress(uint32_t paddr) {
        uint8_t* ptr = page_table_[paddr >> 20];
        if (ptr) [[likely]] {
            ptr += (paddr & static_cast<uint32_t>(0xFFFFF));
            return ptr;
        } else {
            uint8_t* ptr_slow = redirect_paddress_slow(paddr);
            if (ptr_slow)
                return ptr_slow;
        }
        std::stringstream ss;
        ss << "Tried to access bad address: 0x" << std::hex << paddr << std::endl;
        throw ErrorFactory::generate_exception(__func__, __LINE__, ss.str());
    }

    uint8_t* CPUBus::redirect_paddress_slow(uint32_t paddr) {
        #define redir_case(A,B) case A: return reinterpret_cast<uint8_t*>(&B)
        switch (paddr) {
            // RSP internal registers
            redir_case(RSP_STATUS, rcp_.rsp_status_);
            redir_case(RSP_DMA_BUSY, rcp_.rsp_dma_busy_);

            // Video Interface
            redir_case(VI_CTRL_REG, rcp_.vi_ctrl_);
            redir_case(VI_ORIGIN_REG, rcp_.vi_origin_);
            redir_case(VI_WIDTH_REG, rcp_.vi_width_);
            redir_case(VI_V_INTR_REG, rcp_.vi_v_intr_);
            redir_case(VI_V_CURRENT_REG, rcp_.vi_v_current_);
            redir_case(VI_BURST_REG, rcp_.vi_burst_);
            redir_case(VI_V_SYNC_REG, rcp_.vi_v_sync_);
            redir_case(VI_H_SYNC_REG, rcp_.vi_h_sync_);
            redir_case(VI_H_SYNC_LEAP_REG, rcp_.vi_h_sync_leap_);
            redir_case(VI_H_VIDEO_REG, rcp_.vi_h_video_);
            redir_case(VI_V_VIDEO_REG, rcp_.vi_v_video_);
            redir_case(VI_V_BURST_REG, rcp_.vi_v_burst_);
            redir_case(VI_X_SCALE_REG, rcp_.vi_x_scale_);
            redir_case(VI_Y_SCALE_REG, rcp_.vi_y_scale_);
            redir_case(VI_TEST_ADDR_REG, rcp_.vi_test_addr_);
            redir_case(VI_STAGED_DATA_REG, rcp_.vi_staged_data_);

            // Audio Interface
            redir_case(AI_DRAM_ADDR, ai_dram_addr_);
            redir_case(AI_LEN, ai_length_);

            // Peripheral Interface
            redir_case(PI_DRAM_ADDR_REG, pi_dram_addr_);
            redir_case(PI_CART_ADDR_REG, pi_cart_addr_);
            redir_case(PI_RD_LEN_REG, pi_rd_len_);
            redir_case(PI_WR_LEN_REG, pi_wr_len_);
            redir_case(PI_STATUS_REG, pi_status_);
            redir_case(PI_BSD_DOM1_LAT_REG, pi_bsd_dom1_lat_);
            redir_case(PI_BSD_DOM1_PWD_REG, pi_bsd_dom1_pwd_);
            redir_case(PI_BSD_DOM1_PGS_REG, pi_bsd_dom1_pgs_);
            redir_case(PI_BSD_DOM1_RLS_REG, pi_bsd_dom1_rls_);
            redir_case(PI_BSD_DOM2_LAT_REG, pi_bsd_dom2_lat_);
            redir_case(PI_BSD_DOM2_PWD_REG, pi_bsd_dom2_pwd_);
            redir_case(PI_BSD_DOM2_PGS_REG, pi_bsd_dom2_pgs_);
            redir_case(PI_BSD_DOM2_RLS_REG, pi_bsd_dom2_rls_);

            // Serial Interface
            redir_case(SI_STATUS, si_status_);
        }
        #undef redir_case
        if (paddr - 0x1FC00000u < 1984u) {
            return &ipl_[paddr - 0x1FC00000u];
        } else if (paddr - 0x1FC0'07C0u < 64u) {
            return &pif_ram_[paddr - 0x1FC0'07C0u];
        } else if (paddr - 0x04000000u < 4096u) {
            return &rsp_dmem_[paddr - 0x04000000u];
        } else if (paddr - 0x04001000u < 4096u) {
            return &rsp_imem_[paddr - 0x04001000u];
        } else if (paddr - 0x04100000u < 0x100000u) {
            return &rdp_cmem_[paddr - 0x04100000u];
        }
        return nullptr;
    }

    void CPUBus::map_direct_addresses() {
        // https://wheremyfoodat.github.io/software-fastmem/
        const uint32_t PAGE_SIZE = 0x100000;
        // Map rdram
        for (int i = 0; i < 4; i++) {
            page_table_[i] = &rdram_[PAGE_SIZE * i];
        }
        // Map rdram from expansion pak
        for (int i = 4; i < 8; i++) {
            page_table_[i] = &rdram_xpk_[PAGE_SIZE * i];
        }
        // Map cartridge rom
        for (int i = 0x100; i <= 0x1FB; i++) {
            page_table_[i] = &cart_rom_[PAGE_SIZE * (i - 0x100)];
        }
    }
}