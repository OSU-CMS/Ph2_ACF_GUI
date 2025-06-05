#include "tools/OTMeasureOccupancy.h"
#include "HWDescription/Cbc.h"
#include "HWDescription/MPA2.h"
#include "HWDescription/SSA2.h"
#include "HWInterface/MPA2Interface.h"
#include "HWInterface/PSInterface.h"
#include "System/RegisterHelper.h"
#include "Utils/CBCChannelGroupHandler.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/MPAChannelGroupHandler.h"
#include "Utils/Occupancy.h"
#include "Utils/SSAChannelGroupHandler.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

std::string OTMeasureOccupancy::fCalibrationDescription = "Measure channel occupancy";

OTMeasureOccupancy::OTMeasureOccupancy() : Tool() {}

OTMeasureOccupancy::~OTMeasureOccupancy()
{
#ifdef __USE_ROOT__
    delete fDQMHistogramOTMeasureOccupancy;
    fDQMHistogramOTMeasureOccupancy = nullptr;
#endif
}

void OTMeasureOccupancy::Initialise(void)
{
    fRegisterHelper->takeSnapshot();
    // free the registers in case any

    fNumberOfEvents    = findValueInSettings<double>("OTMeasureOccupancy_NumberOfEvents", 10000);
    fCBCtestPulseValue = findValueInSettings<double>("OTMeasureOccupancy_CBCtestPulseValue", 1.);
    fSSAtestPulseValue = findValueInSettings<double>("OTMeasureOccupancy_SSAtestPulseValue", 1.);
    fMPAtestPulseValue = findValueInSettings<double>("OTMeasureOccupancy_MPAtestPulseValue", 1.);
    fForceChannelGroup = findValueInSettings<double>("OTMeasureOccupancy_ForceChannelGroup", 0) > 0;
    fThresholdOffset   = findValueInSettings<double>("OTMeasureOccupancy_ThresholdOffset", 0);

#ifdef __USE_ROOT__
    fDQMHistogramOTMeasureOccupancy = new DQMHistogramOTMeasureOccupancy();
    // Calibration is not running on the SoC: plots are booked during initialization
    fDQMHistogramOTMeasureOccupancy->book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif
}

void OTMeasureOccupancy::ConfigureCalibration() {}

void OTMeasureOccupancy::Running()
{
    LOG(INFO) << "Starting OTMeasureOccupancy measurement.";
    Initialise();
    measureChannelOccupancy();
    LOG(INFO) << "Done with OTMeasureOccupancy.";
    Reset();
}

void OTMeasureOccupancy::Stop(void)
{
    LOG(INFO) << "Stopping OTMeasureOccupancy measurement.";
#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: processing the histograms
    fDQMHistogramOTMeasureOccupancy->process();
#endif
    SaveResults();
    closeFileHandler();
    LOG(INFO) << "OTMeasureOccupancy stopped.";
}

void OTMeasureOccupancy::Pause() {}

void OTMeasureOccupancy::Resume() {}

void OTMeasureOccupancy::Reset() { fRegisterHelper->restoreSnapshot(); }

void OTMeasureOccupancy::measureChannelOccupancy(size_t iteration)
{
    bool is2SModule = fDetectorContainer->getFirstObject()->getFirstObject()->getFrontEndType() == FrontEndType::OuterTracker2S;
    this->setNormalization(true);

    if(is2SModule)
        prepareOccupancyMeasurement2S();
    else
        prepareOccupancyMeasurementPS();

    if(fThresholdOffset != 0) applyThresholdOffset();

    DetectorDataContainer theOccupancyContainer;
    ContainerFactory::copyAndInitStructure<Occupancy>(*fDetectorContainer, theOccupancyContainer);
    fDetectorDataContainer = &theOccupancyContainer;
    measureData(fNumberOfEvents, 65535);

#ifdef __USE_ROOT__
    fDQMHistogramOTMeasureOccupancy->fillOccupancy(theOccupancyContainer, iteration);
#else
    if(fDQMStreamerEnabled)
    {
        ContainerSerialization theOccupancyContainerSerialization("OTMeasureOccupancyOccupancy");
        theOccupancyContainerSerialization.streamByChipContainer(fDQMStreamer, theOccupancyContainer, iteration);
    }
#endif
}

