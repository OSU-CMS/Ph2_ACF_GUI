#include "tools/OTalignLpGBTinputsForBypass.h"
#include "HWInterface/CbcInterface.h"
#include "HWInterface/D19cFWInterface.h"
#include "System/RegisterHelper.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/GenericDataArray.h"
#include "Utils/Utilities.h"
#include "tools/OTPatternCheckerHelper.h"
#include <bitset>

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

std::string OTalignLpGBTinputsForBypass::fCalibrationDescription = "Optimize LpGBT Rx phases to properly decode the inputs from the CICs when set in bypass mode";

OTalignLpGBTinputsForBypass::OTalignLpGBTinputsForBypass() : Tool() {}

OTalignLpGBTinputsForBypass::~OTalignLpGBTinputsForBypass() {}

void OTalignLpGBTinputsForBypass::Initialise(void)
{
    fRegisterHelper->takeSnapshot();
    // free the registers in case any
    fNumberOfTestedBits     = findValueInSettings<double>("OTalignLpGBTinputsForBypass_NumberOfTestedBits", 1e6);
    fNumberOfTestedBitsL12S = findValueInSettings<double>("OTalignLpGBTinputsForBypass_NumberOfTestedBitsL12S", 1e5);

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: plots are booked during initialization
    fDQMHistogramOTalignLpGBTinputsForBypass.book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif

    preparePatternChecker();
}

void OTalignLpGBTinputsForBypass::ConfigureCalibration() {}

void OTalignLpGBTinputsForBypass::preparePatternChecker()
{
    fPatternCheckerHelper = new OTPatternCheckerHelper();
    fPatternCheckerHelper->Inherit(this);
    fPatternCheckerHelper->prepareCalibration();
    fPatternCheckerHelper->setSuppressErrorPrintout(true);
}

void OTalignLpGBTinputsForBypass::Running()
{
    LOG(INFO) << "Starting OTalignLpGBTinputsForBypass measurement.";
    Initialise();
    AlignLpGBTinputs();
    LOG(INFO) << "Done with OTalignLpGBTinputsForBypass.";
    Reset();
}

void OTalignLpGBTinputsForBypass::Stop(void)
{
    LOG(INFO) << "Stopping OTalignLpGBTinputsForBypass measurement.";
#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: processing the histograms
    fDQMHistogramOTalignLpGBTinputsForBypass.process();
#endif
    SaveResults();
    closeFileHandler();
    LOG(INFO) << "OTalignLpGBTinputsForBypass stopped.";
}

void OTalignLpGBTinputsForBypass::Pause() {}

void OTalignLpGBTinputsForBypass::Resume() {}

void OTalignLpGBTinputsForBypass::Reset() { fRegisterHelper->restoreSnapshot(); }

