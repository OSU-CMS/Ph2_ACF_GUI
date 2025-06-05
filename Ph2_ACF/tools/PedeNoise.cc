#include "tools/PedeNoise.h"
#include "HWDescription/BeBoardRegItem.h"
#include "HWDescription/Cbc.h"
#include "HWInterface/D19cFWInterface.h"
#include "HWInterface/D19cPSCounterFWInterface.h"
#include "System/RegisterHelper.h"
#include "Utils/CBCChannelGroupHandler.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/EmptyContainer.h"
#include "Utils/MPAChannelGroupHandler.h"
#include "Utils/Occupancy.h"
#include "Utils/SSAChannelGroupHandler.h"
#include "Utils/ThresholdAndNoise.h"
// #include "boost/format.hpp"
#include <math.h>

#ifdef __USE_ROOT__
#include "DQMUtils/DQMHistogramPedeNoise.h"
#endif

std::string PedeNoise::fCalibrationDescription = "Measure noise and Pedestal/pulse peak";

PedeNoise::PedeNoise() : Tool() {}

PedeNoise::~PedeNoise() { clearDataMembers(); }

void PedeNoise::cleanContainerVector()
{
    for(auto container: fSCurveStripOccupancyMap) fRecycleBin.free(container.second);
    for(auto container: fSCurvePixelOccupancyMap) fRecycleBin.free(container.second);
    fSCurveStripOccupancyMap.clear();
    fSCurvePixelOccupancyMap.clear();
}

void PedeNoise::clearDataMembers()
{
    return;
    // delete fBoardRegContainer;
    delete fThresholdAndNoiseContainer;
    if(fDisableStubLogic)
    {
        delete fStubLogicValue;
        delete fHIPCountValue;
    }
    cleanContainerVector();
}

void PedeNoise::Initialise(bool pAllChan, bool pDisableStubLogic)
{
    fRegisterHelper->takeSnapshot();
    fRegisterHelper->freeFrontEndRegister(FrontEndType::CBC3, "^VCth\\d$");
    fRegisterHelper->freeFrontEndRegister(FrontEndType::CBC3, "^MaskChannel-\\d{3}-to-\\d{3}$");
    fRegisterHelper->freeFrontEndRegister(FrontEndType::MPA2, "^ThDAC\\d$");
    fRegisterHelper->freeFrontEndRegister(FrontEndType::MPA2, "^ENFLAGS_C\\d+_R\\d+$");
    fRegisterHelper->freeFrontEndRegister(FrontEndType::SSA2, "^Bias_THDAC$");
    fRegisterHelper->freeFrontEndRegister(FrontEndType::SSA2, "^ENFLAG_S\\d+$");

    for(auto cBoard: *fDetectorContainer)
    {
        BeBoardRegMap cRegMap      = cBoard->getBeBoardRegMap();
        uint32_t      cTriggerFreq = cRegMap["fc7_daq_cnfg.fast_command_block.user_trigger_frequency"].fValue;

        std::vector<std::pair<std::string, uint32_t>> cRegVec;
        cRegVec.clear();
        cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.user_trigger_frequency", cTriggerFreq});
        cRegVec.push_back({"fc7_daq_ctrl.fast_command_block.control.load_config", 0x1});
        fBeBoardInterface->WriteBoardMultReg(cBoard, cRegVec);
        LOG(INFO) << BOLDYELLOW << "Noise measured on BeBoard#" << +cBoard->getId() << " with a trigger rate of " << cTriggerFreq << "kHz." << RESET;
    }
    fDisableStubLogic = pDisableStubLogic;

    fWithCBC = false;
    fWithSSA = false;
    fWithMPA = false;
    std::vector<FrontEndType> cAllFrontEndTypes;
    for(auto cBoard: *fDetectorContainer)
    {
        auto cFrontEndTypes = cBoard->connectedFrontEndTypes();
        fWithCBC            = std::find(cFrontEndTypes.begin(), cFrontEndTypes.end(), FrontEndType::CBC3) != cFrontEndTypes.end();
        fWithSSA            = std::find(cFrontEndTypes.begin(), cFrontEndTypes.end(), FrontEndType::SSA2) != cFrontEndTypes.end();
        fWithMPA            = std::find(cFrontEndTypes.begin(), cFrontEndTypes.end(), FrontEndType::MPA2) != cFrontEndTypes.end();
        for(auto cFrontEndType: cFrontEndTypes)
        {
            if(std::find(cAllFrontEndTypes.begin(), cAllFrontEndTypes.end(), cFrontEndType) == cAllFrontEndTypes.end()) cAllFrontEndTypes.push_back(cFrontEndType);
        }
    }
    if(fWithCBC) LOG(INFO) << BOLDBLUE << "PedeNoise with CBCs" << RESET;
    if(fWithSSA && !fWithMPA) LOG(INFO) << BOLDBLUE << "PedeNoise with SSAs" << RESET;
    if(fWithMPA && !fWithSSA) LOG(INFO) << BOLDBLUE << "PedeNoise with MPAs" << RESET;
    if(fWithSSA && fWithMPA) LOG(INFO) << BOLDBLUE << "PedeNoise with SSAs+MPAs" << RESET;

    for(auto cFrontEndType: cAllFrontEndTypes)
    {
        if(cFrontEndType == FrontEndType::CBC3)
        {
            CBCChannelGroupHandler theChannelGroupHandler;
            theChannelGroupHandler.setChannelGroupParameters(16, 1, 2); // 16*2*8
            setChannelGroupHandler(theChannelGroupHandler);
        }
        else if(cFrontEndType == FrontEndType::SSA2)
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

    initializeRecycleBin();

    fAllChan = pAllChan;

    fSkipMaskedChannels          = findValueInSettings<double>("SkipMaskedChannels", 0);
    fMaskChannelsFromOtherGroups = findValueInSettings<double>("MaskChannelsFromOtherGroups", 1);
    fPlotSCurves                 = findValueInSettings<double>("PlotSCurves", 0);
    fFitSCurves                  = findValueInSettings<double>("FitSCurves", 0);
    fPulseAmplitude              = findValueInSettings<double>("PedeNoise_PulseAmplitude", 0);
    fPulseAmplitudePix           = findValueInSettings<double>("PedeNoise_PulseAmplitudePix", fPulseAmplitude);
    LOG(INFO) << BOLDMAGENTA << " Strip injection pulse height = " << +fPulseAmplitude << RESET;
    if(fWithMPA) LOG(INFO) << BOLDMAGENTA << " Pixel injection pulse height = " << +fPulseAmplitudePix << RESET;
    fPedeNoiseLimit          = findValueInSettings<double>("PedeNoiseLimit", 10); // NOT IN XML
    fPedeNoiseMask           = findValueInSettings<double>("PedeNoiseMask", 0);   // NOT IN XML
    fPedeNoiseMaskUntrimmed  = findValueInSettings<double>("PedeNoise_MaskUntrimmed", 0);
    fPedeNoiseUntrimmedLimit = findValueInSettings<double>("PedeNoise_UntrimmedLimit", 0.0);
    fEventsPerPoint          = findValueInSettings<double>("Nevents", 10);
    fUseFixRange             = findValueInSettings<double>("PedeNoise_UseFixRange", 0);
    fMinThreshold            = findValueInSettings<double>("PedeNoise_MinThreshold", 0);
    fMaxThreshold            = findValueInSettings<double>("PedeNoise_MaxThreshold", 0);
    fNeventsForValidation    = findValueInSettings<double>("NeventsForValidation", 10000); // NOT IN XML
    fMaskingThreshold        = findValueInSettings<double>("MaskingThreshold", 0.001);     // NOT IN XML
    fPedeNoiseLatency        = findValueInSettings<double>("PedeNoiseLatency", 198);

    bool fastCounterReadout = findValueInSettings<double>("PedeNoise_FastCounterReadout", 1) > 0;

    fNEventsPerBurst = (fEventsPerPoint >= fMaxNevents) ? fMaxNevents : fEventsPerPoint;
    // uint8_t cEnableFastCounterReadout = (uint8_t)findValueInSettings<double>("EnableFastCounterReadout", 0);
    // uint8_t cEnablePairSelect         = (uint8_t)findValueInSettings<double>("EnablePairSelect", 0);
    LOG(INFO) << "Parsed settings:";
    LOG(INFO) << " Nevents = " << fEventsPerPoint;
    // LOG(INFO) << " Fast Counter Readout [PS] " << +cEnableFastCounterReadout << RESET;
    this->SetSkipMaskedChannels(fSkipMaskedChannels);
    if(fFitSCurves) fPlotSCurves = true;

    ContainerFactory::copyAndInitBoard<BeBoardRegMap>(*fDetectorContainer, fBoardRegContainer);
    for(auto cBoard: *fDetectorContainer)
    {
        auto&                cBoardRegNap = fBoardRegContainer.getObject(cBoard->getId())->getSummary<BeBoardRegMap>();
        const BeBoardRegMap& cOrigRegMap  = static_cast<const BeBoard*>(cBoard)->getBeBoardRegMap();
        cBoardRegNap.insert(cOrigRegMap.begin(), cOrigRegMap.end());
    }

    // for now.. force to use async mode here
    bool cForcePSasync = true;
    // event types
    ContainerFactory::copyAndInitBoard<EventType>(*fDetectorContainer, fEventTypes);
    for(auto cBoard: *fDetectorContainer)
    {
        fEventTypes.getObject(cBoard->getId())->getSummary<EventType>() = cBoard->getEventType();
        if(!fWithSSA && !fWithMPA) continue;
        if(!cForcePSasync) continue;
        cBoard->setEventType(EventType::PSAS);
        static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(cBoard))->InitializePSCounterFWInterface(cBoard);
        static_cast<D19cPSCounterFWInterface*>(static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(cBoard))->getL1ReadoutInterface())->configureFastReadout(fastCounterReadout);

        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid) { fReadoutChipInterface->WriteChipReg(cChip, "AnalogueAsync", 1); }
            }
        }
    }