void OTMeasureOccupancy::prepareOccupancyMeasurement2S()
{
    uint8_t calPulseValue = Cbc::convertMIPtoInjectedCharge(fCBCtestPulseValue);
    if(fCBCtestPulseValue > 0)
        LOG(INFO) << BOLDBLUE << "OTMeasureOccupancy::prepareOccupancyMeasurement2S - Preparing 2S to measure occupancy with injection = " << +calPulseValue << RESET;
    else
        LOG(INFO) << BOLDBLUE << "OTMeasureOccupancy::prepareOccupancyMeasurement2S - Preparing 2S to measure occupancy without injection" << RESET;

    CBCChannelGroupHandler theChannelGroupHandler;
    theChannelGroupHandler.setChannelGroupParameters(16, 1, 2);
    setChannelGroupHandler(theChannelGroupHandler);

    // Setting sparsification for simplicity
    for(auto theBoard: *fDetectorContainer)
    {
        fBeBoardInterface->WriteBoardReg(theBoard, "fc7_daq_cnfg.physical_interface_block.cic.2s_sparsified_enable", 0);
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                auto cCic = static_cast<OuterTrackerHybrid*>(theHybrid)->fCic;
                fCicInterface->SetSparsification(cCic, 0);
            }
        }
    }

    setSameDac("TestPulsePotNodeSel", calPulseValue); // injected charge
    bool injectPulse       = fCBCtestPulseValue != 0;
    bool injectAllChannels = !injectPulse;
    if(fForceChannelGroup) injectAllChannels = false;
    this->enableTestPulse(injectPulse);
    this->setTestAllChannels(injectAllChannels);
}

void OTMeasureOccupancy::prepareOccupancyMeasurementPS()
{
    uint8_t calPulseValueSSA = SSA2::convertMIPtoInjectedCharge(fSSAtestPulseValue);
    uint8_t calPulseValueMPA = MPA2::convertMIPtoInjectedCharge(fMPAtestPulseValue);

    LOG(INFO) << BOLDBLUE << "OTMeasureOccupancy::prepareOccupancyMeasurementPS - Preparing PS to measure occupancy with pixel injection = " << +calPulseValueMPA
              << " and strip injection = " << +calPulseValueSSA << RESET;

    SSAChannelGroupHandler theSSAChannelGroupHandler;
    theSSAChannelGroupHandler.setChannelGroupParameters(15, 1, 1);
    setChannelGroupHandler(theSSAChannelGroupHandler, FrontEndType::SSA2);

    MPAChannelGroupHandler theMPAChannelGroupHandler;
    theMPAChannelGroupHandler.setChannelGroupParameters(15, 1, 1);
    setChannelGroupHandler(theMPAChannelGroupHandler, FrontEndType::MPA2);

    bool injectSSApulse = calPulseValueSSA != 0;
    bool injectMPApulse = calPulseValueMPA != 0;

    auto        MPAqueryFunction          = [](const ChipContainer* theChip) { return (static_cast<const ReadoutChip*>(theChip)->getFrontEndType() == FrontEndType::MPA2); };
    std::string theMPAqueryFunctionString = "MPAqueryFunction";
    // settings for MPAs
    fDetectorContainer->addReadoutChipQueryFunction(MPAqueryFunction, theMPAqueryFunctionString);
    auto thePSinterface = static_cast<PSInterface*>(fReadoutChipInterface)->fTheMPA2Interface;
    for(auto theBoard: *fDetectorContainer)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                for(auto theMPA: *theHybrid) { thePSinterface->WriteChipRegBits(theMPA, "Control_1", 0x0, "Mask", 0x03); }
            }
        }
    }
    setSameDac("PixelControl_ALL", 0x1E);           // disable Hip cut, cluster cut to the maximum, mode select to or
    setSameDac("ENFLAGS_ALL", 0x0F);                // Enable all channels
    setSameDac("InjectedCharge", calPulseValueMPA); // injected charge
    fDetectorContainer->removeReadoutChipQueryFunction(theMPAqueryFunctionString);

    auto        SSAqueryFunction          = [](const ChipContainer* theChip) { return (static_cast<const ReadoutChip*>(theChip)->getFrontEndType() == FrontEndType::SSA2); };
    std::string theSSAqueryFunctionString = "SSAqueryFunction";
    // settings for SSAs
    fDetectorContainer->addReadoutChipQueryFunction(SSAqueryFunction, theSSAqueryFunctionString);
    setSameDac("InjectedCharge", calPulseValueSSA); // injected charge
    setSameDac("ReadoutMode", 0x0);                 // normal readout mode
    setSameDac("StripControl2", 0x07);              // disable HIP cut
    setSameDac("control_2", 0x1F);                  // maximize cluster cut and set calpulse duration to 1 40MHz clock cycle
    setSameDac("ENFLAGS", 0x41);                    // use level sampling mode
    fDetectorContainer->removeReadoutChipQueryFunction(theSSAqueryFunctionString);

    bool injectPulse       = injectSSApulse || injectMPApulse;
    bool injectAllChannels = !injectPulse;
    if(fForceChannelGroup) injectAllChannels = false;
    setFWTestPulse(injectPulse);
    this->setTestAllChannels(injectAllChannels);
    this->fMaskChannelsFromOtherGroups = true;
}