void OTalignLpGBTinputsForBypass::AlignLpGBTinputs()
{
    LOG(INFO) << BOLDYELLOW << "OTalignLpGBTinputsForBypass::AlignLpGBTinputs ... start LpGBT phase scan with CIC in bypass mode" << RESET;

    uint8_t numberOfLpgbtPhases = 15;

    std::map<uint8_t, std::map<uint8_t, DetectorDataContainer>> errorRateContainerMap;
    for(uint8_t phyPort = 0; phyPort < 12; ++phyPort)
    {
        for(uint8_t lpgbtPhase = 0; lpgbtPhase < numberOfLpgbtPhases; ++lpgbtPhase)
        {
            ContainerFactory::copyAndInitHybrid<GenericDataArray<float, 4, 2>>(*fDetectorContainer, errorRateContainerMap[lpgbtPhase][phyPort]);
        }
    }

    for(auto* theBoard: *fDetectorContainer)
    {
        bool isPS = theBoard->getFirstObject()->getFrontEndType() == FrontEndType::OuterTrackerPS;

        produceAllPatternAndMasks(theBoard);

        if(isPS) prepareMPAtoSendPatterns(theBoard);

        for(uint8_t phyPort = 0; phyPort < 12; ++phyPort)
        {
            LOG(INFO) << BOLDGREEN << "    Measuring phyPort " << +phyPort << RESET;

            setCICBypass(theBoard, phyPort);

            if(!isPS)
            {
                if(phyPort < 10)
                    prepare2StoSendStubPatterns(theBoard);
                else
                    prepare2StoSendL1Patterns(theBoard);
            }

            for(uint8_t lpgbtPhase = 0; lpgbtPhase < numberOfLpgbtPhases; ++lpgbtPhase)
            {
                for(auto theOpticalGroup: *theBoard)
                {
                    auto& thelpGBT = theOpticalGroup->flpGBT;

                    std::vector<std::string> listOfPhaseRegister = {"EPRX00ChnCntr",
                                                                    "EPRX02ChnCntr",
                                                                    "EPRX10ChnCntr",
                                                                    "EPRX12ChnCntr",
                                                                    "EPRX20ChnCntr",
                                                                    "EPRX22ChnCntr",
                                                                    "EPRX30ChnCntr",
                                                                    "EPRX32ChnCntr",
                                                                    "EPRX40ChnCntr",
                                                                    "EPRX42ChnCntr",
                                                                    "EPRX50ChnCntr",
                                                                    "EPRX52ChnCntr",
                                                                    "EPRX60ChnCntr",
                                                                    "EPRX62ChnCntr"};

                    auto readRegister = flpGBTInterface->ReadChipMultReg(thelpGBT, listOfPhaseRegister);
                    for(auto& theRegister: readRegister) { theRegister.second = (theRegister.second & 0x0F) | (lpgbtPhase << 4); }

                    flpGBTInterface->WriteChipMultReg(thelpGBT, readRegister);
                }

                runPatternMatching(theBoard, errorRateContainerMap[lpgbtPhase], isPS, phyPort);
            }
        }
    }

    for(uint8_t phyPort = 0; phyPort < 12; ++phyPort)
    {
        DetectorDataContainer bestPhaseContainer;
        ContainerFactory::copyAndInitHybrid<GenericDataArray<uint8_t, 4>>(*fDetectorContainer, bestPhaseContainer);

        for(auto* theBoard: *fDetectorContainer)
        {
            for(auto theOpticalGroup: *bestPhaseContainer.getBoard(theBoard->getId()))
            {
                for(auto theHybrid: *theOpticalGroup)
                {
                    auto theOuterTrackerHybrid = static_cast<OuterTrackerHybrid*>(fDetectorContainer->getObject(theBoard->getId())->getObject(theOpticalGroup->getId())->getObject(theHybrid->getId()));
                    auto theCic                = theOuterTrackerHybrid->fCic;
                    for(uint8_t line = 0; line < NUMBER_OF_LINES_PER_CIC_PHY_PORTS; ++line)
                    {
                        GenericDataArray<float, 15, 2> thePhaseEfficiencyList;
                        for(uint8_t lpgbtPhase = 0; lpgbtPhase < numberOfLpgbtPhases; ++lpgbtPhase)
                        {
                            auto phyPortEfficiencyScanList =
                                errorRateContainerMap[lpgbtPhase][phyPort].getHybrid(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId())->getSummary<GenericDataArray<float, 4, 2>>();
                            thePhaseEfficiencyList.at(lpgbtPhase) = phyPortEfficiencyScanList.at(line);
                        }
                        auto theBestPhase                                              = getBestPhase(thePhaseEfficiencyList, theOuterTrackerHybrid, line);
                        theHybrid->getSummary<GenericDataArray<uint8_t, 4>>().at(line) = theBestPhase;
                        theCic->setLpGBTphaseForCICbypass(phyPort, line, theBestPhase);
                    }
                }
            }
        }
#ifdef __USE_ROOT__
        fDQMHistogramOTalignLpGBTinputsForBypass.fillBestPhase(bestPhaseContainer, phyPort);
#else
        if(fDQMStreamer)
        {
            ContainerSerialization theBestPhaseSerialization("OTalignLpGBTinputsForBypassBestPhase");
            theBestPhaseSerialization.streamByHybridContainer(fDQMStreamer, bestPhaseContainer, phyPort);
        }
#endif
    }

    for(uint8_t phyPort = 0; phyPort < 12; ++phyPort)
    {
        for(uint8_t lpgbtPhase = 0; lpgbtPhase < numberOfLpgbtPhases; ++lpgbtPhase)
        {
#ifdef __USE_ROOT__
            fDQMHistogramOTalignLpGBTinputsForBypass.fillMatchingEfficiency(errorRateContainerMap[lpgbtPhase][phyPort], phyPort, lpgbtPhase);
#else
            if(fDQMStreamer)
            {
                ContainerSerialization theMatchingEfficiencySerialization("OTalignLpGBTinputsForBypassMatchingEfficiency");
                theMatchingEfficiencySerialization.streamByHybridContainer(fDQMStreamer, errorRateContainerMap[lpgbtPhase][phyPort], phyPort, lpgbtPhase);
            }
#endif
        }
    }
}

