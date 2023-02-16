#include <iostream>
#include "n64_impl.hxx"
#include "utils.hxx"

namespace TKPEmu::N64 {
    N64::N64() :
        cpubus_(rcp_), 
        cpu_(cpubus_, rcp_)
    {
        Reset();
    }

    bool N64::LoadCartridge(std::string path) {
        return cpu_.cpubus_.LoadCartridge(path);
    }

    bool N64::LoadIPL(std::string path) {
        if (!path.empty()) {
            return cpu_.cpubus_.LoadIPL(path);
        }
        return false;
    }

    void N64::Update() {
        cpubus_.set_interrupt(Devices::Interrupt::AI, true);
        bool interrupt_fired = cpubus_.mi_mask_ & cpubus_.mi_interrupt_;
        if (interrupt_fired) [[unlikely]] {
            uint32_t flags = cpu_.cp0_regs_[CP0_CAUSE].UW._0;
            SetBit(flags, 0, true);
        }
        cpu_.update_pipeline();
    }

    void N64::Reset() {
        cpu_.Reset();
        rcp_.Reset();
    }
}