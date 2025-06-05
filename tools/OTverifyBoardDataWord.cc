#include "tools/OTverifyBoardDataWord.h"
#include "HWDescription/BeBoard.h"
#include "HWInterface/D19cFWInterface.h"
#include "System/RegisterHelper.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/GenericDataArray.h"
#include "Utils/PatternMatcher.h"
#include "Utils/Utilities.h"
#include "tools/OTPatternCheckerHelper.h"
#include <sstream>

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

std::string OTverifyBoardDataWord::fCalibrationDescription = "Use features of the CIC and FW to verify correct alignment of L1 and stub words in the FW";

OTverifyBoardDataWord::OTverifyBoardDataWord() : Tool() {}

OTverifyBoardDataWord::~OTverifyBoardDataWord() { delete fPatternCheckerHelper; }

void OTverifyBoardDataWord::Initialise(void)
{
    fRegisterHelper->takeSnapshot();
    // free the registers in case any
    fNumberOfStubBits     = findValueInSettings<double>("OTverifyBoardDataWord_NumberOfTestedStubBits", 1e8);
    fNumberOfL1Bits       = findValueInSettings<double>("OTverifyBoardDataWord_NumberOfTestedL1Bits", 1e6);
    fDoMatchingInFirmware = findValueInSettings<double>("OTverifyBoardDataWord_DoMatchingInFirmware", 1) > 0;
    fIsKickoff            = findValueInSettings<double>("isKickoff", 0) > 0;

    setUpPatternMatching();
#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: plots are booked during initialization
    fDQMHistogramOTverifyBoardDataWord.book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif
}

void OTverifyBoardDataWord::setUpPatternMatching()
{
    size_t                     numberOfLines = (fDetectorContainer->getFirstObject()->getFirstObject()->getFrontEndType() == FrontEndType::OuterTrackerPS) ? 7 : 6;
    GenericDataArray<float, 2> theInitialBitAndError;
    theInitialBitAndError.at(0) = 0.;
    theInitialBitAndError.at(1) = 0.;
    std::vector<GenericDataArray<float, 2>> theInitialVector(numberOfLines, theInitialBitAndError);
    ContainerFactory::copyAndInitHybrid<std::vector<GenericDataArray<float, 2>>>(*fDetectorContainer, fPatternMatchingBitErrorContainer, theInitialVector);

    fPatternCheckerHelper = new OTPatternCheckerHelper();
    fPatternCheckerHelper->Inherit(this);
    fPatternCheckerHelper->prepareCalibration();
    fPatternCheckerHelper->setSuppressErrorPrintout(true);
}

void OTverifyBoardDataWord::ConfigureCalibration() {}

void OTverifyBoardDataWord::Running()
{
    LOG(INFO) << "Starting OTverifyBoardDataWord measurement.";
    Initialise();
    runIntegrityTest();
    LOG(INFO) << "Done with OTverifyBoardDataWord.";
    Reset();
}

void OTverifyBoardDataWord::Stop(void)
{
    LOG(INFO) << "Stopping OTverifyBoardDataWord measurement.";
#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: processing the histograms
    fDQMHistogramOTverifyBoardDataWord.process();
#endif
    SaveResults();
    closeFileHandler();
    LOG(INFO) << "OTverifyBoardDataWord stopped.";
}

void OTverifyBoardDataWord::Pause() {}

void OTverifyBoardDataWord::Resume() {}

void OTverifyBoardDataWord::Reset() { fRegisterHelper->restoreSnapshot(); }

void OTverifyBoardDataWord::runIntegrityTest()
{
    LOG(INFO) << BOLDYELLOW << "OTverifyBoardDataWord::runIntegrityTest ... start integrity test" << RESET;

    for(auto theBoard: *fDetectorContainer)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup) { prepareHybridForStubIntegrityTest(theHybrid); }
        }

        uint8_t numberOfBytesInSinglePacket = getNumberOfBytesInSinglePacket(theBoard->getFirstObject());

        auto theFWInterface = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(theBoard));
        if(fDoMatchingInFirmware)
            runStubIntegrityTestFirmwareMatch(theBoard);
        else
            runStubIntegrityTestSoftwareMatch(theBoard, theFWInterface, numberOfBytesInSinglePacket);

        prepareFWForL1IntegrityTest(theBoard);

        auto thePatternMatcher = createTheL1PatternMatcher(theBoard);
        runL1IntegrityTest(theBoard, theFWInterface, numberOfBytesInSinglePacket, thePatternMatcher);
    }

