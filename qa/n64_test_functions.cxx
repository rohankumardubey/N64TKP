#include "n64_test_functions.hxx"

namespace TKPEmu::N64 {
    std::string QA::TestResult = "";
    bool QA::TestDillonB(std::filesystem::path path) {
        N64::N64 n64_;
        n64_.LoadCartridge(path);
        for (int i = 0; i < 100; i++) {
            n64_.Update();
            // DillonB tests complete when r30 has any value,
            // and succeed when r30 = -1
            if (n64_.GetCPU().gpr_regs_[30].UD != 0) {
                if (n64_.GetCPU().gpr_regs_[30].D != -1l) {
                    return true;
                } else {
                    TestResult = "Failed DillonB test: " + path.stem().string() + " - r30 != -1";
                    return false;
                }
            }
            TestResult = "Failed DillonB test: " + path.stem().string() + " - exceeded 100 instructions";
            return false;
        }
    }
}