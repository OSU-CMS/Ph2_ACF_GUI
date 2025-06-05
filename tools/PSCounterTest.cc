#include "tools/PSCounterTest.h"
#include "System/RegisterHelper.h"
#include "Utils/ContainerSerialization.h"

#include "HWDescription/BeBoardRegItem.h"
#include "HWDescription/Cbc.h"
#include "HWDescription/MPA2.h"
#include "HWInterface/D19cFWInterface.h"
#include "HWInterface/D19cPSCounterFWInterface.h"
#include "HWInterface/FastCommandInterface.h"
#include "HWInterface/TriggerInterface.h"
#include "Utils/CBCChannelGroupHandler.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/EmptyContainer.h"
#include "Utils/MPAChannelGroupHandler.h"
#include "Utils/Occupancy.h"
#include "Utils/SSAChannelGroupHandler.h"
#include "Utils/ThresholdAndNoise.h"
#include "Utils/Timer.h"
#include "boost/format.hpp"
#include <math.h>

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

std::string PSCounterTest::fCalibrationDescription = "Insert brief calibration description here";

PSCounterTest::PSCounterTest() : Tool() {}

PSCounterTest::~PSCounterTest() {}

void PSCounterTest::Initialise(void)
{
    fRegisterHelper->takeSnapshot();

    fAllChan = true;

    fWithSSA = false;
    fWithMPA = false;
    std::vector<FrontEndType> cAllFrontEndTypes;
    for(auto cBoard: *fDetectorContainer)
    {
        auto cFrontEndTypes = cBoard->connectedFrontEndTypes();
        fWithSSA            = std::find(cFrontEndTypes.begin(), cFrontEndTypes.end(), FrontEndType::SSA2) != cFrontEndTypes.end();
        fWithMPA            = std::find(cFrontEndTypes.begin(), cFrontEndTypes.end(), FrontEndType::MPA2) != cFrontEndTypes.end();
        for(auto cFrontEndType: cFrontEndTypes)
        {
            if(std::find(cAllFrontEndTypes.begin(), cAllFrontEndTypes.end(), cFrontEndType) == cAllFrontEndTypes.end()) cAllFrontEndTypes.push_back(cFrontEndType);
        }
    }

    for(auto cFrontEndType: cAllFrontEndTypes)
    {
        if(cFrontEndType == FrontEndType::SSA2)
        {
            SSAChannelGroupHandler theChannelGroupHandler;
            theChannelGroupHandler.setChannelGroupParameters(1, 1, NSSACHANNELS); // 16*2*8
            setChannelGroupHandler(theChannelGroupHandler, cFrontEndType);
        }
        else if(cFrontEndType == FrontEndType::MPA2)
        {
            MPAChannelGroupHandler theChannelGroupHandler;
            theChannelGroupHandler.setChannelGroupParameters(1, NMPAROWS, NSSACHANNELS); // 16*2*8
            setChannelGroupHandler(theChannelGroupHandler, cFrontEndType);
        }
    }

    for(auto cBoard: *fDetectorContainer)
    {
        if(!fWithSSA && !fWithMPA) continue;

        cBoard->setEventType(EventType::PSAS);
        static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(cBoard))->InitializePSCounterFWInterface(cBoard);
        static_cast<D19cPSCounterFWInterface*>(static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(cBoard))->getL1ReadoutInterface())->configureFastReadout(true);

        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    fReadoutChipInterface->WriteChipReg(cChip, "AnalogueAsync", 1);
                    auto cType = cChip->getFrontEndType();

                    if(cType == FrontEndType::MPA2) { fReadoutChipInterface->WriteChipReg(cChip, "InjectedCharge", 70); }
                    else
                    {
                        fReadoutChipInterface->WriteChipReg(cChip, "InjectedCharge", 70);
                        fReadoutChipInterface->WriteChipReg(cChip, "AsyncDelay", 0x3bf2);
                    }
                } // Tells chip to expect async
            }
        }
    }

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: plots are booked during initialization
    fDQMHistogramPSCounterTest.book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif
}

void PSCounterTest::ConfigureCalibration() {}

