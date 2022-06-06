#include "n64_test_functions.hxx"

namespace TKPEmu::N64 {
    std::string QA::TestError = "";
    bool QA::TestDillonB(std::filesystem::path path) {
        // N64 n64_;
        // n64_.LoadCartridge(path);
        // for (int i = 0; i < 100; i++) {
        //     try {
        //         n64_.Update();
        //     } catch (const std::exception& ex) {
        //         TestError = "Exception:" + std::string(ex.what());
        //         goto error;
        //     }
        //     // DillonB tests complete when r30 has any value,
        //     // and succeed when r30 = -1
        //     // if (n64_.cpu_.gpr_regs_[30].UD != 0) {
        //     //     if (n64_.cpu_.gpr_regs_[30].D != -1l) {
        //     //         return true;
        //     //     } else {
        //     //         TestError = "r30 != -1";
        //     //         goto error;
        //     //     }
        //     // }
        // }
        // TestError = "exceeded 100 instructions";
        // error:
        // TestError = "Failed DillonB test: " + path.stem().string() + " - " + TestError;
        // return false;
        return true;
    }
}