#include "tools/OTinjectionDelayOptimization.h"
#include "HWDescription/BeBoard.h"
#include "HWDescription/Cbc.h"
#include "HWDescription/MPA2.h"
#include "HWDescription/SSA2.h"
#include "System/RegisterHelper.h"
#include "Utils/CBCChannelGroupHandler.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/MPAChannelGroupHandler.h"
#include "Utils/SSAChannelGroupHandler.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

std::string OTinjectionDelayOptimization::fCalibrationDescription = "Optimize delay for injecting calibration pulses";

OTinjectionDelayOptimization::OTinjectionDelayOptimization() : Tool() {}

OTinjectionDelayOptimization::~OTinjectionDelayOptimization() {}

void OTinjectionDelayOptimization::Initialise(void)
{
    fRegisterHelper->takeSnapshot();
    fRegisterHelper->freeBoardRegister("fc7_daq_cnfg.fast_command_block.test_pulse.delay_after_test_pulse");

    fRegisterHelper->freeFrontEndRegister(FrontEndType::CBC3, "^TestPulseDel&ChanGroup$"); // injection delay
    fRegisterHelper->freeFrontEndRegister(FrontEndType::CBC3, "^TriggerLatency1$");        // latency register 1
    fRegisterHelper->freeFrontEndRegister(FrontEndType::CBC3, "^FeCtrl&TrgLat2$");         // latency register 2
    fRegisterHelper->freeFrontEndRegister(FrontEndType::CBC3, "^VCth[12]$");               // threshold

    fRegisterHelper->freeFrontEndRegister(FrontEndType::MPA2, "^DL_ctrl[0-6]$");              // injection delay
    fRegisterHelper->freeFrontEndRegister(FrontEndType::MPA2, "^DL_en$");                     // injection delay enable
    fRegisterHelper->freeFrontEndRegister(FrontEndType::MPA2, "^MemoryControl_[1-2]_R\\d+$"); // latency
    fRegisterHelper->freeFrontEndRegister(FrontEndType::MPA2, "^ThDAC[0-6]$");                // threshold

    fRegisterHelper->freeFrontEndRegister(FrontEndType::SSA2, "^Delay_line$");   // injection delay
    fRegisterHelper->freeFrontEndRegister(FrontEndType::SSA2, "^control_[13]$"); // latency
    fRegisterHelper->freeFrontEndRegister(FrontEndType::SSA2, "^Bias_THDAC$");   // threshold

    fNumberOfEvents                        = findValueInSettings<double>("OTinjectionDelayOptimization_NumberOfEvents", 100);
    fMaximumDelay                          = findValueInSettings<double>("OTinjectionDelayOptimization_MaximumDelay", 150);
    fDelayStep                             = findValueInSettings<double>("OTinjectionDelayOptimization_DelayStep", 1);
    fCBCtestPulseValue                     = findValueInSettings<double>("OTinjectionDelayOptimization_CBCtestPulseValue", 1.);
    fSSAtestPulseValue                     = findValueInSettings<double>("OTinjectionDelayOptimization_SSAtestPulseValue", 0.5);
    fMPAtestPulseValue                     = findValueInSettings<double>("OTinjectionDelayOptimization_MPAtestPulseValue", 0.5);
    fCBCnumberOfSigmaNoiseAwayFromPedestal = findValueInSettings<double>("OTinjectionDelayOptimization_CBCnumberOfSigmaNoiseAwayFromPedestal", 5.);
    fSSAnumberOfSigmaNoiseAwayFromPedestal = findValueInSettings<double>("OTinjectionDelayOptimization_SSAnumberOfSigmaNoiseAwayFromPedestal", 5.);
    fMPAnumberOfSigmaNoiseAwayFromPedestal = findValueInSettings<double>("OTinjectionDelayOptimization_MPAnumberOfSigmaNoiseAwayFromPedestal", 5.);

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: plots are booked during initialization
    fDQMHistogramOTinjectionDelayOptimization.book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif
}

void OTinjectionDelayOptimization::ConfigureCalibration() {}

void OTinjectionDelayOptimization::Running()
{
    LOG(INFO) << "Starting OTinjectionDelayOptimization measurement.";
    Initialise();
    optimizeInjectionDelay();
    LOG(INFO) << "Done with OTinjectionDelayOptimization.";
    Reset();
}

void OTinjectionDelayOptimization::Stop(void)
{
    LOG(INFO) << "Stopping OTinjectionDelayOptimization measurement.";
#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: processing the histograms
    fDQMHistogramOTinjectionDelayOptimization.process();
#endif
    SaveResults();
    closeFileHandler();
    LOG(INFO) << "OTinjectionDelayOptimization stopped.";
}