void OTalignLpGBTinputsForBypass::runPatternMatching(BeBoard* theBoard, std::map<uint8_t, DetectorDataContainer>& errorRatePerPhyPortMap, bool isPS, uint8_t phyPort)
{
    auto theFWinterface = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(theBoard));

    if(!isPS && phyPort >= 10) // L1 for 2S case
    {
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                fBeBoardInterface->WriteBoardReg(theBoard, "fc7_daq_cnfg.physical_interface_block.slvs_debug.hybrid_select", theHybrid->getId());
                fBeBoardInterface->WriteBoardReg(theBoard, "fc7_daq_cnfg.physical_interface_block.slvs_debug.chip_select", 0);

                std::vector<std::vector<uint32_t>> phyPortDataVector(NUMBER_OF_LINES_PER_CIC_PHY_PORTS);
                std::vector<size_t>                phyPortIterationVector(NUMBER_OF_LINES_PER_CIC_PHY_PORTS, 0);

                size_t totalNumberOfRequiredIterations = fNumberOfTestedBitsL12S / fPattern2SL1.getNumberOfMaskedBits();
                size_t minimumNumberOfIterations       = *std::min_element(phyPortIterationVector.begin(), phyPortIterationVector.end());
                size_t totalIterationCounter           = 0;
                while(minimumNumberOfIterations <= totalNumberOfRequiredIterations && totalIterationCounter <= 2 * totalNumberOfRequiredIterations)
                {
                    fBeBoardInterface->WriteBoardReg(theBoard, "fc7_daq_ctrl.fast_command_block.control.stop_trigger", 0x1);
                    usleep(10);
                    fBeBoardInterface->WriteBoardReg(theBoard, "fc7_daq_ctrl.fast_command_block.control.start_trigger", 0x1);

                    auto lineOutputVector = theFWinterface->StubDebug(true, NUMBER_OF_LINES_PER_CIC_PHY_PORTS, false);

                    for(uint8_t line = 0; line < NUMBER_OF_LINES_PER_CIC_PHY_PORTS; ++line)
                    {
                        auto& theLineVector     = lineOutputVector.at(line);
                        int   numberOfZeroWords = std::count(theLineVector.begin(), theLineVector.end(), 0x0);
                        if(numberOfZeroWords <= 1)
                        {
                            phyPortDataVector.at(line).insert(phyPortDataVector.at(line).end(), theLineVector.begin(), theLineVector.end());
                            phyPortIterationVector.at(line)++;
                        }
                    }
                    minimumNumberOfIterations = *std::min_element(phyPortIterationVector.begin(), phyPortIterationVector.end());
                    ++totalIterationCounter;
                }
                for(uint8_t line = 0; line < NUMBER_OF_LINES_PER_CIC_PHY_PORTS; ++line)
                {
                    auto& lineDataVector = phyPortDataVector.at(line);
                    // making sure that problematic lines will be highlighted
                    if(phyPortIterationVector.at(line) < totalNumberOfRequiredIterations)
                    {
                        size_t                iterationSize = 10;
                        std::vector<uint32_t> emptyVector(iterationSize, 0x0);
                        for(size_t emptyPacketCounter = phyPortIterationVector.at(line); emptyPacketCounter < totalNumberOfRequiredIterations; ++emptyPacketCounter)
                        {
                            lineDataVector.insert(lineDataVector.end(), emptyVector.begin(), emptyVector.end());
                        }
                    }

                    auto& matchingEfficiency =
                        errorRatePerPhyPortMap[phyPort].getHybrid(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId())->getSummary<GenericDataArray<float, 4, 2>>().at(line);
                    matchingEfficiency = getMatchingEfficiency2SL1(lineDataVector);
                }
            }
        }
    }
    else
    {
        for(uint8_t line = 0; line < NUMBER_OF_LINES_PER_CIC_PHY_PORTS; ++line)
        {
            BoardDataContainer thePatternCounterCountainer;
            ContainerFactory::copyAndInitHybrid<GenericDataArray<uint64_t, 2>>(*theBoard, thePatternCounterCountainer);
            uint8_t patternId = 0;
            if(!isPS) patternId = (phyPort * 4 + line) % 5;
            fPatternCheckerHelper->patternCheckerTest(&thePatternCounterCountainer, line + 1, fPatternAndMaskContainerMap[patternId], fNumberOfTestedBits, true);

            for(auto theOpticalGroup: *errorRatePerPhyPortMap[phyPort].getBoard(theBoard->getId()))
            {
                for(auto theHybrid: *theOpticalGroup)
                {
                    auto&       theOutputErrorInfo   = theHybrid->getSummary<GenericDataArray<float, 4, 2>>().at(line);
                    const auto& theRecorderdErroInfo = thePatternCounterCountainer.getHybrid(theOpticalGroup->getId(), theHybrid->getId())->getSummary<GenericDataArray<uint64_t, 2>>();

                    theOutputErrorInfo.at(0) = theRecorderdErroInfo.at(0);
                    theOutputErrorInfo.at(1) = theRecorderdErroInfo.at(1);
                }
            }
        }
    }
}