void OTMeasureOccupancy::applyThresholdOffset()
{
    LOG(INFO) << BOLDBLUE << "Adding " << fThresholdOffset << " to the threshold to all chips" << RESET;
    auto calculateNewThreshold = [this](ReadoutChip* theChip, uint16_t currentThreshold, uint16_t thresholdLimit, bool isUpperLimit) -> uint16_t
    {
        bool isBeyondLimit = false;
        if(isUpperLimit)
            isBeyondLimit = (currentThreshold + this->fThresholdOffset) > thresholdLimit;
        else
            isBeyondLimit = currentThreshold < abs(fThresholdOffset);
        if(isBeyondLimit)
        {
            LOG(WARNING) << BOLDYELLOW << "Impossible to " << (isUpperLimit ? "increase" : "decrease") << " threshold " << (isUpperLimit ? "above" : "below") << " " << thresholdLimit << " for Board "
                         << +theChip->getBeBoardId() << " OpticalGroup " << +theChip->getOpticalGroupId() << " Hybrid " << +theChip->getHybridId() << " Chip " << theChip->getId()
                         << ", setting threshold to " << thresholdLimit << RESET;
            return thresholdLimit;
        }
        return currentThreshold + fThresholdOffset;
    };

    for(auto theBoard: *fDetectorContainer)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                for(auto theChip: *theHybrid)
                {
                    uint32_t theCurrentThreshold = fReadoutChipInterface->ReadChipReg(theChip, "Threshold");

                    uint16_t theNewThreshold;
                    bool     isThresholdIncreased = (fThresholdOffset > 0);

                    if(isThresholdIncreased)
                    {
                        if(theChip->getFrontEndType() == FrontEndType::CBC3)
                            theNewThreshold = calculateNewThreshold(theChip, theCurrentThreshold, 1023, isThresholdIncreased);
                        else
                            theNewThreshold = calculateNewThreshold(theChip, theCurrentThreshold, 255, isThresholdIncreased);
                    }
                    else { theNewThreshold = calculateNewThreshold(theChip, theCurrentThreshold, 0, isThresholdIncreased); }

                    fReadoutChipInterface->WriteChipReg(theChip, "Threshold", theNewThreshold);
                }
            }
        }
    }
}
