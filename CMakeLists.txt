cmake_minimum_required(VERSION 3.19)
project(N64TKP)
set(CMAKE_CXX_STANDARD 20)
set(FILES n64_tkpwrapper.cxx core/n64_impl.cxx core/n64_cpu.cxx core/n64_rcp.cxx core/n64_cpubus.cxx core/n64_cpuscheduler.cxx)
add_library(N64TKP ${FILES})
target_include_directories(N64TKP PUBLIC ../)
target_link_libraries(N64TKP)