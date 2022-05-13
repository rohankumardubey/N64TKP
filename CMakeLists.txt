cmake_minimum_required(VERSION 3.19)
project(N64TKP)
set(FILES n64_impl.cxx n64_tkpromdisassembly.cxx n64_tkpwrapper.cxx
    core/n64_cpu.cxx core/n64_cpubus.cxx qa/n64_test_functions.cxx)
add_library(N64TKP ${FILES})