#ifdef __USE_ROOT__
    fDQMHistogramPedeNoise.book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif
}

void PedeNoise::Reset()
{
    for(auto cBoard: *fDetectorContainer)
    {
        auto theEventType = fEventTypes.getObject(cBoard->getId())->getSummary<EventType>();
        cBoard->setEventType(theEventType);
        if(theEventType != EventType::PSAS) { static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(cBoard))->InitalizeL1ReadoutInterface(cBoard); }
    }
    fRegisterHelper->restoreSnapshot();
    resetPointers();
}
void PedeNoise::disableStubLogic()
{
    fStubLogicValue = new DetectorDataContainer();
    fHIPCountValue  = new DetectorDataContainer();
    ContainerFactory::copyAndInitChip<uint16_t>(*fDetectorContainer, *fStubLogicValue);
    ContainerFactory::copyAndInitChip<uint16_t>(*fDetectorContainer, *fHIPCountValue);

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
                        fStubLogicValue->getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() =
                            fReadoutChipInterface->ReadChipReg(static_cast<ReadoutChip*>(cChip), "Pipe&StubInpSel&Ptwidth");
                        fHIPCountValue->getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<uint16_t>() =
                            fReadoutChipInterface->ReadChipReg(static_cast<ReadoutChip*>(cChip), "HIP&TestMode");
                        // fReadoutChipInterface->WriteChipReg(static_cast<ReadoutChip*>(cChip), "Pipe&StubInpSel&Ptwidth", 0x23);
                        // fReadoutChipInterface->WriteChipReg(static_cast<ReadoutChip*>(cChip), "HIP&TestMode", 0x00);
                    }
                }
            }
        }
    }
}