void OTinjectionDelayOptimization::Pause() {}

void OTinjectionDelayOptimization::Resume() {}

void OTinjectionDelayOptimization::Reset() { fRegisterHelper->restoreSnapshot(); }

void OTinjectionDelayOptimization::optimizeInjectionDelay()
{
    bool is2Smodule = (fDetectorContainer->getFirstObject()->getFirstObject()->getFrontEndType() == FrontEndType::OuterTracker2S);

    if(is2Smodule)
        prepareInjectionDelayScan2S();
    else
        prepareInjectionDelayScanPS();

    DetectorDataContainer* theOccupancyContainer = new DetectorDataContainer; // used to store occupancy while running bitWiseScan
    ContainerFactory::copyAndInitStructure<Occupancy>(*fDetectorContainer, *theOccupancyContainer);
    fDetectorDataContainer = theOccupancyContainer;

    std::pair<uint16_t, uint16_t> defaultThresholdAndDelay{0, 0}; // since 0 delay would not be measureable with this procedure (no pedestal) using 0 as not yet found value
    DetectorDataContainer         theBestThresholdAndDelayContainer;
    ContainerFactory::copyAndInitChip<std::pair<uint16_t, uint16_t>>(*fDetectorContainer, theBestThresholdAndDelayContainer, defaultThresholdAndDelay);

    uint16_t              initialThreshold = is2Smodule ? 1023 : 0;
    DetectorDataContainer theHighestThresholdContainer;
    ContainerFactory::copyAndInitChip<uint16_t>(*fDetectorContainer, theHighestThresholdContainer, initialThreshold);

    uint16_t maximumPedestalDelay = is2Smodule ? 25 : 20;
    uint16_t numberOfIterations   = 0;
    bool     isPedestalAveraged   = false;

    for(uint16_t delay = 0; delay <= fMaximumDelay; delay += fDelayStep)
    {
        float targetThreshold = 0.50;
        LOG(INFO) << BOLDBLUE << "Finding threshold corresponging to " << targetThreshold << "% occupancy with total injection delay " << delay << RESET;
        if(is2Smodule)
            setLatencyAndDelay2S(delay);
        else
            setLatencyAndDelayPS(delay);

        theOccupancyContainer->clear();

        bitWiseScan("Threshold", fNumberOfEvents, targetThreshold);
        DetectorDataContainer theThresholdContainer;
        ContainerFactory::copyAndInitChip<uint16_t>(*fDetectorContainer, theThresholdContainer);

        for(auto theBoard: *fDetectorContainer)
        {
            for(auto theOpticalGroup: *theBoard)
            {
                for(auto theHybrid: *theOpticalGroup)
                {
                    for(auto theChip: *theHybrid)
                    {
                        auto  theThreshold = fReadoutChipInterface->ReadChipReg(theChip, "Threshold");
                        auto& theChipBestThresholdAndDelay =
                            theBestThresholdAndDelayContainer.getChip(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId(), theChip->getId())->getSummary<std::pair<uint16_t, uint16_t>>();
                        auto& theChipHighestThreshold = theHighestThresholdContainer.getChip(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId(), theChip->getId())->getSummary<uint16_t>();
                        theThresholdContainer.getChip(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId(), theChip->getId())->getSummary<uint16_t>() = theThreshold;
                        if(delay < maximumPedestalDelay) // still in the plateau, add to the pedestal average
                        {
                            theChipBestThresholdAndDelay.first += theThreshold;
                        }
                        else
                        {
                            if(!isPedestalAveraged)
                            {
                                float expectedNoise         = theChip->getAverageNoise();
                                float distanceFromThreshold = 0;
                                auto  theChipFrontEndType   = theChip->getFrontEndType();
                                if(theChipFrontEndType == FrontEndType::CBC3) distanceFromThreshold = -expectedNoise * fCBCnumberOfSigmaNoiseAwayFromPedestal;
                                if(theChipFrontEndType == FrontEndType::SSA2) distanceFromThreshold = expectedNoise * fSSAnumberOfSigmaNoiseAwayFromPedestal;
                                if(theChipFrontEndType == FrontEndType::MPA2) distanceFromThreshold = expectedNoise * fMPAnumberOfSigmaNoiseAwayFromPedestal;
                                float thePedestal      = float(theChipBestThresholdAndDelay.first) / numberOfIterations;
                                float theBestThreshold = thePedestal + distanceFromThreshold;
                                theChip->setAveragePedestal(thePedestal);
                                theChipBestThresholdAndDelay.first = std::round(theBestThreshold);
                            }
                        }
                        if(is2Smodule)
                        {
                            if(theChipHighestThreshold > theThreshold)
                            {
                                theChipHighestThreshold             = theThreshold;
                                theChipBestThresholdAndDelay.second = delay;
                            }
                        }
                        else
                        {
                            if(theChipHighestThreshold < theThreshold)
                            {
                                theChipHighestThreshold             = theThreshold;
                                theChipBestThresholdAndDelay.second = delay;
                            }
                        }
                    }
                }
            }
        }

        if(delay < maximumPedestalDelay) // still in the plateau, add to the pedestal average
        {
            ++numberOfIterations;
        }
        else if(!isPedestalAveraged) // still in the plateau, add to the pedestal average
        {
            isPedestalAveraged = true;
        }

#ifdef __USE_ROOT__
        fDQMHistogramOTinjectionDelayOptimization.fillThresholdVsDelayScan(delay, theThresholdContainer);
#else
        if(fDQMStreamerEnabled)
        {
            ContainerSerialization theContainerSerialization("OTinjectionDelayOptimizationDelayScan");
            theContainerSerialization.streamByOpticalGroupContainer(fDQMStreamer, theThresholdContainer, delay);
        }
#endif
    }

    // set best delay and latency
    for(auto theBoard: *fDetectorContainer)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                for(auto theChip: *theHybrid)
                {
                    auto theChipAveragePedestalAndBestDelay =
                        theBestThresholdAndDelayContainer.getChip(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId(), theChip->getId())->getSummary<std::pair<uint16_t, uint16_t>>();
                    auto latencyAndDelay = calculateDACsFromTotalDelay(theChipAveragePedestalAndBestDelay.second, is2Smodule);
                    fReadoutChipInterface->WriteChipReg(theChip, "Threshold", theChipAveragePedestalAndBestDelay.first);
                    fReadoutChipInterface->WriteChipReg(theChip, "TriggerLatency", latencyAndDelay.first);
                    auto theChipFrontEndType = theChip->getFrontEndType();
                    if(theChipFrontEndType == FrontEndType::CBC3) fReadoutChipInterface->WriteChipReg(theChip, "TestPulseDelay", latencyAndDelay.second);
                    if(theChipFrontEndType == FrontEndType::SSA2) fReadoutChipInterface->WriteChipReg(theChip, "Delay_line", latencyAndDelay.second + (1 << 7));
                    if(theChipFrontEndType == FrontEndType::MPA2) fReadoutChipInterface->WriteChipReg(theChip, "DL_ctrl", latencyAndDelay.second);
                }
            }
        }
    }

