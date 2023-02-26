#include "n64_cpu.hxx"
#include <cstring>
#include <cassert>
#include <iostream>
#include <bitset>
#include <limits>
#include <sstream>
#include "n64_addresses.hxx"
#include "error_factory.hxx"
#include "utils.hxx"

#define SKIPDEBUGSTUFF 1
#define TKP_INSTR_FUNC void
#define IS_PC_EX(target_pc) ((pc_ - 8) == (target_pc))
constexpr uint64_t LUT[] = {
    0, 0xFF, 0xFFFF, 0, 0xFFFFFFFF, 0, 0, 0, 0xFFFFFFFFFFFFFFFF,
};

namespace TKPEmu::N64::Devices {
    CPU::CPU(CPUBus& cpubus, RCP& rcp) :
        gpr_regs_{},
        fpr_regs_{},
        instr_cache_(KB(16)),
        data_cache_(KB(8)),
        cpubus_(cpubus),
        rcp_(rcp)
    {
    }

    void CPU::Reset() {
        pc_ = 0xBFC0'0000;
        ldi_ = false;
        clear_registers();
        cpubus_.Reset();
        if (cpubus_.IsEverythingLoaded()) {
            // memcpy(cpubus_.redirect_paddress(0x1000), cpubus_.redirect_paddress(0x10001000), 0x100000);
            fill_pipeline();
        }
    }

    TKP_INSTR_FUNC CPU::ERROR() {
        throw ErrorFactory::generate_exception("ERROR opcode reached");
    }

    /**
     * s_SRA
     * 
     * doesn't throw
     */
    TKP_INSTR_FUNC CPU::s_SRA() {
        int32_t sedata = rfex_latch_.fetched_rt.D >> rfex_latch_.instruction.RType.sa;
		exdc_latch_.dest = &gpr_regs_[rfex_latch_.instruction.RType.rd].UB._0;
        exdc_latch_.data = sedata;
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		bypass_register();
	}

    /**
     * s_SLLV
     * 
     * doesn't throw
     */
    TKP_INSTR_FUNC CPU::s_SLLV() {
		exdc_latch_.dest = &gpr_regs_[rfex_latch_.instruction.RType.rd].UB._0;
        int64_t sedata = static_cast<int32_t>(rfex_latch_.fetched_rt.UW._0 << (rfex_latch_.fetched_rs.UD & 0b111111));
        exdc_latch_.data = sedata;
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		bypass_register();
	}

    /**
     * s_SRLV
     * 
     * doesn't throw
     */
    TKP_INSTR_FUNC CPU::s_SRLV() {
        exdc_latch_.dest = &gpr_regs_[rfex_latch_.instruction.RType.rd].UB._0;
        int64_t sedata = static_cast<int64_t>(static_cast<int32_t>(rfex_latch_.fetched_rt.UW._0 >> (rfex_latch_.fetched_rs.UD & 0b111111)));
        exdc_latch_.data = sedata;
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		bypass_register();
	}
    
    TKP_INSTR_FUNC CPU::s_SRAV() {
		int32_t sedata = rfex_latch_.fetched_rt.D >> (rfex_latch_.fetched_rs.UD & 0b11111);
		exdc_latch_.dest = &gpr_regs_[rfex_latch_.instruction.RType.rd].UB._0;
        exdc_latch_.data = sedata;
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		bypass_register();
	}
    
    TKP_INSTR_FUNC CPU::s_SYSCALL() {
		throw ErrorFactory::generate_exception("s_SYSCALL opcode reached");
	}
    
    TKP_INSTR_FUNC CPU::s_BREAK() {
		throw ErrorFactory::generate_exception("s_BREAK opcode reached");
	}

    TKP_INSTR_FUNC CPU::s_SYNC() {
		// Intentionally NOP
	}
    
    TKP_INSTR_FUNC CPU::s_MFHI() {
		exdc_latch_.dest = &gpr_regs_[rfex_latch_.instruction.RType.rd].UB._0;
        exdc_latch_.data = hi_;
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		bypass_register();
	}
    
    TKP_INSTR_FUNC CPU::s_MTHI() {
        hi_ = rfex_latch_.fetched_rs.UD;
	}

    TKP_INSTR_FUNC CPU::s_MFLO() {
		exdc_latch_.dest = &gpr_regs_[rfex_latch_.instruction.RType.rd].UB._0;
        exdc_latch_.data = lo_;
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		bypass_register();
	}
    
    TKP_INSTR_FUNC CPU::s_DSRLV() {
		throw ErrorFactory::generate_exception("s_DSRLV opcode reached");
	}
    
    TKP_INSTR_FUNC CPU::s_DSRAV() {
		throw ErrorFactory::generate_exception("s_DSRAV opcode reached");
	}
    