void PedeNoise::sweepSCurves()
{
    bool originalAllChannelFlag = this->fAllChan;

    if(fPulseAmplitude != 0 && originalAllChannelFlag && fWithCBC)
    {
        this->setTestAllChannels(false);
        LOG(INFO) << RED << "Cannot inject pulse for all channels, test in groups enabled. " << RESET;
    }

    // configure TP amplitude
    for(auto cBoard: *fDetectorContainer)
    {
        if(fWithSSA || fWithMPA)
        {
            // Allow for different SSA and MPA injection amplitudes
            // setSameDacBeBoard(static_cast<BeBoard*>(cBoard), "InjectedCharge", fTestPulseAmplitude);
            for(auto cOpticalGroup: *cBoard)
            {
                for(auto cHybrid: *cOpticalGroup)
                {
                    for(auto cChip: *cHybrid)
                    {
                        auto cType = cChip->getFrontEndType();

                        if(cType == FrontEndType::MPA2) { fReadoutChipInterface->WriteChipReg(cChip, "InjectedCharge", fPulseAmplitudePix); }
                        else { fReadoutChipInterface->WriteChipReg(cChip, "InjectedCharge", fPulseAmplitude); }
                    }
                }
            }
        }

        else
            setSameDacBeBoard(static_cast<BeBoard*>(cBoard), "TestPulsePotNodeSel", fPulseAmplitude);

        setSameDacBeBoard(static_cast<BeBoard*>(cBoard), "TriggerLatency", fPedeNoiseLatency);
    }

    bool forceAllChannels = false;
    if(fPulseAmplitude != 0)
    {
        LOG(INFO) << BOLDYELLOW << "Enabled test pulse. " << RESET;
        this->enableTestPulse(true);
    }
    else
    {
        LOG(INFO) << BOLDYELLOW << "sweepSCurves without TP injection" << RESET;
        this->enableTestPulse(false);
        forceAllChannels = true;
    }

    uint16_t cStripStartValue = 0, cPixelStartValue = 0;
    if(!fUseFixRange)
    {
        this->findPedestal(forceAllChannels);
        cStripStartValue = fMeanStrips;
        cPixelStartValue = fMeanPixels;
    }
    else
    {
        cStripStartValue = (fMaxThreshold + fMinThreshold) / 2.;
        cPixelStartValue = (fMaxThreshold + fMinThreshold) / 2.;
    }
    if(fDisableStubLogic) disableStubLogic();
    if(fWithCBC || fWithSSA) LOG(INFO) << BLUE << "Sweep of Strip S-curves will start at an average threshold of " << cStripStartValue << RESET;
    if(fWithMPA) LOG(INFO) << MAGENTA << "Sweep of Pixel S-curves will start at an average threshold of " << cPixelStartValue << RESET;

    measureSCurves(cStripStartValue, cPixelStartValue);
    this->setTestAllChannels(originalAllChannelFlag);
    LOG(INFO) << BOLDBLUE << "Finished sweeping SCurves..." << RESET;
    return;
}

void PedeNoise::measureNoise()
{
    LOG(INFO) << BOLDBLUE << "sweepSCurves" << RESET;
    sweepSCurves();
    LOG(INFO) << BOLDBLUE << "extractPedeNoise" << RESET;
    extractPedeNoise();
    LOG(INFO) << BOLDBLUE << "producePedeNoisePlots" << RESET;
    producePedeNoisePlots();
    LOG(INFO) << BOLDBLUE << "Done" << RESET;
}

void PedeNoise::Validate()
{
    LOG(INFO) << "Validation: Taking Data with " << fNeventsForValidation << " random triggers!";

    for(auto cBoard: *fDetectorContainer)
    {
        // increase threshold to supress noise
        setThresholdtoNSigma(cBoard, 5);
    }

    for(auto board: *fThresholdAndNoiseContainer) { maskNoisyChannels(board); }

    DetectorDataContainer theOccupancyContainer;
    fDetectorDataContainer = &theOccupancyContainer;
    ContainerFactory::copyAndInitStructure<Occupancy>(*fDetectorContainer, *fDetectorDataContainer);
    bool originalAllChannelFlag = this->fAllChan;

    LOG(INFO) << "Setting all channels";
    this->setTestAllChannels(true);
    LOG(INFO) << "measuring with " << fNeventsForValidation << " events and " << fMaxNevents << " per burst";
    this->measureData(fNeventsForValidation, fMaxNevents);
    LOG(INFO) << "setting al channels v2";
    this->setTestAllChannels(originalAllChannelFlag);
    // now Occupancy measured in OTinjectionOccupancyScan
    // #ifdef __USE_ROOT__
    //     fDQMHistogramPedeNoise.fillValidationPlots(theOccupancyContainer);
    // #else
    //     if(fDQMStreamerEnabled)
    //     {
    //         ContainerSerialization theContainerSerialization("PedeNoiseValidation");
    //         theContainerSerialization.streamByHybridContainer(fDQMStreamer, theOccupancyContainer);
    //     }
    // #endif
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    RegisterVector cRegVec;
                    for(uint16_t row = 0; row < cChip->getNumberOfRows(); ++row)
                    {
                        for(uint16_t col = 0; col < cChip->getNumberOfCols(); ++col)
                        {
                            float occupancy = theOccupancyContainer.getObject(cBoard->getId())
                                                  ->getObject(cOpticalGroup->getId())
                                                  ->getObject(cHybrid->getId())
                                                  ->getObject(cChip->getId())
                                                  ->getChannel<Occupancy>(row, col)
                                                  .fOccupancy;
                            if(occupancy > fMaskingThreshold)
                            {
                                std::string message = "Found a noisy channel on Chip " + getReadoutChipString(cBoard->getId(), cOpticalGroup->getId(), cHybrid->getId(), cChip->getId()) + " row " +
                                                      std::to_string(row) + " col " + std::to_string(col) + " with an occupancy of " + std::to_string(occupancy) + "(>" +
                                                      std::to_string(fMaskingThreshold) + ")";
                                if(fMaskNoisyChannels)
                                {
                                    if(fWithCBC)
                                    {
                                        std::ostringstream oss;
                                        oss << "Channel" << std::setw(3) << std::setfill('0') << (col + 1);
                                        std::string cRegName = oss.str();
                                        cRegVec.push_back({cRegName, 0xFF});
                                    }
                                    if(cChip->getFrontEndType() == FrontEndType::SSA2)
                                    {
                                        std::string cRegName = "THTRIMMING_S" + std::to_string(col + 1);
                                        cRegVec.push_back({cRegName, 0x1F});
                                    }
                                    if(cChip->getFrontEndType() == FrontEndType::MPA2)
                                    {
                                        std::string cRegName = "TrimDAC_C" + std::to_string(col) + "_R" + std::to_string(row);
                                        cRegVec.push_back({cRegName, 0x1F});
                                    }
                                    message += ";  setting offset to 255";
                                }
                                LOG(DEBUG) << RED << message << RESET;
                            }
                        }
                    }

                    fReadoutChipInterface->WriteChipMultReg(cChip, cRegVec);
                }
            }
        }
    }
}