#ifdef __USE_ROOT__
    fDQMHistogramOTinjectionDelayOptimization.fillBestThresholdAndDelay(theBestThresholdAndDelayContainer);
#else
    if(fDQMStreamerEnabled)
    {
        ContainerSerialization theBestValuesSerialization("OTinjectionDelayOptimizationBestValues");
        theBestValuesSerialization.streamByOpticalGroupContainer(fDQMStreamer, theBestThresholdAndDelayContainer);
    }
#endif

    delete theOccupancyContainer;
    fDetectorDataContainer = nullptr;
}

std::pair<uint16_t, uint8_t> OTinjectionDelayOptimization::calculateDACsFromTotalDelay(uint16_t totalDelay, bool is2Smodule) const
{
    uint8_t  numberOfDelayInClockCycle = is2Smodule ? fNumberOfDelayInClockCycle2S : fNumberOfDelayInClockCyclePS;
    uint8_t  delayDAC                  = numberOfDelayInClockCycle - (totalDelay % numberOfDelayInClockCycle);
    uint16_t latencyDAC                = fInitialLatency - (totalDelay / numberOfDelayInClockCycle);
    if(delayDAC == numberOfDelayInClockCycle)
    {
        delayDAC   = 0;
        latencyDAC = latencyDAC + 1;
    }
    return std::make_pair(latencyDAC, delayDAC);
}

