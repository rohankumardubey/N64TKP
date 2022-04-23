#include "n64_cpu.hxx"
#include <cassert> // assert

namespace TKPEmu::N64::Devices {
    void CPU::execute_instruction() {
        EXDC_latch temp {};
        std::swap(exdc_latch_, temp);
        auto& cur_instr = rfex_latch_.instruction;
        auto type = rfex_latch_.instruction_type;
        switch(type) {
            /**
             * ADDI, ADDIU
             * 
             * ADDI throws Integer overflow exception
             */
            case InstructionType::ADDI: 
            case InstructionType::ADDIU: {
                int32_t imm = static_cast<int16_t>(cur_instr.IType.immediate);
                uint64_t seimm = static_cast<int64_t>(imm);
                uint64_t result = 0;
                bool overflow = __builtin_add_overflow(rfex_latch_.fetched_rs.UD, seimm, &result);
                exdc_latch_.dest = cur_instr.IType.rt;
                exdc_latch_.data = result;
                exdc_latch_.write_type = WriteType::REGISTER;
                exdc_latch_.access_type = AccessType::UDOUBLEWORD;
                #if SKIPEXCEPTIONS == 0
                if (overflow && type == InstructionType::ADDI) {
                    // An integer overflow exception occurs if carries out of bits 30 and 31 differ (2’s
                    // complement overflow). The contents of destination register rt is not modified
                    // when an integer overflow exception occurs.
                    exdc_latch_.write_type = WriteType::NONE;
                    throw IntegerOverflowException();
                }
                #endif
                break;
            }
            /**
             * J, JAL
             * 
             * doesn't throw
             */
            case InstructionType::JAL: {
                gpr_regs_[31].UD = pc_; // By the time this instruction is executed, pc is already incremented by 8
                                         // so there's no need to increment here
                [[fallthrough]];
            }
            case InstructionType::J: {
                auto jump_addr = cur_instr.JType.target;
                // combine first 3 bits of pc and jump_addr shifted left by 2
                exdc_latch_.data = (pc_ & 0xF000'0000) | (jump_addr << 2);
                exdc_latch_.dest_direct = &pc_;
                exdc_latch_.write_type = WriteType::REGISTER;
                exdc_latch_.access_type = AccessType::UDOUBLEWORD_DIRECT;
                break;
            }
            /**
             * LUI
             * 
             * doesn't throw
             */
            case InstructionType::LUI: {
                int32_t imm = cur_instr.IType.immediate << 16;
                uint64_t seimm = static_cast<int64_t>(imm);
                exdc_latch_.dest = cur_instr.IType.rt;
                exdc_latch_.data = seimm;
                exdc_latch_.write_type = WriteType::REGISTER;
                exdc_latch_.access_type = AccessType::UDOUBLEWORD;
                break;
            }
            /**
             * ORI
             * 
             * doesn't throw
             */
            case InstructionType::ORI: {
                exdc_latch_.dest = cur_instr.IType.rt;
                exdc_latch_.data = rfex_latch_.fetched_rs.UD | cur_instr.IType.immediate;
                exdc_latch_.write_type = WriteType::REGISTER;
                exdc_latch_.access_type = AccessType::UDOUBLEWORD;
                break;
            }
            /**
             * SD
             * 
             * throws TLB miss exception
             *        TLB invalid exception
             *        TLB modification exception
             *        Bus error exception
             *        Address error exception
             *        Reserved instruction exception (32-bit User or Supervisor mode)
             */
            case InstructionType::SD: {
                int16_t offset = cur_instr.IType.immediate;
                int32_t seoffset = offset;
                auto write_vaddr = seoffset + rfex_latch_.fetched_rs.UW._0;
                auto paddr_s = translate_vaddr(write_vaddr);
                exdc_latch_.dest = paddr_s.paddr;
                exdc_latch_.cached = paddr_s.cached;
                exdc_latch_.data = rfex_latch_.fetched_rt.UD;
                exdc_latch_.write_type = WriteType::MMU;
                exdc_latch_.access_type = AccessType::UDOUBLEWORD;
                #if SKIPEXCEPTIONS == 0
                if ((exdc_latch_.vaddr & 0b111) != 0) { 
                    // From manual:
                    // If either of the loworder two bits of the address are not zero, an address error exception occurs.
                    throw InstructionAddressErrorException();
                }
                if (!mode64_ && opmode_ != OperatingMode::Kernel) {
                    // From manual:
                    // This operation is defined for the VR4300 operating in 64-bit mode and in 32-bit
                    // Kernel mode. Execution of this instruction in 32-bit User or Supervisor mode
                    // causes a reserved instruction exception.
                    throw ReservedInstructionException();
                }
                #endif
                break;
            }
            /**
             * SW
             * 
             * throws TLB miss exception
             *        TLB invalid exception
             *        TLB modification exception
             *        Bus error exception
             *        Address error exception
             */
            case InstructionType::SW: {
                int16_t offset = cur_instr.IType.immediate;
                int32_t seoffset = offset;
                auto write_vaddr = seoffset + rfex_latch_.fetched_rs.UW._0;
                auto paddr_s = translate_vaddr(write_vaddr);
                exdc_latch_.dest = paddr_s.paddr;
                exdc_latch_.cached = paddr_s.cached;
                exdc_latch_.data = rfex_latch_.fetched_rt.UW._0;
                exdc_latch_.write_type = WriteType::MMU;
                exdc_latch_.access_type = AccessType::UWORD;
                #if SKIPEXCEPTIONS == 0
                if ((exdc_latch_.dest & 0b11) != 0) {
                    // From manual:
                    // If either of the loworder two bits of the address are not zero, an address error exception occurs.
                    throw InstructionAddressErrorException();
                }
                #endif
                break;
            }
            /**
             * LBU
             * 
             * throws TLB miss exception
             *        TLB invalid exception
             *        Bus error exception
             *        Address error exception (?)
             */
            case InstructionType::LBU: {
                int16_t offset = cur_instr.IType.immediate;
                int32_t seoffset = offset;
                exdc_latch_.dest = cur_instr.IType.rt;
                exdc_latch_.vaddr = seoffset + rfex_latch_.fetched_rs.UW._0;
                exdc_latch_.write_type = WriteType::LATEREGISTER;
                exdc_latch_.access_type = AccessType::UBYTE;
                break;
            }
            /**
             * LD
             * 
             * throws TLB miss exception
             *        TLB invalid exception
             *        Bus error exception
             *        Address error exception
             */
            case InstructionType::LD: {
                int16_t offset = cur_instr.IType.immediate;
                int32_t seoffset = offset;
                exdc_latch_.dest = cur_instr.IType.rt;
                exdc_latch_.vaddr = seoffset + rfex_latch_.fetched_rs.UW._0;
                exdc_latch_.write_type = WriteType::LATEREGISTER;
                exdc_latch_.access_type = AccessType::UDOUBLEWORD;
                #if SKIPEXCEPTIONS == 0
                if ((exdc_latch_.vaddr & 0b111) != 0) { 
                    // From manual:
                    // If either of the loworder two bits of the address are not zero, an address error exception occurs.
                    throw InstructionAddressErrorException();
                }
                if (!mode64_ && opmode_ != OperatingMode::Kernel) {
                    // From manual:
                    // This operation is defined for the VR4300 operating in 64-bit mode and in 32-bit
                    // Kernel mode. Execution of this instruction in 32-bit User or Supervisor mode
                    // causes a reserved instruction exception.
                    throw ReservedInstructionException();
                }
                #endif
                break;
            }
            /**
             * LHU
             * 
             * throws TLB miss exception
             *        TLB invalid exception
             *        Bus error exception
             *        Address error exception
             */
            case InstructionType::LHU: {
                int16_t offset = cur_instr.IType.immediate;
                int32_t seoffset = offset;
                exdc_latch_.dest = cur_instr.IType.rt;
                exdc_latch_.vaddr = seoffset + rfex_latch_.fetched_rs.UW._0;
                exdc_latch_.write_type = WriteType::LATEREGISTER;
                exdc_latch_.access_type = AccessType::UHALFWORD;
                #if SKIPEXCEPTIONS == 0
                if ((exdc_latch_.vaddr & 0b1) != 0) {
                    // From manual:
                    // If the least-significant bit of the address is not zero, an address error exception occurs.
                    throw InstructionAddressErrorException();
                }
                #endif
                break;
            }
            /**
             * LW
             * 
             * throws TLB miss exception
             *        TLB invalid exception
             *        Bus error exception
             *        Address error exception
             */
            case InstructionType::LW: {
                int16_t offset = cur_instr.IType.immediate;
                int32_t seoffset = offset;
                exdc_latch_.dest = cur_instr.IType.rt;
                exdc_latch_.vaddr = seoffset + rfex_latch_.fetched_rs.UW._0;
                exdc_latch_.write_type = WriteType::LATEREGISTER;
                exdc_latch_.access_type = AccessType::UWORD;
                #if SKIPEXCEPTIONS == 0
                if ((exdc_latch_.vaddr & 0b11) != 0) {
                    // From manual:
                    // If either of the loworder two bits of the address are not zero, an address error exception occurs.
                    throw InstructionAddressErrorException();
                }
                #endif
                break;
            }
            /**
             * ANDI
             * 
             * doesn't throw
             */
            case InstructionType::ANDI: {
                exdc_latch_.dest = cur_instr.IType.rt;
                exdc_latch_.data = rfex_latch_.fetched_rs.UD & cur_instr.IType.immediate;
                exdc_latch_.write_type = WriteType::REGISTER;
                exdc_latch_.access_type = AccessType::UDOUBLEWORD;
                break;
            }
            /**
             * BEQ
             * 
             * doesn't throw
             */
            case InstructionType::BEQ: {
                int16_t offset = cur_instr.IType.immediate << 2;
                int32_t seoffset = offset;
                if (rfex_latch_.fetched_rs.UD == rfex_latch_.fetched_rt.UD) {
                    exdc_latch_.data = pc_ - 4 + seoffset;
                    exdc_latch_.dest_direct = &pc_;
                    exdc_latch_.write_type = WriteType::REGISTER;
                    exdc_latch_.access_type = AccessType::UDOUBLEWORD_DIRECT;
                }
                break;
            }
            /**
             * BNE
             * 
             * doesn't throw
             */
            case InstructionType::BNE: {
                int16_t offset = cur_instr.IType.immediate << 2;
                int32_t seoffset = offset;
                if (rfex_latch_.fetched_rs.UD != rfex_latch_.fetched_rt.UD) {
                    exdc_latch_.data = pc_ - 4 + seoffset;
                    exdc_latch_.dest_direct = &pc_;
                    exdc_latch_.write_type = WriteType::REGISTER;
                    exdc_latch_.access_type = AccessType::UDOUBLEWORD_DIRECT;
                }
                break;
            }
            /**
             * s_ADD, s_ADDU
             * 
             * s_ADD throws IntegerOverflowException
             */
            case InstructionType::s_ADD:
            case InstructionType::s_ADDU: {
                int32_t result = 0;
                bool overflow = __builtin_add_overflow(rfex_latch_.fetched_rt.W._0, rfex_latch_.fetched_rs.W._0, &result);
                exdc_latch_.dest = cur_instr.RType.rd;
                exdc_latch_.data = result;
                exdc_latch_.write_type = WriteType::REGISTER;
                exdc_latch_.access_type = AccessType::WORD;
                #if SKIPEXCEPTIONS == 0
                if (overflow) {
                    // An integer overflow exception occurs if carries out of bits 30 and 31 differ (2’s
                    // complement overflow). The contents of destination register rd is not modified
                    // when an integer overflow exception occurs.
                    exdc_latch_.write_type = WriteType::NONE;
                    throw IntegerOverflowException();
                }
                #endif
                break;
            }
            /**
             * s_JR, s_JALR
             * 
             * throws Address error exception
             */
            case InstructionType::s_JALR: {
                auto reg = (rfex_latch_.instruction.RType.rd == 0) ? 31 : rfex_latch_.instruction.RType.rd;
                gpr_regs_[reg].UD = pc_; // By the time this instruction is executed, pc is already incremented by 8
                                         // so there's no need to increment here
                #if SKIPEXCEPTIONS == 0
                // From manual:
                // Register numbers rs and rd should not be equal, because such an instruction does
                // not have the same effect when re-executed. If they are equal, the contents of rs
                // are destroyed by storing link address. However, if an attempt is made to execute
                // this instruction, an exception will not occur, and the result of executing such an
                // instruction is undefined.
                assert(rfex_latch_.instruction.RType.rd != rfex_latch_.instruction.RType.rs);
                #endif
                [[fallthrough]];
            }
            case InstructionType::s_JR: {
                auto jump_addr = rfex_latch_.fetched_rs.UD;
                exdc_latch_.data = jump_addr;
                exdc_latch_.dest_direct = &pc_;
                exdc_latch_.write_type = WriteType::REGISTER;
                exdc_latch_.access_type = AccessType::UDOUBLEWORD_DIRECT;
                #if SKIPEXCEPTIONS == 0
                if ((jump_addr & 0b11) != 0) {
                    // From manual:
                    // Since instructions must be word-aligned, a Jump Register instruction must
                    // specify a target register (rs) which contains an address whose low-order two bits
                    // are zero. If these low-order two bits are not zero, an address exception will occur
                    // when the jump target instruction is fetched.
                    // TODO: when the jump target instruction is *fetched*. Does it matter that the exc is thrown here?
                    throw InstructionAddressErrorException();
                }
                #endif
                break;
            }
            /**
             * s_SLL
             * 
             * doesn't throw
             */
            case InstructionType::s_SLL: {
                exdc_latch_.dest = cur_instr.RType.rd;
                exdc_latch_.data = rfex_latch_.fetched_rt.UD << cur_instr.RType.sa;
                exdc_latch_.write_type = WriteType::REGISTER;
                exdc_latch_.access_type = AccessType::UDOUBLEWORD;
                break;
            }
            /**
             * s_SLT
             * 
             * doesn't throw
             */
            case InstructionType::s_SLT: {
                if (rfex_latch_.fetched_rs.W._0 < rfex_latch_.fetched_rt.W._0) {
                    exdc_latch_.data = static_cast<uint64_t>(1);
                } else {
                    exdc_latch_.data = static_cast<uint64_t>(0);
                }
                exdc_latch_.dest = cur_instr.RType.rd;
                exdc_latch_.write_type = WriteType::REGISTER;
                exdc_latch_.access_type = AccessType::UDOUBLEWORD;
                break;
            }
            /**
             * NOP
             * 
             * doesn't throw
             */
            case InstructionType::NOP: {
                return;
            }
            default: {
                std::string instr_name;
                if (cur_instr.IType.op == 0) {
                    instr_name = SpecialCodes[cur_instr.RType.func];
                } else {
                    instr_name = OperationCodes[cur_instr.IType.op];
                }
                throw InstructionNotImplementedException(instr_name);
            }
        }
        if (exdc_latch_.write_type == WriteType::REGISTER) {
            // Bypassing (from manual)
            // result is stored during EX instead of waiting for WB when it comes
            // to writing to a register
            // for instructions like ADD that don't need DC stage to write back
            store_register(exdc_latch_.access_type, exdc_latch_.dest, exdc_latch_.dest_direct, exdc_latch_.data);
            exdc_latch_.data.reset();
            exdc_latch_.write_type = WriteType::NONE;
        }
    }
}