void PedeNoise::findPedestal(bool forceAllChannels)
{
    bool originalAllChannelFlag = this->fAllChan;
    if(forceAllChannels) this->setTestAllChannels(true);

    // // figure  out if you should normalize or not
    uint8_t cNormalizationOrig = getNormalization();
    // uint8_t cNormalize         = 0;
    // if(fWithCBC or (fWithSSA && !fWithMPA) or (fWithMPA && !fWithSSA)) { cNormalize = 1; }
    // LOG(INFO) << BOLDBLUE << "normalization will be set to " << +cNormalize << RESET;
    // setNormalization(cNormalize);

    DetectorDataContainer theOccupancyContainer;
    fDetectorDataContainer = &theOccupancyContainer;
    ContainerFactory::copyAndInitStructure<Occupancy>(*fDetectorContainer, *fDetectorDataContainer);
    this->bitWiseScan("Threshold", fEventsPerPoint, 0.56, fNEventsPerBurst);
    if(forceAllChannels) this->setTestAllChannels(originalAllChannelFlag);

    uint8_t cNStripChips = 0, cNPixelChips = 0;
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    uint16_t tmpVthr = 0;
                    if(cChip->getFrontEndType() == FrontEndType::CBC3)
                    {
                        tmpVthr = (static_cast<ReadoutChip*>(cChip)->getReg("VCth1") + (static_cast<ReadoutChip*>(cChip)->getReg("VCth2") << 8));
                        fMeanStrips += tmpVthr;
                        cNStripChips++;
                    }
                    if(cChip->getFrontEndType() == FrontEndType::SSA2)
                    {
                        tmpVthr = static_cast<ReadoutChip*>(cChip)->getReg("Bias_THDAC");
                        fMeanStrips += tmpVthr;
                        cNStripChips++;
                    }
                    if(cChip->getFrontEndType() == FrontEndType::MPA2)
                    {
                        tmpVthr = static_cast<ReadoutChip*>(cChip)->getReg("ThDAC0");
                        fMeanPixels += tmpVthr;
                        cNPixelChips++;
                    }
                }
            }
        }
    }
    fMeanStrips = (cNStripChips > 0) ? fMeanStrips / cNStripChips : 0xFF / 2;
    fMeanPixels = (cNPixelChips > 0) ? fMeanPixels / cNPixelChips : 0xFF / 2;
    if(fWithCBC || fWithSSA) LOG(INFO) << BOLDBLUE << "Found Pedestals on Strip ASICs to be around " << fMeanStrips << RESET;
    if(fWithMPA) LOG(INFO) << BOLDMAGENTA << "Found Pedestals on Pixel ASICs to be around " << fMeanPixels << RESET;
    setNormalization(cNormalizationOrig);
}

