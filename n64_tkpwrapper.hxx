#pragma once
#ifndef TKP_N64_TKPWRAPPER_H
#define TKP_N64_TKPWRAPPER_H
#include "../include/emulator.h"
#include "n64_impl.hxx"
namespace TKPEmu::N64 {
    class N64_TKPWrapper : public Emulator {
	public:
		N64_TKPWrapper(std::any args);
		// void HandleKeyDown(SDL_Keycode key) override;
		// void HandleKeyUp(SDL_Keycode key) override;
		// float* GetScreenData() override;
		// std::string GetEmulatorName() override;
		// std::string GetScreenshotHash() override;
		// bool IsReadyToDraw() override;
    private:
        N64 n64_;
        // void v_log_state() override;
		// void save_state(std::ofstream& ofstream) override;
		// void load_state(std::ifstream& ifstream) override;
		// void start_normal() override;
		// void start_debug() override;
		// void start_console() override;
		// void reset_normal() override;
		// void reset_skip() override;
		bool load_file(std::string path) override;
		// void update() override;
		// std::string print() const override;
    };
}
#endif