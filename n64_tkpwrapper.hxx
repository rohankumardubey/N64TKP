#pragma once
#ifndef TKP_N64_TKPWRAPPER_H
#define TKP_N64_TKPWRAPPER_H
#include "../include/emulator.h"
#include "n64_impl.hxx"

namespace TKPEmu::N64 {
	namespace Applications {
		class N64_RomDisassembly;
	}
    class N64_TKPWrapper : public Emulator {
	public:
		N64_TKPWrapper();
		N64_TKPWrapper(std::any args);
		~N64_TKPWrapper();
		// void HandleKeyDown(SDL_Keycode key) override;
		// void HandleKeyUp(SDL_Keycode key) override;
		void* GetScreenData() override;
		// std::string GetEmulatorName() override;
		// std::string GetScreenshotHash() override;
		bool& IsReadyToDraw() override;
		Devices::CPU& GetCPU() {
			return n64_impl_.cpu_;
		}
		void update() {
			n64_impl_.Update();
		}
    private:
        N64 n64_impl_;
		bool should_draw_ = false;
        // void v_log_state() override;
		// void save_state(std::ofstream& ofstream) override;
		// void load_state(std::ifstream& ifstream) override;
		// void start_normal() override;
		void start_debug() override;
		// void start_console() override;
		// void reset_normal() override;
		void reset_skip() override;
		bool load_file(std::string path) override;
		void clear_screen();
		// void update() override;
		// std::string print() const override;
		friend class TKPEmu::Applications::N64_RomDisassembly;
    };
}
#endif