void PedeNoise::measureSCurves(uint16_t pStripStartValue, uint16_t pPixelStartValue)
{
    auto cChannels     = findValueInSettings<double>("NoiseMeasurementLimit", 1);
    auto cLowerLimitTh = findValueInSettings<double>("PedeNoise_MinThreshold", 0);
    auto cUpperLimitTh = findValueInSettings<double>("PedeNoise_MaxThreshold", 0);
    if(fUseFixRange) LOG(INFO) << BOLDYELLOW << "Scan should be between " << cLowerLimitTh << " and " << cUpperLimitTh << " DAC units" << RESET;

    // adding limit to define what all one and all zero actually mean.. avoid waiting forever during scan!
    float    cMaxOccupancy  = 1.0;
    float    cLimit         = cChannels / (100.);
    int      cMinBreakCount = 10;
    uint16_t cStripValue    = pStripStartValue;
    uint16_t cPixelValue    = pPixelStartValue;
    uint16_t cMaxValue      = (1 << 10) - 1;
    if(fWithSSA || fWithMPA) cMaxValue = (1 << 8) - 1;
    float              cFirstLimit = (fWithCBC) ? 0 : 1;
    std::vector<int>   cSigns{-1, 1};
    std::vector<float> cLimits{cFirstLimit, 1 - cFirstLimit};

    int cCounter = 0;
    for(auto cSign: cSigns)
    {
        bool cStripFirstLim = false, cPixelFirstLim = false;
        bool cLimitFound = false, cStripLimitFound = false, cPixelLimitFound = false;
        int  cStripLimitCounter = 0, cPixelLimitCounter = 0;
        do {
            DetectorDataContainer* theOccupancyContainer = fRecycleBin.get(&ContainerFactory::copyAndInitStructure<Occupancy>, Occupancy());
            fDetectorDataContainer                       = theOccupancyContainer;
            fSCurvePixelOccupancyMap[cPixelValue]        = theOccupancyContainer;
            fSCurveStripOccupancyMap[cStripValue]        = theOccupancyContainer;

            for(auto cBoard: *fDetectorContainer)
            {
                for(auto cOpticalGroup: *cBoard)
                {
                    for(auto cHybrid: *cOpticalGroup)
                    {
                        for(auto cChip: *cHybrid)
                        {
                            auto cType = cChip->getFrontEndType();
                            if(cType == FrontEndType::CBC3 || cType == FrontEndType::SSA2)
                                fReadoutChipInterface->WriteChipReg(cChip, "Threshold", cStripValue);
                            else if(cType == FrontEndType::MPA2)
                                fReadoutChipInterface->WriteChipReg(cChip, "Threshold", cPixelValue);
                        }
                    }
                }
            }
            this->measureData(fEventsPerPoint, fNEventsPerBurst);

            // Retrieve occupancy for strip and pixel chips
            theOccupancyContainer->normalizeAndAverageContainers(fDetectorContainer, getChannelGroupHandlerContainer(), fEventsPerPoint);
            float   cStripGlobalOccupancy = 0, cPixelGlobalOccupancy = 0;
            uint8_t cNStripChips = 0, cNPixelChips = 0;
            for(auto cBoard: *fDetectorContainer)
            {
                // std::cout << GREEN << "Reading back from Board fc7_daq_cnfg.fast_command_block.trigger_source = " << fBeBoardInterface->ReadBoardReg(cBoard,
                // "fc7_daq_cnfg.fast_command_block.trigger_source") << RESET << std::endl; std::cout << GREEN << "Reading back from Board fc7_daq_cnfg.fast_command_block.delay_after_test_pulse = " <<
                // fBeBoardInterface->ReadBoardReg(cBoard, "fc7_daq_cnfg.fast_command_block.test_pulse.delay_after_test_pulse") << RESET << std::endl; std::cout << GREEN << "Reading back from Board
                // fc7_daq_cnfg.fast_command_block.en_test_pulse = " << fBeBoardInterface->ReadBoardReg(cBoard, "fc7_daq_cnfg.fast_command_block.test_pulse.en_test_pulse") << RESET << std::endl;
                // std::cout << GREEN << "Reading TriggerLatency from CBC = " << fReadoutChipInterface->ReadChipReg(cBoard->getFirstObject()->getFirstObject()->getFirstObject(), "TriggerLatency") <<
                // RESET << std::endl; std::cout << GREEN << "Reading TestPulsePotNodeSel from CBC = " <<
                // fReadoutChipInterface->ReadChipReg(cBoard->getFirstObject()->getFirstObject()->getFirstObject(), "TestPulsePotNodeSel") << RESET << std::endl; std::cout << GREEN << "Reading
                // TestPulse from CBC = " << fReadoutChipInterface->ReadChipReg(cBoard->getFirstObject()->getFirstObject()->getFirstObject(), "TestPulse") << RESET << std::endl;
                auto cBoardIdx = cBoard->getId();
                for(auto cOpticalGroup: *cBoard)
                {
                    auto cOpticalGroupIdx = cOpticalGroup->getId();
                    for(auto cHybrid: *cOpticalGroup)
                    {
                        auto cHybridIdx = cHybrid->getId();
                        for(auto cChip: *cHybrid)
                        {
                            auto cChipIdx = cChip->getId();
                            auto cType    = cChip->getFrontEndType();
                            auto cChipOccupancy =
                                theOccupancyContainer->getObject(cBoardIdx)->getObject(cOpticalGroupIdx)->getObject(cHybridIdx)->getObject(cChipIdx)->getSummary<Occupancy, Occupancy>().fOccupancy;
                            if(cType == FrontEndType::CBC3 || cType == FrontEndType::SSA2)
                            {
                                cNStripChips++;
                                cStripGlobalOccupancy += cChipOccupancy;
                            }
                            else if(cType == FrontEndType::MPA2)
                            {
                                cNPixelChips++;
                                cPixelGlobalOccupancy += cChipOccupancy;
                            }
                        }
                    }
                }
            }
            cStripGlobalOccupancy /= cNStripChips;
            cPixelGlobalOccupancy /= cNPixelChips;

#ifdef __USE_ROOT__
            if(fPlotSCurves) fDQMHistogramPedeNoise.fillSCurvePlots(cStripValue, cPixelValue, *theOccupancyContainer);
#else
            if(fDQMStreamerEnabled)
            {
                if(fPlotSCurves)
                {
                    ContainerSerialization theContainerSerialization("PedeNoiseSCurve");
                    theContainerSerialization.streamByHybridContainer(fDQMStreamer, *theOccupancyContainer, cStripValue, cPixelValue);
                }
            }
#endif

            auto cStripDistanceFromTarget = std::fabs(std::min(cStripGlobalOccupancy, cMaxOccupancy) - (cLimits.at(cCounter)));
            auto cPixelDistanceFromTarget = std::fabs(std::min(cPixelGlobalOccupancy, cMaxOccupancy) - (cLimits.at(cCounter)));

            if(fWithCBC || fWithSSA)
            {
                LOG(INFO) << BOLDBLUE << "Strip Threshold =  " << +cStripValue << " -- Occupancy = " << std::setprecision(2) << std::fixed << +cStripGlobalOccupancy
                          << " -- Distance from target = " << cStripDistanceFromTarget * 100 << "\t..Incrementing limit found counter "
                          << " -- current value is " << +cStripLimitCounter << RESET;
            }
            if(fWithMPA)
            {
                LOG(INFO) << BOLDMAGENTA << "Pixel Threshold =  " << +cPixelValue << " -- Occupancy = " << std::setprecision(2) << std::fixed << +cPixelGlobalOccupancy
                          << " -- Distance from target = " << cPixelDistanceFromTarget * 100 << "\t..Incrementing limit found counter "
                          << " -- current value is " << +cPixelLimitCounter << RESET;
            }

            if(cStripDistanceFromTarget <= cLimit || cStripFirstLim) // || globalOccupancy>1.0)
            {
                cStripFirstLim = true;
                cStripLimitCounter++;
            }
            if(cPixelDistanceFromTarget <= cLimit || cPixelFirstLim) // || globalOccupancy>1.0)
            {
                cPixelFirstLim = true;
                cPixelLimitCounter++;
            }

            if(!cStripLimitFound) cStripValue += cSign;
            if(!cPixelLimitFound) cPixelValue += cSign;
            if(!fUseFixRange)
            {
                if(!fWithMPA && (fWithSSA || fWithCBC))
                {
                    cStripLimitFound = (cStripValue == 0 || cStripValue >= cMaxValue) || (cStripLimitCounter >= cMinBreakCount);
                    cLimitFound      = cStripLimitFound;
                }
                else if(fWithMPA && !fWithSSA)
                {
                    cPixelLimitFound = (cPixelValue == 0 || cPixelValue >= cMaxValue) || (cPixelLimitCounter >= cMinBreakCount);
                    cLimitFound      = cPixelLimitFound;
                }
                else if(fWithSSA && fWithMPA)
                {
                    cStripLimitFound = (cStripValue == 0 || cStripValue >= cMaxValue) || (cStripLimitCounter >= cMinBreakCount);
                    cPixelLimitFound = (cPixelValue == 0 || cPixelValue >= cMaxValue) || (cPixelLimitCounter >= cMinBreakCount);
                    cLimitFound      = cStripLimitFound && cPixelLimitFound;
                }
                if(cLimitFound) { LOG(INFO) << BOLDYELLOW << "Switching sign during auto scan .." << RESET; }
            }
            else
            {
                cStripLimitFound = (cSign < 0) ? (cStripValue == cLowerLimitTh) : (cStripValue == cUpperLimitTh);
                cPixelLimitFound = (cSign < 0) ? (cPixelValue == cLowerLimitTh) : (cPixelValue == cUpperLimitTh);

                if(!fWithMPA && (fWithSSA || fWithCBC))
                    cLimitFound = cStripLimitFound;
                else if(fWithMPA && !fWithSSA)
                    cLimitFound = cPixelLimitFound;
                else if(fWithSSA && fWithMPA)
                    cLimitFound = cStripLimitFound && cPixelLimitFound;

                if(cLimitFound) { LOG(INFO) << BOLDYELLOW << "Switching sign because threshold limit was reached .." << RESET; }
            }
        } while(!cLimitFound);

        cCounter++;
        cStripValue = pStripStartValue + cSign;
        cPixelValue = pPixelStartValue + cSign;
    }

    LOG(DEBUG) << YELLOW << "Found minimal and maximal occupancy " << cMinBreakCount << " times, SCurves finished! " << RESET;
}
void PedeNoise::extractPedeNoise()
{
    fThresholdAndNoiseContainer = new DetectorDataContainer();
    ContainerFactory::copyAndInitStructure<ThresholdAndNoise>(*fDetectorContainer, *fThresholdAndNoiseContainer);

    std::map<uint16_t, DetectorDataContainer*>::reverse_iterator previousStripIterator = fSCurveStripOccupancyMap.rend();
    std::map<uint16_t, DetectorDataContainer*>::reverse_iterator previousPixelIterator = fSCurvePixelOccupancyMap.rend();

    uint16_t counter = 0;

    for(std::map<uint16_t, DetectorDataContainer*>::reverse_iterator mStripIt = fSCurveStripOccupancyMap.rbegin(), mPixelIt = fSCurvePixelOccupancyMap.rbegin();
        mStripIt != fSCurveStripOccupancyMap.rend() && mPixelIt != fSCurvePixelOccupancyMap.rend();
        ++mStripIt, ++mPixelIt)
    {
        if(fWithCBC || (!fWithMPA && fWithSSA))
        {
            if(previousStripIterator == fSCurveStripOccupancyMap.rend())
            {
                previousStripIterator = mStripIt;
                continue;
            }
            if(fSCurveStripOccupancyMap.size() - 1 == counter) break;
        }
        else if(fWithSSA && fWithMPA)
        {
            if(previousStripIterator == fSCurveStripOccupancyMap.rend() && previousPixelIterator == fSCurvePixelOccupancyMap.rend())
            {
                previousStripIterator = mStripIt;
                previousPixelIterator = mPixelIt;
                continue;
            }
            if((fSCurveStripOccupancyMap.size() - 1 == counter) && (fSCurvePixelOccupancyMap.size() - 1 == counter)) break;
        }
        else if(!fWithSSA && fWithMPA)
        {
            if(previousPixelIterator == fSCurvePixelOccupancyMap.rend())
            {
                previousPixelIterator = mPixelIt;
                continue;
            }
            if(fSCurvePixelOccupancyMap.size() - 1 == counter) break;
        }

        for(auto board: *fDetectorContainer)
        {
            for(auto opticalGroup: *board)
            {
                for(auto hybrid: *opticalGroup)
                {
                    for(auto chip: *hybrid)
                    {
                        for(uint16_t row = 0; row < chip->getNumberOfRows(); ++row)
                        {
                            for(uint16_t col = 0; col < chip->getNumberOfCols(); ++col)
                            {
                                if(!getChannelGroupHandlerContainer()
                                        ->getObject(board->getId())
                                        ->getObject(opticalGroup->getId())
                                        ->getObject(hybrid->getId())
                                        ->getObject(chip->getId())
                                        ->getSummary<std::shared_ptr<ChannelGroupHandler>>()
                                        ->allChannelGroup()
                                        ->isChannelEnabled(row, col))
                                    continue;

                                float currentOccupancy = 0, previousOccupancy = 0, binCenter = 0;
                                auto  cType = chip->getFrontEndType();
                                if(cType == FrontEndType::CBC3 || cType == FrontEndType::SSA2)
                                {
                                    if(mStripIt == fSCurveStripOccupancyMap.rend())
                                    {
                                        mStripIt--;
                                        continue;
                                    }
                                    previousOccupancy = (previousStripIterator)
                                                            ->second->getObject(board->getId())
                                                            ->getObject(opticalGroup->getId())
                                                            ->getObject(hybrid->getId())
                                                            ->getObject(chip->getId())
                                                            ->getChannel<Occupancy>(row, col)
                                                            .fOccupancy;
                                    currentOccupancy = mStripIt->second->getObject(board->getId())
                                                           ->getObject(opticalGroup->getId())
                                                           ->getObject(hybrid->getId())
                                                           ->getObject(chip->getId())
                                                           ->getChannel<Occupancy>(row, col)
                                                           .fOccupancy;
                                    binCenter = (mStripIt->first + (previousStripIterator)->first) / 2.;
                                    // if(cType == FrontEndType::SSA2)
                                    //     if((previousOccupancy > currentOccupancy)) { continue; } // helps when trimming near the pedestal
                                }
                                else if(cType == FrontEndType::MPA2)
                                {
                                    if(mPixelIt == fSCurvePixelOccupancyMap.rend())
                                    {
                                        mPixelIt--;
                                        continue;
                                    }
                                    previousOccupancy = (previousPixelIterator)
                                                            ->second->getObject(board->getId())
                                                            ->getObject(opticalGroup->getId())
                                                            ->getObject(hybrid->getId())
                                                            ->getObject(chip->getId())
                                                            ->getChannel<Occupancy>(row, col)
                                                            .fOccupancy;
                                    currentOccupancy = mPixelIt->second->getObject(board->getId())
                                                           ->getObject(opticalGroup->getId())
                                                           ->getObject(hybrid->getId())
                                                           ->getObject(chip->getId())
                                                           ->getChannel<Occupancy>(row, col)
                                                           .fOccupancy;
                                    binCenter = (mPixelIt->first + (previousPixelIterator)->first) / 2.;
                                    // if(previousOccupancy > currentOccupancy) { continue; }
                                }
                                // avoid  MPA and SSA pedestal peak to influence the noise calculation
                                if(previousOccupancy > 1) previousOccupancy = 1;
                                if(currentOccupancy > 1) currentOccupancy = 1;

                                fThresholdAndNoiseContainer->getObject(board->getId())
                                    ->getObject(opticalGroup->getId())
                                    ->getObject(hybrid->getId())
                                    ->getObject(chip->getId())
                                    ->getChannel<ThresholdAndNoise>(row, col)
                                    .fThreshold += binCenter * (previousOccupancy - currentOccupancy);

                                // if (iChannel>1800){
                                fThresholdAndNoiseContainer->getObject(board->getId())
                                    ->getObject(opticalGroup->getId())
                                    ->getObject(hybrid->getId())
                                    ->getObject(chip->getId())
                                    ->getChannel<ThresholdAndNoise>(row, col)
                                    .fNoise += binCenter * binCenter * (previousOccupancy - currentOccupancy); //}

                                fThresholdAndNoiseContainer->getObject(board->getId())
                                    ->getObject(opticalGroup->getId())
                                    ->getObject(hybrid->getId())
                                    ->getObject(chip->getId())
                                    ->getChannel<ThresholdAndNoise>(row, col)
                                    .fThresholdError += previousOccupancy - currentOccupancy;
                            }
                        }
                    }
                }
            }
        }
        previousStripIterator = mStripIt;
        previousPixelIterator = mPixelIt;
        ++counter;
    }

    // calculate the averages and ship
    // figure  out if you should normalize or not
    uint8_t cNormalizationOrig = getNormalization();
    uint8_t cNormalize         = 1;
    LOG(INFO) << BOLDBLUE << "normalization will be set to " << +cNormalize << RESET;
    setNormalization(cNormalize);

    for(auto board: *fThresholdAndNoiseContainer)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                for(auto chip: *hybrid)
                {
                    for(uint16_t row = 0; row < chip->getNumberOfRows(); ++row)
                    {
                        for(uint16_t col = 0; col < chip->getNumberOfCols(); ++col)
                        {
                            if(!getChannelGroupHandlerContainer()
                                    ->getObject(board->getId())
                                    ->getObject(opticalGroup->getId())
                                    ->getObject(hybrid->getId())
                                    ->getObject(chip->getId())
                                    ->getSummary<std::shared_ptr<ChannelGroupHandler>>()
                                    ->allChannelGroup()
                                    ->isChannelEnabled(row, col))
                                continue;
                            chip->getChannel<ThresholdAndNoise>(row, col).fThreshold /= chip->getChannel<ThresholdAndNoise>(row, col).fThresholdError;
                            chip->getChannel<ThresholdAndNoise>(row, col).fNoise /= chip->getChannel<ThresholdAndNoise>(row, col).fThresholdError;
                            chip->getChannel<ThresholdAndNoise>(row, col).fNoise =
                                sqrt(chip->getChannel<ThresholdAndNoise>(row, col).fNoise -
                                     (chip->getChannel<ThresholdAndNoise>(row, col).fThreshold * chip->getChannel<ThresholdAndNoise>(row, col).fThreshold));

                            if(isnan(chip->getChannel<ThresholdAndNoise>(row, col).fNoise) || isinf(chip->getChannel<ThresholdAndNoise>(row, col).fNoise))
                            {
                                LOG(WARNING) << BOLDYELLOW << "Problem in deriving noise for Board " << board->getId() << " Optical Group " << opticalGroup->getId() << " Hybrid " << hybrid->getId()
                                             << " ReadoutChip " << chip->getId() << " Channel row " << row << " col " << col << ", forcing it to 0." << RESET;
                                chip->getChannel<ThresholdAndNoise>(row, col).fNoise = 0.;
                            }
                            if(isnan(chip->getChannel<ThresholdAndNoise>(row, col).fThreshold) || isinf(chip->getChannel<ThresholdAndNoise>(row, col).fThreshold))
                            {
                                LOG(WARNING) << BOLDYELLOW << "Problem in deriving threshold for Board " << board->getId() << " Optical Group " << opticalGroup->getId() << " Hybrid "
                                             << hybrid->getId() << " ReadoutChip " << chip->getId() << " Channel row " << row << " col " << col << ", forcing it to 0." << RESET;
                                chip->getChannel<ThresholdAndNoise>(row, col).fThreshold = 0.;
                            }
                            chip->getChannel<ThresholdAndNoise>(row, col).fThresholdError = 1;
                            chip->getChannel<ThresholdAndNoise>(row, col).fNoiseError     = 1;
                        }
                    }
                }
            }
        }
        board->normalizeAndAverageContainers(fDetectorContainer->getObject(board->getId()), getChannelGroupHandlerContainer()->getObject(board->getId()), 0);
    }
    setNormalization(cNormalizationOrig);

    // saving average noise for each chip
    for(auto theBoard: *fDetectorContainer)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                for(auto theChip: *theHybrid)
                {
                    const auto& noiseAndThreshold =
                        fThresholdAndNoiseContainer->getChip(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId(), theChip->getId())->getSummary<ThresholdAndNoise, ThresholdAndNoise>();
                    theChip->setAverageNoise(noiseAndThreshold.fNoise);
                }
            }
        }
    }
}

