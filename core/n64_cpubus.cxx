#include <fstream>
#include <sstream>
#include "n64_cpu.hxx"

namespace TKPEmu::N64::Devices {
    CPUBus::CPUBus() {
        map_direct_addresses();
    }

    bool CPUBus::LoadFromFile(std::string path) {
        std::ifstream ifs(path, std::ios::in | std::ios::binary);
        if (ifs.is_open()) {
            ifs.unsetf(std::ios::skipws);
            ifs.seekg(0, std::ios::end);
            std::streampos size = ifs.tellg();
            ifs.seekg(0, std::ios::beg);
            ifs.read(reinterpret_cast<char*>(cart_rom_.data()), size);
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
        uint8_t* ptr = cart_rom_.data() + paddr;
        uint32_t ret = __builtin_bswap32(*reinterpret_cast<uint32_t*>(ptr));
        return ret;
    }

    uint8_t* CPUBus::redirect_paddress (uint32_t paddr) {
        uint8_t* ptr = page_table_[paddr >> 20];
        if (ptr) {
            return ptr;
        } else {
            if (paddr - 0x1FC0'0'7C0u < 64u) {
                return &pif_ram_[paddr - 0x1FC0'07C0u];
            }
        }
        temp_addresses_[(paddr) & 0xFF] = 0;
        temp_addresses_[(paddr + 1) & 0xFF] = 0;
        temp_addresses_[(paddr + 2) & 0xFF] = 0;
        temp_addresses_[(paddr + 3) & 0xFF] = 0;
        return &temp_addresses_[paddr & 0xFF];
        std::stringstream ss;
        ss << "Tried to access bad address: 0x" << std::hex << paddr << std::endl;
        throw NotImplementedException(ss.str().c_str());
    }

    void CPUBus::map_direct_addresses() {
        // https://wheremyfoodat.github.io/software-fastmem/
        const uint32_t PAGE_SIZE = 0x100000;
        // Map rdram
        for (int i = 0; i < 4; i++) {
            page_table_[i] = &rdram_[PAGE_SIZE * i];
            page_table_[i] = &rdram_[PAGE_SIZE * i];
        }
        // Map rdram from expansion pak
        for (int i = 4; i < 8; i++) {
            page_table_[i] = &rdram_xpk_[PAGE_SIZE * i];
            page_table_[i] = &rdram_xpk_[PAGE_SIZE * i];
        }
        // Map cartridge rom
        for (int i = 0x100; i < 0x1FB; i++) {
            page_table_[i] = &cart_rom_[PAGE_SIZE * i];
            page_table_[i] = &cart_rom_[PAGE_SIZE * i];
        }
    }
}