std::pair<std::vector<uint32_t>, std::vector<uint32_t>> OTalignLpGBTinputsForBypass::getFullPatternAndMask(uint32_t thePattern, bool is10G)
{
    uint32_t theWord = 0;
    if(is10G)
    {
        uint32_t doubleDigitPattern = 0;
        for(uint8_t bit = 0; bit < 8; ++bit)
        {
            uint16_t singleBit = (thePattern >> bit) & 0x1;
            doubleDigitPattern |= ((singleBit << (2 * bit)) | singleBit << (2 * bit + 1));
        }
        for(size_t wordNumber = 0; wordNumber < 2; ++wordNumber) { theWord |= ((doubleDigitPattern & 0xffff) << wordNumber * 16); }
    }
    else
    {
        for(size_t wordNumber = 0; wordNumber < 4; ++wordNumber) { theWord |= ((thePattern & 0xff) << wordNumber * 8); }
    }
    std::vector<uint32_t> fullPattern(4);
    std::vector<uint32_t> fullPatternMask(4);
    for(size_t wordNumber = 0; wordNumber < 4; ++wordNumber)
    {
        fullPattern[wordNumber]     = theWord;
        fullPatternMask[wordNumber] = 0xffffffff;
    }

    return std::make_pair(fullPattern, fullPatternMask);
}

void OTalignLpGBTinputsForBypass::produceAllPatternAndMasks(BeBoard* theBoard)
{
    bool isA2Smodule = theBoard->getFirstObject()->getFrontEndType() == FrontEndType::OuterTracker2S;

    if(isA2Smodule)
    {
        for(uint8_t line = 0; line < fStubPattern2S.size(); ++line)
        {
            auto thePatternAndMask = getFullPatternAndMask(fStubPattern2S.at(line), false);
            ContainerFactory::copyAndInitHybrid<std::pair<std::vector<uint32_t>, std::vector<uint32_t>>>(*theBoard, fPatternAndMaskContainerMap[line], thePatternAndMask);
        }
    }
    else
    {
        bool is10G             = (static_cast<D19clpGBTInterface*>(flpGBTInterface)->GetChipRate(theBoard->getFirstObject()->flpGBT) == 10);
        auto thePatternAndMask = getFullPatternAndMask(fShiftRegisterPatternMPA, is10G);
        ContainerFactory::copyAndInitHybrid<std::pair<std::vector<uint32_t>, std::vector<uint32_t>>>(*theBoard, fPatternAndMaskContainerMap[0], thePatternAndMask);
    }
}