void PedeNoise::producePedeNoisePlots()
{
#ifdef __USE_ROOT__
    fDQMHistogramPedeNoise.fillPedestalAndNoisePlots(*fThresholdAndNoiseContainer);

    // Storing noise and pedestal average and RMS values on the summaryTree
    // for(auto board: *fThresholdAndNoiseContainer)
    // {
    //     for(auto opticalGroup: *board)
    //     {
    //         for(auto module: *opticalGroup)
    //         {
    //             for(auto chip: *module)
    //             {
    //                 fillSummaryTree("AvgNoiseSSA" + std::to_string(chip->getId()), chip->getSummary<ThresholdAndNoise, ThresholdAndNoise>().fNoise);          // For GUI summaryTree
    //                 fillSummaryTree("StDvNoiseSSA" + std::to_string(chip->getId()), chip->getSummary<ThresholdAndNoise, ThresholdAndNoise>().fNoiseError);    // For GUI summaryTree
    //                 fillSummaryTree("AvgPedeSSA" + std::to_string(chip->getId()), chip->getSummary<ThresholdAndNoise, ThresholdAndNoise>().fThreshold);       // For GUI summaryTree
    //                 fillSummaryTree("StDvPedeSSA" + std::to_string(chip->getId()), chip->getSummary<ThresholdAndNoise, ThresholdAndNoise>().fThresholdError); // For GUI summaryTree
    //             }
    //         }
    //     }
    // }

#else
    if(fDQMStreamerEnabled)
    {
        ContainerSerialization theContainerSerialization("PedeNoiseThresholdAndNoise");
        theContainerSerialization.streamByHybridContainer(fDQMStreamer, *fThresholdAndNoiseContainer);
    }
#endif
}

