#include "n64_cpu.hxx"
#include <cstring> // memcpy

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
        pipeline_.resize(1);
        pipeline_[0] = PipelineStage::IC;
    }

    bool CPU::execute_stage(PipelineStage stage, size_t process_no) {
        switch(stage) {
            case PipelineStage::IC: {
                IC(process_no);
                return true;
            }
            case PipelineStage::RF: {
                RF(process_no);
                return true;
            }
            case PipelineStage::EX: {
                EX(process_no);
                return true;
            }
            case PipelineStage::DC: {
                DC(process_no);
                return true;
            }
            case PipelineStage::WB: {
                WB(process_no);
                return true;
            }
            default: {
                return false;
            }
        }
    }

    CPU::PipelineStageRet CPU::IC(PipelineStageArgs process_no) {
        // Fetch the current process instruction
        auto paddr_s = translate_vaddr(pc_);
        icrf_latch_.instruction.Full = cpubus_.fetch_instruction_uncached(paddr_s.paddr);
        pc_ += 4;
    }

    CPU::PipelineStageRet CPU::RF(PipelineStageArgs process_no) {
        // Fetch the registers
        auto fetched_rs = gpr_regs_[icrf_latch_.instruction.RType.rs];
        auto fetched_rt = gpr_regs_[icrf_latch_.instruction.RType.rt];
        InstructionType type;
        // Get instruction from previous fetch
        if (icrf_latch_.instruction.Full == 0) {
            type = InstructionType::NOP;
        } else {
            auto opcode = icrf_latch_.instruction.IType.op;
            switch(opcode) {
                case 0: {
                    auto func = icrf_latch_.instruction.RType.func;
                    type = SpecialInstructionTypeTable[func];
                    break;
                }
                default: {
                    type = InstructionTypeTable[opcode];
                    break;
                }
            }
        }
        rfex_latch_.fetched_rs.UD = fetched_rs.UD;
        rfex_latch_.fetched_rt.UD = fetched_rt.UD;
        rfex_latch_.instruction = icrf_latch_.instruction;
        rfex_latch_.instruction_type = type;
    }

    CPU::PipelineStageRet CPU::EX(PipelineStageArgs process_no) {
        execute_instruction(process_no);
    }

    CPU::PipelineStageRet CPU::DC(PipelineStageArgs process_no) {
        // unimplemented atm
        dcwb_latch_.data.reset();
        dcwb_latch_.access_type = exdc_latch_.access_type;
        dcwb_latch_.dest = exdc_latch_.dest;
        dcwb_latch_.write_type = exdc_latch_.write_type;
        dcwb_latch_.cached = exdc_latch_.cached;
        switch (exdc_latch_.write_type) {
            case WriteType::LATEREGISTER: {
                auto paddr_s = translate_vaddr(exdc_latch_.vaddr);
                uint32_t paddr = paddr_s.paddr;
                dcwb_latch_.cached = paddr_s.cached;
                load_memory(dcwb_latch_.cached, dcwb_latch_.access_type,
                            dcwb_latch_.data, paddr);
                break;
            }
            default: {
                break;
            }
        }
        if (!dcwb_latch_.data.has_value()) {
            dcwb_latch_.data = exdc_latch_.data;
        }
    }

    CPU::PipelineStageRet CPU::WB(PipelineStageArgs process_no) {
        if (dcwb_latch_.data.has_value()) {
            switch(dcwb_latch_.write_type) {
                case WriteType::MMU: {
                    store_memory(dcwb_latch_.cached, dcwb_latch_.access_type,
                            dcwb_latch_.data, dcwb_latch_.dest);
                    break;
                }
                case WriteType::LATEREGISTER: {
                    store_register(dcwb_latch_.access_type, dcwb_latch_.dest, nullptr, dcwb_latch_.data);
                    break;
                }
                case WriteType::REGISTER: {
                    throw std::logic_error("Bad write type: should've used LATEREGISTER");
                }
                default: {
                    break;
                }
            }
        }
    }

    void CPU::update_pipeline() {
        for (auto& process : pipeline_) {
            PipelineStage& ps = process;
            execute_stage(ps, 0);
            ps = static_cast<PipelineStage>(static_cast<int>(ps) + 1);
        }
        if (pipeline_.front() == PipelineStage::NOP) {
            pipeline_.pop_front();
        }
        pipeline_.push_back(PipelineStage::IC);
        //pipeline_cur_instr_.push_back({});
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

    void CPU::store_register(AccessType access_type, uint32_t dest, uint64_t* dest_direct, std::any data) {
        switch(access_type) {
            case AccessType::BYTE: {
                gpr_regs_[dest].UB._0
                    = std::any_cast<uint8_t>(data);
                break;
            }
            case AccessType::HALFWORD: {
                int64_t temp = static_cast<int16_t>(std::any_cast<uint16_t>(data));
                gpr_regs_[dest].UD = temp;
                break;
            }
            case AccessType::HALFWORD_UNSIGNED: {
                uint64_t temp = std::any_cast<uint16_t>(data);
                gpr_regs_[dest].UD = temp;
                break;
            }
            case AccessType::WORD: {
                gpr_regs_[dest].UW._0
                    = std::any_cast<uint32_t>(data);
                break;
            }
            case AccessType::DOUBLEWORD: {
                gpr_regs_[dest].UD
                    = std::any_cast<uint64_t>(data);
                break;
            }
            case AccessType::DOUBLEWORD_DIRECT: {
                if (dest_direct) {
                    *dest_direct = std::any_cast<uint64_t>(data);
                } else {
                    throw std::logic_error("Tried to dereference a null ptr");
                }
                break;
            }
            default: {
                throw NotImplementedException(__func__);
            }
        }
    }

    void CPU::store_memory(bool cached, AccessType type, std::any data_any, uint32_t paddr) {
        if (!cached) {
            uint8_t* loc = cpubus_.redirect_paddress(paddr);
            switch(type) {
                case AccessType::DOUBLEWORD: {
                    auto data = std::any_cast<uint64_t>(data_any);
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
                    auto data = std::any_cast<uint32_t>(data_any);
                    *(loc++) = (data >> 24) & 0xFF;
                    *(loc++) = (data >> 16) & 0xFF;
                    *(loc++) = (data >> 8)  & 0xFF;
                    *(loc)   = data & 0xFF;
                    break;
                }
                case AccessType::HALFWORD: {
                    auto data = std::any_cast<uint16_t>(data_any);
                    *(loc++) = (data >> 8) & 0xFF;
                    *(loc)   = data & 0xFF;
                    break;
                }
                case AccessType::BYTE: {
                    auto data = std::any_cast<uint8_t>(data_any);
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
    void CPU::load_memory(bool cached, AccessType type, std::any& data_any, uint32_t paddr) {
        if (!cached) {
            uint8_t* loc = cpubus_.redirect_paddress(paddr);
            switch(type) {
                case AccessType::DOUBLEWORD: {
                    uint64_t data = 0;
                    std::memcpy(&data, loc, 8);
                    data_any = __builtin_bswap64(data);
                    break;
                }
                case AccessType::WORD: {
                    uint32_t data = 0;
                    std::memcpy(&data, loc, 4);
                    data_any = __builtin_bswap32(data);
                    break;
                }
                case AccessType::HALFWORD: {
                    uint16_t data = 0;
                    int32_t sedata = static_cast<int16_t>(data);
                    std::memcpy(&data, loc, 2);
                    data_any = __builtin_bswap16(data);
                    break;
                }
                case AccessType::HALFWORD_UNSIGNED: {
                    uint16_t data = 0;
                    std::memcpy(&data, loc, 2);
                    data_any = __builtin_bswap16(data);
                    break;
                }
                case AccessType::BYTE: {
                    uint8_t data = 0;
                    data = *(loc);
                    data_any = data;
                    break;
                }
                default: {
                    // Do nothing purposefully
                    // TODO: figure out if other access types are used and remove this
                    break;
                }
            }
        } else {
            // currently not implemented
        }
    }
}