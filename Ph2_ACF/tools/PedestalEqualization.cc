#include "tools/PedestalEqualization.h"
#include "HWDescription/ReadoutChip.h"
#include "HWInterface/D19cFWInterface.h"
#include "HWInterface/D19cPSCounterFWInterface.h"
#include "System/RegisterHelper.h"
#include "Utils/CBCChannelGroupHandler.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/DataContainer.h"
#include "Utils/MPAChannelGroupHandler.h"
#include "Utils/Occupancy.h"
#include "Utils/SSAChannelGroupHandler.h"

using namespace Ph2_System;
using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;

std::string PedestalEqualization::fCalibrationDescription = "Equalize the pedestal/threshold for all channels";

PedestalEqualization::PedestalEqualization() : Tool() {}

PedestalEqualization::~PedestalEqualization() {}

void PedestalEqualization::Initialise(bool pAllChan, bool pDisableStubLogic)
{
    fRegisterHelper->takeSnapshot();
    fRegisterHelper->freeFrontEndRegister(FrontEndType::CBC3, "^Channel\\d{3}$");
    fRegisterHelper->freeFrontEndRegister(FrontEndType::MPA2, "^TrimDAC_C\\d+_R\\d+$");
    fRegisterHelper->freeFrontEndRegister(FrontEndType::SSA2, "^THTRIMMING_S\\d+$");

    fDisableStubLogic = pDisableStubLogic;

    DetectorDataContainer theOccupancyContainer;
    fDetectorDataContainer = &theOccupancyContainer;
    ContainerFactory::copyAndInitStructure<Occupancy>(*fDetectorContainer, *fDetectorDataContainer);

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
    if(fWithCBC) LOG(INFO) << BOLDBLUE << "PedestalEqualization with CBCs" << RESET;
    if(fWithSSA && !fWithMPA) LOG(INFO) << BOLDBLUE << "PedestalEqualization with SSAs" << RESET;
    if(fWithMPA && !fWithSSA) LOG(INFO) << BOLDBLUE << "PedestalEqualization with MPAs" << RESET;
    if(fWithSSA && fWithMPA) LOG(INFO) << BOLDBLUE << "PedestalEqualization with SSAs+MPAs" << RESET;

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

    this->fAllChan = pAllChan;

    fSkipMaskedChannels                = findValueInSettings<double>("SkipMaskedChannels", 0);
    fMaskChannelsFromOtherGroups       = findValueInSettings<double>("MaskChannelsFromOtherGroups", 1);
    fCheckLoop                         = findValueInSettings<double>("VerificationLoop", 1);
    fPedestalEqualizationMaskUntrimmed = findValueInSettings<double>("PedestalEqualization_MaskUntrimmed", 0);
    fFullScan                          = findValueInSettings<double>("FullScan", 0);

    fPedestalEqualizationFullScanStart = findValueInSettings<double>("PedestalEqualization_FullScanStart", 110);
    fPedestalEqualizationFullScanCAP   = findValueInSettings<double>("PedestalEqualizationFullScanCAP", 1.0);

    fTestPulseAmplitude    = findValueInSettings<double>("PedestalEqualization_PulseAmplitude", 0);
    fTestPulseAmplitudePix = findValueInSettings<double>("PedestalEqualization_PulseAmplitudePix", fTestPulseAmplitude);

    if(fFullScan)
    {
        fTestPulseAmplitude    = findValueInSettings<double>("PedestalEqualization_PulseAmplitudeFullScan", 0);
        fTestPulseAmplitudePix = findValueInSettings<double>("PedestalEqualization_PulseAmplitudePixFullScan", 0);
    }

    fEventsPerPoint          = findValueInSettings<double>("Nevents", 10);
    fNEventsPerBurst         = (fEventsPerPoint >= fMaxNevents) ? fMaxNevents : -1;
    fOccupancyAtPedestal     = findValueInSettings<double>("PedestalEqualization_Occupancy", 0.56);
    uint8_t cDefTargetOffset = (fWithCBC) ? 0x7F : 0xF;
    fTargetOffset            = findValueInSettings<double>("PedestalEqualizationTargetOffset", cDefTargetOffset);
    bool fastCounterReadout  = findValueInSettings<double>("PedestalEqualization_FastCounterReadout", 1) > 0;

    LOG(INFO) << BOLDBLUE << "PedestalEqualization::Initialise Occupancy at pedestal is " << fOccupancyAtPedestal << " target offset is " << +fTargetOffset << RESET;
    this->SetSkipMaskedChannels(fSkipMaskedChannels);

    if(fTestPulseAmplitude == 0)
        fTestPulse = 0;
    else
        fTestPulse = 1;

#ifdef __USE_ROOT__
    fDQMHistogramPedestalEqualization.book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif

    ContainerFactory::copyAndInitBoard<BeBoardRegMap>(*fDetectorContainer, fBoardRegContainer);
    for(auto cBoard: *fDetectorContainer)
    {
        auto&                cBoardRegNap = fBoardRegContainer.getObject(cBoard->getId())->getSummary<BeBoardRegMap>();
        const BeBoardRegMap& cOrigRegMap  = static_cast<const BeBoard*>(cBoard)->getBeBoardRegMap();
        cBoardRegNap.insert(cOrigRegMap.begin(), cOrigRegMap.end());
    }

    // event types
    ContainerFactory::copyAndInitBoard<EventType>(*fDetectorContainer, fEventTypes);
    bool cForcePSasync = true;
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

    if(fDisableStubLogic)
    {
        // ContainerFactory::copyAndInitChip<uint8_t>(*fDetectorContainer, fStubLogicCointainer);
        // ContainerFactory::copyAndInitChip<uint8_t>(*fDetectorContainer, fHIPCountCointainer);

        for(auto board: *fDetectorContainer)
        {
            for(auto opticalGroup: *board)
            {
                for(auto hybrid: *opticalGroup)
                {
                    for(auto chip: *hybrid)
                    {
                        ReadoutChip* theChip = static_cast<ReadoutChip*>(chip);
                        // if it is a CBC3, disable the stub logic for this procedure
                        if(theChip->getFrontEndType() == FrontEndType::CBC3)
                        {
                            LOG(INFO) << BOLDBLUE << "Chip Type = CBC3 - thus disabling Stub logic for offset tuning for CBC " << +chip->getId() << RESET;
                            // fStubLogicCointainer.getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getObject(chip->getId())->getSummary<uint8_t>() =
                            //     fReadoutChipInterface->ReadChipReg(theChip, "Pipe&StubInpSel&Ptwidth");
                            // uint8_t value = fReadoutChipInterface->ReadChipReg(theChip, "HIP&TestMode");
                            // fHIPCountCointainer.getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getObject(chip->getId())->getSummary<uint8_t>() =
                            // value;
                            static_cast<CbcInterface*>(fReadoutChipInterface)->enableHipSuppression(theChip, false, true, 0);
                        }
                    }
                }
            }
        }
    }

    LOG(INFO) << "Parsed settings:";
    LOG(INFO) << "	Nevents = " << fEventsPerPoint;
    LOG(INFO) << "	TestPulseAmplitude = " << int(fTestPulseAmplitude);
    LOG(INFO) << "  Target Vcth determined algorithmically for Chip";
    LOG(INFO) << "  Target Offset = 0x" << std::hex << +fTargetOffset;
}
void PedestalEqualization::Reset()
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

