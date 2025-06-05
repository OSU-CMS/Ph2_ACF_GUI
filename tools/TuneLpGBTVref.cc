#include "tools/TuneLpGBTVref.h"
#include "MonitorUtils/DetectorMonitor.h"
#include "System/RegisterHelper.h"

std::string TuneLpGBTVref::fCalibrationDescription = "Tune Vref value for LpGBT ADC";

TuneLpGBTVref::TuneLpGBTVref() : Tool() {}

TuneLpGBTVref::~TuneLpGBTVref() {}

void TuneLpGBTVref::Initialise()
{
    fRegisterHelper->takeSnapshot();
    fRegisterHelper->freeFrontEndRegister(FrontEndType::LpGBT, "VREFTUNE");

    fDoManualVrefTuning = (findValueInSettings<double>("DoManualVrefTuning", 0) > 0);
}

void TuneLpGBTVref::reset() { fRegisterHelper->restoreSnapshot(); }

void TuneLpGBTVref::tuneVref()
{
    for(const auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            auto& clpGBT = cOpticalGroup->flpGBT;
            if(clpGBT == nullptr) continue;
            if(fDoManualVrefTuning)
            {
                flpGBTInterface->ConfigureInternalMonitoring(clpGBT, 0);
                flpGBTInterface->TuneVref(clpGBT);
            }
            else { flpGBTInterface->AutoTuneVref(clpGBT); }
        }
    }
}

void TuneLpGBTVref::Running()
{
    if(fDetectorMonitor != nullptr) fDetectorMonitor->pauseMonitoring();
    Initialise();
    tuneVref();
    reset();
    if(fDetectorMonitor != nullptr) fDetectorMonitor->resumeMonitoring();
}

void TuneLpGBTVref::Stop() {}

void TuneLpGBTVref::Pause() {}

void TuneLpGBTVref::Resume() {}
