#include <fstream>
#include <iterator>
#include "n64_cpu.hxx"

namespace TKPEmu::N64::Devices {
    bool CPUBus::LoadFromFile(std::string path) {
        std::ifstream ifs(path, std::ios::in | std::ios::binary);
        if (ifs.is_open()) {
            ifs.unsetf(std::ios::skipws);
            ifs.seekg(0, std::ios::end);
            std::streampos size = ifs.tellg();
            ifs.seekg(0, std::ios::beg);
            rom_.clear();
            rom_.resize(size);
            ifs.read(reinterpret_cast<char*>(rom_.data()), size);
            rom_.shrink_to_fit();
        } else {
            return false;
        }
        return true;
    }
    MemDataUW CPUBus::fetch_instruction_uncached(MemDataUW paddr) {
        if (paddr >= rom_.size()) [[unlikely]] {
            // TODO: remove this check for speed?
            throw std::exception();
        }
        // Loads in big endian
        // should only work for .z64 files
        return rom_[paddr] << 24 | rom_[paddr + 1] << 16 | rom_[paddr + 2] << 8 | rom_[paddr + 3];
    }
    MemDataUW& CPUBus::redirect_paddress (MemDataUW paddr) {
        switch(paddr & 0xF000'0000) {
            case 0x0000'0000: {
                if (paddr < 0x0040'0000) {        // 0x00000000	0x003FFFFF	RDRAM	                        RDRAM located on motherboard
                } else if (paddr < 0x0080'0000) { // 0x00400000	0x007FFFFF	RDRAM	                        RDRAM from Expansion Pak (if present)
                } else if (paddr < 0x03F0'0000) { // 0x00800000	0x03EFFFFF	Reserved	                    Unknown usage
                    throw NotImplementedException(__func__);
                } else if (paddr < 0x0400'0000) { // 0x03F00000	0x03FFFFFF	RDRAM Registers	                RDRAM configuration registers
                } else if (paddr < 0x0400'1000) { // 0x04000000	0x04000FFF	RSP DMEM                        RSP Data Memory
                } else if (paddr < 0x0400'2000) { // 0x04001000	0x04001FFF	RSP IMEM                        RSP Instruction Memory
                } else if (paddr < 0x0404'0000) { // 0x04002000	0x0403FFFF	Unknown                         Unknown
                    throw NotImplementedException(__func__);
                } else if (paddr < 0x0410'0000) { // 0x04040000	0x040FFFFF	RSP Registers	                RSP DMAs, status, semaphore, program counter, IMEM BIST status
                } else if (paddr < 0x0420'0000) { // 0x04100000	0x041FFFFF	RDP Command Registers	        RDP DMAs, clock counters for: clock, buffer busy, pipe busy, and TMEM load
                } else if (paddr < 0x0430'0000) { // 0x04200000	0x042FFFFF	RDP Span Registers              TMEM BIST status, DP Span testing mode
                } else if (paddr < 0x0440'0000) { // 0x04300000	0x043FFFFF	MIPS Interface (MI)	            Init mode, ebus test mode, RDRAM register mode, hardware version, interrupt status, interrupt masks
                } else if (paddr < 0x0450'0000) { // 0x04400000	0x044FFFFF	Video Interface (VI)	        Video control registers
                } else if (paddr < 0x0460'0000) { // 0x04500000	0x045FFFFF	Audio Interface (AI)	        Audio DMAs, Audio DAC clock divider
                } else if (paddr < 0x0470'0000) { // 0x04600000	0x046FFFFF	Peripheral Interface (PI)	    Cartridge port DMAs, status, Domain 1 and 2 speed/latency/page-size controls
                } else if (paddr < 0x0480'0000) { // 0x04700000	0x047FFFFF	RDRAM Interface (RI)	        Operating mode, current load, refresh/select config, latency, error and bank status
                } else if (paddr < 0x0490'0000) { // 0x04800000	0x048FFFFF	Serial Interface (SI)	        SI DMAs, PIF status
                } else if (paddr < 0x0500'0000) { // 0x04900000	0x04FFFFFF	Unused	                        Unused
                    throw NotImplementedException(__func__);
                } else if (paddr < 0x0600'0000) { // 0x05000000	0x05FFFFFF	Cartridge Domain 2 Address 1	N64DD control registers (if present)

                } else if (paddr < 0x0800'0000) { // 0x06000000	0x07FFFFFF	Cartridge Domain 1 Address 1	N64DD IPL ROM (if present)
                    
                } else {                          // 0x08000000	0x0FFFFFFF	Cartridge Domain 2 Address 2	Cartridge SRAM (if present)
                    
                }
                break;
            }
            case 0x1000'0000:
            case 0x2000'0000:
            case 0x3000'0000:
            case 0x4000'0000:
            case 0x5000'0000:
            case 0x6000'0000:
            case 0x7000'0000: {
                if (paddr < 0x1FC0'0000) {         // 0x10000000 0x1FBFFFFF	Cartridge Domain 1 Address 2	Cartridge ROM
                } else if (paddr < 0x1FC007C0) {   // 0x1FC00000 0x1FC007BF	PIF ROM (IPL1/2)	            Executed on boot
                } else if (paddr < 0x1FC0'0800) {  // 0x1FC007C0 0x1FC007FF	PIF RAM	                        Controller and EEPROM communication, and during IPL1/2 is used to read startup data from the PIF
                } else if (paddr < 0x1FD0'0000) {  // 0x1FC00800 0x1FCFFFFF	Reserved	                    Unknown usage
                    throw NotImplementedException(__func__);
                } else {                           // 0x1FD00000 0x7FFFFFFF	Cartridge Domain 1 Address 3	Mapped to same address range on physical cartridge port
                    throw NotImplementedException(__func__);
                }
                break;
            }
            default: {
                throw NotImplementedException(__func__);
                break;
            }
        }
    }
}