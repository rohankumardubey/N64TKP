#include "n64_cpu.hxx"
#include <iostream>
#include "utils.hxx"
#include "n64_addresses.hxx"

namespace TKPEmu::N64::Devices {
    void CPU::handle_event() {
        switch (scheduler_.top().type) {
            case SchedulerEventType::Interrupt: {
                check_interrupts();
                break;
            }
            case SchedulerEventType::Count: {
                if ((scheduler_.top().time >> 1) == cp0_regs_[CP0_COMPARE].UD)
                    fire_count();
                else
                    VERBOSE(std::cout << "Compare changed before firing " << (scheduler_.top().time) << " " << cp0_regs_[CP0_COMPARE].UD << std::endl;)
                break;
            }
            case SchedulerEventType::Vi: {
                std::cout << "vi event" << std::endl;
                uint32_t flags = cp0_regs_[CP0_CAUSE].UW._0;
                SetBit(flags, 8, true);
                cp0_regs_[CP0_CAUSE].UW._0 = flags;
                cpubus_.mi_interrupt_ |= 1 << 3;
                queue_event(SchedulerEventType::Interrupt, 1);
                std::cout << "jumped from: " << std::hex << pc_ << std::endl;
                uint64_t temp =  rcp_.vi_v_intr_;
                invalidate_hwio(VI_V_INTR, temp);
                break;
            }
        }
        scheduler_.pop();
    }

    void CPU::queue_event(SchedulerEventType type, int time) {
        VERBOSE(std::cout << "queued event at: " << cpubus_.time_ + time << " current time: " << cpubus_.time_ << std::endl;)
        SchedulerEvent event(type, cpubus_.time_ + time);
        scheduler_.push(event);
    }
}