#include "n64_cpu.h"

namespace TKPEmu::N64::Devices {
    GPR& CPU::get_gpr(int index) {
        GPR& ret = gpr_regs_[index];
        // For r1-31, this does nothing, but for r0 it sets the value to 0
        // since r0 is hardcoded to 0
        // Note: should be faster than an if instruction but not confirmed
        ret *= sgn(index);
        return ret;
    }
    GPR& CPU::set_gpr(int index, GPR data) {
        GPR& ret = gpr_regs_[index];
        // See get_gpr
        data *= sgn(index);
        ret = data;
        return ret;   
    }
}