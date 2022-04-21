#include "n64_cpu.hxx"

namespace TKPEmu::N64::Devices {
    void CPU::execute_instruction(PipelineStageArgs process_no) {
        RFEX_latch& rfex_latch = pipeline_latches_[process_no].rfex_latch_;
        EXDC_latch temp {};
        std::swap(pipeline_latches_[process_no].exdc_latch_, temp);
        EXDC_latch& exdc_latch = pipeline_latches_[process_no].exdc_latch_;
        auto& cur_instr = rfex_latch.instruction;
        switch(rfex_latch.instruction_type) {
            /**
             * LUI
             * 
             * doesn't throw
             */
            case InstructionType::LUI: {
                int32_t imm = cur_instr.IType.immediate << 16;
                // Sign extend the immediate
                uint64_t seimm = static_cast<int64_t>(imm);
                exdc_latch.dest = cur_instr.IType.rt;
                exdc_latch.data = seimm;
                exdc_latch.write_type = WriteType::REGISTER;
                exdc_latch.access_type = AccessType::DOUBLEWORD;
                break;
            }
            /**
             * ORI
             * 
             * doesn't throw
             */
            case InstructionType::ORI: {
                exdc_latch.dest = cur_instr.IType.rt;
                exdc_latch.data = rfex_latch.fetched_rs.UD | cur_instr.IType.immediate;
                exdc_latch.write_type = WriteType::REGISTER;
                exdc_latch.access_type = AccessType::DOUBLEWORD;
                break;
            }
            /**
             * s_ADD
             * 
             * throws IntegerOverflowException
             */
            case InstructionType::s_ADD: {
                
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
                auto write_vaddr = seoffset + rfex_latch.fetched_rs.UW._0;
                auto paddr_s = translate_vaddr(write_vaddr);
                exdc_latch.dest = paddr_s.paddr;
                exdc_latch.cached = paddr_s.cached;
                exdc_latch.data = rfex_latch.fetched_rt.UW._0;
                exdc_latch.write_type = WriteType::MMU;
                exdc_latch.access_type = AccessType::WORD;
                #if SKIPEXCEPTIONS == 0
                if ((exdc_latch.dest & 0b11) != 0) {
                    // From manual:
                    // If either of the loworder two bits of the address are not zero, an address error exception occurs.
                    throw InstructionAddressErrorException();
                }
                #endif
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
                exdc_latch.dest = cur_instr.IType.rt;
                exdc_latch.vaddr = seoffset + rfex_latch.fetched_rs.UW._0;
                exdc_latch.write_type = WriteType::LATEREGISTER;
                exdc_latch.access_type = AccessType::DOUBLEWORD;
                #if SKIPEXCEPTIONS == 0
                if ((exdc_latch.vaddr & 0b111) != 0) { 
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
                exdc_latch.dest = cur_instr.IType.rt;
                exdc_latch.vaddr = seoffset + rfex_latch.fetched_rs.UW._0;
                exdc_latch.write_type = WriteType::LATEREGISTER;
                exdc_latch.access_type = AccessType::HALFWORD_UNSIGNED;
                #if SKIPEXCEPTIONS == 0
                if ((exdc_latch.vaddr & 0b1) != 0) {
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
                exdc_latch.dest = cur_instr.IType.rt;
                exdc_latch.vaddr = seoffset + rfex_latch.fetched_rs.UW._0;
                exdc_latch.write_type = WriteType::LATEREGISTER;
                exdc_latch.access_type = AccessType::WORD;
                #if SKIPEXCEPTIONS == 0
                if ((exdc_latch.vaddr & 0b11) != 0) {
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
                exdc_latch.dest = cur_instr.IType.rt;
                exdc_latch.data = rfex_latch.fetched_rs.UD & cur_instr.IType.immediate;
                exdc_latch.write_type = WriteType::REGISTER;
                exdc_latch.access_type = AccessType::DOUBLEWORD;
                break;
            }
            /**
             * BNE
             * 
             * doesn't throw
             */
            case InstructionType::BNE: {
                // Shifts immediate left by 2 and sign extends
                int16_t offset = cur_instr.IType.immediate << 2;
                int32_t seoffset = offset;
                if (rfex_latch.fetched_rs.UD != rfex_latch.fetched_rt.UD) {
                    exdc_latch.data = pc_ + seoffset;
                    exdc_latch.dest_direct = &pc_;
                    exdc_latch.write_type = WriteType::REGISTER;
                    exdc_latch.access_type = AccessType::DOUBLEWORD_DIRECT;
                }
                break;
            }
            case InstructionType::s_JR: {
                auto jump_addr = rfex_latch.fetched_rs.UD;
                exdc_latch.data = jump_addr;
                exdc_latch.dest_direct = &pc_;
                exdc_latch.write_type = WriteType::REGISTER;
                exdc_latch.access_type = AccessType::DOUBLEWORD_DIRECT;
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
             * NOP
             * 
             * doesn't throw
             */
            case InstructionType::NOP: {
                return;
            }
            default: {
                throw InstructionNotImplementedException(OperationCodes[cur_instr.IType.op]);
            }
        }
        if (exdc_latch.write_type == WriteType::REGISTER) {
            // Bypassing (from manual)
            // result is stored during EX instead of waiting for WB when it comes
            // to writing to a register
            // for instructions like ADD that don't need DC stage to write back
            store_register(exdc_latch.access_type, exdc_latch.dest, exdc_latch.dest_direct, exdc_latch.data);
            exdc_latch.data.reset();
            exdc_latch.write_type = WriteType::NONE;
        }
    }
}