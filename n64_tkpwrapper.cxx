#include "n64_tkpwrapper.hxx"

namespace TKPEmu::N64 {
	bool N64_TKPWrapper::load_file(std::string path) {
		bool opened = n64_.LoadCartridge(path);
		if (SkipBoot) {
			
		}
		return opened;
	}
}