#include "n64_cpu.h"

namespace TKPEmu::N64::Devices {
    uint32_t CPU::get_curr_ld_addr() {
        // To be used with immediate instructions, returns a sign-extended offset added to register base
        return gpr_regs_[instr_.IType.rs].UW[0] + static_cast<int16_t>(instr_.IType.immediate);
    }
    void CPU::LB() {
        uint64_t sign_extended_addr = get_curr_ld_addr();
        gpr_regs_[instr_.IType.rt].UB[0] = m_load_b(sign_extended_addr);
        gpr_regs_[instr_.IType.rt].W[0] = gpr_regs_[instr_.IType.rt].B[0];
    }
    void CPU::LBU() {
        uint64_t sign_extended_addr = get_curr_ld_addr();
        gpr_regs_[instr_.IType.rt].UB[0] = m_load_b(sign_extended_addr);
        gpr_regs_[instr_.IType.rt].UW[0] = gpr_regs_[instr_.IType.rt].UB[0];
    }
    void CPU::LH() {
        uint64_t sign_extended_addr = get_curr_ld_addr();
        gpr_regs_[instr_.IType.rt].UHW[0] = m_load_b(sign_extended_addr);
        gpr_regs_[instr_.IType.rt].W[0] = gpr_regs_[instr_.IType.rt].HW[0];
    }
    void CPU::LHU() {
        uint64_t sign_extended_addr = get_curr_ld_addr();
        gpr_regs_[instr_.IType.rt].UHW[0] = m_load_b(sign_extended_addr);
        gpr_regs_[instr_.IType.rt].UW[0] = gpr_regs_[instr_.IType.rt].UHW[0];
    }
    void CPU::LW() {
        uint64_t sign_extended_addr = get_curr_ld_addr();
        gpr_regs_[instr_.IType.rt].UW[0] = m_load_b(sign_extended_addr);
    }
    void CPU::LWU() {
        uint64_t sign_extended_addr = get_curr_ld_addr();
        gpr_regs_[instr_.IType.rt].UW[0] = m_load_b(sign_extended_addr);
    }
    void CPU::LWL() {

    }
}