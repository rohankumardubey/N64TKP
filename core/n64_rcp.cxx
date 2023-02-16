#include "n64_rcp.hxx"

namespace TKPEmu::N64::Devices {
    void RCP::Reset() {
        // These default values are in little endian
        rsp_status_ = 0x01000000;
        rsp_dma_busy_ = 0;
    }
}