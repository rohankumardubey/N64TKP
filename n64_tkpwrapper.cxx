#include "n64_tkpwrapper.hxx"
#include <GL/glew.h>

namespace TKPEmu::N64 {
	N64_TKPWrapper::N64_TKPWrapper() : n64_impl_(EmulatorImage.format) {
		// TODO: rarely some games have different resolutions
		EmulatorImage.width = 320;
		EmulatorImage.height = 240;
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
			GL_RGBA,
			EmulatorImage.width,
			EmulatorImage.height,
			0,
			GL_RGBA,
			GL_FLOAT,
			NULL
		);
		glBindTexture(GL_TEXTURE_2D, 0);
		EmulatorImage.texture = image_texture;
		clear_screen();
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
	
	void N64_TKPWrapper::clear_screen() {
        n64_impl_.clear_screen();
		should_draw_ = true;
    }

	bool N64_TKPWrapper::load_file(std::string path) {
		bool opened = n64_impl_.LoadCartridge(path);
		if (SkipBoot) {
			
		}
		Loaded = opened;
		return opened;
	}
	
	void N64_TKPWrapper::start_debug() {
		Paused = true;
	}

	void N64_TKPWrapper::reset_skip() {
		n64_impl_.Reset();
	}
	
	void* N64_TKPWrapper::GetScreenData() {
		return n64_impl_.GetColorData();
	}
}