#ifdef __USE_ROOT__
    fDQMHistogramOTverifyBoardDataWord.fillPatternErrorRate(fPatternMatchingBitErrorContainer);
#else
    if(fDQMStreamerEnabled)
    {
        ContainerSerialization theMatchingEfficiencyContainerSerialization("OTverifyBoardDataWordMatchingEfficiency");
        theMatchingEfficiencyContainerSerialization.streamByOpticalGroupContainer(fDQMStreamer, fPatternMatchingBitErrorContainer);
    }
#endif
}

PatternMatcher OTverifyBoardDataWord::createTheL1PatternMatcher(Ph2_HwDescription::BeBoard* theBoard)
{
    bool isPS = theBoard->getFirstObject()->getFrontEndType() == FrontEndType::OuterTrackerPS;

    PatternMatcher thePatternMatcher;
    thePatternMatcher.addToPattern(0x0ffffffe, 0xffffffff, 32); // CIC header plus 0 in front added in the transmission
    thePatternMatcher.addToPattern(0x0, 0x1ff, 9);
    thePatternMatcher.addToPattern(0x0, 0x0, 9);
    thePatternMatcher.addToPattern(0, 0x7f, 7);
    thePatternMatcher.addToPattern(0x0, 0x1, 1);
    if(isPS)
    {
        thePatternMatcher.addToPattern(0, 0x7f, 7);
        thePatternMatcher.addToPattern(0, 0x7, 3); // padding
        thePatternMatcher.addToPattern(0xaaaaaaa, 0xfffffff, 28);
    }
    else
    {
        thePatternMatcher.addToPattern(0x0a, 0x3f, 6); // padding
        thePatternMatcher.addToPattern(0xaaaaaaaa, 0xffffffff, 32);
    }

    for(size_t index = 0; index < 47; ++index) { thePatternMatcher.addToPattern(0xaaaaaaaa, 0xffffffff, 32); }

    return thePatternMatcher;
}

void OTverifyBoardDataWord::runStubIntegrityTestFirmwareMatch(BeBoard* theBoard, bool runAlignment)
{
    LOG(INFO) << BOLDMAGENTA << "Running runStubIntegrityTest" << RESET;

    size_t cNlines = (theBoard->getFirstObject()->getFrontEndType() == FrontEndType::OuterTrackerPS) ? 7 : 6;

    std::vector<uint32_t> pattern{0xeaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa};
    std::vector<uint32_t> patternMask{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff};
    if(static_cast<D19clpGBTInterface*>(flpGBTInterface)->GetChipRate(theBoard->getFirstObject()->flpGBT) == 5) pattern.at(2) = 0xeaaaaaaa;

    for(size_t line = 1; line < cNlines; ++line)
    {
        BoardDataContainer thePatternCounterCountainer;
        ContainerFactory::copyAndInitHybrid<GenericDataArray<uint64_t, 2>>(*theBoard, thePatternCounterCountainer);
        fPatternCheckerHelper->patternCheckerTest(&thePatternCounterCountainer, line, pattern, patternMask, fNumberOfStubBits, runAlignment);

        for(auto theOpticalGroup: *fPatternMatchingBitErrorContainer.getBoard(theBoard->getId()))
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                auto&       theOutputErrorInfo   = theHybrid->getSummary<std::vector<GenericDataArray<float, 2>>>().at(line);
                const auto& theRecorderdErroInfo = thePatternCounterCountainer.getHybrid(theOpticalGroup->getId(), theHybrid->getId())->getSummary<GenericDataArray<uint64_t, 2>>();

                theOutputErrorInfo.at(0) = theRecorderdErroInfo.at(0);
                theOutputErrorInfo.at(1) = theRecorderdErroInfo.at(1);
            }
        }
    }
}

