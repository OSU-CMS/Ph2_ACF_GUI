#include "tools/OTinjectionOccupancyScan.h"
#include "System/RegisterHelper.h"
#include "Utils/ContainerSerialization.h"
#ifdef __USE_ROOT__
#include "DQMUtils/DQMHistogramOTinjectionOccupancyScan.h"
#endif

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

std::string OTinjectionOccupancyScan::fCalibrationDescription = "Measure occupancy for different pulse injection";

OTinjectionOccupancyScan::OTinjectionOccupancyScan() : OTMeasureOccupancy() {}

OTinjectionOccupancyScan::~OTinjectionOccupancyScan() {}

void OTinjectionOccupancyScan::Initialise(void)
{
    fRegisterHelper->takeSnapshot();
    // free the registers in case any

    fListOfPulseValues              = convertStringToFloatList(findValueInSettings<std::string>("OTinjectionOccupancyScan_ListOfInjectedPulses", "0, 0.25, 0.5, 1., 2."));
    fNumberOfEventsWithoutInjection = findValueInSettings<double>("OTinjectionOccupancyScan_NumberOfEventsWithoutInjection", 1000000);
    fNumberOfEventsWithInjection    = findValueInSettings<double>("OTinjectionOccupancyScan_NumberOfEventsWithInjection", 1000);

    fForceChannelGroup = false;
    fThresholdOffset   = 0;

#ifdef __USE_ROOT__
    fDQMHistogramOTMeasureOccupancy = new DQMHistogramOTinjectionOccupancyScan();
    // Calibration is not running on the SoC: plots are booked during initialization
    fDQMHistogramOTMeasureOccupancy->book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif
}

void OTinjectionOccupancyScan::ConfigureCalibration() {}

void OTinjectionOccupancyScan::Running()
{
    LOG(INFO) << "Starting OTinjectionOccupancyScan measurement.";
    Initialise();
    scanInjection();
    LOG(INFO) << "Done with OTinjectionOccupancyScan.";
    Reset();
}

void OTinjectionOccupancyScan::Stop(void)
{
    LOG(INFO) << "Stopping OTinjectionOccupancyScan measurement.";
#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: processing the histograms
    fDQMHistogramOTMeasureOccupancy->process();
#endif
    SaveResults();
    closeFileHandler();
    LOG(INFO) << "OTinjectionOccupancyScan stopped.";
}

void OTinjectionOccupancyScan::Pause() {}

void OTinjectionOccupancyScan::Resume() {}

void OTinjectionOccupancyScan::Reset() { fRegisterHelper->restoreSnapshot(); }

void OTinjectionOccupancyScan::scanInjection()
{
    for(size_t iteration = 0; iteration < fListOfPulseValues.size(); ++iteration)
    {
        fCBCtestPulseValue = fSSAtestPulseValue = fMPAtestPulseValue = fListOfPulseValues.at(iteration);
        fNumberOfEvents                                              = (fCBCtestPulseValue == 0.) ? fNumberOfEventsWithoutInjection : fNumberOfEventsWithInjection;
        measureChannelOccupancy(iteration);
    }
}
