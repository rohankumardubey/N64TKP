#include "n64_cpu.hxx"
#include <iostream>
#include "utils.hxx"
#include "n64_addresses.hxx"

namespace TKPEmu::N64::Devices {
    void CPU::handle_event() {
        auto event_type = scheduler_.top().type;
        switch (event_type) {
            case SchedulerEventType::Interrupt: {
                check_interrupts();
                break;
            }
            case SchedulerEventType::Count: {
                if ((scheduler_.top().time >> 1) == cp0_regs_[CP0_COMPARE].UD) {
                    // fire_count();
                } else
                    VERBOSE(std::cout << "Compare changed before firing " << (scheduler_.top().time) << " " << cp0_regs_[CP0_COMPARE].UD << std::endl;)
                break;
            }
            case SchedulerEventType::Vi: {
                // queue next interrupt
                uint64_t temp =  rcp_.vi_v_intr_;
                invalidate_hwio(VI_V_INTR, temp);
                [[fallthrough]];
            }
            case SchedulerEventType::Si:
            case SchedulerEventType::Pi: {
                cpubus_.mi_interrupt_ |= 1 << static_cast<int>(event_type);
                bool interrupt = cpubus_.mi_interrupt_ & cpubus_.mi_mask_;
                CP0Cause.IP2 = interrupt;
                if (interrupt) {
                    queue_event(SchedulerEventType::Interrupt, 0);
                }
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