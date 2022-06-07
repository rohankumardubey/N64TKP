#include "n64_cpu.hxx"
#include <cstring> // memcpy
#include <cassert> // assert
#include <iostream>
#include "../include/error_factory.hxx"
#include "n64_addresses.hxx"
// #include <valgrind/callgrind.h>

namespace TKPEmu::N64::Devices {
    CPU::CPU(CPUBus& cpubus, RCP& rcp, GLuint& text_format)  :
        gpr_regs_{},
        fpr_regs_{},
        instr_cache_(KB(16)),
        data_cache_(KB(8)),
        cpubus_(cpubus),
        rcp_(rcp),
        text_format_(text_format)
    {
        Reset();
    }

    void CPU::Reset() {
        pc_ = 0x80001000;
        clear_registers();
        pipeline_ = 0b00001;
        pipeline_cur_instr_.clear();
        instructions_ran_ = 1;
        pipeline_cur_instr_.push_back(EMPTY_INSTRUCTION);
        cpubus_.Reset();
    }

    bool CPU::execute_stage(PipelineStage stage) {
        gpr_regs_[0].UD = 0;
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
                #if SKIPDEBUGSTUFF == 0
                pipeline_cur_instr_.pop_front();
                #endif
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
        #if SKIPDEBUGSTUFF == 0
        pipeline_cur_instr_.back() = icrf_latch_.instruction.Full;
        #endif
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
                case static_cast<int>(InstructionType::SPECIAL): {
                    auto func = icrf_latch_.instruction.RType.func;
                    type = SpecialInstructionTypeTable[func];
                    break;
                }
                case static_cast<int>(InstructionType::COP0): {
                    
                    break;
                }
                default: {
                    type = InstructionTypeTable[opcode];
                    break;
                }
            }
        }
        rfex_latch_.fetched_rs_i = icrf_latch_.instruction.RType.rs;
        rfex_latch_.fetched_rt_i = icrf_latch_.instruction.RType.rt;
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
        dcwb_latch_.data = 0;
        dcwb_latch_.access_type = exdc_latch_.access_type;
        dcwb_latch_.dest = exdc_latch_.dest;
        dcwb_latch_.write_type = exdc_latch_.write_type;
        dcwb_latch_.cached = exdc_latch_.cached;
        dcwb_latch_.paddr = exdc_latch_.paddr;
        switch (exdc_latch_.write_type) {
            case WriteType::LATEREGISTER: {
                auto paddr_s = translate_vaddr(exdc_latch_.vaddr);
                uint32_t paddr = paddr_s.paddr;
                dcwb_latch_.cached = paddr_s.cached;
                load_memory(dcwb_latch_.cached, paddr, dcwb_latch_.data, dcwb_latch_.access_type);
                // Result is cast to uint64_t in order to zero extend
                dcwb_latch_.access_type = AccessType::UDOUBLEWORD;
                if (rfex_latch_.fetched_rt_i == exdc_latch_.fetched_rt_i) {
                    // TODO: LoadInterlock hack This may be wrong
                    rfex_latch_.fetched_rt.UD = dcwb_latch_.data;
                }
                if (rfex_latch_.fetched_rs_i == exdc_latch_.fetched_rt_i) {
                    // TODO: LoadInterlock hack This may be wrong
                    rfex_latch_.fetched_rs.UD = dcwb_latch_.data;
                }
                break;
            }
            default: {
                dcwb_latch_.data = exdc_latch_.data;
                break;
            }
        }
    }

    CPU::PipelineStageRet CPU::WB(PipelineStageArgs) {
        switch(dcwb_latch_.write_type) {
            case WriteType::MMU: {
                store_memory(dcwb_latch_.cached, dcwb_latch_.paddr, dcwb_latch_.data, dcwb_latch_.access_type);
                break;
            }
            case WriteType::LATEREGISTER: {
                store_register(dcwb_latch_.dest, dcwb_latch_.data, dcwb_latch_.access_type);
                // Bypass the rfex latch and update regs
                // TODO: Wrong? Don't know
                rfex_latch_.fetched_rs = gpr_regs_[rfex_latch_.fetched_rs_i];
                rfex_latch_.fetched_rt = gpr_regs_[rfex_latch_.fetched_rt_i];
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
            //cached = true;
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
        TranslatedAddress t_addr = { paddr, cached };
        return std::move(t_addr);
    }

    void CPU::store_register(uint8_t* dest, uint64_t data, int size) {
        std::memcpy(dest, &data, size);
    }
    void CPU::invalidate_hwio(uint32_t addr, uint64_t data) {
        if (data != 0)
        switch (addr) {
            case PI_WR_LEN_REG: {
                std::memcpy(&cpubus_.rdram_[__builtin_bswap32(cpubus_.pi_dram_addr_)], cpubus_.redirect_paddress(__builtin_bswap32(cpubus_.pi_cart_addr_)), data);
                break;
            }
            case VI_CTRL_REG: {
                auto format = data & 0b11;
                if (format == 0b10)
                    text_format_ = GL_RGB5;
                else if (format == 0b11)
                    text_format_ = GL_RGBA;
                break;
            }
            case VI_ORIGIN_REG: {
                rcp_.framebuffer_ptr_ = cpubus_.redirect_paddress(data & 0xFFFFFF);
                break;
            }
        }
    }
    void CPU::store_memory(bool cached, uint32_t paddr, uint64_t& data, int size) {
        invalidate_hwio(paddr, data);
        if (!cached) {
            uint8_t* loc = cpubus_.redirect_paddress(paddr);
            uint64_t temp = __builtin_bswap64(data);
            temp >>= 8 * (AccessType::UDOUBLEWORD - size);
            std::memcpy(loc, &temp, size);
        } else {
            // currently not implemented
        }
    }
    void CPU::load_memory(bool cached, uint32_t paddr, uint64_t& data, int size) {
        if (!cached) {
            uint8_t* loc = cpubus_.redirect_paddress(paddr);
            uint64_t temp = 0;
            std::memcpy(&temp, loc, size);
            temp = __builtin_bswap64(temp);
            temp >>= 8 * (AccessType::UDOUBLEWORD - size);
            data = temp;
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

    void CPU::update_pipeline() {
        // CALLGRIND_START_INSTRUMENTATION;
        #pragma GCC unroll 5
        for (int i = 4; i >= 0; --i) {
            execute_stage(static_cast<PipelineStage>(pipeline_ & (1 << i)));
            ++cp0_regs_[CP0_COUNT].UW._0;
            if (cp0_regs_[CP0_COUNT].UW._0 == cp0_regs_[CP0_COMPARE].UW._0) [[unlikely]] {
                // interrupt
            }
        }
        pipeline_ <<= 1;
        pipeline_ |= 1;
        #if SKIPDEBUGSTUFF == 0
        ++instructions_ran_;
        pipeline_cur_instr_.push_back(EMPTY_INSTRUCTION);
        #endif
        // CALLGRIND_STOP_INSTRUMENTATION;
    }

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
            // case InstructionType::DADDI:
            // case InstructionType::DADDIU:
            case InstructionType::ADDI: 
            case InstructionType::ADDIU: {
                int32_t seimm = static_cast<int16_t>(cur_instr.IType.immediate);
                int32_t result = 0;
                bool overflow = __builtin_add_overflow(rfex_latch_.fetched_rs.W._0, seimm, &result);
                exdc_latch_.dest = &gpr_regs_[cur_instr.IType.rt].UB._0;
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
                exdc_latch_.dest = reinterpret_cast<uint8_t*>(&pc_);
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
                exdc_latch_.dest = &gpr_regs_[cur_instr.IType.rt].UB._0;
                exdc_latch_.data = seimm;
                exdc_latch_.write_type = WriteType::REGISTER;
                exdc_latch_.access_type = AccessType::UDOUBLEWORD;
                break;
            }
            /**
             * COP0
             */
            case InstructionType::COP0: {
                if (cur_instr.Full & CPzOPERATION_BIT) {
                    execute_cp0_instruction(cur_instr);
                } else {
                    throw ErrorFactory::generate_exception(__func__, __LINE__, "CP0 CFC/CTC operations are illegal");
                }
                break;
            }
            /**
             * ORI
             * 
             * doesn't throw
             */
            case InstructionType::ORI: {
                exdc_latch_.dest = &gpr_regs_[cur_instr.IType.rt].UB._0;
                exdc_latch_.data = rfex_latch_.fetched_rs.UD | cur_instr.IType.immediate;
                exdc_latch_.write_type = WriteType::REGISTER;
                exdc_latch_.access_type = AccessType::UDOUBLEWORD;
                break;
            }
            /**
             * s_AND
             * 
             * doesn't throw
             */
            case InstructionType::s_AND: {
                exdc_latch_.dest = &gpr_regs_[cur_instr.RType.rd].UB._0;
                exdc_latch_.data = rfex_latch_.fetched_rs.UD & rfex_latch_.fetched_rt.UD;
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
                exdc_latch_.paddr = paddr_s.paddr;
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
                exdc_latch_.paddr = paddr_s.paddr;
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
             * SH
             * 
             * throws TLB miss exception
             *        TLB invalid exception
             *        TLB modification exception
             *        Bus error exception
             *        Address error exception
             */
            case InstructionType::SH: {
                int16_t offset = cur_instr.IType.immediate;
                int32_t seoffset = offset;
                auto write_vaddr = seoffset + rfex_latch_.fetched_rs.UW._0;
                auto paddr_s = translate_vaddr(write_vaddr);
                exdc_latch_.paddr = paddr_s.paddr;
                exdc_latch_.cached = paddr_s.cached;
                exdc_latch_.data = rfex_latch_.fetched_rt.UH._0;
                exdc_latch_.write_type = WriteType::MMU;
                exdc_latch_.access_type = AccessType::UHALFWORD;
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
             * LB, LBU
             * 
             * throws TLB miss exception
             *        TLB invalid exception
             *        Bus error exception
             *        Address error exception (?)
             */
            case InstructionType::LB:
            case InstructionType::LBU: {
                int16_t offset = cur_instr.IType.immediate;
                int32_t seoffset = offset;
                exdc_latch_.dest = &gpr_regs_[cur_instr.IType.rt].UB._0;
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
                exdc_latch_.dest = &gpr_regs_[cur_instr.IType.rt].UB._0;
                exdc_latch_.vaddr = seoffset + rfex_latch_.fetched_rs.UW._0;
                exdc_latch_.write_type = WriteType::LATEREGISTER;
                exdc_latch_.access_type = AccessType::UDOUBLEWORD;
                exdc_latch_.fetched_rt_i = cur_instr.IType.rt;
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
                exdc_latch_.dest = &gpr_regs_[cur_instr.IType.rt].UB._0;
                exdc_latch_.vaddr = seoffset + rfex_latch_.fetched_rs.UW._0;
                exdc_latch_.write_type = WriteType::LATEREGISTER;
                exdc_latch_.access_type = AccessType::UHALFWORD;
                exdc_latch_.fetched_rt_i = cur_instr.IType.rt;
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
             * LH
             * 
             * throws TLB miss exception
             *        TLB invalid exception
             *        Bus error exception
             *        Address error exception
             */
            case InstructionType::LH: {
                int16_t offset = cur_instr.IType.immediate;
                int32_t seoffset = offset;
                exdc_latch_.dest = &gpr_regs_[cur_instr.IType.rt].UB._0;
                exdc_latch_.vaddr = seoffset + rfex_latch_.fetched_rs.UW._0;
                exdc_latch_.write_type = WriteType::LATEREGISTER;
                exdc_latch_.access_type = AccessType::UHALFWORD;
                exdc_latch_.fetched_rt_i = cur_instr.IType.rt;
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
             * LW, LWU
             * 
             * throws TLB miss exception
             *        TLB invalid exception
             *        Bus error exception
             *        Address error exception
             */
            case InstructionType::LWU:
            case InstructionType::LW: {
                int16_t offset = cur_instr.IType.immediate;
                int32_t seoffset = offset;
                exdc_latch_.dest = &gpr_regs_[cur_instr.IType.rt].UB._0;
                exdc_latch_.vaddr = seoffset + rfex_latch_.fetched_rs.UW._0;
                exdc_latch_.write_type = WriteType::LATEREGISTER;
                exdc_latch_.access_type = AccessType::UWORD;
                exdc_latch_.fetched_rt_i = cur_instr.IType.rt;
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
                exdc_latch_.dest = &gpr_regs_[cur_instr.IType.rt].UB._0;
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
                    exdc_latch_.dest = reinterpret_cast<uint8_t*>(&pc_);
                    exdc_latch_.write_type = WriteType::REGISTER;
                    exdc_latch_.access_type = AccessType::UDOUBLEWORD_DIRECT;
                }
                break;
            }
            /**
             * BEQL
             * 
             * doesn't throw
             */
            case InstructionType::BEQL: {
                 int16_t offset = cur_instr.IType.immediate << 2;
                int32_t seoffset = offset;
                if (rfex_latch_.fetched_rs.UD == rfex_latch_.fetched_rt.UD) {
                    exdc_latch_.data = pc_ - 4 + seoffset;
                    exdc_latch_.dest = reinterpret_cast<uint8_t*>(&pc_);
                    exdc_latch_.write_type = WriteType::REGISTER;
                    exdc_latch_.access_type = AccessType::UDOUBLEWORD_DIRECT;
                } else {
                    // Discard delay slot instruction
                    icrf_latch_.instruction.Full = 0;
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
                    exdc_latch_.dest = reinterpret_cast<uint8_t*>(&pc_);
                    exdc_latch_.write_type = WriteType::REGISTER;
                    exdc_latch_.access_type = AccessType::UDOUBLEWORD_DIRECT;
                }
                break;
            }
            /**
             * BNEL
             * 
             * doesn't throw
             */
            case InstructionType::BNEL: {
                int16_t offset = cur_instr.IType.immediate << 2;
                int32_t seoffset = offset;
                if (rfex_latch_.fetched_rs.UD != rfex_latch_.fetched_rt.UD) {
                    exdc_latch_.data = pc_ - 4 + seoffset;
                    exdc_latch_.dest = reinterpret_cast<uint8_t*>(&pc_);
                    exdc_latch_.write_type = WriteType::REGISTER;
                    exdc_latch_.access_type = AccessType::UDOUBLEWORD_DIRECT;
                } else {
                    // Discard delay slot instruction
                    icrf_latch_.instruction.Full = 0;
                }
                break;
            }
            /**
             * BLEZL
             * 
             * doesn't throw
             */
            case InstructionType::BLEZL: {
                int16_t offset = cur_instr.IType.immediate << 2;
                int32_t seoffset = offset;
                if (rfex_latch_.fetched_rs.D <= 0) {
                    exdc_latch_.data = pc_ - 4 + seoffset;
                    exdc_latch_.dest = reinterpret_cast<uint8_t*>(&pc_);
                    exdc_latch_.write_type = WriteType::REGISTER;
                    exdc_latch_.access_type = AccessType::UDOUBLEWORD_DIRECT;
                } else {
                    // Discard delay slot instruction
                    icrf_latch_.instruction.Full = 0;
                }
                break;
            }
            /**
             * BGTZ
             * 
             * doesn't throw
             */
            case InstructionType::BGTZ: {
                int16_t offset = cur_instr.IType.immediate << 2;
                int32_t seoffset = offset;
                if (rfex_latch_.fetched_rs.D > 0) {
                    exdc_latch_.data = pc_ - 4 + seoffset;
                    exdc_latch_.dest = reinterpret_cast<uint8_t*>(&pc_);
                    exdc_latch_.write_type = WriteType::REGISTER;
                    exdc_latch_.access_type = AccessType::UDOUBLEWORD_DIRECT;
                }
                break;
            }
            /**
             * TGE
             * 
             * throws TrapException
             */
            case InstructionType::s_TGE: {
                if (rfex_latch_.fetched_rs.UD >= rfex_latch_.fetched_rt.UD)
                    throw ErrorFactory::generate_exception(__func__, __LINE__, "TrapException was thrown");
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
                exdc_latch_.dest = &gpr_regs_[cur_instr.RType.rd].UB._0;
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
                exdc_latch_.dest = reinterpret_cast<uint8_t*>(&pc_);
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
             * s_DSLL32
             * 
             * throws Reserved instruction exception
             */
            case InstructionType::s_DSLL32: {
                exdc_latch_.dest = &gpr_regs_[cur_instr.RType.rd].UB._0;
                exdc_latch_.data = rfex_latch_.fetched_rt.UD << (cur_instr.RType.sa + 32);
                exdc_latch_.write_type = WriteType::REGISTER;
                exdc_latch_.access_type = AccessType::UDOUBLEWORD;
                break;
            }
            /**
             * s_DSLLV
             * 
             * throws Reserved instruction exception
             */
            case InstructionType::s_DSLLV: {
                exdc_latch_.dest = &gpr_regs_[cur_instr.RType.rd].UB._0;
                exdc_latch_.data = rfex_latch_.fetched_rt.UD << (rfex_latch_.fetched_rs.UD & 0b111111);
                exdc_latch_.write_type = WriteType::REGISTER;
                exdc_latch_.access_type = AccessType::UDOUBLEWORD;
                break;
            }
            /**
             * s_SLL, s_DSLL
             * 
             * throws Reserved instruction exception
             */
            // case InstructionType::s_DSLL:
            case InstructionType::s_SLL: {
                exdc_latch_.dest = &gpr_regs_[cur_instr.RType.rd].UB._0;
                uint32_t zedata = rfex_latch_.fetched_rt.UW._0 << cur_instr.RType.sa;
                exdc_latch_.data = zedata;
                exdc_latch_.write_type = WriteType::REGISTER;
                exdc_latch_.access_type = AccessType::UDOUBLEWORD;
                break;
            }
            /**
             * s_SRL
             * 
             * doesn't throw
             */
            case InstructionType::s_SRL: {
                exdc_latch_.dest = &gpr_regs_[cur_instr.RType.rd].UB._0;
                exdc_latch_.data = rfex_latch_.fetched_rt.UD >> cur_instr.RType.sa;
                exdc_latch_.write_type = WriteType::REGISTER;
                exdc_latch_.access_type = AccessType::UDOUBLEWORD;
                break;
            }
            /**
             * s_DSRA32
             * 
             * throws Reserved instruction exception
             */
            case InstructionType::s_DSRA32: {
                exdc_latch_.dest = &gpr_regs_[cur_instr.RType.rd].UB._0;
                int64_t sedata = rfex_latch_.fetched_rt.D >> (cur_instr.RType.sa + 32);
                exdc_latch_.data = sedata;
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
                exdc_latch_.dest = &gpr_regs_[cur_instr.RType.rd].UB._0;
                exdc_latch_.write_type = WriteType::REGISTER;
                exdc_latch_.access_type = AccessType::UDOUBLEWORD;
                break;
            }
            /**
             * s_SLTU
             * 
             * doesn't throw 
             */
            case InstructionType::s_SLTU: {
                if (rfex_latch_.fetched_rs.UW._0 < rfex_latch_.fetched_rt.UW._0) {
                    exdc_latch_.data = static_cast<uint64_t>(1);
                } else {
                    exdc_latch_.data = static_cast<uint64_t>(0);
                }
                exdc_latch_.dest = &gpr_regs_[cur_instr.RType.rd].UB._0;
                exdc_latch_.write_type = WriteType::REGISTER;
                exdc_latch_.access_type = AccessType::UDOUBLEWORD;
                break;
            }
            /**
             * s_NOR
             * 
             * doesn't throw
             */
            case InstructionType::s_NOR: {
                exdc_latch_.dest = &gpr_regs_[cur_instr.RType.rd].UB._0;
                exdc_latch_.data = ~(rfex_latch_.fetched_rs.UD | rfex_latch_.fetched_rt.UD);
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
                throw ErrorFactory::generate_exception(__func__, __LINE__, std::string("Instruction not yet implemented: ") + instr_name);
            }
        }
        if (exdc_latch_.write_type == WriteType::REGISTER) {
            // Bypassing (from manual)
            // result is stored during EX instead of waiting for WB when it comes
            // to writing to a register
            // for instructions like ADD that don't need DC stage to write back
            store_register(exdc_latch_.dest, exdc_latch_.data, exdc_latch_.access_type);
            exdc_latch_.write_type = WriteType::NONE;
        }
    }
    void CPU::execute_cp0_instruction(const Instruction& instr) {
        uint32_t func = instr.RType.rs;
        switch (func) {
            /**
             * MTC0
             * 
             * throws Coprocessor unusable exception
             */
            case 0b00100: {
                int64_t sedata = gpr_regs_[instr.RType.rd].W._0;
                exdc_latch_.dest = &cp0_regs_[instr.RType.rt].UB._0;
                exdc_latch_.data = sedata;
                exdc_latch_.write_type = WriteType::LATEREGISTER;
                exdc_latch_.access_type = AccessType::UDOUBLEWORD;
                break;
            }
            /**
             * MFC0
             * 
             * throws Coprocessor unusable exception
             */
            case 0b00000: {
                int64_t sedata = cp0_regs_[instr.RType.rd].W._0;
                exdc_latch_.dest = &gpr_regs_[instr.RType.rt].UB._0;
                exdc_latch_.data = sedata;
                exdc_latch_.write_type = WriteType::LATEREGISTER;
                exdc_latch_.access_type = AccessType::UDOUBLEWORD;
                break;
            }
        }
    }
}