void OTverifyBoardDataWord::runStubIntegrityTestSoftwareMatch(BeBoard* theBoard, D19cFWInterface* theFWInterface, uint8_t numberOfBytesInSinglePacket)
{
    LOG(INFO) << BOLDMAGENTA << "Running runStubIntegrityTest" << RESET;
    size_t numberOfIterations = fNumberOfStubBits / 320;
    fBeBoardInterface->WriteBoardReg(theBoard, "fc7_daq_cnfg.physical_interface_block.slvs_debug.chip_select", 0);

    for(auto theOpticalGroup: *theBoard)
    {
        if(fIsKickoff && (theOpticalGroup->getFrontEndType() == FrontEndType::OuterTracker2S))
            LOG(INFO) << BOLDYELLOW << "Attention! ignoring failures on right hybrid CIC line 4 due to bug in kickoff SEH!" << RESET;
        size_t cNlines = (theOpticalGroup->getFrontEndType() == FrontEndType::OuterTrackerPS) ? 6 : 5;
        for(auto theHybrid: *theOpticalGroup)
        {
            auto& theHybridPatternMatchingEfficiency = fPatternMatchingBitErrorContainer.getObject(theBoard->getId())
                                                           ->getObject(theOpticalGroup->getId())
                                                           ->getObject(theHybrid->getId())
                                                           ->getSummary<std::vector<GenericDataArray<float, 2>>>();

            fBeBoardInterface->WriteBoardReg(fDetectorContainer->getObject(theHybrid->getBeBoardId()), "fc7_daq_cnfg.physical_interface_block.slvs_debug.hybrid_select", theHybrid->getId());

            for(size_t iteration = 0; iteration < numberOfIterations; iteration++)
            {
                auto lineOutputVector = theFWInterface->StubDebug(true, cNlines, false);
                for(size_t lineIndex = 0; lineIndex < lineOutputVector.size(); ++lineIndex)
                {
                    size_t wordBitSize = lineOutputVector.at(lineIndex).size() * 32;

                    theHybridPatternMatchingEfficiency.at(lineIndex + 1).at(0) += wordBitSize;
                    if(!isStubPatternMatched(lineOutputVector.at(lineIndex), numberOfBytesInSinglePacket, fFlagCharacter, fIdleCharacter))
                    {
                        theHybridPatternMatchingEfficiency.at(lineIndex + 1).at(1) += wordBitSize;
                        if(!(fIsKickoff && ((theHybrid->getId() % 2) == 0) && ((lineIndex) == 4) && (theOpticalGroup->getFrontEndType() == FrontEndType::OuterTracker2S)))
                            LOG(ERROR) << BOLDRED << "Error on stub line " << lineIndex + 1 << " occurred in iteration number " << +iteration << RESET;
                    }
                }
            }
        }
    }
}

void OTverifyBoardDataWord::prepareHybridForStubIntegrityTest(Hybrid* theHybrid)
{
    auto& cCic = static_cast<OuterTrackerHybrid*>(theHybrid)->fCic;
    fCicInterface->SelectOutput(cCic, true);
    fCicInterface->EnableFEs(cCic, {0, 1, 2, 3, 4, 5, 6, 7}, false);
}