void OTalignLpGBTinputsForBypass::prepareMPAtoSendPatterns(BeBoard* theBoard)
{
    auto theMPAinterface = static_cast<PSInterface*>(fReadoutChipInterface)->fTheMPA2Interface;

    for(auto theOpticalGroup: *theBoard)
    {
        for(auto theHybrid: *theOpticalGroup)
        {
            for(auto theChip: *theHybrid)
            {
                if(theChip->getFrontEndType() != FrontEndType::MPA2) continue;
                theMPAinterface->WriteChipReg(theChip, "LFSR_data", fShiftRegisterPatternMPA);
                theMPAinterface->WriteChipRegBits(theChip, "Control_1", 0x2, "Mask", 0x03); // Enable shift register
                theMPAinterface->WriteChipRegBits(theChip, "ConfSLVS", 7, "Mask", 0x07);    // set slvs current to the maximum
            }
        }
    }
}

void OTalignLpGBTinputsForBypass::prepare2StoSendStubPatterns(BeBoard* theBoard)
{
    auto theCbcInterface = static_cast<CbcInterface*>(fReadoutChipInterface);

    for(auto theOpticalGroup: *theBoard)
    {
        for(auto theHybrid: *theOpticalGroup)
        {
            for(auto theChip: *theHybrid)
            {
                // switch on HitOr
                fReadoutChipInterface->WriteChipReg(theChip, "HitOr", 1);
                // set PtCut to maximum
                fReadoutChipInterface->WriteChipReg(theChip, "PtCut", 14);
                // disable cluster cut
                fReadoutChipInterface->WriteChipReg(theChip, "ClusterCut", 4);
                theCbcInterface->selectLogicMode(theChip, "Sampled", true, true);

                std::vector<std::pair<std::string, uint16_t>> theRegisterVector;
                theRegisterVector.push_back({"Bend7", 0x0A}); // forcing Bend7 (bending = 0) to ouput 0xA
                // theRegisterVector.push_back({"Bend8", 0x0C});              // forcing Bend8 (bending = 1) to ouput 0xC
                theRegisterVector.push_back({"CoincWind&Offset12", 0x00}); // set stub window offset to 0
                theRegisterVector.push_back({"CoincWind&Offset34", 0x00}); // set stub window offset to 0
                fReadoutChipInterface->WriteChipMultReg(theChip, theRegisterVector);

                std::vector<std::pair<uint8_t, int>> stubSeedAndBend{{fStubPattern2S.at(0), 0}, {fStubPattern2S.at(1), 0}, {fStubPattern2S.at(2), 0}};
                theCbcInterface->injectStubs(theChip, stubSeedAndBend);
            }
        }
    }
}

