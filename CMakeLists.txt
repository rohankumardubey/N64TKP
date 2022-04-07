cmake_minimum_required(VERSION 3.19)
project(N64TKP)
file(GLOB_RECURSE SRC_FILES "*.cxx")
add_library(N64TKP ${SRC_FILES})