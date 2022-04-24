#include "n64_cpu.hxx"
#include <cstring> // memcpy
#include <cassert> // assert

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
        clear_registers();
        pipeline_.clear();
        pipeline_cur_instr_.clear();
        pipeline_.push_back(PipelineStage::IC);
        instructions_ran_ = 1;
        pipeline_cur_instr_.push_back(EMPTY_INSTRUCTION);
        cpubus_.Reset();
    }

    bool CPU::execute_stage(PipelineStage stage) {
        switch(stage) {
            case PipelineStage::IC: {
                IC();
                return true;
            }
            case PipelineStage::RF: {
                RF();
                return true;
            }
            case PipelineStage::EX: {
                EX();
                return true;
            }
            case PipelineStage::DC: {
                DC();
                return true;
            }
            case PipelineStage::WB: {
                WB();
                return true;
            }
            default: {
                return false;
            }
        }
    }

    CPU::PipelineStageRet CPU::IC(PipelineStageArgs) {
        // Fetch the current process instruction
        auto paddr_s = translate_vaddr(pc_);
        icrf_latch_.instruction.Full = cpubus_.fetch_instruction_uncached(paddr_s.paddr);
        // IC instructions are always at the back
        pipeline_cur_instr_.back() = icrf_latch_.instruction.Full;
        pc_ += 4;
    }

    CPU::PipelineStageRet CPU::RF(PipelineStageArgs) {
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

    CPU::PipelineStageRet CPU::EX(PipelineStageArgs) {
        execute_instruction();
    }

    CPU::PipelineStageRet CPU::DC(PipelineStageArgs) {
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
                // Result is cast to uint64_t in order to zero extend
                dcwb_latch_.access_type = AccessType::UDOUBLEWORD;
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

    CPU::PipelineStageRet CPU::WB(PipelineStageArgs) {
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
            execute_stage(ps);
            ps = static_cast<PipelineStage>(static_cast<int>(ps) + 1);
        }
        if (pipeline_.front() == PipelineStage::NOP) {
            pipeline_.pop_front();
            pipeline_cur_instr_.pop_front();
        }
        pipeline_.push_back(PipelineStage::IC);
        ++instructions_ran_;
        pipeline_cur_instr_.push_back(EMPTY_INSTRUCTION);
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

    void CPU::store_register(AccessType access_type, uint32_t dest, uint64_t* dest_direct, std::any data_any) {
        switch(access_type) {
            case AccessType::UBYTE: {
                assert((typeid(uint8_t) == data_any.type()) && "Assertion failed - bad any cast: expected uint8_t");
                gpr_regs_[dest].UB._0
                    = std::any_cast<uint8_t>(data_any);
                break;
            }
            case AccessType::UHALFWORD: {
                assert((typeid(uint16_t) == data_any.type()) && "Assertion failed - bad any cast: expected uint16_t");
                uint64_t temp = std::any_cast<uint16_t>(data_any);
                gpr_regs_[dest].UD = temp;
                break;
            }
            case AccessType::HALFWORD: {
                assert((typeid(int16_t) == data_any.type()) && "Assertion failed - bad any cast: expected int16_t");
                int64_t temp = static_cast<int16_t>(std::any_cast<uint16_t>(data_any));
                gpr_regs_[dest].UD = temp;
                break;
            }
            case AccessType::UWORD: {
                assert((typeid(uint32_t) == data_any.type()) && "Assertion failed - bad any cast: expected uint32_t");
                gpr_regs_[dest].UW._0
                    = std::any_cast<uint32_t>(data_any);
                break;
            }
            case AccessType::WORD: {
                assert((typeid(int32_t) == data_any.type()) && "Assertion failed - bad any cast: expected int32_t");
                gpr_regs_[dest].W._0
                    = std::any_cast<int32_t>(data_any);
                break;
            }
            case AccessType::UDOUBLEWORD: {
                assert((typeid(uint64_t) == data_any.type()) && "Assertion failed - bad any cast: expected uint64_t");
                gpr_regs_[dest].UD
                    = std::any_cast<uint64_t>(data_any);
                break;
            }
            case AccessType::UDOUBLEWORD_DIRECT: {
                assert((typeid(uint64_t) == data_any.type()) && "Assertion failed - bad any cast: expected uint64_t direct");
                if (dest_direct) {
                    *dest_direct = std::any_cast<uint64_t>(data_any);
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
                case AccessType::UBYTE: {
                    assert((typeid(uint8_t) == data_any.type()) && "Assertion failed - bad any cast: expected uint8_t");
                    auto data = std::any_cast<uint8_t>(data_any);
                    *(loc) = data & 0xFF;
                    break;
                }
                case AccessType::UHALFWORD: {
                    assert((typeid(uint16_t) == data_any.type()) && "Assertion failed - bad any cast: expected uint16_t");
                    auto data = std::any_cast<uint16_t>(data_any);
                    *(loc++) = (data >> 8) & 0xFF;
                    *(loc)   = data & 0xFF;
                    break;
                }
                case AccessType::UWORD: {
                    assert((typeid(uint32_t) == data_any.type()) && "Assertion failed - bad any cast: expected uint32_t");
                    auto data = std::any_cast<uint32_t>(data_any);
                    *(loc++) = (data >> 24) & 0xFF;
                    *(loc++) = (data >> 16) & 0xFF;
                    *(loc++) = (data >> 8)  & 0xFF;
                    *(loc)   = data & 0xFF;
                    break;
                }
                case AccessType::UDOUBLEWORD: {
                    assert((typeid(uint64_t) == data_any.type()) && "Assertion failed - bad any cast: expected uint64_t");
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
        assert((!data_any.has_value()) && "Assertion failed - data_any should be empty when it reaches load_memory");
        if (!cached) {
            uint8_t* loc = cpubus_.redirect_paddress(paddr);
            switch(type) {
                case AccessType::UDOUBLEWORD: {
                    uint64_t data = 0;
                    std::memcpy(&data, loc, 8);
                    data_any = static_cast<uint64_t>(__builtin_bswap64(data));
                    break;
                }
                case AccessType::UWORD: {
                    uint32_t data = 0;
                    std::memcpy(&data, loc, 4);
                    data_any = static_cast<uint64_t>(__builtin_bswap32(data));
                    break;
                }
                case AccessType::UHALFWORD: {
                    uint16_t data = 0;
                    int32_t sedata = static_cast<int16_t>(data);
                    std::memcpy(&data, loc, 2);
                    data_any = static_cast<uint64_t>(__builtin_bswap16(data));
                    break;
                }
                case AccessType::HALFWORD: {
                    uint16_t data = 0;
                    std::memcpy(&data, loc, 2);
                    data_any = static_cast<uint64_t>(__builtin_bswap16(data));
                    break;
                }
                case AccessType::UBYTE: {
                    uint8_t data = 0;
                    data = *(loc);
                    data_any = static_cast<uint64_t>(data);
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
    void CPU::clear_registers() {
        for (auto& reg : gpr_regs_) {
            reg.UD = 0;
        }
        for (auto& reg : fpr_regs_) {
            reg = 0.0;
        }
    }
}