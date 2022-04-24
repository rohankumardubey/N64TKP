#pragma once
#ifndef TKP_N64_TEST_FUNCS_H
#define TKP_N64_TEST_FUNCS_H
#include <string>
#include <filesystem>
#include "../n64_impl.hxx"

namespace TKPEmu::N64 {
    struct QA {
        static std::string TestError;
        // These test functions operate differently on different
        // test roms depending on what each test rom outputs
        // when it passes
        static bool TestDillonB(std::filesystem::path path);
    };
}
#endif