void OTinjectionDelayOptimization::prepareInjectionDelayScan2S()
{
    this->enableTestPulse(true);
    LOG(INFO) << BOLDBLUE << "OTinjectionDelayOptimization::injectionDelayScan2S - Scanning Delay for 2S module" << RESET;

    CBCChannelGroupHandler theChannelGroupHandler(std::bitset<NCHANNELS>(CBC_CHANNEL_GROUP_BITSET));
    theChannelGroupHandler.setChannelGroupParameters(16, 1, 2);
    setChannelGroupHandler(theChannelGroupHandler);

    this->setTestAllChannels(false);
    // Setting sparsification for simplicity
    for(auto theBoard: *fDetectorContainer)
    {
        fBeBoardInterface->WriteBoardReg(theBoard, "fc7_daq_cnfg.physical_interface_block.cic.2s_sparsified_enable", 0);
        fBeBoardInterface->WriteBoardReg(theBoard, "fc7_daq_cnfg.fast_command_block.test_pulse.delay_after_test_pulse", fInitialLatency - 1);
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                auto cCic = static_cast<OuterTrackerHybrid*>(theHybrid)->fCic;
                fCicInterface->SetSparsification(cCic, 0);
            }
        }
    }

    setSameDac("HitOr", 1);                                                                 // using logical OR
    setSameDac("TestPulsePotNodeSel", Cbc::convertMIPtoInjectedCharge(fCBCtestPulseValue)); // injected charge

    // Disable stub logic, as done in PedeNoise
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    if(cChip->getFrontEndType() == FrontEndType::CBC3)
                    {
                        LOG(INFO) << BOLDBLUE << "Chip Type = CBC3 - thus disabling Stub logic for pedestal and noise measurement." << RESET;
                        static_cast<CbcInterface*>(fReadoutChipInterface)->enableHipSuppression(cChip, false, true, 0);
                    }
                }
            }
        }
    }
}

void OTinjectionDelayOptimization::setLatencyAndDelay2S(uint16_t totalInjectionDelay)
{
    auto latencyAndDelay = calculateDACsFromTotalDelay(totalInjectionDelay, true);
    setSameDac("TriggerLatency", latencyAndDelay.first);
    setSameDac("TestPulseDelay", latencyAndDelay.second);
}

