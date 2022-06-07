#include "n64_tkpwrapper.hxx"
#include <GL/glew.h>
// #include <valgrind/callgrind.h>

#ifndef CALLGRIND_START_INSTRUMENTATION
#define NO_PROFILING
#define CALLGRIND_START_INSTRUMENTATION
#endif
#ifndef CALLGRIND_STOP_INSTRUMENTATION
#define CALLGRIND_STOP_INSTRUMENTATION
#endif

namespace TKPEmu::N64 {
	N64_TKPWrapper::N64_TKPWrapper() : n64_impl_(EmulatorImage.format) {
		// TODO: rarely some games have different resolutions
		EmulatorImage.width = 320;
		EmulatorImage.height = 240;
		EmulatorImage.type = GL_UNSIGNED_BYTE;
		GLuint image_texture;
		glGenTextures(1, &image_texture);
		glBindTexture(GL_TEXTURE_2D, image_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, image_texture);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RGB,
			EmulatorImage.width,
			EmulatorImage.height,
			0,
			GL_RGB,
			GL_UNSIGNED_BYTE,
			NULL
		);
		glBindTexture(GL_TEXTURE_2D, 0);
		EmulatorImage.texture = image_texture;
	}

	N64_TKPWrapper::N64_TKPWrapper(std::any args) : N64_TKPWrapper() {

	}

	N64_TKPWrapper::~N64_TKPWrapper() {
		if (start_options != EmuStartOptions::Console)
			glDeleteTextures(1, &EmulatorImage.texture);
	}

	bool& N64_TKPWrapper::IsReadyToDraw() {
		return should_draw_;
	}
	
	bool N64_TKPWrapper::load_file(std::string path) {
		bool opened = n64_impl_.LoadCartridge(path);
		Loaded = opened;
		return opened;
	}
	
	void N64_TKPWrapper::start_debug() {
		auto func = [this]() {
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
				if (cur_frame_instrs_ == INSTRS_PER_FRAME) [[unlikely]] {
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
				std::lock_guard<std::mutex> lg(DebugUpdateMutex);
				Step.store(false);
				update();
				goto begin;
			}
		};
		UpdateThread = std::thread(func);
		UpdateThread.detach();
	}

	void N64_TKPWrapper::reset_skip() {
		n64_impl_.Reset();
	}
	
	void* N64_TKPWrapper::GetScreenData() {
		return n64_impl_.GetColorData();
	}
}