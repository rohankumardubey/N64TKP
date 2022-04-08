#include "n64_cpu.hxx"

namespace TKPEmu::N64::Devices {

    CPU::CPU() :
        instr_cache_(KB(16)),
        data_cache_(KB(8))
    {

    }

    CPU::ICFret CPU::ICF(ICFargs) {
        
    }

    CPU::SelAddrSpace32ret CPU::select_addr_space32(SelAddrSpace32args addr) {
        // VR4300 manual page 136 table 5-3
        unsigned _3msb = addr >> 29;
        uint32_t physical_addr = 0;
        switch(_3msb) {
            case 0b100:
            // kseg0
            physical_addr = translate_kseg0(addr);
            break;
            case 0b101:
            // kseg1
            physical_addr = translate_kseg1(addr);
            break;
            case 0b110:
            // ksseg
            break;
            case 0b111:
            // kseg3
            break;
            default:
            // kuseg
            // From manual:
            // In Kernel mode, when KX = 0 in the Status register, and the most-significant bit
            // of the virtual address is cleared, the kuseg virtual address space is selected; it
            // covers the current 231 bytes (2 GB) user address space. The virtual address is
            // extended with the contents of the 8-bit ASID field to form a unique virtual
            // address.
            physical_addr = translate_kuseg(addr);
            break;
        }
    }

