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
}