void PedestalEqualization::FindVplus()
{
    // original tool flags
    bool    originalAllChannelFlag = this->fAllChan;
    uint8_t cNormalizationOrig     = getNormalization();

    // figure  out if you should normalize or not
    uint cNormalize = 1;
    LOG(INFO) << BOLDBLUE << "normalization will be set to " << +cNormalize << RESET;
    setNormalization(cNormalize);

    if(fTestPulse)
    {
        this->enableTestPulse(true);
        for(auto cBoard: *fDetectorContainer)
        {
            // Allow for different SSA and MPA injection amplitudes
            // setSameDacBeBoard(static_cast<BeBoard*>(cBoard), "InjectedCharge", fTestPulseAmplitude);
            if(fWithSSA or fWithMPA)
                for(auto cOpticalGroup: *cBoard)
                {
                    for(auto cHybrid: *cOpticalGroup)
                    {
                        for(auto cChip: *cHybrid)
                        {
                            auto cType = cChip->getFrontEndType();
                            if(cType == FrontEndType::MPA2)
                                fReadoutChipInterface->WriteChipReg(cChip, "InjectedCharge", fTestPulseAmplitudePix);
                            else
                                fReadoutChipInterface->WriteChipReg(cChip, "InjectedCharge", fTestPulseAmplitude);
                        }
                    }
                }

            else
                setSameDacBeBoard(static_cast<BeBoard*>(cBoard), "TestPulsePotNodeSel", fTestPulseAmplitude);
        }
        LOG(INFO) << BLUE << "Enabled test pulse. " << RESET;
    }
    else
        this->enableTestPulse(fWithSSA or fWithMPA);

    LOG(INFO) << BOLDBLUE << "Setting threshold trim registers to mid-range value...0x" << std::hex << +fTargetOffset << std::dec << RESET;
    if(fWithCBC)
        setSameLocalDac("ChannelOffset", fTargetOffset);
    else
        setSameLocalDac("ThresholdTrim", fTargetOffset);

    LOG(INFO) << BOLDBLUE << "Finding threshold at which to equalize offsets - searching for threshold where <Occupancy>/Chip is " << fOccupancyAtPedestal << RESET;
    uint8_t cTargetVcth = 0x0;
    setSameDac("Threshold", cTargetVcth);
    this->setTestAllChannels(true);
    DetectorDataContainer theOccupancyContainer;
    fDetectorDataContainer = &theOccupancyContainer;
    ContainerFactory::copyAndInitStructure<Occupancy>(*fDetectorContainer, *fDetectorDataContainer);

    if(fFullScan)
        this->fullScan("Threshold", fEventsPerPoint, fOccupancyAtPedestal, fNEventsPerBurst, fPedestalEqualizationFullScanStart);
    else
        this->bitWiseScan("Threshold", fEventsPerPoint, fOccupancyAtPedestal, fNEventsPerBurst);

    // LOG(INFO) << BOLDBLUE << "Setting threshold trim registers to max value..." << RESET;
    if(fWithCBC)
        setSameLocalDac("ChannelOffset", 0xFF);
    else
        setSameLocalDac("ThresholdTrim", 0x1F);

    // store thresholds
    DetectorDataContainer theVcthContainer;
    ContainerFactory::copyAndInitChip<uint16_t>(*fDetectorContainer, theVcthContainer);

    float cMeanStripsValue = 0., cMeanPixelsValue = 0.;
    float cNStripChips = 0., cNPixelChips = 0.;
    for(auto board: theVcthContainer) // for on boards - begin
    {
        for(auto opticalGroup: *board) // for on opticalGroup - begin
        {
            for(auto hybrid: *opticalGroup) // for on hybrid - begin
            {
                // nChip += hybrid->size();
                for(auto chip: *hybrid) // for on chip - begin
                {
                    ReadoutChip* theChip =
                        static_cast<ReadoutChip*>(fDetectorContainer->getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getObject(chip->getId()));
                    uint16_t tmpVthr = 0;
                    auto     cType   = theChip->getFrontEndType();
                    if(cType == FrontEndType::CBC3) tmpVthr = (theChip->getReg("VCth1") + (theChip->getReg("VCth2") << 8));
                    if(cType == FrontEndType::SSA2) tmpVthr = theChip->getReg("Bias_THDAC");
                    if(cType == FrontEndType::MPA2) tmpVthr = theChip->getReg("ThDAC0");
                    chip->getSummary<uint16_t>() = tmpVthr;
                    LOG(INFO) << GREEN << "VCth value for BeBoard " << +board->getId() << " OpticalGroup " << +opticalGroup->getId() << " Hybrid " << +hybrid->getId() << " Chip " << +chip->getId()
                              << " = " << tmpVthr << RESET;
                    uint32_t ENCHAN  = theChip->getChipOriginalMask()->getNumberOfEnabledChannels();
                    uint32_t TOTCHAN = chip->size();
                    LOG(DEBUG) << GREEN << "NCHANNELS " << ENCHAN << " TOTCHAN " << TOTCHAN << RESET;
                    if(cType == FrontEndType::SSA2 || cType == FrontEndType::CBC3)
                    {
                        cNStripChips += float(ENCHAN) / float(TOTCHAN);
                        cMeanStripsValue += tmpVthr * (float(ENCHAN) / float(TOTCHAN));
                        LOG(DEBUG) << "MeanStripsValue : " << +cMeanStripsValue << " -- NStripChips : " << +cNStripChips << RESET;
                    }
                    else if(cType == FrontEndType::MPA2)
                    {
                        cNPixelChips += float(ENCHAN) / float(TOTCHAN);
                        cMeanPixelsValue += tmpVthr * (float(ENCHAN) / float(TOTCHAN));
                        LOG(DEBUG) << "MeanPixelsValue : " << +cMeanPixelsValue << " -- NPixelChips : " << +cNPixelChips << RESET;
                    }

                } // for on chip - end
            } // for on hybrid - end
        } // for on opticalGroup - end
    } // for on board - end
    // Vplus was relevant for CBC2, now it is saving the threshold... not relevant anymore
    // #ifdef __USE_ROOT__
    //     fDQMHistogramPedestalEqualization.fillVplusPlots(theVcthContainer);
    // #else
    //     if(fDQMStreamerEnabled)
    //     {
    //         ContainerSerialization theContainerSerialization("PedestalEqualizationVCth");
    //         theContainerSerialization.streamByHybridContainer(fDQMStreamer, theVcthContainer);
    //     }
    // #endif

    fStripTargetVcth = uint16_t(cMeanStripsValue / cNStripChips);
    fPixelTargetVcth = uint16_t(cMeanPixelsValue / cNPixelChips);
    if(fUseMean)
    {
        if(fWithCBC || fWithSSA) LOG(INFO) << BOLDBLUE << "Mean VCth value of all strip chips is " << fStripTargetVcth << " - using as TargetVcth value for all strip chips!" << RESET;
        if(fWithMPA) LOG(INFO) << BOLDBLUE << "Mean VCth value of all pixel chips is " << fPixelTargetVcth << " - using as TargetVcth value for all pixel chips!" << RESET;
        for(auto cBoard: *fDetectorContainer)
        {
            for(auto cOpticalGroup: *cBoard)
            {
                for(auto cHybrid: *cOpticalGroup)
                {
                    for(auto cChip: *cHybrid)
                    {
                        auto cType = cChip->getFrontEndType();
                        if(cType == FrontEndType::SSA2 || cType == FrontEndType::CBC3) { fReadoutChipInterface->WriteChipReg(cChip, "Threshold", fStripTargetVcth); }
                        else if(cType == FrontEndType::MPA2) { fReadoutChipInterface->WriteChipReg(cChip, "Threshold", fPixelTargetVcth); }
                    }
                }
            }
        }
    }
    this->setTestAllChannels(originalAllChannelFlag);
    setNormalization(cNormalizationOrig);
}

