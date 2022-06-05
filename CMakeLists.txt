cmake_minimum_required(VERSION 3.19)
project(N64TKP)
set(FILES n64_impl.cxx n64_tkpromdisassembly.cxx n64_tkpwrapper.cxx
    n64_cpu.cxx n64_cpubus.cxx qa/n64_test_functions.cxx)
add_library(N64TKP ${FILES})