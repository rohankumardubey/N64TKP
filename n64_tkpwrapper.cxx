#include "n64_tkpwrapper.hxx"

namespace TKPEmu::N64 {
	N64_TKPWrapper::N64_TKPWrapper(std::any args) {}

	bool N64_TKPWrapper::load_file(std::string path) {
		bool opened = n64_impl_.LoadCartridge(path);
		if (SkipBoot) {
			
		}
		return opened;
	}
}