    // uint32_t CPU::get_curr_ld_addr() {
    //     // To be used with immediate instructions, returns a sign-extended offset added to register base
    //     return gpr_regs_[instr_.IType.rs].UW._0 + static_cast<int16_t>(instr_.IType.immediate);
    // }
    // void CPU::LB() {
    //     uint32_t sign_extended_addr = get_curr_ld_addr();
    //     gpr_regs_[instr_.IType.rt].UB._0 = m_load_b(sign_extended_addr);
    //     gpr_regs_[instr_.IType.rt].D = gpr_regs_[instr_.IType.rt].B._0;
    // }
    // void CPU::LBU() {
    //     uint32_t sign_extended_addr = get_curr_ld_addr();
    //     gpr_regs_[instr_.IType.rt].UB._0 = m_load_b(sign_extended_addr);
    //     gpr_regs_[instr_.IType.rt].UD = gpr_regs_[instr_.IType.rt].UB._0;
    // }
    // void CPU::LH() {
    //     uint32_t sign_extended_addr = get_curr_ld_addr();
    //     gpr_regs_[instr_.IType.rt].UH._0 = m_load_h(sign_extended_addr);
    //     gpr_regs_[instr_.IType.rt].D = gpr_regs_[instr_.IType.rt].H._0;
    // }
    // void CPU::LHU() {
    //     uint32_t sign_extended_addr = get_curr_ld_addr();
    //     gpr_regs_[instr_.IType.rt].UH._0 = m_load_h(sign_extended_addr);
    //     gpr_regs_[instr_.IType.rt].UD = gpr_regs_[instr_.IType.rt].UH._0;
    // }
    // void CPU::LW() {
    //     uint32_t sign_extended_addr = get_curr_ld_addr();
    //     gpr_regs_[instr_.IType.rt].W._0 = m_load_w(sign_extended_addr);
    // }
    // void CPU::LWU() {
    //     uint32_t sign_extended_addr = get_curr_ld_addr();
    //     gpr_regs_[instr_.IType.rt].UW._0 = m_load_w(sign_extended_addr);
    // }
    // void CPU::LWL() {
    //     static constexpr uint32_t masks[4] { 
    //         0x00000000,
    //         0x000000FF,
    //         0x0000FFFF,
    //         0x00FFFFFF, 
    //     };
    //     uint32_t sign_extended_addr = get_curr_ld_addr();
    //     uint8_t shift = sign_extended_addr & 0b11;
    //     uint32_t temp = m_load_w(sign_extended_addr & ~0b11);
    //     gpr_regs_[instr_.IType.rt].UD &= (
    //         masks[shift]
    //     );
    //     gpr_regs_[instr_.IType.rt].D |= (
    //         temp << (shift * 8)
    //     );
    // }
    // void CPU::LWR() {
    //     static constexpr uint32_t masks[4] { 
    //         0x00000000,
    //         0xFF000000,
    //         0xFFFF0000,
    //         0xFFFFFF00, 
    //     };
    //     uint32_t sign_extended_addr = get_curr_ld_addr();
    //     uint8_t shift = sign_extended_addr & 0b11;
    //     uint32_t temp = m_load_w(sign_extended_addr & ~0b11);
    //     gpr_regs_[instr_.IType.rt].UD &= (
    //         masks[shift]
    //     );
    //     gpr_regs_[instr_.IType.rt].D |= (
    //         temp >> (shift * 8)
    //     );
    // }
    // void CPU::LDL() {
    //     static constexpr uint64_t masks[8] { 
    //         0x0000000000000000,
    //         0x00000000000000FF,
    //         0x000000000000FFFF,
    //         0x0000000000FFFFFF, 
    //         0x00000000FFFFFFFF, 
    //         0x000000FFFFFFFFFF, 
    //         0x0000FFFFFFFFFFFF, 
    //         0x00FFFFFFFFFFFFFF, 
    //     };
    //     uint32_t sign_extended_addr = get_curr_ld_addr();
    //     uint8_t shift = sign_extended_addr & 0b111;
    //     uint64_t temp = m_load_d(sign_extended_addr & ~0b111);
    //     gpr_regs_[instr_.IType.rt].UD &= (
    //         masks[shift]
    //     );
    //     gpr_regs_[instr_.IType.rt].D |= (
    //         temp << (shift * 8)
    //     );
    // }
    // void CPU::LDR() {
    //     static constexpr uint64_t masks[8] { 
    //         0x0000000000000000,
    //         0xFF00000000000000,
    //         0xFFFF000000000000,
    //         0xFFFFFF0000000000,
    //         0xFFFFFFFF00000000,
    //         0xFFFFFFFFFF000000,
    //         0xFFFFFFFFFFFF0000,
    //         0xFFFFFFFFFFFFFF00,
    //     };
    //     uint32_t sign_extended_addr = get_curr_ld_addr();
    //     uint8_t shift = sign_extended_addr & 0b111;
    //     uint64_t temp = m_load_w(sign_extended_addr & ~0b111);
    //     gpr_regs_[instr_.IType.rt].UD &= (
    //         masks[shift]
    //     );
    //     gpr_regs_[instr_.IType.rt].D |= (
    //         temp >> (shift * 8)
    //     );
    // }
    // void CPU::SB() {
    //     uint32_t sign_extended_addr = get_curr_ld_addr();
    //     m_store_b(sign_extended_addr, gpr_regs_[instr_.IType.rt].UB._0);
    // }
    // void CPU::SWL() {
    //     uint32_t sign_extended_addr = get_curr_ld_addr();
    //     m_store_w(sign_extended_addr, gpr_regs_[instr_.IType.rt].UW._1);
    // }
    // void CPU::SW() {
    //     uint32_t sign_extended_addr = get_curr_ld_addr();
    //     m_store_b(sign_extended_addr, gpr_regs_[instr_.IType.rt].UW._0);
    // }
    // void CPU::SC() {
    //     uint32_t sign_extended_addr = get_curr_ld_addr();
    //     if (llbit_) {
    //         m_store_w(sign_extended_addr, gpr_regs_[instr_.IType.rt].UW._0);
    //     }
    //     gpr_regs_[instr_.IType.rt].W._0 = llbit_;
    // }
    // void CPU::SCD() {
    //     uint32_t sign_extended_addr = get_curr_ld_addr();
    //     if (llbit_) {
    //         m_store_w(sign_extended_addr, gpr_regs_[instr_.IType.rt].UD);
    //     }
    //     gpr_regs_[instr_.IType.rt].W._0 = llbit_;
    // }
    // void CPU::SD() {
    //     uint32_t sign_extended_addr = get_curr_ld_addr();
    //     m_store_d(sign_extended_addr, gpr_regs_[instr_.IType.rt].UD);
    // }
}