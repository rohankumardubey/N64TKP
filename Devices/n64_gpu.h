#pragma once
#ifndef TKPEMU_N64_RCP_H
#define TKPEMU_N64_RCP_H
#include "n64_rsp.h"
#include "n64_rdp.h"
#include "n64_ve.h"

namespace TKPEmu::N64::Devices {
    // GPU, Reality Co-processor
    // Made of three components: RSP, RDP, VE
    class RCP {
    public:
        RCP();
    private:
        using RSP = TKPEmu::N64::Devices::RSP;
        using RDP = TKPEmu::N64::Devices::RDP;
        using VE = TKPEmu::N64::Devices::VE;
        RSP rsp_;
        RDP rdp_;
        VE ve_;
    };
}
#endif