bool OTverifyBoardDataWord::isStubPatternMatched(const std::vector<uint32_t>& theWordVector, uint8_t numberOfBytesInSinglePacket, uint8_t flagCharacter, uint8_t idleCharacter)
{
    // create a mask that is 0xFF for 5G and 0xFFFF for 10G modules
    uint16_t mask = 0xFF;
    if(numberOfBytesInSinglePacket == 2) mask = 0xFFFF;

    uint8_t numberOfIdleCharacters = (numberOfBytesInSinglePacket == 1) ? 7 : 15;

    enum SearchPatternStatus
    {
        Idle,
        FlagFound,
        Error
    } status = Idle;

    uint8_t  numberOfSinglePacketsInOneWord    = sizeof(uint32_t) / numberOfBytesInSinglePacket;
    uint16_t totalNumberOfSinglePackets        = theWordVector.size() * numberOfSinglePacketsInOneWord;
    uint16_t currentSinglePacketNumber         = 0;
    uint8_t  numberOfConsecutiveIdleCharacters = 0;
    bool     firstFlagCharacterFound           = false;
    while(currentSinglePacketNumber < totalNumberOfSinglePackets)
    {
        uint16_t currentSinglePacket =
            ((theWordVector.at(currentSinglePacketNumber / numberOfSinglePacketsInOneWord)) >> (currentSinglePacketNumber % numberOfSinglePacketsInOneWord * 8 * numberOfBytesInSinglePacket)) & mask;
        ++currentSinglePacketNumber;
        for(int byteShift = 0; byteShift < numberOfBytesInSinglePacket; ++byteShift)
        {
            uint8_t currentByte = (currentSinglePacket >> (8 * byteShift)) & 0xFF;
            switch(status)
            {
            case SearchPatternStatus::Idle: // I am in Idle, looking for flagCharacter
            {
                if(currentByte == idleCharacter)
                {
                    ++numberOfConsecutiveIdleCharacters;
                    if(numberOfConsecutiveIdleCharacters > numberOfIdleCharacters) // too many Idle characters!!!
                    {
                        status = SearchPatternStatus::Error;
                    }
                }
                else if(currentByte == flagCharacter)
                {
                    if(firstFlagCharacterFound && numberOfConsecutiveIdleCharacters != numberOfIdleCharacters) // not enough idle characters!!!
                    {
                        status = SearchPatternStatus::Error;
                    }
                    else
                    {
                        firstFlagCharacterFound = true;
                        status                  = SearchPatternStatus::FlagFound;
                    }
                }
                else // unrecognized character!!!
                {
                    status = SearchPatternStatus::Error;
                }

                break;
            }

            case SearchPatternStatus::FlagFound: // I found the flag, now I expect to fo back to Idle
            {
                numberOfConsecutiveIdleCharacters = 0;
                if(currentByte == idleCharacter)
                {
                    ++numberOfConsecutiveIdleCharacters;
                    status = SearchPatternStatus::Idle;
                }
                else // no idle character found after flag!!!
                {
                    status = SearchPatternStatus::Error;
                }
                break;
            }

            case SearchPatternStatus::Error: // error case
            {
                LOG(DEBUG) << BOLDRED << "OTverifyBoardDataWord::isStubPatternMatched - Error, expected pattern not found" << RESET;
                LOG(DEBUG) << BOLDRED << getPatternPrintout(theWordVector, numberOfBytesInSinglePacket, true) << RESET;
                return false;
            }

            default: // this shold never happen
            {
                LOG(ERROR) << BOLDRED << "OTverifyBoardDataWord::isStubPatternMatched - Error, state machine went into default state, it should never happen" << RESET;
                return false;
            }
            }
        }
    }

    return true;
}

void OTverifyBoardDataWord::runL1IntegrityTest(BeBoard*            theBoard,
                                               D19cFWInterface*    theFWInterface,
                                               uint8_t             numberOfBytesInSinglePacket,
                                               PatternMatcher&     thePatternMatcher,
                                               BoardDataContainer* theAlignmentResultContainer)
{
    LOG(INFO) << BOLDMAGENTA << "Running runL1IntegrityTest" << RESET;

    float numberOfMatchedBits = thePatternMatcher.getNumberOfMaskedBits();

    size_t numberOfIterations = std::ceil(fNumberOfL1Bits / numberOfMatchedBits);

    for(auto theOpticalGroup: *theBoard)
    {
        for(auto theHybrid: *theOpticalGroup)
        {
            auto& theHybridPatternMatchingEfficiency = fPatternMatchingBitErrorContainer.getObject(theBoard->getId())
                                                           ->getObject(theOpticalGroup->getId())
                                                           ->getObject(theHybrid->getId())
                                                           ->getSummary<std::vector<GenericDataArray<float, 2>>>();

            if(theAlignmentResultContainer != nullptr)
            {
                if(!theAlignmentResultContainer->getHybrid(theOpticalGroup->getId(), theHybrid->getId())->getSummary<bool>())
                {
                    theHybridPatternMatchingEfficiency.at(0).at(0) = numberOfMatchedBits;
                    theHybridPatternMatchingEfficiency.at(0).at(1) = numberOfMatchedBits;
                    continue;
                }
            }

            prepareHybridForL1IntegrityTest(theHybrid);

            size_t numberOfIgnoredPatterns = 0;
            for(size_t iteration = 0; iteration < numberOfIterations;)
            {
                auto lineOutputVector        = theFWInterface->L1ADebug(1, false);
                auto orderedLineOutputVector = reorderPattern(lineOutputVector, numberOfBytesInSinglePacket);

                float numberOrErrorBits = numberOfMatchedBits - thePatternMatcher.countMatchingBits(orderedLineOutputVector);
                if(numberOrErrorBits > 0)
                {
                    if(std::all_of(orderedLineOutputVector.begin(), orderedLineOutputVector.end(), [](int i) { return i == 0; }))
                    {
                        ++numberOfIgnoredPatterns;
                        continue;
                    }
                    size_t numberOfEmpyWords = 0;
                    for(auto theWord: orderedLineOutputVector)
                    {
                        if(theWord == 0) ++numberOfEmpyWords;
                    }
                    if(numberOfIgnoredPatterns < numberOfIterations / 10 && numberOfEmpyWords > 1)
                    {
                        ++numberOfIgnoredPatterns;
                        continue; // means very likely the fifo did not save properly the data
                    }
                    if(fPrintError) LOG(INFO) << BOLDRED << "Pattern did not match for iteration number " << +iteration << RESET;

                    LOG(DEBUG) << BOLDRED << "OTverifyBoardDataWord::runL1IntegrityTest - Error, expected L1 pattern not found for Board " << +theBoard->getId() << " OpticalGroup "
                               << +theOpticalGroup->getId() << " Hybrid " << +theHybrid->getId() << " on iteration number " << +iteration << RESET;
                    LOG(DEBUG) << BOLDRED << "L1 data received    " << getPatternPrintout(orderedLineOutputVector, numberOfBytesInSinglePacket) << RESET;
                    LOG(DEBUG) << BOLDRED << "L1 pattern expected " << getPatternPrintout(thePatternMatcher.getPattern(), numberOfBytesInSinglePacket) << RESET;
                    LOG(DEBUG) << BOLDRED << "L1 pattern mask     " << getPatternPrintout(thePatternMatcher.getMask(), numberOfBytesInSinglePacket) << RESET;
                }
                theHybridPatternMatchingEfficiency.at(0).at(0) += numberOfMatchedBits;
                theHybridPatternMatchingEfficiency.at(0).at(1) += numberOrErrorBits;
                ++iteration;
            }
        }
    }
}

