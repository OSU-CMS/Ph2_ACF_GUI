#include "tools/OTVTRXLightOff.h"
#include "HWDescription/VTRx.h"
#include "HWInterface/BeBoardInterface.h"
#include "HWInterface/D19cFWInterface.h"
#include "HWInterface/D19cOpticalInterface.h"
#include "HWInterface/VTRxInterface.h"
#include "MonitorUtils/DetectorMonitor.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

OTVTRXLightOff::OTVTRXLightOff() : Tool() {}

OTVTRXLightOff::~OTVTRXLightOff() {}

// Initialization function
void OTVTRXLightOff::Initialise() {}

// State machine control functions
void OTVTRXLightOff::Running()
{
    Initialise();
    TurnOffLight();
    fKeepRunning = false;
}

void OTVTRXLightOff::TurnOffLight()
{
    if(fDetectorMonitor != nullptr) fDetectorMonitor->pauseMonitoring();

    for(const auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard) { fVTRxInterface->WriteChipReg(cOpticalGroup->fVTRx, "GCD", 0, false); }

        LOG(INFO) << BOLDRED << "Turn off light ouptput of SFP cages" << RESET;
        D19cFWInterface* pInterface = static_cast<D19cFWInterface*>(fBeBoardFWMap.find(cBoard->getId())->second);
        pInterface->WriteReg("fc7_daq_cnfg.optical_block.enable.l8", 0xFF);
        pInterface->WriteReg("fc7_daq_cnfg.optical_block.enable.l12", 0xFF);
    }
    LOG(INFO) << BOLDRED << "SFP cages dark, VTRX asleep, good night..." << RESET;
    // exit(0);
}

void OTVTRXLightOff::Stop() {}

void OTVTRXLightOff::Pause() {}

void OTVTRXLightOff::Resume() {}