void OTalignLpGBTinputsForBypass::prepare2StoSendL1Patterns(BeBoard* theBoard)
{
    uint32_t triggerFrequency        = 1000; // do not change or it will not match padding 0s
    uint8_t  fakeHeaderChannelNumber = 24;
    fPattern2SL1.clear();
    fPattern2SL1.addToPattern(0x3, 0x3, 2); // CBC header
    fPattern2SL1.addToPattern(0x0, 0x0, 2); // error flags
    fPattern2SL1.addToPattern(0x0, 0x0, 9); // pipe address
    fPattern2SL1.addToPattern(0x0, 0x0, 9); // L1 counter

    for(uint8_t fakeHeaderChannel = 0; fakeHeaderChannel < fakeHeaderChannelNumber; ++fakeHeaderChannel)
    {
        fPattern2SL1.addToPattern(0x1, 0x1, 1); // fake channel header
    }

    for(uint8_t alternatedChannels = fakeHeaderChannelNumber; alternatedChannels < NCHANNELS; ++alternatedChannels)
    {
        fPattern2SL1.addToPattern((alternatedChannels + 1) % 2, 0x1, 1); // enable even numbers
    }

    uint32_t bitsBetweenConsecutiveTriggers = 40000 / triggerFrequency * 8;

    for(uint16_t paddingZeros = fPattern2SL1.getNumberOfPatternBits(); paddingZeros < bitsBetweenConsecutiveTriggers; ++paddingZeros)
    {
        fPattern2SL1.addToPattern(0, 0x1, 1); // padding zeros
    }

    auto                                          theCbcInterface = static_cast<CbcInterface*>(fReadoutChipInterface);
    std::vector<std::pair<std::string, uint32_t>> registerVector;
    registerVector.push_back({"fc7_daq_cnfg.fast_command_block.trigger_source", 3});
    registerVector.push_back({"fc7_daq_cnfg.fast_command_block.triggers_to_accept", 0});
    registerVector.push_back({"fc7_daq_cnfg.fast_command_block.user_trigger_frequency", triggerFrequency});
    registerVector.push_back({"fc7_daq_ctrl.fast_command_block.control.load_config", 0x1});
    fBeBoardInterface->WriteBoardMultReg(theBoard, registerVector);
    // fBeBoardInterface->WriteBoardReg(theBoard, "fc7_daq_ctrl.fast_command_block.control.start_trigger", 0x1);

    for(auto theOpticalGroup: *theBoard)
    {
        for(auto theHybrid: *theOpticalGroup)
        {
            for(auto theChip: *theHybrid)
            {
                fReadoutChipInterface->WriteChipReg(theChip, "ClusterCut", 4);
                fReadoutChipInterface->WriteChipReg(theChip, "VCth", 1023);
                theCbcInterface->selectLogicMode(static_cast<ReadoutChip*>(theChip), "Sampled", true, true);

                auto cChannelMask = std::make_shared<ChannelGroup<1, NCHANNELS>>();
                cChannelMask->disableAllChannels();
                for(uint8_t cChannel = 0; cChannel < NCHANNELS; cChannel += 2) cChannelMask->enableChannel(0, cChannel); // generate a hit in every Nth channel
                for(uint8_t cChannel = 0; cChannel < fakeHeaderChannelNumber; ++cChannel)
                    cChannelMask->enableChannel(0, cChannel); // generate a hit in the first 32 channels to create a sort of fake header
                fReadoutChipInterface->maskChannelGroup(static_cast<ReadoutChip*>(theChip), cChannelMask);
            }
        }
    }
}

void OTalignLpGBTinputsForBypass::setCICBypass(BeBoard* theBoard, uint8_t phyPort)
{
    for(auto theOpticalGroup: *theBoard)
    {
        for(auto theHybrid: *theOpticalGroup)
        {
            auto& theCic = static_cast<OuterTrackerHybrid*>(theHybrid)->fCic;
            fCicInterface->SelectOutput(theCic, false);
            fCicInterface->WriteChipReg(theCic, "MUX_CTRL", 0x10 | phyPort);
        }
    }
}

