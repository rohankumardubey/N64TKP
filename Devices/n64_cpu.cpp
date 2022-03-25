#include "n64_cpu.h"

namespace TKPEmu::N64::Devices {
    uint32_t CPU::get_curr_ld_addr() {
        // To be used with immediate instructions, returns a sign-extended offset added to register base
        return gpr_regs_[instr_.IType.rs].UW[0] + static_cast<int16_t>(instr_.IType.immediate);
    }
    void CPU::LB() {
        uint32_t sign_extended_addr = get_curr_ld_addr();
        gpr_regs_[instr_.IType.rt].UB[0] = m_load_b(sign_extended_addr);
        gpr_regs_[instr_.IType.rt].DW = gpr_regs_[instr_.IType.rt].B[0];
    }
    void CPU::LBU() {
        uint32_t sign_extended_addr = get_curr_ld_addr();
        gpr_regs_[instr_.IType.rt].UB[0] = m_load_b(sign_extended_addr);
        gpr_regs_[instr_.IType.rt].UDW = gpr_regs_[instr_.IType.rt].UB[0];
    }
    void CPU::LH() {
        uint32_t sign_extended_addr = get_curr_ld_addr();
        gpr_regs_[instr_.IType.rt].UHW[0] = m_load_hw(sign_extended_addr);
        gpr_regs_[instr_.IType.rt].DW = gpr_regs_[instr_.IType.rt].HW[0];
    }
    void CPU::LHU() {
        uint32_t sign_extended_addr = get_curr_ld_addr();
        gpr_regs_[instr_.IType.rt].UHW[0] = m_load_hw(sign_extended_addr);
        gpr_regs_[instr_.IType.rt].UDW = gpr_regs_[instr_.IType.rt].UHW[0];
    }
    void CPU::LW() {
        uint32_t sign_extended_addr = get_curr_ld_addr();
        gpr_regs_[instr_.IType.rt].W[0] = m_load_w(sign_extended_addr);
    }
    void CPU::LWU() {
        uint32_t sign_extended_addr = get_curr_ld_addr();
        gpr_regs_[instr_.IType.rt].UW[0] = m_load_w(sign_extended_addr);
    }
    void CPU::LWL() {
        static constexpr uint32_t masks[4] { 
            0x00000000,
            0x000000FF,
            0x0000FFFF,
            0x00FFFFFF, 
        };
        uint32_t sign_extended_addr = get_curr_ld_addr();
        uint8_t shift = sign_extended_addr & 0b11;
        uint32_t temp = m_load_w(sign_extended_addr & ~0b11);
        gpr_regs_[instr_.IType.rt].UDW &= (
            masks[shift]
        );
        gpr_regs_[instr_.IType.rt].DW |= (
            temp << (shift * 8)
        );
    }
    void CPU::LWR() {
        static constexpr uint32_t masks[4] { 
            0x00000000,
            0xFF000000,
            0xFFFF0000,
            0xFFFFFF00, 
        };
        uint32_t sign_extended_addr = get_curr_ld_addr();
        uint8_t shift = sign_extended_addr & 0b11;
        uint32_t temp = m_load_w(sign_extended_addr & ~0b11);
        gpr_regs_[instr_.IType.rt].UDW &= (
            masks[shift]
        );
        gpr_regs_[instr_.IType.rt].DW |= (
            temp >> (shift * 8)
        );
    }
    void CPU::LDL() {
        static constexpr uint64_t masks[8] { 
            0x0000000000000000,
            0x00000000000000FF,
            0x000000000000FFFF,
            0x0000000000FFFFFF, 
            0x00000000FFFFFFFF, 
            0x000000FFFFFFFFFF, 
            0x0000FFFFFFFFFFFF, 
            0x00FFFFFFFFFFFFFF, 
        };
        uint32_t sign_extended_addr = get_curr_ld_addr();
        uint8_t shift = sign_extended_addr & 0b111;
        uint64_t temp = m_load_dw(sign_extended_addr & ~0b111);
        gpr_regs_[instr_.IType.rt].UDW &= (
            masks[shift]
        );
        gpr_regs_[instr_.IType.rt].DW |= (
            temp << (shift * 8)
        );
    }
    void CPU::LDR() {
        static constexpr uint64_t masks[8] { 
            0x0000000000000000,
            0xFF00000000000000,
            0xFFFF000000000000,
            0xFFFFFF0000000000,
            0xFFFFFFFF00000000,
            0xFFFFFFFFFF000000,
            0xFFFFFFFFFFFF0000,
            0xFFFFFFFFFFFFFF00,
        };
        uint32_t sign_extended_addr = get_curr_ld_addr();
        uint8_t shift = sign_extended_addr & 0b111;
        uint64_t temp = m_load_dw(sign_extended_addr & ~0b111);
        gpr_regs_[instr_.IType.rt].UDW &= (
            masks[shift]
        );
        gpr_regs_[instr_.IType.rt].DW |= (
            temp >> (shift * 8)
        );
    }
    void CPU::SC() {
        uint32_t sign_extended_addr = get_curr_ld_addr();
        if (llbit_) {
            m_store_w(sign_extended_addr, gpr_regs_[instr_.IType.rt].UW[0]);
        }
        gpr_regs_[instr_.IType.rt].W[0] = llbit_;
    }
    void CPU::SCD() {
        uint32_t sign_extended_addr = get_curr_ld_addr();
        if (llbit_) {
            m_store_w(sign_extended_addr, gpr_regs_[instr_.IType.rt].UDW);
        }
        gpr_regs_[instr_.IType.rt].W[0] = llbit_;
    }
    void CPU::SD() {
        uint32_t sign_extended_addr = get_curr_ld_addr();
        m_store_w(sign_extended_addr, gpr_regs_[instr_.IType.rt].UDW);
    }
}