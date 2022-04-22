#pragma once
#ifndef TKP_N64_EXCEPTIONS_H
#define TKP_N64_EXCEPTIONS_H

namespace TKPEmu::N64 {
    class InstructionNotImplementedException : public std::logic_error {
    public:
        InstructionNotImplementedException(std::string instr) : std::logic_error{std::string("Instruction not yet implemented: ") + instr} {}
    };
    class NotImplementedException : public std::logic_error {
    public:
        NotImplementedException(std::string arg) : std::logic_error{std::string("Not implemented exception. Message: ") + arg} {}
    };
    class InstructionAddressErrorException :  public std::runtime_error {
    public:
        InstructionAddressErrorException() : std::runtime_error{"InstructionAddressError exception"} {};
    };
    class ReservedInstructionException : public std::runtime_error {
    public:
        ReservedInstructionException() : std::runtime_error{"ReservedInstruction exception"} {};
    };
    class IntegerOverflowException : public std::runtime_error {
    public:
        IntegerOverflowException() : std::runtime_error{"IntegerOverflow exception"} {};
    };
}
#endif