void PedeNoise::setThresholdtoNSigma(BoardContainer* board, float pNSigma)
{
    for(auto opticalGroup: *board)
    {
        for(auto hybrid: *opticalGroup)
        {
            for(auto chip: *hybrid)
            {
                uint32_t cChipId = chip->getId();

                float cPedestal = fThresholdAndNoiseContainer->getObject(board->getId())
                                      ->getObject(opticalGroup->getId())
                                      ->getObject(hybrid->getId())
                                      ->getObject(chip->getId())
                                      ->getSummary<ThresholdAndNoise, ThresholdAndNoise>()
                                      .fThreshold;
                float cNoise = fThresholdAndNoiseContainer->getObject(board->getId())
                                   ->getObject(opticalGroup->getId())
                                   ->getObject(hybrid->getId())
                                   ->getObject(chip->getId())
                                   ->getSummary<ThresholdAndNoise, ThresholdAndNoise>()
                                   .fNoise;

                int      cDiff                  = -pNSigma * cNoise;
                uint16_t cThresholdWorkingPoint = round(cPedestal + cDiff);

                if(pNSigma > 0)
                    LOG(INFO) << "Changing Threshold on Chip " << +cChipId << " by " << cDiff << " to " << +cThresholdWorkingPoint << " VCth units to supress noise!"
                              << " NOISE IS " << cNoise << " SIGMA " << pNSigma;
                else
                {
                    LOG(INFO) << "Changing Threshold on Chip " << +cChipId << " back to the pedestal at " << +cPedestal;
                    cThresholdWorkingPoint = cPedestal;
                }
                fReadoutChipInterface->WriteChipReg(chip, "Threshold", cThresholdWorkingPoint);
            }
        }
    }
}