void OTinjectionDelayOptimization::prepareInjectionDelayScanPS()
{
    LOG(INFO) << BOLDBLUE << "OTinjectionDelayOptimization::injectionDelayScanPS - Scanning Delay for PS module" << RESET;

    setFWTestPulse(true);

    for(auto theBoard: *fDetectorContainer)
    {
        fBeBoardInterface->WriteBoardReg(theBoard, "fc7_daq_cnfg.fast_command_block.test_pulse.delay_after_test_pulse", fInitialLatency + 3);
        fBeBoardInterface->WriteBoardReg(theBoard, "fc7_daq_cnfg.fast_command_block.test_pulse.en_fast_reset", 1);
    }

    this->setTestAllChannels(true);
    this->setNormalization(true);

    auto        MPAqueryFunction          = [](const ChipContainer* theChip) { return (static_cast<const ReadoutChip*>(theChip)->getFrontEndType() == FrontEndType::MPA2); };
    std::string theMPAqueryFunctionString = "MPAqueryFunction";
    // settings for MPAs
    fDetectorContainer->addReadoutChipQueryFunction(MPAqueryFunction, theMPAqueryFunctionString);
    setSameDac("Control_1", 0x0);                                                       // set Readout mode to normal
    setSameDac("InjectedCharge", MPA2::convertMIPtoInjectedCharge(fMPAtestPulseValue)); // injected charge
    setSameDac("PixelControl_ALL", 0x1D);                                               // disable Hip cut, cluster cut to the maximum, mode select level
    setSameDac("ENFLAGS_ALL", 0x4E);                                                    // use level sampling mode and enable analog pulse
    setSameDac("DL_en", 0x7F);                                                          // enable injection delay on all bias blocks
    fDetectorContainer->removeReadoutChipQueryFunction(theMPAqueryFunctionString);

    auto        SSAqueryFunction          = [](const ChipContainer* theChip) { return (static_cast<const ReadoutChip*>(theChip)->getFrontEndType() == FrontEndType::SSA2); };
    std::string theSSAqueryFunctionString = "SSAqueryFunction";
    // settings for SSAs
    fDetectorContainer->addReadoutChipQueryFunction(SSAqueryFunction, theSSAqueryFunctionString);
    setSameDac("StripControl2", 0x07);                                                  // disable HIP cut
    setSameDac("InjectedCharge", SSA2::convertMIPtoInjectedCharge(fSSAtestPulseValue)); // injected charge
    setSameDac("ENFLAGS", 0x30);                                                        // use level sampling mode and enable analog pulse
    setSameDac("ReadoutMode", 0x0);                                                     // normal readout mode
    setSameDac("control_2", 0x0F);                                                      // maximize cluster cut
    setSameDac("CalPulse_duration", 1);                                                 // set calpulse duration to 1 40MHz clock cycle
    fDetectorContainer->removeReadoutChipQueryFunction(theSSAqueryFunctionString);

    // Enabling 1 every N columns and corresponding rows in a diagonal pattern
    ChannelGroup<NMPAROWS, NSSACHANNELS> theMPAChannelGroup;
    theMPAChannelGroup.disableAllChannels();
    uint16_t initialCol                 = 2;
    uint16_t colsToSkip                 = 20;
    uint16_t currentRow                 = 1;
    uint16_t rowsToSkip                 = 1;
    uint16_t totalNumberOfPixelClusters = 0;
    for(uint16_t col = initialCol; col < NSSACHANNELS; col += colsToSkip)
    {
        theMPAChannelGroup.enableChannel(currentRow % NMPAROWS, col);
        currentRow += rowsToSkip;
        ++totalNumberOfPixelClusters;
    }
    if(totalNumberOfPixelClusters > 127)
    {
        std::cerr << __PRETTY_FUNCTION__ << " [" << __LINE__ << "] Total number of pixel cluster exceeding CIC limit! Aborting..." << std::endl;
        abort();
    }

    MPAChannelGroupHandler theChannelGroupHandlerMPA;
    theChannelGroupHandlerMPA.setCustomChannelGroup(theMPAChannelGroup);
    theChannelGroupHandlerMPA.setChannelGroupParameters(1, NMPAROWS, NSSACHANNELS);
    setChannelGroupHandler(theChannelGroupHandlerMPA, FrontEndType::MPA2);

    // Enabling 1 every N columns
    ChannelGroup<1, NSSACHANNELS> theSSAChannelGroup;
    theSSAChannelGroup.disableAllChannels();
    uint16_t initialStrip               = 3;
    uint16_t stripsToSkip               = 20;
    uint16_t totalNumberOfStripClusters = 0;
    for(uint16_t col = initialStrip; col < NSSACHANNELS; col += stripsToSkip)
    {
        theSSAChannelGroup.enableChannel(0, col);
        ++totalNumberOfStripClusters;
    }
    if(totalNumberOfStripClusters > 127)
    {
        std::cerr << __PRETTY_FUNCTION__ << " [" << __LINE__ << "] Total number of strip cluster exceeding CIC limit! Aborting..." << std::endl;
        abort();
    }

    SSAChannelGroupHandler theChannelGroupHandlerSSA;
    theChannelGroupHandlerSSA.setCustomChannelGroup(theSSAChannelGroup);
    theChannelGroupHandlerSSA.setChannelGroupParameters(1, 1, NSSACHANNELS);
    setChannelGroupHandler(theChannelGroupHandlerSSA, FrontEndType::SSA2);

    for(auto theBoard: *fDetectorContainer)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                for(auto theChip: *theHybrid)
                {
                    auto theChannelGroupHandler = getChannelGroupHandlerContainer()
                                                      ->getChip(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId(), theChip->getId())
                                                      ->getSummary<std::shared_ptr<ChannelGroupHandler>>();

                    fReadoutChipInterface->maskChannelsAndSetInjectionSchema(theChip, theChannelGroupHandler->getTestGroup(-1), true, true);
                }
            }
        }
    }
}

void OTinjectionDelayOptimization::setLatencyAndDelayPS(uint16_t totalInjectionDelay)
{
    auto        MPAqueryFunction          = [](const ChipContainer* theChip) { return (static_cast<const ReadoutChip*>(theChip)->getFrontEndType() == FrontEndType::MPA2); };
    std::string theMPAqueryFunctionString = "MPAqueryFunction";

    auto        SSAqueryFunction          = [](const ChipContainer* theChip) { return (static_cast<const ReadoutChip*>(theChip)->getFrontEndType() == FrontEndType::SSA2); };
    std::string theSSAqueryFunctionString = "SSAqueryFunction";

    auto latencyAndDelay = calculateDACsFromTotalDelay(totalInjectionDelay, false);
    setSameDac("TriggerLatency", latencyAndDelay.first);

    // setting delay for SSAs
    fDetectorContainer->addReadoutChipQueryFunction(SSAqueryFunction, theSSAqueryFunctionString);
    setSameDac("Delay_line", latencyAndDelay.second + (1 << 7)); // in the SSA the MSB of the delay register need to be set to 1 to enable the delay
    fDetectorContainer->removeReadoutChipQueryFunction(theSSAqueryFunctionString);

    // setting delay for MPAs
    fDetectorContainer->addReadoutChipQueryFunction(MPAqueryFunction, theMPAqueryFunctionString);
    setSameDac("DL_ctrl", latencyAndDelay.second);
    fDetectorContainer->removeReadoutChipQueryFunction(theMPAqueryFunctionString);
}
