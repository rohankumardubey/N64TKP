#include "n64_cpu.hxx"

namespace TKPEmu::N64::Devices {
    void CPU::execute_instruction() {
        auto& cur_instr = rfex_latch_.instruction;
        EXDC_latch temp;
        std::swap(exdc_latch_, temp);
        switch(rfex_latch_.instruction_type) {
            /**
             * LUI
             * 
             * doesn't throw
             */
            case InstructionType::LUI: {
                int32_t imm = cur_instr.IType.immediate << 16;
                // Sign extend the immediate
                uint64_t seimm = static_cast<int64_t>(imm);
                exdc_latch_.dest = cur_instr.IType.rt;
                exdc_latch_.data = seimm;
                exdc_latch_.write_type = WriteType::REGISTER;
                exdc_latch_.access_type = AccessType::DOUBLEWORD;
                break;
            }
            /**
             * ORI
             * 
             * doesn't throw
             */
            case InstructionType::ORI: {
                exdc_latch_.dest = cur_instr.IType.rt;
                exdc_latch_.data = rfex_latch_.fetched_rt.UD | cur_instr.IType.immediate;
                exdc_latch_.write_type = WriteType::REGISTER;
                exdc_latch_.access_type = AccessType::DOUBLEWORD;
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
                auto write_vaddr = seoffset + rfex_latch_.fetched_rs.UW._0;
                auto paddr_s = translate_vaddr(write_vaddr);
                exdc_latch_.dest = paddr_s.paddr;
                exdc_latch_.cached = paddr_s.cached;
                exdc_latch_.write_type = WriteType::MMU;
                exdc_latch_.data = rfex_latch_.fetched_rt.UW._0;
                exdc_latch_.access_type = AccessType::WORD;
                #if SKIPEXCEPTIONS == 0
                if ((exdc_latch_.dest & 0b11) != 0) {
                    // From manual:
                    // If either of the loworder two bits of the address are not zero, an address error exception occurs.
                    throw InstructionAddressErrorException();
                }
                #endif
                break;
            }
            default: {
                throw InstructionNotImplementedException(OperationCodes[cur_instr.IType.op]);
            }
        }
        if (exdc_latch_.write_type == WriteType::REGISTER) {
            // Bypassing
            // result is stored during EX instead of waiting for WB when it comes
            // to writing to a register
            store_register();
        }
    }
}