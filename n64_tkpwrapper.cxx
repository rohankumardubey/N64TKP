#include "n64_tkpwrapper.hxx"
#include "core/n64_tkpargs.hxx"
#include <include/emulator_factory.h>
#include <iostream>
#include <boost/stacktrace.hpp>
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

	N64_TKPWrapper::~N64_TKPWrapper() {}

	bool& N64_TKPWrapper::IsReadyToDraw() {
		return should_draw_;
	}
	
	bool N64_TKPWrapper::load_file(std::string path) {
		bool ipl_loaded = ipl_loaded_;
		if (!ipl_loaded) {
			const EmulatorUserData& user_data = EmulatorFactory::GetEmulatorUserData()[static_cast<int>(EmuType::N64)];
			auto ipl_path = user_data.Get("IPLPath");
			if (!std::filesystem::exists(ipl_path)) {
				throw std::runtime_error("Missing IPL path!");
			}
			bool ipl_status = n64_impl_.LoadIPL(ipl_path);
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
		Paused = false; // TODO: get from settings
		Stopped = false;
		Step = false;
		Reset();
		bool stopped_break = false;
		if (Paused) {
			goto paused;
		}
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
		} catch (std::exception& ex) {
			std::cout << ex.what() << "\n" << boost::stacktrace::stacktrace() << std::endl;
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