#pragma once
#ifndef TKP_N64_EXCEPTIONS_H
#define TKP_N64_EXCEPTIONS_H

namespace TKPEmu::N64 {
    class InstructionNotImplementedException : public std::logic_error {
    public:
        InstructionNotImplementedException(uint32_t instr) : std::logic_error{"Instruction not yet implemented: " + std::to_string(instr)} {}
    };
    class NotImplementedException : public std::logic_error {
    public:
        NotImplementedException(std::string arg) : std::logic_error{std::string("Not implemented exception. Message: ") + arg} {}
    };
    enum class PipelineException {
        None,
        InstructionAddressErrorException,
        InstructionTLBException,
        InstructionBusErrorException,
        SYSCALLInstructionException,
        BreakpointInstructionException,
        CoprocessorUnusableException,
        ReservedInstructionException,
        ExternalResetException,
        ExternalNMIException,
        IntegerOverflowException,
        TRAPInstructionException,
        FloatingPointException,
        DataAddressErrorException,
        DataTLBException,
        ReferencetoWatchAddressException,
        InterruptException,
        DataBusErrorException
    };
}
#endif