#include "n64_rcp.hxx"

namespace TKPEmu::N64::Devices {
    void RCP::Reset() {
        rsp_status_ = 1;
        rsp_dma_busy_ = 0;
    }
}