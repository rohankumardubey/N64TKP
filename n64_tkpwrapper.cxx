#include "n64_tkpwrapper.hxx"
#include <GL/glew.h>

namespace TKPEmu::N64 {
	N64_TKPWrapper::N64_TKPWrapper() {
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
        for (std::size_t i = 0; i < screen_color_data_.size(); i++) {
            if ((i & 0b11) == 0b11)
                screen_color_data_[i] = 1.0f;
        }
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
	
	float* N64_TKPWrapper::GetScreenData() {
		return &screen_color_data_[0];
	}
}