void PedeNoise::maskNoisyChannels(BoardDataContainer* board)
{
    for(auto opticalGroup: *board)
    {
        for(auto hybrid: *opticalGroup)
        {
            for(auto chip: *hybrid)
            {
                auto chipDC = fDetectorContainer->getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getObject(chip->getId());
                // auto cType = chipDC->getFrontEndType();

                // uint32_t       NCH = NCHANNELS;
                // if(cType == FrontEndType::CBC3)
                //   NCH = NCHANNELS;
                // else if(cType == FrontEndType::SSA2)
                //  NCH = NSSACHANNELS;
                // else if(cType == FrontEndType::MPA2)
                //  NCH = NMPAROWS * NSSACHANNELS;

                /*float fMean=1.0;
                        if(cType == FrontEndType::MPA2)
                     fMean=2.7;
                        if(cType == FrontEndType::SSA2)
                     fMean=4.2;*/
                auto     cOriginalMask = chipDC->getChipOriginalMask();
                uint32_t nMask         = 0;
                for(uint16_t row = 0; row < chip->getNumberOfRows(); ++row)
                {
                    for(uint16_t col = 0; col < chip->getNumberOfCols(); ++col)
                    {
                        float cPedestal = chip->getSummary<ThresholdAndNoise, ThresholdAndNoise>().fThreshold;
                        // float cNoise = chip->getSummary<ThresholdAndNoise, ThresholdAndNoise>().fNoise;
                        if(fPedeNoiseMask and (chip->getChannel<ThresholdAndNoise>(row, col).fNoise > fPedeNoiseLimit))
                        {
                            nMask += 1;
                            LOG(INFO) << BOLDYELLOW << "Masking row: " << row << " col " << col << " with a noise of " << chip->getChannel<ThresholdAndNoise>(row, col).fNoise
                                      << ", which is over the limit of " << fPedeNoiseLimit << RESET;
                            cOriginalMask->disableChannel(row, col);
                        }
                        if(fPedeNoiseMaskUntrimmed and std::fabs(chip->getChannel<ThresholdAndNoise>(row, col).fThreshold - cPedestal) > fPedeNoiseUntrimmedLimit)
                        {
                            uint8_t thetrim = fReadoutChipInterface->ReadChipReg(static_cast<ReadoutChip*>(chipDC), "TrimDAC_C" + std::to_string(col) + "_R" + std::to_string(row));

                            nMask += 1;
                            LOG(INFO) << BOLDYELLOW << "Masking row: " << row << " col " << col << " with a pedestal difference of "
                                      << std::fabs(chip->getChannel<ThresholdAndNoise>(row, col).fThreshold - cPedestal) << ", which is over the limit of " << fPedeNoiseUntrimmedLimit
                                      << " trimval: " << +thetrim << RESET;
                            cOriginalMask->disableChannel(row, col);
                        }

                        // LOG(INFO) << BOLDYELLOW << "snorp SUMMARY TH "<<cPedestal <<RESET;
                        // LOG(INFO) << BOLDYELLOW << "snorp SUMMARY NOI "<<cNoise <<RESET;
                        // LOG(INFO) << BOLDYELLOW << "fPedeNoiseLimit "<<fPedeNoiseLimit<< " fPedeNoiseMask "<<fPedeNoiseMask <<RESET;
                        // LOG(INFO) << BOLDYELLOW << "Noise "<<iChannel<< ": "<<chip->getChannel<ThresholdAndNoise>(row, col).fNoise <<RESET;
                        // LOG(INFO) << BOLDYELLOW << "Thresh "<<iChannel<< ": "<<chip->getChannel<ThresholdAndNoise>(row, col).fThreshold  <<RESET;
                    }
                }
                // fReadoutChipInterface->maskChannelGroup(chipDC,cOriginalMask);
                if(nMask > 0) LOG(INFO) << BOLDYELLOW << "PedeNoise masked " << nMask << " channels..." << RESET;
                fReadoutChipInterface->ConfigureChipOriginalMask(chipDC);
            }
        }
    }
}

void PedeNoise::writeObjects()
{
#ifdef __USE_ROOT__
    fDQMHistogramPedeNoise.process();
#endif
}

void PedeNoise::ConfigureCalibration() {}

void PedeNoise::Running()
{
    LOG(INFO) << "Starting noise measurement";
    Initialise(true, true);
    // auto myFunction = [](const Ph2_HwDescription::ReadoutChip *theChip){
    //     std::cout<<"Using it"<<std::endl;
    //     return (theChip->getId()==0);
    //     };
    // HybridContainer::SetQueryFunction(myFunction);
    measureNoise();
    // HybridContainer::ResetQueryFunction();
    // Validate();
    LOG(INFO) << "Done with noise";
    Reset();
}

void PedeNoise::Stop()
{
    LOG(INFO) << "Stopping noise measurement";
    writeObjects();
    closeFileHandler();
    clearDataMembers();
    LOG(INFO) << "Noise measurement stopped.";
}

void PedeNoise::Pause() {}

void PedeNoise::Resume() {}