uint8_t OTalignLpGBTinputsForBypass::getBestPhase(const GenericDataArray<float, 15, 2>& thePhaseEfficiencyList, Hybrid* theHybrid, uint8_t line)
{
    // convert into a vector of error rates and find minimum
    std::vector<float> theErrorRateVector;
    float              theMinimumErrorRate = 1.;
    for(const auto& theTestedBitsAndErrors: thePhaseEfficiencyList)
    {
        float theTestedBits = theTestedBitsAndErrors.at(0);
        float theError      = theTestedBitsAndErrors.at(1);
        theErrorRateVector.push_back(theTestedBits > 0 ? theError / theTestedBits : 1.);
        if(theErrorRateVector.back() < theMinimumErrorRate) theMinimumErrorRate = theErrorRateVector.back();
    }

    // find minimum sequences

    bool                                     minimumFound = false;
    std::vector<std::pair<uint8_t, uint8_t>> minimumPhaseRanges;
    for(uint8_t phase = 0; phase < theErrorRateVector.size(); ++phase)
    {
        if(theErrorRateVector.at(phase) == theMinimumErrorRate)
        {
            if(!minimumFound)
            {
                minimumFound = true;
                minimumPhaseRanges.push_back({phase, phase});
            }
            else { minimumPhaseRanges.back().second = phase; }
        }
        else
            minimumFound = false;
    }

    // find longest minimum sequences
    uint8_t longestSequenceRange = minimumPhaseRanges.at(0).second - minimumPhaseRanges.at(0).first + 1;
    uint8_t longestSequenceIndex = 0;
    for(size_t index = 0; index < minimumPhaseRanges.size(); ++index)
    {
        uint8_t sequenceRange = minimumPhaseRanges.at(index).second - minimumPhaseRanges.at(index).first + 1;
        if(sequenceRange > longestSequenceRange)
        {
            longestSequenceRange = sequenceRange;
            longestSequenceIndex = index;
        }
    }

    // handle cases when only one minimum or no minumum is found
    if(longestSequenceRange > 8)
    {
        if(minimumPhaseRanges.at(longestSequenceIndex).first > 0) return minimumPhaseRanges.at(longestSequenceIndex).first + 4;
        if(minimumPhaseRanges.at(longestSequenceIndex).second < 14)
            return minimumPhaseRanges.at(longestSequenceIndex).second - 4;
        else
            return 7;
    }

    uint8_t longestSequenceCenter = (minimumPhaseRanges.at(longestSequenceIndex).second + minimumPhaseRanges.at(longestSequenceIndex).first) / 2;

    if(longestSequenceRange % 2 == 0)
    {
        uint8_t firstMinimumPhase           = minimumPhaseRanges.at(longestSequenceIndex).first;
        uint8_t lastMinimumPhase            = minimumPhaseRanges.at(longestSequenceIndex).second;
        uint8_t previousToFirstMinimumPhase = firstMinimumPhase == 0 ? (theErrorRateVector.size() - 1) : firstMinimumPhase - 1;
        uint8_t followingToLastMinimumPhase = lastMinimumPhase == (theErrorRateVector.size() - 1) ? 0 : lastMinimumPhase + 1;
        if(theErrorRateVector.at(followingToLastMinimumPhase) < theErrorRateVector.at(previousToFirstMinimumPhase)) { ++longestSequenceCenter; }
    }

    return longestSequenceCenter;
}

GenericDataArray<float, 2> OTalignLpGBTinputsForBypass::getMatchingEfficiency2SL1(std::vector<uint32_t> inputDataVector)
{
    GenericDataArray<float, 2> theTestedBitAndError;
    theTestedBitAndError.at(0)          = 0;
    theTestedBitAndError.at(1)          = 0;
    uint8_t numberOfWordsPerAcquisition = 10; // 10 32-bit-words per acquisition;
    float   numberOfAcquisitions        = inputDataVector.size() / numberOfWordsPerAcquisition;
    for(size_t acquisitionNumber = 0; acquisitionNumber < numberOfAcquisitions; ++acquisitionNumber)
    {
        size_t                firstIndex = acquisitionNumber * numberOfWordsPerAcquisition;
        size_t                lastIndex  = firstIndex + numberOfWordsPerAcquisition;
        std::vector<uint32_t> singleAcquisitionInputDataVector(inputDataVector.begin() + firstIndex, inputDataVector.begin() + lastIndex);
        auto                  reorderedSingleAcquisitionInputDataVector = reorderPattern(singleAcquisitionInputDataVector, 1);
        auto                  maximumEfficiency                         = fPattern2SL1.getNumberOfMatchingBitsForAllBitshifts<320>(reorderedSingleAcquisitionInputDataVector);
        theTestedBitAndError.at(1) += maximumEfficiency;
    }

    theTestedBitAndError.at(0) = fPattern2SL1.getNumberOfMaskedBits() * numberOfAcquisitions;
    theTestedBitAndError.at(1) = theTestedBitAndError.at(0) - theTestedBitAndError.at(1);

    return theTestedBitAndError;
}