void PedestalEqualization::FindOffsets()
{
    // figure  out if you should normalize or not
    uint8_t cNormalizationOrig = getNormalization();
    uint8_t cNormalize         = 1;
    LOG(INFO) << BOLDBLUE << "normalization will be set to " << +cNormalize << RESET;
    setNormalization(cNormalize);

    float cOccupancyAtPedestal = fOccupancyAtPedestal;
    LOG(INFO) << BOLDBLUE << "Finding offsets..." << RESET;
    // just to be sure, configure the correct VCth and VPlus values

    if(fUseMean)
    {
        if(fWithCBC || fWithSSA) LOG(INFO) << BOLDBLUE << "Mean VCth value of all strip chips is " << fStripTargetVcth << " - using as TargetVcth value for all strip chips!" << RESET;
        if(fWithMPA) LOG(INFO) << BOLDBLUE << "Mean VCth value of all pixel chips is " << fPixelTargetVcth << " - using as TargetVcth value for all pixel chips!" << RESET;
        for(auto cBoard: *fDetectorContainer)
        {
            for(auto cOpticalGroup: *cBoard)
            {
                for(auto cHybrid: *cOpticalGroup)
                {
                    for(auto cChip: *cHybrid)
                    {
                        auto cType = cChip->getFrontEndType();
                        if(cType == FrontEndType::SSA2 || cType == FrontEndType::CBC3) { fReadoutChipInterface->WriteChipReg(cChip, "Threshold", fStripTargetVcth); }
                        else if(cType == FrontEndType::MPA2) { fReadoutChipInterface->WriteChipReg(cChip, "Threshold", fPixelTargetVcth); }
                    }
                }
            }
        }
    }
    DetectorDataContainer theOccupancyContainer;
    fDetectorDataContainer = &theOccupancyContainer;
    ContainerFactory::copyAndInitStructure<Occupancy>(*fDetectorContainer, *fDetectorDataContainer);

    if(fWithCBC) this->bitWiseScan("ChannelOffset", fEventsPerPoint, cOccupancyAtPedestal, fNEventsPerBurst);

    if(fWithSSA or fWithMPA)
    {
        if(fFullScan) { this->fullScan("ThresholdTrim", fEventsPerPoint, cOccupancyAtPedestal, fNEventsPerBurst, 31, fPedestalEqualizationMaskUntrimmed); }
        else
            this->bitWiseScan("ThresholdTrim", fEventsPerPoint, cOccupancyAtPedestal, fNEventsPerBurst);
    }

    DetectorDataContainer theOffsetsCointainer;
    ContainerFactory::copyAndInitChannel<uint8_t>(*fDetectorContainer, theOffsetsCointainer);

    for(auto board: theOffsetsCointainer) // for on boards - begin
    {
        for(auto opticalGroup: *board) // for on opticalGroup - begin
        {
            for(auto hybrid: *opticalGroup) // for on hybrid - begin
            {
                for(auto chip: *hybrid) // for on chip - begin
                {
                    // if(fDisableStubLogic and fWithCBC)
                    // {
                    //     ReadoutChip* theChip =
                    //     static_cast<ReadoutChip*>(fDetectorContainer->getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getObject(chip->getId()));

                    //     uint8_t stubLogicValue =
                    //     fStubLogicCointainer.getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getObject(chip->getId())->getSummary<uint8_t>();
                    //     fReadoutChipInterface->WriteChipReg(theChip, "Pipe&StubInpSel&Ptwidth", stubLogicValue);

                    //     uint8_t HIPCountValue =
                    //     fHIPCountCointainer.getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getObject(chip->getId())->getSummary<uint8_t>();
                    //     fReadoutChipInterface->WriteChipReg(theChip, "HIP&TestMode", HIPCountValue);
                    // }

                    int          cMeanOffset = 0;
                    ReadoutChip* roc = static_cast<ReadoutChip*>(fDetectorContainer->getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getObject(chip->getId()));
                    auto         cType = roc->getFrontEndType();

                    for(uint16_t row = 0; row < roc->getNumberOfRows(); ++row)
                    {
                        for(uint16_t col = 0; col < roc->getNumberOfCols(); ++col)
                        {
                            std::string cRegName;
                            if(cType == FrontEndType::CBC3)
                            {
                                char charRegName[20];
                                cRegName = sprintf(charRegName, "Channel%03d", col + 1);
                                cRegName = charRegName;
                            }
                            if(cType == FrontEndType::SSA2) cRegName = "THTRIMMING_S" + std::to_string(col + 1);
                            if(cType == FrontEndType::MPA2) cRegName = "TrimDAC_C" + std::to_string(col) + "_R" + std::to_string(row);
                            auto channel = roc->getReg(cRegName);
                            cMeanOffset += roc->getReg(cRegName);
                            chip->getChannel<uint8_t>(row, col) = channel;
                        }
                    }

                    LOG(INFO) << BOLDRED << "Mean offset on Chip" << +chip->getId() << " is : " << (cMeanOffset) / (double)roc->getNumberOfChannels() << " Vcth units." << RESET;
                } // for on chip - end
            } // for on hybrid - end
        } // for on opticalGroup - end
    } // for on board - end
#ifdef __USE_ROOT__
    fDQMHistogramPedestalEqualization.fillOccupancyPlots(theOccupancyContainer);
    fDQMHistogramPedestalEqualization.fillOffsetPlots(theOffsetsCointainer);
#else
    if(fDQMStreamerEnabled)
    {
        ContainerSerialization theOccupancyContainerSerialization("PedestalEqualizationOccupancy");
        theOccupancyContainerSerialization.streamByHybridContainer(fDQMStreamer, theOccupancyContainer);
        ContainerSerialization theOffsetContainerSerialization("PedestalEqualizationOffset");
        theOffsetContainerSerialization.streamByHybridContainer(fDQMStreamer, theOffsetsCointainer);
    }
#endif

    setNormalization(cNormalizationOrig);
}

void PedestalEqualization::writeObjects()
{
    this->SaveResults();
#ifdef __USE_ROOT__
    fDQMHistogramPedestalEqualization.process();
#endif
}

// State machine control functions

void PedestalEqualization::ConfigureCalibration() {}

void PedestalEqualization::Running()
{
    LOG(INFO) << "Starting Pedestal Equalization";
    Initialise(true, true);
    FindVplus();
    FindOffsets();
    LOG(INFO) << "Done with Pedestal Equalization";
    Reset();
}

void PedestalEqualization::Stop()
{
    LOG(INFO) << "Stopping Pedestal Equalization.";
    writeObjects();
    closeFileHandler();
    LOG(INFO) << "Pedestal Equalization stopped.";
}

void PedestalEqualization::Pause() {}

void PedestalEqualization::Resume() {}
