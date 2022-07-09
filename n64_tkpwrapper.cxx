#include "n64_tkpwrapper.hxx"
#include "n64_tkpargs.hxx"
#include <GL/glew.h>
#include <iostream>
// #include <valgrind/callgrind.h>

#ifndef CALLGRIND_START_INSTRUMENTATION
#define NO_PROFILING
#define CALLGRIND_START_INSTRUMENTATION
#endif
#ifndef CALLGRIND_STOP_INSTRUMENTATION
#define CALLGRIND_STOP_INSTRUMENTATION
#endif

namespace TKPEmu::N64 {
	bool N64_TKPWrapper::ipl_loaded_ = false;
	N64_TKPWrapper::N64_TKPWrapper() : n64_impl_() {}

	// N64_TKPWrapper::N64_TKPWrapper(std::unique_ptr<OptionsBase> args) : N64_TKPWrapper() {
	// 	// auto args_s = std::any_cast<N64Args>(args);
	// 	// IPLPath = args_s.IPLPath;
	// }

	N64_TKPWrapper::~N64_TKPWrapper() {}

	bool& N64_TKPWrapper::IsReadyToDraw() {
		return should_draw_;
	}
	
	bool N64_TKPWrapper::load_file(std::string path) {
		bool ipl_loaded = ipl_loaded_;
		if (!ipl_loaded) {
			bool ipl_status = n64_impl_.LoadIPL(IPLPath);
			ipl_loaded = ipl_status;
		}
		bool opened = n64_impl_.LoadCartridge(path);
		Loaded = opened && ipl_loaded;
		return Loaded;
	}
	
	void N64_TKPWrapper::start() {
		std::lock_guard<std::mutex> lguard(ThreadStartedMutex);
		Loaded = true;
		Loaded.notify_all();
		Paused = true;
		Stopped = false;
		Step = false;
		Reset();
		bool stopped_break = false;
		goto paused;
		begin:
		CALLGRIND_START_INSTRUMENTATION;
		frame_start = std::chrono::system_clock::now();
		while (true) {
			update();
			++cur_frame_instrs_;
			#ifdef NO_PROFILING
			if (cur_frame_instrs_ >= INSTRS_PER_FRAME) [[unlikely]] {
			#else
			if (cur_frame_instrs_ == 5000) [[unlikely]] {
				stopped_break = true;
				break;
			#endif
				auto end = std::chrono::system_clock::now();
				auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end - frame_start).count();
				LastFrameTime = dur;
				cur_frame_instrs_ = 0;
				if (Stopped.load()) {
					stopped_break = true;
					break;
				}
				if (Paused.load()) {
					break;
				}
				should_draw_ = true;
				frame_start = std::chrono::system_clock::now();
			}
		}
		CALLGRIND_STOP_INSTRUMENTATION;
		paused:
		if (!stopped_break) {
			Step.wait(false);
			Step.store(false);
			update();
			goto begin;
		}
	}

	void N64_TKPWrapper::reset() {		
		try {
			n64_impl_.Reset();
		} catch (...) {
			cur_frame_instrs_ = INSTRS_PER_FRAME - 1;
			Stopped.store(true);
		}
	}

	void N64_TKPWrapper::update() {
		try {
			n64_impl_.Update();
		} catch (...) {
			cur_frame_instrs_ = INSTRS_PER_FRAME - 1;
			Stopped.store(true);
		}
	}

	void N64_TKPWrapper::HandleKeyDown(uint32_t key) {

	}

	void N64_TKPWrapper::HandleKeyUp(uint32_t key) {

	}

	void N64_TKPWrapper::v_extra_close()  {
		cur_frame_instrs_ = INSTRS_PER_FRAME - 1;
	}
	
	void* N64_TKPWrapper::GetScreenData() {
		return n64_impl_.GetColorData();
	}
	bool N64_TKPWrapper::poll_uncommon_request(const Request& request) {
		return false;
	}
}