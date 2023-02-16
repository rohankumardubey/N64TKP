#pragma once
#ifndef TKP_N64_TKPWRAPPER_H
#define TKP_N64_TKPWRAPPER_H
#include "../include/emulator.h"
#include "core/n64_impl.hxx"
#include <chrono>

namespace TKPEmu::N64 {
	namespace Applications {
		class N64_RomDisassembly;
	}
    class N64_TKPWrapper : public Emulator {
		TKP_EMULATOR(N64_TKPWrapper);
	public:
		uint64_t LastFrameTime = 0;
    private:
        N64 n64_impl_;
		bool should_draw_ = false;
		static bool ipl_loaded_;
		void update();
		void v_extra_close() override;
		bool& IsResized() override { return n64_impl_.cpu_.should_resize_; }
		int GetBitdepth() override { return n64_impl_.GetBitdepth(); }
		int GetWidth() override { return n64_impl_.GetWidth(); }
		int GetHeight() override { return n64_impl_.GetHeight(); }
		std::chrono::system_clock::time_point frame_start = std::chrono::system_clock::now();
		uint64_t cur_frame_instrs_ = 0;
		friend class TKPEmu::Applications::N64_RomDisassembly;
    };
}
#endif