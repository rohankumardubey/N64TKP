#include "n64_cpu.hxx"

namespace TKPEmu::N64::Devices {
    uint32_t CPU::get_curr_ld_addr() {
        // To be used with immediate instructions, returns a sign-extended offset added to register base
        return gpr_regs_[instr_.IType.rs].UW._0 + static_cast<int16_t>(instr_.IType.immediate);
    }
    void CPU::LB() {
        uint32_t sign_extended_addr = get_curr_ld_addr();
        gpr_regs_[instr_.IType.rt].UB._0 = m_load_b(sign_extended_addr);
        gpr_regs_[instr_.IType.rt].D = gpr_regs_[instr_.IType.rt].B._0;
    }
    void CPU::LBU() {
        uint32_t sign_extended_addr = get_curr_ld_addr();
        gpr_regs_[instr_.IType.rt].UB._0 = m_load_b(sign_extended_addr);
        gpr_regs_[instr_.IType.rt].UD = gpr_regs_[instr_.IType.rt].UB._0;
    }
    void CPU::LH() {
        uint32_t sign_extended_addr = get_curr_ld_addr();
        gpr_regs_[instr_.IType.rt].UH._0 = m_load_h(sign_extended_addr);
        gpr_regs_[instr_.IType.rt].D = gpr_regs_[instr_.IType.rt].H._0;
    }
    void CPU::LHU() {
        uint32_t sign_extended_addr = get_curr_ld_addr();
        gpr_regs_[instr_.IType.rt].UH._0 = m_load_h(sign_extended_addr);
        gpr_regs_[instr_.IType.rt].UD = gpr_regs_[instr_.IType.rt].UH._0;
    }
    void CPU::LW() {
        uint32_t sign_extended_addr = get_curr_ld_addr();
        gpr_regs_[instr_.IType.rt].W._0 = m_load_w(sign_extended_addr);
    }
    void CPU::LWU() {
        uint32_t sign_extended_addr = get_curr_ld_addr();
        gpr_regs_[instr_.IType.rt].UW._0 = m_load_w(sign_extended_addr);
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
        gpr_regs_[instr_.IType.rt].UD &= (
            masks[shift]
        );
        gpr_regs_[instr_.IType.rt].D |= (
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
        gpr_regs_[instr_.IType.rt].UD &= (
            masks[shift]
        );
        gpr_regs_[instr_.IType.rt].D |= (
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
        uint64_t temp = m_load_d(sign_extended_addr & ~0b111);
        gpr_regs_[instr_.IType.rt].UD &= (
            masks[shift]
        );
        gpr_regs_[instr_.IType.rt].D |= (
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
        uint64_t temp = m_load_w(sign_extended_addr & ~0b111);
        gpr_regs_[instr_.IType.rt].UD &= (
            masks[shift]
        );
        gpr_regs_[instr_.IType.rt].D |= (
            temp >> (shift * 8)
        );
    }
    void CPU::SB() {
        uint32_t sign_extended_addr = get_curr_ld_addr();
        m_store_b(sign_extended_addr, gpr_regs_[instr_.IType.rt].UB._0);
    }
    void CPU::SWL() {
        uint32_t sign_extended_addr = get_curr_ld_addr();
        m_store_w(sign_extended_addr, gpr_regs_[instr_.IType.rt].UW._1);
    }
    void CPU::SW() {
        uint32_t sign_extended_addr = get_curr_ld_addr();
        m_store_b(sign_extended_addr, gpr_regs_[instr_.IType.rt].UW._0);
    }
    void CPU::SC() {
        uint32_t sign_extended_addr = get_curr_ld_addr();
        if (llbit_) {
            m_store_w(sign_extended_addr, gpr_regs_[instr_.IType.rt].UW._0);
        }
        gpr_regs_[instr_.IType.rt].W._0 = llbit_;
    }
    void CPU::SCD() {
        uint32_t sign_extended_addr = get_curr_ld_addr();
        if (llbit_) {
            m_store_w(sign_extended_addr, gpr_regs_[instr_.IType.rt].UD);
        }
        gpr_regs_[instr_.IType.rt].W._0 = llbit_;
    }
    void CPU::SD() {
        uint32_t sign_extended_addr = get_curr_ld_addr();
        m_store_d(sign_extended_addr, gpr_regs_[instr_.IType.rt].UD);
    }
}