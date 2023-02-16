#include "debugger.hxx"
#define GDBSTUB_IMPLEMENTATION
#include "gdbstub.h"
#include <stdio.h>
#include <algorithm>
#include <iostream>

using context_t = Debugger;

// shamelessly stolen from dillon
const char* target_xml =
        "<?xml version=\"1.0\"?>"
        "<!DOCTYPE feature SYSTEM \"gdb-target.dtd\">"
        "<target version=\"1.0\">"
        "<architecture>mips:4000</architecture>"
        "<osabi>none</osabi>"
        "<feature name=\"org.gnu.gdb.mips.cpu\">"
        "        <reg name=\"r0\" bitsize=\"64\" regnum=\"0\"/>"
        "        <reg name=\"r1\" bitsize=\"64\"/>"
        "        <reg name=\"r2\" bitsize=\"64\"/>"
        "        <reg name=\"r3\" bitsize=\"64\"/>"
        "        <reg name=\"r4\" bitsize=\"64\"/>"
        "        <reg name=\"r5\" bitsize=\"64\"/>"
        "        <reg name=\"r6\" bitsize=\"64\"/>"
        "        <reg name=\"r7\" bitsize=\"64\"/>"
        "        <reg name=\"r8\" bitsize=\"64\"/>"
        "        <reg name=\"r9\" bitsize=\"64\"/>"
        "        <reg name=\"r10\" bitsize=\"64\"/>"
        "        <reg name=\"r11\" bitsize=\"64\"/>"
        "        <reg name=\"r12\" bitsize=\"64\"/>"
        "        <reg name=\"r13\" bitsize=\"64\"/>"
        "        <reg name=\"r14\" bitsize=\"64\"/>"
        "        <reg name=\"r15\" bitsize=\"64\"/>"
        "        <reg name=\"r16\" bitsize=\"64\"/>"
        "        <reg name=\"r17\" bitsize=\"64\"/>"
        "        <reg name=\"r18\" bitsize=\"64\"/>"
        "        <reg name=\"r19\" bitsize=\"64\"/>"
        "        <reg name=\"r20\" bitsize=\"64\"/>"
        "        <reg name=\"r21\" bitsize=\"64\"/>"
        "        <reg name=\"r22\" bitsize=\"64\"/>"
        "        <reg name=\"r23\" bitsize=\"64\"/>"
        "        <reg name=\"r24\" bitsize=\"64\"/>"
        "        <reg name=\"r25\" bitsize=\"64\"/>"
        "        <reg name=\"r26\" bitsize=\"64\"/>"
        "        <reg name=\"r27\" bitsize=\"64\"/>"
        "        <reg name=\"r28\" bitsize=\"64\"/>"
        "        <reg name=\"r29\" bitsize=\"64\"/>"
        "        <reg name=\"r30\" bitsize=\"64\"/>"
        "        <reg name=\"r31\" bitsize=\"64\"/>"
        "        <reg name=\"lo\" bitsize=\"64\" regnum=\"33\"/>"
        "        <reg name=\"hi\" bitsize=\"64\" regnum=\"34\"/>"
        "        <reg name=\"pc\" bitsize=\"64\" regnum=\"37\"/>"
        "        </feature>"
        "<feature name=\"org.gnu.gdb.mips.cp0\">"
        "        <reg name=\"status\" bitsize=\"32\" regnum=\"32\"/>"
        "        <reg name=\"badvaddr\" bitsize=\"32\" regnum=\"35\"/>"
        "        <reg name=\"cause\" bitsize=\"32\" regnum=\"36\"/>"
        "        </feature>"
        "<!-- TODO fix the sizes here. How do we deal with configurable sizes? -->"
        "<feature name=\"org.gnu.gdb.mips.fpu\">"
        "        <reg name=\"f0\" bitsize=\"32\" type=\"ieee_single\" regnum=\"38\"/>"
        "        <reg name=\"f1\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f2\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f3\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f4\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f5\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f6\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f7\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f8\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f9\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f10\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f11\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f12\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f13\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f14\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f15\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f16\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f17\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f18\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f19\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f20\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f21\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f22\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f23\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f24\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f25\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f26\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f27\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f28\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f29\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f30\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"f31\" bitsize=\"32\" type=\"ieee_single\"/>"
        "        <reg name=\"fcsr\" bitsize=\"32\" group=\"float\"/>"
        "        <reg name=\"fir\" bitsize=\"32\" group=\"float\"/>"
        "        </feature>"
        "</target>";

