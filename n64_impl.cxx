#include "n64_impl.hxx"

namespace TKPEmu::N64 {
    bool N64::LoadCartridge(std::string path) {
        return cpubus_.LoadFromFile(path);
    }
}