void PSCounterTest::Running()
{
    LOG(INFO) << "Starting PSCounterTest measurement.";
    Initialise();
    Timer scanTimer;
    scanTimer.start();

    // uint16_t thrOffset = findValueInSettings<double>("PSCounterTest_thrOffset", 0);
    // RunFast(65 + thrOffset, 200 + thrOffset);

    for(uint16_t thrOffset = 0; thrOffset <= 30; thrOffset += 1)
    {
        RunFast(65 + thrOffset, 200 + thrOffset);
        // RunFast(thrOffset, thrOffset);
        // sleep(10);
    }

    scanTimer.stop();
    scanTimer.show("Scan time: ");

    LOG(INFO) << "Done with PSCounterTest.";
    Reset();
}

void PSCounterTest::RunFast(uint16_t stripThreshold, uint16_t pixelThreshold)
{
    std::cout << "Running" << std::endl;
    std::cout << __PRETTY_FUNCTION__ << " [" << __LINE__ << "] stripThreshold = " << stripThreshold << std::endl;
    std::cout << __PRETTY_FUNCTION__ << " [" << __LINE__ << "] pixelThreshold = " << pixelThreshold << std::endl;
    int eventsPerPoint = 100;
    // int eventsPerPoint = 0xfe;

    for(auto cBoard: *fDetectorContainer)
    {
        if(fWithSSA || fWithMPA)
        {
            // Allow for different SSA and MPA injection amplitudes
            for(auto cOpticalGroup: *cBoard)
            {
                for(auto cHybrid: *cOpticalGroup)
                {
                    for(auto cChip: *cHybrid)
                    {
                        auto cType = cChip->getFrontEndType();

                        if(cType == FrontEndType::MPA2) { fReadoutChipInterface->WriteChipReg(cChip, "Threshold", pixelThreshold); }
                        else { fReadoutChipInterface->WriteChipReg(cChip, "Threshold", stripThreshold); }
                    }
                }
            }
        }

        DetectorDataContainer theOccupancyContainer;
        ContainerFactory::copyAndInitStructure<Occupancy>(*fDetectorContainer, theOccupancyContainer);
        fDetectorDataContainer = &theOccupancyContainer;
        measureData(eventsPerPoint);

#ifdef __USE_ROOT__
        fDQMHistogramPSCounterTest.fillSCurvePlots(stripThreshold, pixelThreshold, theOccupancyContainer);
#else
        if(fDQMStreamerEnabled)
        {
            ContainerSerialization theContainerSerialization("PedeNoiseSCurve");
            theContainerSerialization.streamByHybridContainer(fDQMStreamer, theOccupancyContainer, stripThreshold, pixelThreshold);
        }
#endif

        uint16_t mpa_lsb11 = fReadoutChipInterface->ReadChipReg(cBoard->getFirstObject()->getFirstObject()->getObject(8), MPA2::getPixelRegisterName("ReadCounter_LSB", 1, 1));
        uint16_t mpa_msb11 = fReadoutChipInterface->ReadChipReg(cBoard->getFirstObject()->getFirstObject()->getObject(8), MPA2::getPixelRegisterName("ReadCounter_MSB", 1, 1));

        std::cout << "MPA=" << std::dec << (mpa_lsb11 | (mpa_msb11 << 8)) << std::endl;

        uint16_t ssa_lsb1 = fReadoutChipInterface->ReadChipReg(cBoard->getFirstObject()->getFirstObject()->getObject(0), SSA2::getStripRegisterName("AC_ReadCounterLSB", 1));
        uint16_t ssa_msb1 = fReadoutChipInterface->ReadChipReg(cBoard->getFirstObject()->getFirstObject()->getObject(0), SSA2::getStripRegisterName("AC_ReadCounterMSB", 1));

        std::cout << "SSA=" << std::dec << (ssa_lsb1 | (ssa_msb1 << 8)) << std::endl;
    }
}

void PSCounterTest::Stop(void)
{
    LOG(INFO) << "Stopping PSCounterTest measurement.";
#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: processing the histograms
    fDQMHistogramPSCounterTest.process();
#endif
    SaveResults();
    closeFileHandler();
    LOG(INFO) << "PSCounterTest stopped.";
}

void PSCounterTest::Pause() {}

void PSCounterTest::Resume() {}

void PSCounterTest::Reset() { fRegisterHelper->restoreSnapshot(); }