const char* memory_map =
"<?xml version=\"1.0\"?>"
"<memory-map>"
    "<!-- KUSEG - TLB mapped, treat it as a giant block of RAM. Not ideal, but not sure how else to deal with it -->"
    "<memory type=\"ram\" start=\"0x0000000000000000\" length=\"0x80000000\"/>"

    "<!-- KSEG0 hardware mapped, full copy of the memory map goes here -->" // TODO finish
    "<memory type=\"ram\" start=\"0xffffffff80000000\" length=\"0x800000\"/>" // RDRAM
    "<memory type=\"ram\" start=\"0xffffffff84000000\" length=\"0x1000\"/>" // RSP DMEM
    "<memory type=\"ram\" start=\"0xffffffff84001000\" length=\"0x1000\"/>" // RSP IMEM
    "<memory type=\"rom\" start=\"0xffffffff9fc00000\" length=\"0x7c0\"/>" // PIF ROM

    "<!-- KSEG1 hardware mapped, full copy of the memory map goes here -->" // TODO finish
    "<memory type=\"ram\" start=\"0xffffffffa0000000\" length=\"0x800000\"/>" // RDRAM
    "<memory type=\"ram\" start=\"0xffffffffa4000000\" length=\"0x1000\"/>" // RSP DMEM
    "<memory type=\"ram\" start=\"0xffffffffa4001000\" length=\"0x1000\"/>" // RSP IMEM
    "<memory type=\"rom\" start=\"0xffffffffbfc00000\" length=\"0x7c0\"/>" // PIF ROM
"</memory-map>";

void gdb_connected(context_t* ctx) {
    // std::cout << "Connected!" << std::endl;
}

void gdb_disconnected(context_t* ctx) {
    // std::cout << "Disconnected!" << std::endl;
}

void gdb_start(context_t* ctx) {
    ctx->Queue(Event::Start);
}

void gdb_stop(context_t* ctx) {
    ctx->Queue(Event::Stop);
}

void gdb_step(context_t* ctx) {
    ctx->Queue(Event::Step);
}

void gdb_set_breakpoint(context_t* ctx, uint32_t address) {
    ctx->AddBreakpoint(address);
}

void gdb_clear_breakpoint(context_t* ctx, uint32_t address) {
    ctx->ClearBreakpoint(address);
}

ssize_t gdb_get_memory(context_t* ctx, char * buffer, size_t buffer_length, uint32_t address, size_t length) {
    printf("Getting memory %08X, %08lX\n", address, length);
    return snprintf(buffer, buffer_length, "00000000");
}

ssize_t gdb_get_register_value(context_t* ctx, char * buffer, size_t buffer_length, int reg) {
    printf("Getting register value #%d\n", reg);
    return snprintf(buffer, buffer_length, "00000000");
}

ssize_t gdb_get_general_registers(context_t* ctx, char * buffer, size_t buffer_length) {
    printf("Getting general registers\n");
    return snprintf(buffer, buffer_length, "00000000");
}

bool at_breakpoint() {
    // Detect breakpoint logic
    return false;
}

Debugger::Debugger() {
    gdbstub_config_t config;
    config.port = 5678;
    config.user_data = this;
    config.connected = (gdbstub_connected_t)gdb_connected;
    config.disconnected = (gdbstub_disconnected_t)gdb_disconnected;
    config.start = (gdbstub_start_t)gdb_start;
    config.stop = (gdbstub_stop_t)gdb_stop;
    config.step = (gdbstub_step_t)gdb_step;
    config.set_breakpoint = (gdbstub_set_breakpoint_t)gdb_set_breakpoint;
    config.clear_breakpoint = (gdbstub_clear_breakpoint_t)gdb_clear_breakpoint;
    config.get_memory = (gdbstub_get_memory_t)gdb_get_memory;
    config.get_register_value = (gdbstub_get_register_value_t)gdb_get_register_value;
    config.get_general_registers = (gdbstub_get_general_registers_t)gdb_get_general_registers;
    config.target_config = target_xml;
    config.target_config_length = sizeof(target_xml);
    config.memory_map = memory_map;
    config.memory_map_length = sizeof(memory_map);
    
    gdb_ = gdbstub_init(config);
    if (!gdb_) {
        fprintf(stderr, "failed to create gdbstub\n");
        return;
    }
    // bool running = true;
    // while (running)
    // {
    //     if (at_breakpoint()) {
    //         gdbstub_breakpoint_hit(gdb);
    //     }
        
    //     // Do not call more than a couple times per second
    //     gdbstub_tick(gdb);
    // }
}

Debugger::~Debugger() {
    gdbstub_term(gdb_);
}

void Debugger::AddBreakpoint(uint32_t address) {
    addresses_.push_back(address);
}

void Debugger::ClearBreakpoint(uint32_t address) {
    auto it = std::find(addresses_.begin(), addresses_.end(), address);
    if (it == addresses_.end()) {
        return;
    }
    addresses_.erase(it);
}

void Debugger::Queue(Event event) {

}

void Debugger::TickGdbStub() {
    gdbstub_tick(gdb_);
}