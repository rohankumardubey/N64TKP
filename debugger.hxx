#ifndef DEBUGGER_HXX
#define DEBUGGER_HXX
#include "gdbstub.h"
#include <vector>
#include <queue>

enum class Event {
    Start,
    Stop,
    Step,
};

class Debugger {
public:
    Debugger();
    ~Debugger();
    void AddBreakpoint(uint32_t address);
    void ClearBreakpoint(uint32_t address);
    void Queue(Event event);

    void TickGdbStub();
private:
    gdbstub_t* gdb_;
    std::vector<uint32_t> addresses_;
    std::queue<Event> events_;
};
#endif