bool OTverifyBoardDataWord::isL1HeaderFound(const std::vector<uint32_t>& theWordVector, uint8_t numberOfBytesInSinglePacket, uint32_t header, uint32_t headerMask)
{
    auto orderedLineOutputVector = reorderPattern(theWordVector, numberOfBytesInSinglePacket);

    std::pair<bool, size_t> isFoundAndWhere = matchPattern(orderedLineOutputVector, numberOfBytesInSinglePacket, header, headerMask);
    return isFoundAndWhere.first;
}

void OTverifyBoardDataWord::prepareHybridForL1IntegrityTest(Ph2_HwDescription::Hybrid* theHybrid)
{
    // select lines for slvs debug
    fBeBoardInterface->WriteBoardReg(fDetectorContainer->getObject(theHybrid->getBeBoardId()), "fc7_daq_cnfg.physical_interface_block.slvs_debug.hybrid_select", theHybrid->getId());
    fBeBoardInterface->WriteBoardReg(fDetectorContainer->getObject(theHybrid->getBeBoardId()), "fc7_daq_cnfg.physical_interface_block.slvs_debug.chip_select", 0);
}
void OTverifyBoardDataWord::prepareFWForL1IntegrityTest(Ph2_HwDescription::BeBoard* theBoard, uint32_t theTriggerFrequency)
{
    // Set board trigger configuration for L1 alignment
    std::vector<std::pair<std::string, uint32_t>> cVecReg;
    cVecReg.push_back({"fc7_daq_cnfg.fast_command_block.triggers_to_accept", 0});
    cVecReg.push_back({"fc7_daq_cnfg.fast_command_block.misc.backpressure_enable", 0});
    cVecReg.push_back({"fc7_daq_cnfg.fast_command_block.user_trigger_frequency", theTriggerFrequency});
    cVecReg.push_back({"fc7_daq_cnfg.fast_command_block.misc.trigger_multiplicity", 0});
    cVecReg.push_back({"fc7_daq_ctrl.fast_command_block.control.load_config", 0x1});
    cVecReg.push_back({"fc7_daq_cnfg.tlu_block.tlu_enabled", 0x0});
    cVecReg.push_back({"fc7_daq_cnfg.readout_block.global.data_handshake_enable", 0x1});
    fBeBoardInterface->WriteBoardMultReg(theBoard, cVecReg);

    for(auto theOpticalGroup: *theBoard)
    {
        for(auto theHybrid: *theOpticalGroup) { fCicInterface->SetSparsification(static_cast<OuterTrackerHybrid*>(theHybrid)->fCic, true); }
    }
}

uint8_t OTverifyBoardDataWord::getNumberOfBytesInSinglePacket(OpticalGroup* cOpticalGroup) const
{
    return (static_cast<D19clpGBTInterface*>(flpGBTInterface)->GetChipRate(cOpticalGroup->flpGBT) == 10) ? 2 : 1;
}