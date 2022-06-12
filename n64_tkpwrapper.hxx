#pragma once
#ifndef TKP_N64_TKPWRAPPER_H
#define TKP_N64_TKPWRAPPER_H
#include "../include/emulator.h"
#include "n64_impl.hxx"
#include <chrono>

namespace TKPEmu::N64 {
	constexpr auto INSTRS_PER_FRAME = (93'750'000 / 60);
	namespace Applications {
		class N64_RomDisassembly;
	}
    class N64_TKPWrapper : public Emulator {
		TKP_EMULATOR(N64_TKPWrapper);
	public:
		uint64_t LastFrameTime = 0;
		std::string IPLPath;
    private:
        N64 n64_impl_;
		bool should_draw_ = false;
		static bool ipl_loaded_;
		void update();
		void v_extra_close() override;
		std::chrono::system_clock::time_point frame_start = std::chrono::system_clock::now();
		uint64_t cur_frame_instrs_ = 0;
		friend class TKPEmu::Applications::N64_RomDisassembly;
    };
}
#endif