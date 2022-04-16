#include "n64_cpu.hxx"

namespace TKPEmu::N64::Devices {

    CPU::CPU() :
        instr_cache_(KB(16)),
        data_cache_(KB(8))
    {
        Reset();
    }

    void CPU::Reset() {
        pc_ = 0x1000;
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

    CPU::PipelineStageRet CPU::IC(PipelineStageArgs process_no) {
        // Fetch the current process instruction
        pipeline_storage_[process_no].instruction.Full
            = select_addr_space32(pc_);
        pc_ += 4;
    }

    CPU::PipelineStageRet CPU::RF(PipelineStageArgs process_no) {
        // Fetch the registers
        auto opcode = pipeline_storage_[process_no].instruction.IType.op;
        switch(opcode) {
            case 0: {
                auto func = pipeline_storage_[process_no].instruction.RType.func;
                pipeline_storage_[process_no].type = SpecialInstructionTypeTable[func];
                break;
            }
            default: {
                pipeline_storage_[process_no].type = InstructionTypeTable[opcode];
                break;
            }
        }
    }

    CPU::PipelineStageRet CPU::EX(PipelineStageArgs process_no) {
        PipelineException exception = PipelineException::None;
        Instruction& cur_instr = pipeline_storage_[process_no].instruction;
        switch(pipeline_storage_[process_no].type) {
            /**
             * ADD - throws IntegerOverflowException
             * 
             * 
             */
            case InstructionType::s_ADD: {
                MemDataW op1 = gpr_regs_[cur_instr.RType.rs].W._0;
                MemDataW op2 = gpr_regs_[cur_instr.RType.rt].W._0;
                // Sign extend the operands
                MemDataD seop1 = op1;
                MemDataD seop2 = op2;
                MemDataD result = seop1 + seop2;
                // Check for integeroverflowexception - missing
                gpr_regs_[cur_instr.RType.rd].D = result;
                break;
            }
            default: {
                throw InstructionNotImplementedException(cur_instr.Full);
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

    CPU::DirectMapRet CPU::translate_kseg0(DirectMapArgs addr) noexcept {
        return addr - KSEG0_START;
    }

    CPU::DirectMapRet CPU::translate_kseg1(DirectMapArgs addr) noexcept {
        return addr - KSEG1_START;
    }

    CPU::TLBRet CPU::translate_kuseg(TLBArgs) {
        throw NotImplementedException(__func__);
    }

    CPU::SelAddrSpace32Ret CPU::select_addr_space32(SelAddrSpace32Args addr) {
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
        return physical_addr;
    }
}