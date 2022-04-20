#include "n64_cpu.hxx"
#include <iostream>

namespace TKPEmu::N64::Devices {

    CPU::CPU() :
        gpr_regs_{},
        fpr_regs_{},
        instr_cache_(KB(16)),
        data_cache_(KB(8))
    {
        Reset();
    }

    void CPU::Reset() {
        pc_ = 0x80001000;
        for (int i = 0; i < 5; i++) {
            std::queue<PipelineStage> empty;
            std::swap(pipeline_[i], empty);
            // Initialize pipeline like this:
            // 1. IC
            // 2. NOP IC
            // 3. NOP NOP IC
            // 4. NOP NOP NOP IC
            // 5. NOP NOP NOP NOP IC
            for (int j = 0; j < i; j++) {
                pipeline_[i].push(PipelineStage::NOP);
            }
            pipeline_[i].push(PipelineStage::IC);
        }
    }

    bool CPU::execute_instruction(PipelineStage stage, size_t process_no) {
        switch(stage) {
            case PipelineStage::IC: {
                IC(process_no);
                pipeline_[process_no].push(PipelineStage::RF);
                return true;
            }
            case PipelineStage::RF: {
                RF(process_no);
                pipeline_[process_no].push(PipelineStage::EX);
                return true;
            }
            case PipelineStage::EX: {
                EX(process_no);
                pipeline_[process_no].push(PipelineStage::DC);
                return true;
            }
            case PipelineStage::DC: {
                DC(process_no);
                pipeline_[process_no].push(PipelineStage::WB);
                return true;
            }
            case PipelineStage::WB: {
                WB(process_no);
                pipeline_[process_no].push(PipelineStage::IC);
                return true;
            }
            default: {
                return false;
            }
        }
    }

    void CPU::register_write(size_t process_no) {
        PipelineStorage& cur_storage = pipeline_storage_[process_no];
        switch(cur_storage.access_type) {
            case AccessType::BYTE: {
                *std::any_cast<uint8_t*>(cur_storage.write_loc)
                    = std::any_cast<uint8_t>(cur_storage.data);
                break;
            }
            case AccessType::HALFWORD: {
                *std::any_cast<uint16_t*>(cur_storage.write_loc)
                    = std::any_cast<uint16_t>(cur_storage.data);
                break;
            }
            case AccessType::WORD: {
                *std::any_cast<uint32_t*>(cur_storage.write_loc)
                    = std::any_cast<uint32_t>(cur_storage.data);
                break;
            }
            case AccessType::DOUBLEWORD: {
                *std::any_cast<uint64_t*>(cur_storage.write_loc)
                    = std::any_cast<uint64_t>(cur_storage.data);
                break;
            }
            default: {
                throw NotImplementedException(__func__);
            }
        }
    }

    CPU::PipelineStageRet CPU::IC(PipelineStageArgs process_no) {
        // Fetch the current process instruction
        auto paddr_s = translate_vaddr(pc_);
        auto& cur_storage = pipeline_storage_[process_no];
        cur_storage.instruction.Full = cpubus_.fetch_instruction_uncached(paddr_s.paddr);
        cur_storage.cached = paddr_s.cached;
    }

    CPU::PipelineStageRet CPU::RF(PipelineStageArgs process_no) {
        // Fetch the registers

        // Get instruction from previous fetch
        auto opcode = pipeline_storage_[process_no].instruction.IType.op;
        switch(opcode) {
            case 0: {
                auto func = pipeline_storage_[process_no].instruction.RType.func;
                pipeline_storage_[process_no].instr_type = SpecialInstructionTypeTable[func];
                break;
            }
            default: {
                pipeline_storage_[process_no].instr_type = InstructionTypeTable[opcode];
                break;
            }
        }
        pc_ += 4;
    }