    TKP_INSTR_FUNC CPU::s_MULT() {
		uint64_t res = static_cast<int64_t>(rfex_latch_.fetched_rs.W._0) * rfex_latch_.fetched_rt.W._0;
        lo_ = static_cast<int64_t>(static_cast<int32_t>(res & 0xFFFF'FFFF));
        hi_ = static_cast<int64_t>(static_cast<int32_t>(res >> 32));
	}
    
    TKP_INSTR_FUNC CPU::s_MULTU() {
		uint64_t res = static_cast<uint64_t>(rfex_latch_.fetched_rs.UW._0) * rfex_latch_.fetched_rt.UW._0;
        lo_ = static_cast<int64_t>(static_cast<int32_t>(res & 0xFFFF'FFFF));
        hi_ = static_cast<int64_t>(static_cast<int32_t>(res >> 32));
	}
    
    TKP_INSTR_FUNC CPU::s_DIV() {
		if (rfex_latch_.fetched_rt.W._0 == 0) [[unlikely]]
        {
            lo_ = -1 + 2 * (rfex_latch_.fetched_rs.UW._0 >> 31);
            hi_ = rfex_latch_.fetched_rs.UD;
            return;
        }
        if (rfex_latch_.fetched_rs.W._0 == std::numeric_limits<decltype(rfex_latch_.fetched_rs.W._0)>::min() && rfex_latch_.fetched_rt.W._0 == -1) {
            // -2147483648 / 1 edge case
            lo_ = static_cast<int64_t>(rfex_latch_.fetched_rs.W._0);
            hi_ = 0;
            return;
        }
        lo_ = static_cast<int64_t>(static_cast<int32_t>(rfex_latch_.fetched_rs.W._0 / rfex_latch_.fetched_rt.W._0));
        hi_ = static_cast<int64_t>(static_cast<int32_t>(rfex_latch_.fetched_rs.W._0 % rfex_latch_.fetched_rt.W._0));
	}
    
    TKP_INSTR_FUNC CPU::s_DIVU() {
		if (rfex_latch_.fetched_rt.UW._0 == 0) [[unlikely]]
        {
            lo_ = -1;
            hi_ = rfex_latch_.fetched_rs.UD;
            return;
        }
        lo_ = static_cast<int64_t>(static_cast<int32_t>(rfex_latch_.fetched_rs.UW._0 / rfex_latch_.fetched_rt.UW._0));
        hi_ = static_cast<int64_t>(static_cast<int32_t>(rfex_latch_.fetched_rs.UW._0 % rfex_latch_.fetched_rt.UW._0));
	}
    
    TKP_INSTR_FUNC CPU::s_DMULT() {
		throw ErrorFactory::generate_exception("s_DMULT opcode reached");
	}
    
    TKP_INSTR_FUNC CPU::s_DMULTU() {
		throw ErrorFactory::generate_exception("s_DMULTU opcode reached");
	}
    
    TKP_INSTR_FUNC CPU::s_DDIV() {
		throw ErrorFactory::generate_exception("s_DDIV opcode reached");
	}
    
    TKP_INSTR_FUNC CPU::s_DDIVU() {
		throw ErrorFactory::generate_exception("s_DDIVU opcode reached");
	}
    
    TKP_INSTR_FUNC CPU::s_SUB() {
		int32_t result = 0;
		bool overflow = __builtin_sub_overflow(rfex_latch_.fetched_rs.W._0, rfex_latch_.fetched_rt.W._0, &result);
		exdc_latch_.dest = &gpr_regs_[rfex_latch_.instruction.RType.rd].UB._0;
        exdc_latch_.data = static_cast<int64_t>(result);
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		bypass_register();
        #if SKIPEXCEPTIONS == 0
        if (overflow) {
            // An integer overflow exception occurs if carries out of bits 30 and 31 differ (2’s
            // complement overflow). The contents of destination register rt is not modified
            // when an integer overflow exception occurs.
            throw IntegerOverflowException();
        }
        #endif
	}
    
    TKP_INSTR_FUNC CPU::s_SUBU() {
        uint32_t result = 0;
		bool overflow = __builtin_sub_overflow(rfex_latch_.fetched_rs.UW._0, rfex_latch_.fetched_rt.UW._0, &result);
		exdc_latch_.dest = &gpr_regs_[rfex_latch_.instruction.RType.rd].UB._0;
        exdc_latch_.data = static_cast<int64_t>(static_cast<int32_t>(result));
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		bypass_register();
	}
    
    /**
     * s_OR
     * 
     * doesn't throw
     */
    TKP_INSTR_FUNC CPU::s_OR() {
		exdc_latch_.dest = &gpr_regs_[rfex_latch_.instruction.RType.rd].UB._0;
        exdc_latch_.data = rfex_latch_.fetched_rs.UD | rfex_latch_.fetched_rt.UD;
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		bypass_register();
	}
    
    TKP_INSTR_FUNC CPU::s_XOR() {
		exdc_latch_.dest = &gpr_regs_[rfex_latch_.instruction.RType.rd].UB._0;
        exdc_latch_.data = rfex_latch_.fetched_rs.UD ^ rfex_latch_.fetched_rt.UD;
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		bypass_register();
	}
    
    TKP_INSTR_FUNC CPU::s_DADD() {
		throw ErrorFactory::generate_exception("s_DADD opcode reached");
	}
    
    TKP_INSTR_FUNC CPU::s_DADDU() {
		throw ErrorFactory::generate_exception("s_DADDU opcode reached");
	}
    
    TKP_INSTR_FUNC CPU::s_DSUB() {
		throw ErrorFactory::generate_exception("s_DSUB opcode reached");
	}
    
    TKP_INSTR_FUNC CPU::s_DSUBU() {
		throw ErrorFactory::generate_exception("s_DSUBU opcode reached");
	}
    
    TKP_INSTR_FUNC CPU::s_TGEU() {
		throw ErrorFactory::generate_exception("s_TGEU opcode reached");
	}
    
    TKP_INSTR_FUNC CPU::s_TLT() {
		throw ErrorFactory::generate_exception("s_TLT opcode reached");
	}
    
    TKP_INSTR_FUNC CPU::s_TLTU() {
		throw ErrorFactory::generate_exception("s_TLTU opcode reached");
	}
    
    TKP_INSTR_FUNC CPU::s_TEQ() {
		throw ErrorFactory::generate_exception("s_TEQ opcode reached");
	}
    
    TKP_INSTR_FUNC CPU::s_TNE() {
		throw ErrorFactory::generate_exception("s_TNE opcode reached");
	}
    
    /**
     * s_DSLL
     * 
     * throws ReservedInstructionException
     */
    TKP_INSTR_FUNC CPU::s_DSLL() {
		exdc_latch_.dest = &gpr_regs_[rfex_latch_.instruction.RType.rd].UB._0;
        exdc_latch_.data = rfex_latch_.fetched_rt.UD << rfex_latch_.instruction.RType.sa;
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		bypass_register();
	}
    
    TKP_INSTR_FUNC CPU::s_DSRL() {
		throw ErrorFactory::generate_exception("s_DSRL opcode reached");
	}
    
    TKP_INSTR_FUNC CPU::s_DSRA() {
		throw ErrorFactory::generate_exception("s_DSRA opcode reached");
	}
    
    TKP_INSTR_FUNC CPU::s_DSRL32() {
		throw ErrorFactory::generate_exception("s_DSRL32 opcode reached");
	}
    
    TKP_INSTR_FUNC CPU::SPECIAL() {
        (SpecialTable[rfex_latch_.instruction.RType.func])(this);
	}

    TKP_INSTR_FUNC CPU::REGIMM() {
		(RegImmTable[rfex_latch_.instruction.RType.rt])(this);
	}
    
    TKP_INSTR_FUNC CPU::BLEZ() {
		int16_t offset = rfex_latch_.instruction.IType.immediate << 2;
        int32_t seoffset = offset;
        if (rfex_latch_.fetched_rs.D <= 0) {
            exdc_latch_.data = pc_ - 4 + seoffset;
            exdc_latch_.dest = reinterpret_cast<uint8_t*>(&pc_);
            exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		    bypass_register();
        }
        exdc_latch_.was_branch = true;
	}
    
    /**
     * s_SLTI
     * 
     * doesn't throw
     */
    TKP_INSTR_FUNC CPU::SLTI() {
        int64_t seimm = static_cast<int16_t>(rfex_latch_.instruction.IType.immediate);
        exdc_latch_.dest = &gpr_regs_[rfex_latch_.instruction.IType.rt].UB._0;
        exdc_latch_.data = rfex_latch_.fetched_rs.D < seimm;
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		bypass_register();
	}
    
    TKP_INSTR_FUNC CPU::XORI() {
		exdc_latch_.dest = &gpr_regs_[rfex_latch_.instruction.IType.rt].UB._0;
        exdc_latch_.data = rfex_latch_.fetched_rs.UD ^ rfex_latch_.instruction.IType.immediate;
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		bypass_register();
	}
    
    TKP_INSTR_FUNC CPU::COP1() {
        (FloatTable[rfex_latch_.instruction.RType.func])(this);
	}
    
    TKP_INSTR_FUNC CPU::COP2() {
		throw ErrorFactory::generate_exception("COP2 opcode reached");
	}
    
    TKP_INSTR_FUNC CPU::BGTZL() {
		throw ErrorFactory::generate_exception("BGTZL opcode reached");
	}
    
    TKP_INSTR_FUNC CPU::DADDIU() {
		int64_t seimm = static_cast<int16_t>(rfex_latch_.instruction.IType.immediate);
        int64_t result = 0;
        bool overflow = __builtin_add_overflow(rfex_latch_.fetched_rs.D, seimm, &result);
        exdc_latch_.dest = &gpr_regs_[rfex_latch_.instruction.IType.rt].UB._0;
        exdc_latch_.data = result;
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		bypass_register();
	}
    
    TKP_INSTR_FUNC CPU::LDL() {
		throw ErrorFactory::generate_exception("LDL opcode reached");
	}
    
    TKP_INSTR_FUNC CPU::LDR() {
		throw ErrorFactory::generate_exception("LDR opcode reached");
	}
    
    TKP_INSTR_FUNC CPU::LWL() {
        int16_t offset = rfex_latch_.instruction.IType.immediate;
        int32_t seoffset = offset;
        exdc_latch_.vaddr = seoffset + rfex_latch_.fetched_rs.UW._0;
        auto paddr = translate_vaddr(exdc_latch_.vaddr);
        uint64_t data;
        uint64_t address = seoffset + rfex_latch_.fetched_rs.UW._0;
        uint32_t shift = 8 * ((address ^ 0) & 3);
        uint32_t mask = 0xFFFFFFFF << shift;
        load_memory(false, paddr.paddr & ~3, data, 4);
        gpr_regs_[rfex_latch_.instruction.IType.rt].UD = static_cast<int64_t>(static_cast<int32_t>((gpr_regs_[rfex_latch_.instruction.IType.rt].UW._0 & ~mask) | (data << shift)));
        // exdc_latch_.write_type = WriteType::LATEREGISTER;
        // exdc_latch_.access_type = AccessType::UWORD;
        // detect_ldi();
	}
    
    TKP_INSTR_FUNC CPU::LWR() {
		int16_t offset = rfex_latch_.instruction.IType.immediate;
        int32_t seoffset = offset;
        exdc_latch_.vaddr = seoffset + rfex_latch_.fetched_rs.UW._0;
        auto paddr = translate_vaddr(exdc_latch_.vaddr);
        uint64_t data;
        uint64_t address = seoffset + rfex_latch_.fetched_rs.UW._0;
        uint32_t shift = 8 * ((address ^ 3) & 3);
        uint32_t mask = 0xFFFFFFFF >> shift;
        load_memory(false, paddr.paddr & ~3, data, 4);
        gpr_regs_[rfex_latch_.instruction.IType.rt].UD = static_cast<int64_t>(static_cast<int32_t>((gpr_regs_[rfex_latch_.instruction.IType.rt].UW._0 & ~mask) | data >> shift));
	}
    
    TKP_INSTR_FUNC CPU::SB() {
		int16_t offset = rfex_latch_.instruction.IType.immediate;
        int32_t seoffset = offset;
        auto write_vaddr = seoffset + rfex_latch_.fetched_rs.UW._0;
        auto paddr_s = translate_vaddr(write_vaddr);
        exdc_latch_.paddr = paddr_s.paddr;
        exdc_latch_.cached = paddr_s.cached;
        exdc_latch_.data = rfex_latch_.fetched_rt.UB._0;
        exdc_latch_.write_type = WriteType::MMU;
        exdc_latch_.access_type = AccessType::UBYTE;
	}
    
    TKP_INSTR_FUNC CPU::SWL() {
        constexpr static uint32_t mask[4] = { 0x00000000, 0xFF000000, 0xFFFF0000, 0xFFFFFF00 };
        constexpr static uint32_t shift[4] = { 0, 8, 16, 24 };
		int16_t offset = rfex_latch_.instruction.IType.immediate;
        int32_t seoffset = offset;
        auto write_vaddr = (static_cast<uint32_t>(seoffset) & ~0b11) + rfex_latch_.fetched_rs.UW._0;
        auto addr_off = rfex_latch_.instruction.IType.immediate & 0b11;
        auto paddr_s = translate_vaddr(write_vaddr);
        exdc_latch_.paddr = paddr_s.paddr;
        exdc_latch_.cached = paddr_s.cached;
        // TODO: Fix this hack, dont load_memory
        load_memory(exdc_latch_.cached, exdc_latch_.paddr, exdc_latch_.data, 4);
        exdc_latch_.data &= mask[addr_off];
        exdc_latch_.data |= rfex_latch_.fetched_rt.UW._0 >> shift[addr_off];
        exdc_latch_.write_type = WriteType::MMU;
        exdc_latch_.access_type = AccessType::UWORD;
	}
    
    TKP_INSTR_FUNC CPU::SDL() {
		constexpr static uint64_t mask[8] = { 0, 0xFF00000000000000, 0xFFFF000000000000, 0xFFFFFF0000000000, 0xFFFFFFFF00000000, 0xFFFFFFFFFF000000, 0xFFFFFFFFFFFF0000, 0xFFFFFFFFFFFFFF00 };
        constexpr static uint32_t shift[8] = { 0, 8, 16, 24, 32, 40, 48, 56 };
		int16_t offset = rfex_latch_.instruction.IType.immediate;
        int32_t seoffset = offset;
        auto write_vaddr = (static_cast<uint32_t>(seoffset) & ~0b111) + rfex_latch_.fetched_rs.UW._0;
        auto addr_off = rfex_latch_.instruction.IType.immediate & 0b111;
        auto paddr_s = translate_vaddr(write_vaddr);
        exdc_latch_.paddr = paddr_s.paddr;
        exdc_latch_.cached = paddr_s.cached;
        // TODO: Fix this hack, dont load_memory
        load_memory(exdc_latch_.cached, exdc_latch_.paddr, exdc_latch_.data, 8);
        exdc_latch_.data &= mask[addr_off];
        exdc_latch_.data |= rfex_latch_.fetched_rt.UD >> shift[addr_off];
        exdc_latch_.write_type = WriteType::MMU;
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
	}
    
    TKP_INSTR_FUNC CPU::CACHE() {
		// throw ErrorFactory::generate_exception("CACHE opcode reached");
	}
    
    TKP_INSTR_FUNC CPU::LWC1() {
		VERBOSE(std::cout << "LWC1 is unimplemented" << std::endl;)
	}
    
    TKP_INSTR_FUNC CPU::LWC2() {
		throw ErrorFactory::generate_exception("LWC2 opcode reached");
	}
    
    TKP_INSTR_FUNC CPU::LLD() {
		throw ErrorFactory::generate_exception("LLD opcode reached");
	}
    
    TKP_INSTR_FUNC CPU::LDC1() {
		int16_t offset = rfex_latch_.instruction.IType.immediate;
        int32_t seoffset = offset;
        uint32_t vaddr = seoffset + rfex_latch_.fetched_rs.UW._0;
        uint64_t data;
        load_memory(false, translate_vaddr(vaddr).paddr, data, 8);
        fpr_regs_[rfex_latch_.instruction.FType.ft] = *reinterpret_cast<double*>(&data);
	}
    
    TKP_INSTR_FUNC CPU::LDC2() {
		throw ErrorFactory::generate_exception("LDC2 opcode reached");
	}
    
    TKP_INSTR_FUNC CPU::SC() {
		throw ErrorFactory::generate_exception("SC opcode reached");
	}
    
    TKP_INSTR_FUNC CPU::SWC1() {
		// throw ErrorFactory::generate_exception("SWC1 opcode reached");
	}
    
    TKP_INSTR_FUNC CPU::SWC2() {
		throw ErrorFactory::generate_exception("SWC2 opcode reached");
	}
    
    TKP_INSTR_FUNC CPU::SCD() {
		throw ErrorFactory::generate_exception("SCD opcode reached");
	}
    
    TKP_INSTR_FUNC CPU::SDC1() {
		int16_t offset = rfex_latch_.instruction.IType.immediate;
        int32_t seoffset = offset;
        uint32_t vaddr = seoffset + rfex_latch_.fetched_rs.UW._0;
        uint64_t data = *reinterpret_cast<uint64_t*>(&fpr_regs_[rfex_latch_.instruction.FType.ft]);
        store_memory(false, translate_vaddr(vaddr).paddr, data, 8);
	}
    
    TKP_INSTR_FUNC CPU::SDC2() {
		throw ErrorFactory::generate_exception("SDC2 opcode reached");
	}

    TKP_INSTR_FUNC CPU::SLTIU() {
        int64_t seimm = static_cast<int16_t>(rfex_latch_.instruction.IType.immediate);
        exdc_latch_.dest = &gpr_regs_[rfex_latch_.instruction.IType.rt].UB._0;
        exdc_latch_.data = rfex_latch_.fetched_rs.UD < seimm;
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		bypass_register();
	}
    
    TKP_INSTR_FUNC CPU::SDR() {
		constexpr static uint64_t mask[8] = { 0x00FFFFFFFFFFFFFF, 0x0000FFFFFFFFFFFF, 0x000000FFFFFFFFFF, 0x00000000FFFFFFFF, 0x0000000000FFFFFF, 0x000000000000FFFF, 0x00000000000000FF, 0x0000000000000000 };
        constexpr static uint32_t shift[8] = { 56, 48, 40, 32, 24, 16, 8, 0 };
		int16_t offset = rfex_latch_.instruction.IType.immediate;
        int32_t seoffset = offset;
        auto write_vaddr = (static_cast<uint32_t>(seoffset) & ~0b111) + rfex_latch_.fetched_rs.UW._0;
        auto addr_off = rfex_latch_.instruction.IType.immediate & 0b111;
        auto paddr_s = translate_vaddr(write_vaddr);
        exdc_latch_.paddr = paddr_s.paddr;
        exdc_latch_.cached = paddr_s.cached;
        // TODO: Fix this hack, dont load_memory
        load_memory(exdc_latch_.cached, exdc_latch_.paddr, exdc_latch_.data, 8);
        exdc_latch_.data &= mask[addr_off];
        exdc_latch_.data |= rfex_latch_.fetched_rt.UD << shift[addr_off];
        exdc_latch_.write_type = WriteType::MMU;
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
	}
    
    TKP_INSTR_FUNC CPU::SWR() {
		constexpr static uint32_t mask[4] = { 0x00FFFFFF, 0x0000FFFF, 0x000000FF, 0x00000000 };
        constexpr static uint32_t shift[4] = { 24, 16, 8, 0 };
		int16_t offset = rfex_latch_.instruction.IType.immediate;
        int32_t seoffset = offset;
        auto write_vaddr = (static_cast<uint32_t>(seoffset) & ~0b11) + rfex_latch_.fetched_rs.UW._0;
        auto addr_off = rfex_latch_.instruction.IType.immediate & 0b11;
        auto paddr_s = translate_vaddr(write_vaddr);
        exdc_latch_.paddr = paddr_s.paddr;
        exdc_latch_.cached = paddr_s.cached;
        // TODO: Fix this hack, dont load_memory
        load_memory(exdc_latch_.cached, exdc_latch_.paddr, exdc_latch_.data, 4);
        exdc_latch_.data &= mask[addr_off];
        exdc_latch_.data |= rfex_latch_.fetched_rt.UW._0 << shift[addr_off];
        exdc_latch_.write_type = WriteType::MMU;
        exdc_latch_.access_type = AccessType::UWORD;
	}
    
    TKP_INSTR_FUNC CPU::LL() {
		throw ErrorFactory::generate_exception("LL opcode reached");
	}
    
    TKP_INSTR_FUNC CPU::s_MTLO() {
		lo_ = rfex_latch_.fetched_rs.UD;
	}
    
    /**
     * ADDI, ADDIU
     * 
     * ADDI throws Integer overflow exception
     */
    TKP_INSTR_FUNC CPU::ADDIU() {
        int32_t seimm = static_cast<int16_t>(rfex_latch_.instruction.IType.immediate);
        int32_t result = 0;
        bool overflow = __builtin_add_overflow(rfex_latch_.fetched_rs.W._0, seimm, &result);
        exdc_latch_.dest = &gpr_regs_[rfex_latch_.instruction.IType.rt].UB._0;
        exdc_latch_.data = static_cast<int64_t>(result);
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		bypass_register();
    }
    TKP_INSTR_FUNC CPU::ADDI() {
        int32_t seimm = static_cast<int16_t>(rfex_latch_.instruction.IType.immediate);
        int32_t result = 0;
        bool overflow = __builtin_add_overflow(rfex_latch_.fetched_rs.W._0, seimm, &result);
        exdc_latch_.dest = &gpr_regs_[rfex_latch_.instruction.IType.rt].UB._0;
        exdc_latch_.data = result;
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		bypass_register();
        #if SKIPEXCEPTIONS == 0
        if (overflow) {
            // An integer overflow exception occurs if carries out of bits 30 and 31 differ (2’s
            // complement overflow). The contents of destination register rt is not modified
            // when an integer overflow exception occurs.
            throw IntegerOverflowException();
        }
        #endif
    }
    /**
     * DADDI
     * 
     * throws IntegerOverflowException
     *        ReservedInstructionException
     */
    TKP_INSTR_FUNC CPU::DADDI() {
        int64_t seimm = static_cast<int16_t>(rfex_latch_.instruction.IType.immediate);
        int64_t result = 0;
        bool overflow = __builtin_add_overflow(rfex_latch_.fetched_rs.D, seimm, &result);
        exdc_latch_.dest = &gpr_regs_[rfex_latch_.instruction.IType.rt].UB._0;
        exdc_latch_.data = result;
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		bypass_register();
        #if SKIPEXCEPTIONS == 0
        if (overflow) {
            // An integer overflow exception occurs if carries out of bits 30 and 31 differ (2’s
            // complement overflow). The contents of destination register rt is not modified
            // when an integer overflow exception occurs.
            throw IntegerOverflowException();
        }
        #endif
    }
    /**
     * J, JAL
     * 
     * doesn't throw
     */
    TKP_INSTR_FUNC CPU::JAL() {
        gpr_regs_[31].UD = pc_; // By the time this instruction is executed, pc is already incremented by 8
                                    // so there's no need to increment by 8
        J();
    }
    TKP_INSTR_FUNC CPU::J() {
        auto jump_addr = rfex_latch_.instruction.JType.target;
        // combine first 3 bits of pc and jump_addr shifted left by 2
        exdc_latch_.data = (pc_ & 0xF000'0000) | (jump_addr << 2);
        exdc_latch_.dest = reinterpret_cast<uint8_t*>(&pc_);
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		bypass_register();
        exdc_latch_.was_branch = true;
    }
    /**
     * LUI
     * 
     * doesn't throw
     */
    TKP_INSTR_FUNC CPU::LUI() {
        int32_t imm = rfex_latch_.instruction.IType.immediate << 16;
        uint64_t seimm = static_cast<int64_t>(imm);
        exdc_latch_.dest = &gpr_regs_[rfex_latch_.instruction.IType.rt].UB._0;
        exdc_latch_.data = seimm;
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		bypass_register();
    }
    /**
     * COP0
     */
    TKP_INSTR_FUNC CPU::COP0() {
        execute_cp0_instruction(rfex_latch_.instruction);
    }
    /**
     * ORI
     * 
     * doesn't throw
     */
    TKP_INSTR_FUNC CPU::ORI() {
        exdc_latch_.dest = &gpr_regs_[rfex_latch_.instruction.IType.rt].UB._0;
        exdc_latch_.data = rfex_latch_.fetched_rs.UD | rfex_latch_.instruction.IType.immediate;
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		bypass_register();
    }
    /**
     * s_AND
     * 
     * doesn't throw
     */
    TKP_INSTR_FUNC CPU::s_AND() {
        exdc_latch_.dest = &gpr_regs_[rfex_latch_.instruction.RType.rd].UB._0;
        exdc_latch_.data = rfex_latch_.fetched_rs.UD & rfex_latch_.fetched_rt.UD;
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		bypass_register();
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
    TKP_INSTR_FUNC CPU::SD() {
        int16_t offset = rfex_latch_.instruction.IType.immediate;
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
    TKP_INSTR_FUNC CPU::SW() {
        int16_t offset = rfex_latch_.instruction.IType.immediate;
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
    TKP_INSTR_FUNC CPU::SH() {
        int16_t offset = rfex_latch_.instruction.IType.immediate;
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
    }
    /**
     * LB, LBU
     * 
     * throws TLB miss exception
     *        TLB invalid exception
     *        Bus error exception
     *        Address error exception (?)
     */
    TKP_INSTR_FUNC CPU::LB() {
        LBU();
        exdc_latch_.sign_extend = true;
    }
    TKP_INSTR_FUNC CPU::LBU() {
        int16_t offset = rfex_latch_.instruction.IType.immediate;
        int32_t seoffset = offset;
        exdc_latch_.dest = &gpr_regs_[rfex_latch_.instruction.IType.rt].UB._0;
        exdc_latch_.vaddr = seoffset + rfex_latch_.fetched_rs.UW._0;
        exdc_latch_.write_type = WriteType::LATEREGISTER;
        exdc_latch_.access_type = AccessType::UBYTE;
        detect_ldi();
    }
    /**
     * LD
     * 
     * throws TLB miss exception
     *        TLB invalid exception
     *        Bus error exception
     *        Address error exception
     */
    TKP_INSTR_FUNC CPU::LD() {
        int16_t offset = rfex_latch_.instruction.IType.immediate;
        int32_t seoffset = offset;
        exdc_latch_.dest = &gpr_regs_[rfex_latch_.instruction.IType.rt].UB._0;
        exdc_latch_.vaddr = seoffset + rfex_latch_.fetched_rs.UW._0;
        exdc_latch_.write_type = WriteType::LATEREGISTER;
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
        detect_ldi();
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
    }
    /**
     * LH, LHU
     * 
     * throws TLB miss exception
     *        TLB invalid exception
     *        Bus error exception
     *        Address error exception
     */
    TKP_INSTR_FUNC CPU::LHU() {
        int16_t offset = rfex_latch_.instruction.IType.immediate;
        int32_t seoffset = offset;
        exdc_latch_.dest = &gpr_regs_[rfex_latch_.instruction.IType.rt].UB._0;
        exdc_latch_.vaddr = seoffset + rfex_latch_.fetched_rs.UW._0;
        exdc_latch_.write_type = WriteType::LATEREGISTER;
        exdc_latch_.access_type = AccessType::UHALFWORD;
        detect_ldi();
    }
    TKP_INSTR_FUNC CPU::LH() {
        LHU();
        exdc_latch_.sign_extend = true;
        #if SKIPEXCEPTIONS == 0
        if ((exdc_latch_.vaddr & 0b1) != 0) {
            // From manual:
            // If the least-significant bit of the address is not zero, an address error exception occurs.
            throw InstructionAddressErrorException();
        }
        #endif
    }
    /**
     * LW, LWU
     * 
     * throws TLB miss exception
     *        TLB invalid exception
     *        Bus error exception
     *        Address error exception
     */
    TKP_INSTR_FUNC CPU::LWU() {
        int16_t offset = rfex_latch_.instruction.IType.immediate;
        int32_t seoffset = offset;
        exdc_latch_.dest = &gpr_regs_[rfex_latch_.instruction.IType.rt].UB._0;
        exdc_latch_.vaddr = seoffset + rfex_latch_.fetched_rs.UW._0;
        exdc_latch_.write_type = WriteType::LATEREGISTER;
        exdc_latch_.access_type = AccessType::UWORD;
        detect_ldi();
    }
    TKP_INSTR_FUNC CPU::LW() {
        LWU();
        exdc_latch_.sign_extend = true;
        #if SKIPEXCEPTIONS == 0
        if ((exdc_latch_.vaddr & 0b11) != 0) {
            // From manual:
            // If either of the loworder two bits of the address are not zero, an address error exception occurs.
            throw InstructionAddressErrorException();
        }
        #endif
    }
    /**
     * ANDI
     * 
     * doesn't throw
     */
    TKP_INSTR_FUNC CPU::ANDI() {
        exdc_latch_.dest = &gpr_regs_[rfex_latch_.instruction.IType.rt].UB._0;
        exdc_latch_.data = rfex_latch_.fetched_rs.UD & rfex_latch_.instruction.IType.immediate;
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		bypass_register();
    }
    /**
     * BEQ
     * 
     * doesn't throw
     */
    TKP_INSTR_FUNC CPU::BEQ() {
        int16_t offset = rfex_latch_.instruction.IType.immediate << 2;
        int32_t seoffset = offset;
        if (rfex_latch_.fetched_rs.UD == rfex_latch_.fetched_rt.UD) {
            exdc_latch_.data = pc_ - 4 + seoffset;
            exdc_latch_.dest = reinterpret_cast<uint8_t*>(&pc_);
            exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		    bypass_register();
        }
        exdc_latch_.was_branch = true;
    }
    /**
     * BEQL
     * 
     * doesn't throw
     */
    TKP_INSTR_FUNC CPU::BEQL() {
        int16_t offset = rfex_latch_.instruction.IType.immediate << 2;
        int32_t seoffset = offset;
        if (rfex_latch_.fetched_rs.UD == rfex_latch_.fetched_rt.UD) {
            exdc_latch_.data = pc_ - 4 + seoffset;
            exdc_latch_.dest = reinterpret_cast<uint8_t*>(&pc_);
            exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		    bypass_register();
        } else {
            // Discard delay slot instruction
            icrf_latch_.instruction.Full = 0;
            was_ldi_ = true; // don't log next instruction
        }
        exdc_latch_.was_branch = true;
    }
    /**
     * BNE
     * 
     * doesn't throw
     */
    TKP_INSTR_FUNC CPU::BNE() {
        int16_t offset = rfex_latch_.instruction.IType.immediate << 2;
        int32_t seoffset = offset;
        if (rfex_latch_.fetched_rs.UD != rfex_latch_.fetched_rt.UD) {
            exdc_latch_.data = pc_ - 4 + seoffset;
            exdc_latch_.dest = reinterpret_cast<uint8_t*>(&pc_);
            exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		    bypass_register();
        }
        exdc_latch_.was_branch = true;
    }
    /**
     * BNEL
     * 
     * doesn't throw
     */
    TKP_INSTR_FUNC CPU::BNEL() {
        int16_t offset = rfex_latch_.instruction.IType.immediate << 2;
        int32_t seoffset = offset;
        if (rfex_latch_.fetched_rs.UD != rfex_latch_.fetched_rt.UD) {
            exdc_latch_.data = pc_ - 4 + seoffset;
            exdc_latch_.dest = reinterpret_cast<uint8_t*>(&pc_);
            exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		    bypass_register();
        } else {
            // Discard delay slot instruction
            icrf_latch_.instruction.Full = 0;
            was_ldi_ = true; // don't log next instruction
        }
        exdc_latch_.was_branch = true;
    }
    /**
     * BLEZL
     * 
     * doesn't throw
     */
    TKP_INSTR_FUNC CPU::BLEZL() {
        int16_t offset = rfex_latch_.instruction.IType.immediate << 2;
        int32_t seoffset = offset;
        if (rfex_latch_.fetched_rs.D <= 0) {
            exdc_latch_.data = pc_ - 4 + seoffset;
            exdc_latch_.dest = reinterpret_cast<uint8_t*>(&pc_);
            exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		    bypass_register();
        } else {
            // Discard delay slot instruction
            icrf_latch_.instruction.Full = 0;
            was_ldi_ = true; // don't log next instruction
        }
        exdc_latch_.was_branch = true;
    }
    /**
     * BGTZ
     * 
     * doesn't throw
     */
    TKP_INSTR_FUNC CPU::BGTZ() {
        int16_t offset = rfex_latch_.instruction.IType.immediate << 2;
        int32_t seoffset = offset;
        if (rfex_latch_.fetched_rs.D > 0) {
            exdc_latch_.data = pc_ - 4 + seoffset;
            exdc_latch_.dest = reinterpret_cast<uint8_t*>(&pc_);
            exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		    bypass_register();
        }
        exdc_latch_.was_branch = true;
    }
    /**
     * TGE
     * 
     * throws TrapException
     */
    TKP_INSTR_FUNC CPU::s_TGE() {
        if (rfex_latch_.fetched_rs.UD >= rfex_latch_.fetched_rt.UD)
            throw 0; //ErrorFactory::generate_exception("TrapException was thrown");static_assert(false);
    }
    /**
     * s_ADD, s_ADDU
     * 
     * s_ADD throws IntegerOverflowException
     */
    TKP_INSTR_FUNC CPU::s_ADD() {
        int32_t result = 0;
        bool overflow = __builtin_add_overflow(rfex_latch_.fetched_rt.W._0, rfex_latch_.fetched_rs.W._0, &result);
        exdc_latch_.dest = &gpr_regs_[rfex_latch_.instruction.RType.rd].UB._0;
        exdc_latch_.data = static_cast<int64_t>(result);
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		bypass_register();
        #if SKIPEXCEPTIONS == 0
        if (overflow) {
            // An integer overflow exception occurs if carries out of bits 30 and 31 differ (2’s
            // complement overflow). The contents of destination register rd is not modified
            // when an integer overflow exception occurs.
            throw IntegerOverflowException();
        }
        #endif
    }
    TKP_INSTR_FUNC CPU::s_ADDU() {
        int32_t result = 0;
        bool overflow = __builtin_add_overflow(rfex_latch_.fetched_rt.W._0, rfex_latch_.fetched_rs.W._0, &result);
        exdc_latch_.dest = &gpr_regs_[rfex_latch_.instruction.RType.rd].UB._0;
        exdc_latch_.data = static_cast<int64_t>(result);
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		bypass_register();
    }
    /**
     * s_JR, s_JALR
     * 
     * throws Address error exception
     */
    TKP_INSTR_FUNC CPU::s_JALR() {
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
        s_JR();
    }
    TKP_INSTR_FUNC CPU::s_JR() {
        auto jump_addr = rfex_latch_.fetched_rs.UD;
        exdc_latch_.data = jump_addr;
        exdc_latch_.dest = reinterpret_cast<uint8_t*>(&pc_);
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		bypass_register();
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
        exdc_latch_.was_branch = true;
    }
    /**
     * s_DSLL32
     * 
     * throws Reserved instruction exception
     */
    TKP_INSTR_FUNC CPU::s_DSLL32() {
        exdc_latch_.dest = &gpr_regs_[rfex_latch_.instruction.RType.rd].UB._0;
        exdc_latch_.data = rfex_latch_.fetched_rt.UD << (rfex_latch_.instruction.RType.sa + 32);
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		bypass_register();
    }
    /**
     * s_DSLLV
     * 
     * throws Reserved instruction exception
     */
    TKP_INSTR_FUNC CPU::s_DSLLV() {
        exdc_latch_.dest = &gpr_regs_[rfex_latch_.instruction.RType.rd].UB._0;
        exdc_latch_.data = rfex_latch_.fetched_rt.UD << (rfex_latch_.fetched_rs.UD & 0b111111);
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		bypass_register();
    }
    /**
     * s_SLL
     * 
     * throws Reserved instruction exception
     */
    TKP_INSTR_FUNC CPU::s_SLL() {
        exdc_latch_.dest = &gpr_regs_[rfex_latch_.instruction.RType.rd].UB._0;
        int32_t sedata = rfex_latch_.fetched_rt.UW._0 << rfex_latch_.instruction.RType.sa;
        exdc_latch_.data = static_cast<int64_t>(sedata);
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		bypass_register();
    }
    /**
     * s_SRL
     * 
     * doesn't throw
     */
    TKP_INSTR_FUNC CPU::s_SRL() {
        exdc_latch_.dest = &gpr_regs_[rfex_latch_.instruction.RType.rd].UB._0;
        int64_t sedata = static_cast<int64_t>(static_cast<int32_t>(rfex_latch_.fetched_rt.UW._0 >> rfex_latch_.instruction.RType.sa));
        exdc_latch_.data = sedata;
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		bypass_register();
    }
    /**
     * s_DSRA32
     * 
     * throws Reserved instruction exception
     */
    TKP_INSTR_FUNC CPU::s_DSRA32() {
        exdc_latch_.dest = &gpr_regs_[rfex_latch_.instruction.RType.rd].UB._0;
        int64_t sedata = rfex_latch_.fetched_rt.D >> (rfex_latch_.instruction.RType.sa + 32);
        exdc_latch_.data = sedata;
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		bypass_register();
    }
    /**
     * s_SLT
     * 
     * doesn't throw
     */
    TKP_INSTR_FUNC CPU::s_SLT() {
        exdc_latch_.dest = &gpr_regs_[rfex_latch_.instruction.RType.rd].UB._0;
        exdc_latch_.data = rfex_latch_.fetched_rs.D < rfex_latch_.fetched_rt.D;
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		bypass_register();
    }
    /**
     * s_SLTU
     * 
     * doesn't throw 
     */
    TKP_INSTR_FUNC CPU::s_SLTU() {
        exdc_latch_.dest = &gpr_regs_[rfex_latch_.instruction.RType.rd].UB._0;
        exdc_latch_.data = rfex_latch_.fetched_rs.UD < rfex_latch_.fetched_rt.UD;
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		bypass_register();
    }
    /**
     * s_NOR
     * 
     * doesn't throw
     */
    TKP_INSTR_FUNC CPU::s_NOR() {
        exdc_latch_.dest = &gpr_regs_[rfex_latch_.instruction.RType.rd].UB._0;
        exdc_latch_.data = ~(rfex_latch_.fetched_rs.UD | rfex_latch_.fetched_rt.UD);
        exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		bypass_register();
    }

    TKP_INSTR_FUNC CPU::r_BLTZ() {
		throw ErrorFactory::generate_exception("r_BLTZ opcode reached");
    }
    
    TKP_INSTR_FUNC CPU::r_BGEZ() {
        int16_t offset = rfex_latch_.instruction.IType.immediate << 2;
        int32_t seoffset = offset;
        if (rfex_latch_.fetched_rs.W._0 >= 0) {
            exdc_latch_.data = pc_ - 4 + seoffset;
            exdc_latch_.dest = reinterpret_cast<uint8_t*>(&pc_);
            exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		    bypass_register();
        }
        exdc_latch_.was_branch = true;
    }
    
    TKP_INSTR_FUNC CPU::r_BLTZL() {
        throw ErrorFactory::generate_exception("r_BLTZL opcode reached");
    }
    
    TKP_INSTR_FUNC CPU::r_BGEZL() {
        int16_t offset = rfex_latch_.instruction.IType.immediate << 2;
        int32_t seoffset = offset;
        if (rfex_latch_.fetched_rs.W._0 >= 0) {
            exdc_latch_.data = pc_ - 4 + seoffset;
            exdc_latch_.dest = reinterpret_cast<uint8_t*>(&pc_);
            exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		    bypass_register();
        } else {
            icrf_latch_.instruction.Full = 0;
        }
        exdc_latch_.was_branch = true;
    }
    
    TKP_INSTR_FUNC CPU::r_TGEI() {
        throw ErrorFactory::generate_exception("r_TGEI opcode reached");
    }
    
    TKP_INSTR_FUNC CPU::r_TGEIU() {
        throw ErrorFactory::generate_exception("r_TGEIU opcode reached");
    }
    
    TKP_INSTR_FUNC CPU::r_TLTI() {
        throw ErrorFactory::generate_exception("r_TLTI opcode reached");
    }
    
    TKP_INSTR_FUNC CPU::r_TLTIU() {
        throw ErrorFactory::generate_exception("r_TLTIU opcode reached");
    }
    
    TKP_INSTR_FUNC CPU::r_TEQI() {
        throw ErrorFactory::generate_exception("r_TEQI opcode reached");
    }
    
    TKP_INSTR_FUNC CPU::r_TNEI() {
        throw ErrorFactory::generate_exception("r_TNEI opcode reached");
    }
    
    TKP_INSTR_FUNC CPU::r_BLTZAL() {
        throw ErrorFactory::generate_exception("r_BLTZAL opcode reached");
    }
    
    TKP_INSTR_FUNC CPU::r_BGEZAL() {
        int16_t offset = rfex_latch_.instruction.IType.immediate << 2;
        int32_t seoffset = offset;
        gpr_regs_[31].UD = pc_;
        if (rfex_latch_.fetched_rs.D >= 0) {
            exdc_latch_.data = pc_ - 4 + seoffset;
            exdc_latch_.dest = reinterpret_cast<uint8_t*>(&pc_);
            exdc_latch_.access_type = AccessType::UDOUBLEWORD;
		    bypass_register();
        }
        exdc_latch_.was_branch = true;
    }
    
    TKP_INSTR_FUNC CPU::r_BLTZALL() {
        throw ErrorFactory::generate_exception("r_BLTZALL opcode reached");
    }
    
    TKP_INSTR_FUNC CPU::r_BGEZALL() {
        throw ErrorFactory::generate_exception("r_BGEZALL opcode reached");
    }

    #define fpu_instr bool is_double = (rfex_latch_.instruction.FType.fmt == 17)

    TKP_INSTR_FUNC CPU::f_ADD() {
        fpu_instr;
        if (is_double) {
            fpr_regs_[rfex_latch_.instruction.FType.fd] = fpr_regs_[rfex_latch_.instruction.FType.fs] + fpr_regs_[rfex_latch_.instruction.FType.ft];
        }
    }
    
    TKP_INSTR_FUNC CPU::f_SUB() {
        // VERBOSE(std::cout << "f_SUB not implemented" << std::endl;)
        VERBOSE(std::cout << "f_SUB not implemented" << std::endl;)
    }
    
    TKP_INSTR_FUNC CPU::f_MUL() {
        VERBOSE(std::cout << "f_MUL not implemented" << std::endl;)
    }
    
    TKP_INSTR_FUNC CPU::f_DIV() {
        // VERBOSE(std::cout << "f_DIV not implemented" << std::endl;)
        VERBOSE(std::cout << "f_DIV not implemented" << std::endl;)
    }
    
    TKP_INSTR_FUNC CPU::f_SQRT() {
        VERBOSE(std::cout << "f_SQRT not implemented" << std::endl;)
    }
    
    TKP_INSTR_FUNC CPU::f_ABS() {
        VERBOSE(std::cout << "f_ABS not implemented" << std::endl;)
    }
    
    TKP_INSTR_FUNC CPU::f_MOV() {
        // VERBOSE(std::cout << "f_MOV not implemented" << std::endl;)
        VERBOSE(std::cout << "f_MOV not implemented" << std::endl;)
    }
    
    TKP_INSTR_FUNC CPU::f_NEG() {
        VERBOSE(std::cout << "f_NEG not implemented" << std::endl;)
    }
    
    TKP_INSTR_FUNC CPU::f_ROUNDL() {
        VERBOSE(std::cout << "f_ROUNDL not implemented" << std::endl;)
    }
    
    TKP_INSTR_FUNC CPU::f_TRUNCL() {
        VERBOSE(std::cout << "f_TRUNCL not implemented" << std::endl;)
    }
    
    TKP_INSTR_FUNC CPU::f_CEILL() {
        VERBOSE(std::cout << "f_CEILL not implemented" << std::endl;)
    }
    
    TKP_INSTR_FUNC CPU::f_FLOORL() {
        VERBOSE(std::cout << "f_FLOORL not implemented" << std::endl;)
    }
    
    TKP_INSTR_FUNC CPU::f_ROUNDW() {
        VERBOSE(std::cout << "f_ROUNDW not implemented" << std::endl;)
    }
    
    TKP_INSTR_FUNC CPU::f_TRUNCW() {
        VERBOSE(std::cout << "f_TRUNCW not implemented" << std::endl;)
    }
    
    TKP_INSTR_FUNC CPU::f_CEILW() {
        VERBOSE(std::cout << "f_CEILW not implemented" << std::endl;)
    }
    
    TKP_INSTR_FUNC CPU::f_FLOORW() {
        VERBOSE(std::cout << "f_FLOORW not implemented" << std::endl;)
    }
    
    TKP_INSTR_FUNC CPU::f_CVTS() {
        // VERBOSE(std::cout << "f_CVTS not implemented" << std::endl;)
        std::cout << "Warning: f_CVTS unimplemented" << std::endl;
    }
    
    TKP_INSTR_FUNC CPU::f_CVTD() {
        // VERBOSE(std::cout << "f_CVTD not implemented" << std::endl;)
        std::cout << "Warning: f_CVTD unimplemented" << std::endl;
    }
    
    TKP_INSTR_FUNC CPU::f_CVTW() {
        VERBOSE(std::cout << "f_CVTW not implemented" << std::endl;)
    }
    
    TKP_INSTR_FUNC CPU::f_CVTL() {
        VERBOSE(std::cout << "f_CVTL not implemented" << std::endl;)
    }
    
    TKP_INSTR_FUNC CPU::f_CF() {
        VERBOSE(std::cout << "f_CF not implemented" << std::endl;)
    }
    
    TKP_INSTR_FUNC CPU::f_CUN() {
        VERBOSE(std::cout << "f_CUN not implemented" << std::endl;)
    }
    
    TKP_INSTR_FUNC CPU::f_CEQ() {
        VERBOSE(std::cout << "f_CEQ not implemented" << std::endl;)
    }
    
    TKP_INSTR_FUNC CPU::f_CUEQ() {
        VERBOSE(std::cout << "f_CUEQ not implemented" << std::endl;)
    }
    
    TKP_INSTR_FUNC CPU::f_COLT() {
        VERBOSE(std::cout << "f_COLT not implemented" << std::endl;)
    }
    
    TKP_INSTR_FUNC CPU::f_CULT() {
        VERBOSE(std::cout << "f_CULT not implemented" << std::endl;)
    }
    
    TKP_INSTR_FUNC CPU::f_COLE() {
        VERBOSE(std::cout << "f_COLE not implemented" << std::endl;)
    }
    
    TKP_INSTR_FUNC CPU::f_CULE() {
        VERBOSE(std::cout << "f_CULE not implemented" << std::endl;)
    }
    
    TKP_INSTR_FUNC CPU::f_CSF() {
        VERBOSE(std::cout << "f_CSF not implemented" << std::endl;)
    }
    
    TKP_INSTR_FUNC CPU::f_CNGLE() {
        VERBOSE(std::cout << "f_CNGLE not implemented" << std::endl;)
    }
    
    TKP_INSTR_FUNC CPU::f_CSEQ() {
        VERBOSE(std::cout << "f_CSEQ not implemented" << std::endl;)
    }
    
    TKP_INSTR_FUNC CPU::f_CNGL() {
        VERBOSE(std::cout << "f_CNGL not implemented" << std::endl;)
    }
    
    TKP_INSTR_FUNC CPU::f_CLT() {
        VERBOSE(std::cout << "f_CLT not implemented" << std::endl;)
    }
    
    TKP_INSTR_FUNC CPU::f_CNGE() {
        VERBOSE(std::cout << "f_CNGE not implemented" << std::endl;)
    }
    
    TKP_INSTR_FUNC CPU::f_CLE() {
        // VERBOSE(std::cout << "f_CLE not implemented" << std::endl;)
        VERBOSE(std::cout << "f_CLE not implemented" << std::endl;)
    }
    
    TKP_INSTR_FUNC CPU::f_CNGT() {
        VERBOSE(std::cout << "f_CNGT not implemented" << std::endl;)
    }

    #undef fpu_instr

    CPU::PipelineStageRet CPU::IC(PipelineStageArgs) {
        // Fetch the current process instruction
        auto paddr_s = translate_vaddr(pc_);
        icrf_latch_.instruction.Full = cpubus_.fetch_instruction_uncached(paddr_s.paddr);
        pc_ += 4;
    }

    CPU::PipelineStageRet CPU::RF(PipelineStageArgs) {
        // Fetch the registers
        rfex_latch_.fetched_rt_i = icrf_latch_.instruction.RType.rt;
        rfex_latch_.fetched_rs.UD = gpr_regs_[icrf_latch_.instruction.RType.rs].UD;
        rfex_latch_.fetched_rt.UD = gpr_regs_[icrf_latch_.instruction.RType.rt].UD;
        rfex_latch_.instruction = icrf_latch_.instruction;
    }

    CPU::PipelineStageRet CPU::EX(PipelineStageArgs) {
        if (!was_ldi_) {
            // printf("r0: %016x r1: %016x r2: %016x r3: %016x r4: %016x r5: %016x r6: %016x r7: %016x r8: %016x r9: %016x r10: %016x r11: %016x r12: %016x r13: %016x r14: %016x r15: %016x r16: %016x r17: %016x r18: %016x r19: %016x r20: %016x r21: %016x r22: %016x r23: %016x r24: %016x r25: %016x r26: %016x r27: %016x r28: %016x r29: %016x r30: %016x r31: %016x\n", gpr_regs_[0].UD, gpr_regs_[1].UD, gpr_regs_[2].UD, gpr_regs_[3].UD, gpr_regs_[4].UD, gpr_regs_[5].UD, gpr_regs_[6].UD, gpr_regs_[7].UD, gpr_regs_[8].UD, gpr_regs_[9].UD, gpr_regs_[10].UD, gpr_regs_[11].UD, gpr_regs_[12].UD, gpr_regs_[13].UD, gpr_regs_[14].UD, gpr_regs_[15].UD, gpr_regs_[16].UD, gpr_regs_[17].UD, gpr_regs_[18].UD, gpr_regs_[19].UD, gpr_regs_[20].UD, gpr_regs_[21].UD, gpr_regs_[22].UD, gpr_regs_[23].UD, gpr_regs_[24].UD, gpr_regs_[25].UD, gpr_regs_[26].UD, gpr_regs_[27].UD, gpr_regs_[28].UD, gpr_regs_[29].UD, gpr_regs_[30].UD, gpr_regs_[31].UD);
            // printf("%08x r0: %016x r1: %016x r2: %016x r3: %016x r4: %016x r5: %016x r6: %016x r7: %016x r8: %016x r9: %016x r10: %016x r11: %016x r12: %016x r13: %016x r14: %016x r15: %016x r16: %016x r17: %016x r18: %016x r19: %016x r20: %016x r21: %016x r22: %016x r23: %016x r24: %016x r25: %016x r26: %016x r27: %016x r28: %016x r29: %016x r30: %016x r31: %016x\n", rfex_latch_.instruction.Full, gpr_regs_[0].UD, gpr_regs_[1].UD, gpr_regs_[2].UD, gpr_regs_[3].UD, gpr_regs_[4].UD, gpr_regs_[5].UD, gpr_regs_[6].UD, gpr_regs_[7].UD, gpr_regs_[8].UD, gpr_regs_[9].UD, gpr_regs_[10].UD, gpr_regs_[11].UD, gpr_regs_[12].UD, gpr_regs_[13].UD, gpr_regs_[14].UD, gpr_regs_[15].UD, gpr_regs_[16].UD, gpr_regs_[17].UD, gpr_regs_[18].UD, gpr_regs_[19].UD, gpr_regs_[20].UD, gpr_regs_[21].UD, gpr_regs_[22].UD, gpr_regs_[23].UD, gpr_regs_[24].UD, gpr_regs_[25].UD, gpr_regs_[26].UD, gpr_regs_[27].UD, gpr_regs_[28].UD, gpr_regs_[29].UD, gpr_regs_[30].UD, gpr_regs_[31].UD);
        }
        was_ldi_ = false;
        exdc_latch_.write_type = WriteType::NONE;
        exdc_latch_.access_type = AccessType::NONE;
        exdc_latch_.was_branch = false;
        execute_instruction();
    }

    CPU::PipelineStageRet CPU::DC(PipelineStageArgs) {
        dcwb_latch_.data = 0;
        dcwb_latch_.access_type = exdc_latch_.access_type;
        dcwb_latch_.dest = exdc_latch_.dest;
        dcwb_latch_.write_type = exdc_latch_.write_type;
        dcwb_latch_.cached = exdc_latch_.cached;
        dcwb_latch_.paddr = exdc_latch_.paddr;
        if (exdc_latch_.write_type == WriteType::LATEREGISTER) {
            auto paddr_s = translate_vaddr(exdc_latch_.vaddr);
            uint32_t paddr = paddr_s.paddr;
            dcwb_latch_.cached = paddr_s.cached;
            load_memory(dcwb_latch_.cached, paddr, dcwb_latch_.data, dcwb_latch_.access_type);
            // Result is cast to uint64_t in order to zero extend
            dcwb_latch_.access_type = AccessType::UDOUBLEWORD;
            // if (ldi_) { // This IF can work uncommented if register bypassing would work
                // TODO: implement register bypassing from WB to EX and remove the comment above
                // Write early so RF fetches the correct data
                // TODO: technically not accurate behavior but the results are as expected
            store_register(dcwb_latch_.dest, dcwb_latch_.data, dcwb_latch_.access_type);
            dcwb_latch_.write_type = WriteType::NONE;
            ldi_ = false;
            // }
        } else {
            dcwb_latch_.data = exdc_latch_.data;
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
                break;
            }
            default: {
                break;
            }
        }
    }

    TranslatedAddress CPU::translate_vaddr(uint32_t addr) {
        // Fast vaddr translation
        // TODO: broken if uses kuseg
        return { addr - KSEG0_START - ((addr >> 29) & 1) * 0x2000'0000, false };
    }
    void CPU::store_register(uint8_t* dest8, uint64_t data, int size) {
        // std::memcpy(dest, &data, size);
        uint64_t* dest = (uint64_t*)(dest8);
        uint64_t mask = LUT[size];
        *dest = (*dest & ~mask) | (data & mask);
    }
    void CPU::invalidate_hwio(uint32_t addr, uint64_t& data) {
        if (data != 0)
        switch (addr) {
            case PI_STATUS: {
                cpubus_.pi_status_ = 0;
                data = 0;
                break;
            }
            case PI_RD_LEN: {
                VERBOSE(std::cout << "PI_RD_LEN!" << std::endl;)
                // std::memcpy(&cpubus_.rdram_[__builtin_bswap32(cpubus_.pi_cart_addr_)], cpubus_.redirect_paddress(__builtin_bswap32(cpubus_.pi_dram_addr_)), data + 1);
                break;
            }
            case PI_WR_LEN: {
                VERBOSE(std::cout << "PI_WR_LEN" << std::endl;)
                std::memcpy(&cpubus_.rdram_[__builtin_bswap32(cpubus_.pi_dram_addr_)], cpubus_.redirect_paddress(__builtin_bswap32(cpubus_.pi_cart_addr_)), data + 1);
                break;
            }
            case VI_CTRL: {
                auto format = data & 0b11;
                if (format == 0b10) {
                    VERBOSE(std::cout << "rgb5" << std::endl;)
                    rcp_.bitdepth_ = GL_UNSIGNED_SHORT_5_5_5_1_;
                } else if (format == 0b11)
                    rcp_.bitdepth_ = GL_UNSIGNED_BYTE_;
                break;
            }
            case VI_ORIGIN: {
                rcp_.framebuffer_ptr_ = cpubus_.redirect_paddress(data & 0xFFFFFF);
                break;
            }
            case VI_WIDTH: {
                VERBOSE(std::cout << "vi_width: " << std::dec << data << std::endl;)
                rcp_.width_ = data;
                rcp_.height_ = (480.0f / 640.0f) * data;
                should_resize_ = true;
                break;
            }
            case VI_V_CURRENT: {
                cpubus_.set_interrupt(Interrupt::VI, false);
                break;
            }
            case VI_V_INTR: {
                rcp_.vi_v_intr_ = data;
                data &= 0x3ff;
                VERBOSE(std::cout << "vi_intr: " << data << std::endl;)
                if (data == 0x3ff || data == 0)
                    break;
                uint64_t mod = cpubus_.time_ % (93'750'000 / 60);
                uint64_t time_per = (93'750'000 / 60) / rcp_.num_halflines_;
                uint64_t cur_line = mod / time_per;
                int lines_left = (data > cur_line) ? (data - cur_line) : (rcp_.num_halflines_ - cur_line + data);
                std::cout << "cur line: " << cur_line << " lines_left: " << lines_left << " data: " << data << std::endl;
                queue_event(SchedulerEventType::Vi, time_per * lines_left);
                break;
            }
            case VI_V_SYNC: {
                rcp_.num_halflines_ = data >> 1;
                break;
            }
            case MI_MASK: {
                // todo: make into a loop
                if (data & 0b10) {
                    cpubus_.mi_mask_ |= 1;
                }
                if (data & 0b1000) {
                    cpubus_.mi_mask_ |= 1 << 1;
                }
                if (data & 0b100000) {
                    cpubus_.mi_mask_ |= 1 << 2;
                }
                if (data & 0b10000000) {
                    cpubus_.mi_mask_ |= 1 << 3;
                }
                if (data & 0b1000000000) {
                    cpubus_.mi_mask_ |= 1 << 4;
                }
                if (data & 0b100000000000) {
                    cpubus_.mi_mask_ |= 1 << 5;
                }
                if (data & 0b1) {
                    cpubus_.mi_mask_ &= ~1;
                }
                if (data & 0b100) {
                    cpubus_.mi_mask_ &= ~(1 << 1);
                }
                if (data & 0b10000) {
                    cpubus_.mi_mask_ &= ~(1 << 2);
                }
                if (data & 0b1000000) {
                    cpubus_.mi_mask_ &= ~(1 << 3);
                }
                if (data & 0b100000000) {
                    cpubus_.mi_mask_ &= ~(1 << 4);
                }
                if (data & 0b10000000000) {
                    cpubus_.mi_mask_ &= ~(1 << 5);
                }
                data = cpubus_.mi_mask_;
                break;
            }
            case PIF_COMMAND: {
                VERBOSE(std::cout << "PIF_COMMAND: " << std::bitset<8>(data) << std::endl;)
                if (data & 0x20) {
                    data = 0x80;
                    cpubus_.pif_ram_[0x32] = 0;
                    cpubus_.pif_ram_[0x33] = 0;
                    cpubus_.pif_ram_[0x34] = 0;
                    cpubus_.pif_ram_[0x35] = 0;
                    cpubus_.pif_ram_[0x36] = 0;
                    cpubus_.pif_ram_[0x37] = 0;
                }
                if (data & 0x40) {
                    cpubus_.pif_ram_.fill(0);
                    data = 0;
                }
                break;
            }
        }
    }
    void CPU::store_memory(bool cached, uint32_t paddr, uint64_t& data, int size) {
        invalidate_hwio(paddr, data);
        // if (!cached) {
        uint8_t* loc = cpubus_.redirect_paddress(paddr);
        uint64_t temp = __builtin_bswap64(data);
        temp >>= 8 * (AccessType::UDOUBLEWORD - size);
        std::memcpy(loc, &temp, size);
        // } else {
        //     // currently not implemented
        // }
    }
    void CPU::load_memory(bool cached, uint32_t paddr, uint64_t& data, int size) {
        uint8_t* loc = cpubus_.redirect_paddress(paddr);
        uint64_t temp = 0;
        std::memcpy(&temp, loc, size);
        temp = __builtin_bswap64(temp);
        temp >>= 8 * (AccessType::UDOUBLEWORD - size);
        // Sign extend loaded word
        if (exdc_latch_.sign_extend) {
            switch (size) {
                case 1:
                    temp = static_cast<int64_t>(static_cast<int8_t>(temp));
                    break;
                case 2:
                    temp = static_cast<int64_t>(static_cast<int16_t>(temp));
                    break;
                case 4:
                    temp = static_cast<int64_t>(static_cast<int32_t>(temp));
                    break;
            }
            exdc_latch_.sign_extend = false;
        }
        data = temp;
    }
    
    void CPU::clear_registers() {
        for (auto& reg : gpr_regs_) {
            reg.UD = 0;
        }
        for (auto& reg : fpr_regs_) {
            reg = 0.0;
        }
        for (auto& reg : cp0_regs_) {
            reg.UD = 0;
        }
    }

    // TODO: probably safe to remove
    void CPU::fill_pipeline() {
        icrf_latch_ = {};
        rfex_latch_ = {};
        exdc_latch_ = {};
        dcwb_latch_ = {};
        IC();
        
        RF();
        IC();

        EX();
        RF();
        IC();

        DC();
        EX();
        RF();
        IC();
    }

    void CPU::update_pipeline() {
        while (scheduler_.size() && cpubus_.time_ >= scheduler_.top().time) [[unlikely]]
            handle_event();
        WB();
        DC();
        EX();
        if (!ldi_) [[likely]] {
            gpr_regs_[0].UD = 0;
            RF();
            IC();
        }
        ++cpubus_.time_;
    }

    void CPU::check_interrupts() {
        if ((cpubus_.mi_interrupt_ & cpubus_.mi_mask_) != 0) {
            bool interrupts_pending = cp0_regs_[CP0_CAUSE].UB._1 & CP0Status.IM;
            bool interrupts_enabled = cp0_regs_[CP0_STATUS].UD & 0b1;
            bool currently_handling_exception = cp0_regs_[CP0_STATUS].UD & 0b10 & 0;
            bool currently_handling_error = cp0_regs_[CP0_STATUS].UD & 0b100;
            bool should_service_interrupt = interrupts_pending
                                    && interrupts_enabled
                                    && !currently_handling_exception
                                    && !currently_handling_error;
            if (should_service_interrupt) {
                std::cout << "Handle exception!" << std::endl;
                handle_exception(ExceptionType::Interrupt);
            } else {
                std::cout << "Mask pass but can't handle exception:\nEXL: " << currently_handling_exception << "\nERL: " << currently_handling_error << "\nIE: " << interrupts_enabled << "\nIP: " << interrupts_pending << std::endl;
                std::cout << "CP0Status.IM:" << std::bitset<8>(CP0Status.IM) << "\nCP0Cause.IP:" << std::bitset<8>(cp0_regs_[CP0_CAUSE].UH._1);
            }
        } else {
            std::cout << std::bitset<8>(cpubus_.mi_interrupt_) << "\n" << std::bitset<8>(cpubus_.mi_mask_) << std::endl;
        }
    }

    void CPU::handle_exception(ExceptionType exception) {
        if (!CP0Status.EXL) {
            auto new_pc = pc_ - 8;
            if (exdc_latch_.was_branch) {
                // currently executing branch delay slot
                new_pc -= 4;
            }
            CP0Cause.BD = exdc_latch_.was_branch;
            cp0_regs_[CP0_EPC].UD = new_pc;
        }
        CP0Status.EXL = true;
        switch (exception) {
            case ExceptionType::Interrupt:
                CP0Cause.ExCode = 0;
                pc_ = 0x8000'0180;
            break;
        }
    }

    void CPU::fire_count() {
        uint32_t flags = cp0_regs_[CP0_CAUSE].UW._0;
        SetBit(flags, 15, true);
        cp0_regs_[CP0_CAUSE].UW._0 = flags;
        cp0_regs_[CP0_COUNT].UD = cp0_regs_[CP0_COMPARE].UD;
        std::cout << "Fired count!" << std::endl;
        // queue_event(SchedulerEventType::Interrupt, 1);
    }

    void CPU::execute_instruction() {
        (InstructionTable[rfex_latch_.instruction.IType.op])(this);
    }

    void CPU::bypass_register() {
        store_register(exdc_latch_.dest, exdc_latch_.data, exdc_latch_.access_type);
        exdc_latch_.write_type = WriteType::NONE;
    }

    void CPU::detect_ldi() {
        ldi_ = (rfex_latch_.fetched_rt_i == icrf_latch_.instruction.RType.rt || rfex_latch_.fetched_rt_i == icrf_latch_.instruction.RType.rs);
        // Insert NOP so next EX doesn't re-execute the load in case ldi = true
        rfex_latch_.instruction.Full = 0;
        was_ldi_ = ldi_;
    }

    void CPU::execute_cp0_instruction(const Instruction& instr) {
        uint32_t func = instr.RType.rs;
        if (func & 0b10000) {
            // Coprocessor function
            switch (instr.RType.func) {
                /**
                 * ERET
                 * 
                 * throws Coprocessor unusable exception
                */
                case 0b011000: {
                    if ((gpr_regs_[CP0_STATUS].UD & 0b10) == 1) {
                        std::cout << "error eret to : " << std::hex << cp0_regs_[CP0_EPC].UD << std::endl;
                        pc_ = cp0_regs_[CP0_ERROREPC].UD;
                        cp0_regs_[CP0_STATUS].UD &= ~0b100;
                    } else {
                        std::cout << "eret to : " << std::hex << cp0_regs_[CP0_EPC].UD << std::endl;
                        pc_ = cp0_regs_[CP0_EPC].UD;
                        cp0_regs_[CP0_STATUS].UD &= ~0b10;
                    }
                    // ERET doesn't run delay slot instruction
                    llbit_ = 0;
                    icrf_latch_.instruction.Full = 0;
                    break;
                }
                default: {
                    break;
                }
            }
        } else {
            switch (func & 0b1111) {
                /**
                 * MTC0
                 * 
                 * throws Coprocessor unusable exception
                 */
                case 0b0100: {
                    int64_t sedata = gpr_regs_[instr.RType.rt].W._0;
                    VERBOSE(std::cout << "Write to CP0 reg: " << CP0String(instr.RType.rd) << " " << "data: " << std::hex << sedata << std::endl;)
                    switch (instr.RType.rd) {
                        case CP0_COMPARE: {
                            if (sedata != 0) {
                                auto time_left = sedata - (cpubus_.time_ >> 1);
                                queue_event(SchedulerEventType::Count, time_left * 2);
                            }
                            break;  
                        }
                    }
                    exdc_latch_.dest = &cp0_regs_[instr.RType.rd].UB._0;
                    exdc_latch_.data = sedata;
                    exdc_latch_.access_type = AccessType::UDOUBLEWORD;
                    bypass_register();
                    break;
                }
                /**
                 * MFC0
                 * 
                 * throws Coprocessor unusable exception
                 */
                case 0b0000: {
                    VERBOSE(std::cout << "Read from CP0 reg: " << CP0String(instr.RType.rd) << std::endl;)
                    int64_t sedata = cp0_regs_[instr.RType.rd].W._0;
                    if (instr.RType.rd == CP0_COUNT) {
                        sedata = cpubus_.time_ >> 1;
                    }
                    exdc_latch_.dest = &gpr_regs_[instr.RType.rt].UB._0;
                    exdc_latch_.data = sedata;
                    exdc_latch_.access_type = AccessType::UDOUBLEWORD;
                    bypass_register();
                    break;
                }
                default: {
                    std::stringstream ss;
                    ss << "Unimplemented CP0 microcode:" << std::bitset<5>(func) << " - full:" << std::bitset<32>(instr.Full);
                    throw ErrorFactory::generate_exception(ss.str());
                }
            }
        }
    }
}