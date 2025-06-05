#include "tools/OTVTRxLightYieldScan.h"
#include "HWDescription/VTRx.h"
#include "HWInterface/D19cFWInterface.h"
#include "HWInterface/D19clpGBTInterface.h"
#include "HWInterface/VTRxInterface.h"
#include "MonitorUtils/DetectorMonitor.h"
#include "System/RegisterHelper.h"
#include "Utils/ContainerSerialization.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

std::string OTVTRxLightYieldScan::fCalibrationDescription = "Scan VTRx+ ouput bias and modulation settings and measure the light power";

OTVTRxLightYieldScan::OTVTRxLightYieldScan() : Tool() {}

OTVTRxLightYieldScan::~OTVTRxLightYieldScan() {}

void OTVTRxLightYieldScan::Initialise(void)
{
    fRegisterHelper->takeSnapshot();
    // free the registers in case any

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: plots are booked during initialization
    fDQMHistogramOTVTRxLightYieldScan.book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif
}

void OTVTRxLightYieldScan::ConfigureCalibration() {}

void OTVTRxLightYieldScan::Running()
{
    LOG(INFO) << "Starting OTVTRxLightYieldScan measurement.";
    Initialise();
    scanVTRxLightYield();
    LOG(INFO) << "Done with OTVTRxLightYieldScan.";
    Reset();
}

void OTVTRxLightYieldScan::Stop(void)
{
    LOG(INFO) << "Stopping OTVTRxLightYieldScan measurement.";
#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: processing the histograms
    fDQMHistogramOTVTRxLightYieldScan.process();
#endif
    SaveResults();
    closeFileHandler();
    LOG(INFO) << "OTVTRxLightYieldScan stopped.";
}

void OTVTRxLightYieldScan::Pause() {}

void OTVTRxLightYieldScan::Resume() {}

void OTVTRxLightYieldScan::Reset() { fRegisterHelper->restoreSnapshot(); }

void OTVTRxLightYieldScan::scanVTRxLightYield()
{
    if(fDetectorMonitor != nullptr) fDetectorMonitor->pauseMonitoring();

    for(int biasValue = 40; biasValue <= 56; biasValue += 4)
    {
        LOG(INFO) << BOLDYELLOW << "Setting VTRx bias to 0x" << std::hex << +biasValue << std::dec << RESET;

        for(int modulationValue = 24; modulationValue <= 40; modulationValue += 4)
        {
            LOG(INFO) << BOLDMAGENTA << "    Setting VTRx modulation to 0x" << std::hex << +modulationValue << std::dec << RESET;

            DetectorDataContainer theOpticalPowerContainer;
            ContainerFactory::copyAndInitOpticalGroup<float>(*fDetectorContainer, theOpticalPowerContainer);
            std::vector<std::pair<std::string, uint16_t>> theRegisterValues;
            theRegisterValues.push_back({"CH1BIAS", biasValue});
            theRegisterValues.push_back({"CH1MOD", modulationValue | 0x80});
            for(auto theBoard: *fDetectorContainer)
            {
                D19cFWInterface* theFWinterface = dynamic_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(theBoard));
                for(auto theOpticalGroup: *theBoard)
                {
                    fVTRxInterface->WriteChipMultReg(theOpticalGroup->fVTRx, theRegisterValues, false);
                    usleep(10000);
                    theOpticalPowerContainer.getOpticalGroup(theBoard->getId(), theOpticalGroup->getId())->getSummary<float>() = theFWinterface->GetSFPParameter(theOpticalGroup, "RX");
                }
            }
#ifdef __USE_ROOT__
            fDQMHistogramOTVTRxLightYieldScan.fillOpticalPower(theOpticalPowerContainer, biasValue, modulationValue);
#else
            if(fDQMStreamer)
            {
                ContainerSerialization theLightYieldSerialization("OTVTRxLightYieldScanOpticalPower");
                theLightYieldSerialization.streamByBoardContainer(fDQMStreamer, theOpticalPowerContainer, biasValue, modulationValue);
            }
#endif
        }
    }

    if(fDetectorMonitor != nullptr) fDetectorMonitor->resumeMonitoring();
}