    CPU::PipelineStageRet CPU::EX(PipelineStageArgs process_no) {
        PipelineStorage& cur_storage = pipeline_storage_[process_no];
        Instruction& cur_instr = cur_storage.instruction;
        cur_storage.paddr = 0;
        cur_storage.vaddr = 0;
        cur_storage.write_loc.reset();
        cur_storage.data.reset();
        switch(cur_storage.instr_type) {
            /**
             * LUI
             * 
             * doesn't throw
             */
            case InstructionType::LUI: {
                int32_t imm = cur_instr.IType.immediate << 16;
                // Sign extend the immediate
                uint64_t seimm = static_cast<int64_t>(imm);
                cur_storage.write_loc = &gpr_regs_[cur_instr.IType.rt].UD;
                cur_storage.data = seimm;
                cur_storage.write_type = WriteType::REGISTER;
                cur_storage.access_type = AccessType::DOUBLEWORD;
                break;
            }
            /**
             * ORI
             * 
             * doesn't throw
             */
            case InstructionType::ORI: {
                cur_storage.write_loc = &gpr_regs_[cur_instr.IType.rt].UD;
                cur_storage.data = gpr_regs_[cur_instr.IType.rt].UD | cur_instr.IType.immediate;
                cur_storage.write_type = WriteType::REGISTER;
                cur_storage.access_type = AccessType::DOUBLEWORD;
                break;
            }
            /**
             * s_ADD
             * 
             * throws IntegerOverflowException
             */
            case InstructionType::s_ADD: {
                int32_t op1 = gpr_regs_[cur_instr.RType.rs].W._0;
                int32_t op2 = gpr_regs_[cur_instr.RType.rt].W._0;
                // Sign extend the operands
                int64_t seop1 = op1;
                int64_t seop2 = op2;
                int64_t result = seop1 + seop2;
                // TODO: Check for integeroverflowexception - missing
                cur_storage.write_loc = &gpr_regs_[cur_instr.RType.rd].D;
                cur_storage.data = result;
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
                #if SKIPEXCEPTIONS == 0
                if ((cur_storage.vaddr & 0b11) != 0) {
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
    }

    CPU::PipelineStageRet CPU::DC(PipelineStageArgs process_no) {
        // unimplemented atm
    }

    CPU::PipelineStageRet CPU::WB(PipelineStageArgs process_no) {
        PipelineStorage& cur_storage = pipeline_storage_[process_no];
        if (cur_storage.write_loc.has_value() &&
            cur_storage.data.has_value()) {
            switch(cur_storage.write_type) {
                case WriteType::REGISTER: {
                    register_write(process_no);
                    break;
                }
                case WriteType::MMU: {
                    store_memory(cur_storage.cached, cur_storage.access_type,
                            std::any_cast<uint64_t>(cur_storage.data), cur_storage.paddr);
                    break;
                }
            }
        }
    }

    void CPU::update_pipeline() {
        for (size_t i = 0; i < 5; i++) {
            auto& process = pipeline_[i];
            PipelineStage ps = process.front();
            process.pop();
            execute_instruction(ps, i);
        }
    }

    uint32_t CPU::translate_kseg0(uint32_t vaddr) noexcept {
        return vaddr - KSEG0_START;
    }

    uint32_t CPU::translate_kseg1(uint32_t vaddr) noexcept {
        return vaddr - KSEG1_START;
    }

    uint32_t CPU::translate_kuseg(uint32_t vaddr) noexcept {
        throw NotImplementedException(__func__);
    }

    TranslatedAddress CPU::translate_vaddr(uint32_t addr) {
        // VR4300 manual page 136 table 5-3
        unsigned _3msb = addr >> 29;
        uint32_t paddr = 0;
        bool cached = false;
        switch(_3msb) {
            case 0b100:
            // kseg0
            // cached, non tlb
            paddr = translate_kseg0(addr);
            cached = true;
            break;
            case 0b101:
            // kseg1
            // uncached, non tlb
            paddr = translate_kseg1(addr);
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
            paddr = translate_kuseg(addr);
            cached = true;
            break;
        }
        return { paddr, cached };
    }

    void CPU::store_memory(bool cached, AccessType type, uint64_t data, uint32_t paddr, uint64_t vaddr) {
        if (!cached) {
            uint8_t* loc = cpubus_.redirect_paddress(paddr);
            switch(type) {
                case AccessType::DOUBLEWORD: {
                    *(loc++) = (data >> 56) & 0xFF;
                    *(loc++) = (data >> 48) & 0xFF;
                    *(loc++) = (data >> 40) & 0xFF;
                    *(loc++) = (data >> 32) & 0xFF;
                    *(loc++) = (data >> 24) & 0xFF;
                    *(loc++) = (data >> 16) & 0xFF;
                    *(loc++) = (data >> 8)  & 0xFF;
                    *(loc)   = data & 0xFF;
                    break;
                }
                case AccessType::WORD: {
                    *(loc++) = (data >> 24) & 0xFF;
                    *(loc++) = (data >> 16) & 0xFF;
                    *(loc++) = (data >> 8)  & 0xFF;
                    *(loc)   = data & 0xFF;
                    break;
                }
                case AccessType::HALFWORD: {
                    *(loc++) = (data >> 8) & 0xFF;
                    *(loc)   = data & 0xFF;
                    break;
                }
                case AccessType::BYTE: {
                    *(loc) = data & 0xFF;
                    break;
                }
                default: {
                    // Do nothing purposefully
                    break;
                }
            }
        } else {
            // currently not implemented
        }
    }
}