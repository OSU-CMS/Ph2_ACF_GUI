#include "tools/OTCICtoLpGBTecv.h"
#include "HWDescription/BeBoard.h"
#include "HWInterface/D19cBackendAlignmentFWInterface.h"
#include "HWInterface/D19cFWInterface.h"
#include "HWInterface/D19cTriggerInterface.h"
#include "System/RegisterHelper.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/GenericDataArray.h"
#include "Utils/PatternMatcher.h"
#include "tools/OTPatternCheckerHelper.h"
#include <algorithm>
#include <unordered_set>

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

std::string OTCICtoLpGBTecv::fCalibrationDescription = "Performs the Electric Chain Validation between the lpGBT and the CIC using the algorithms implemented in OTverifyBoardDataWord.";

OTCICtoLpGBTecv::OTCICtoLpGBTecv() : OTverifyBoardDataWord() {}

OTCICtoLpGBTecv::~OTCICtoLpGBTecv() {}

void OTCICtoLpGBTecv::Initialise(void)
{
    fPrintError = false;
    fRegisterHelper->takeSnapshot();
    // free the registers in case any
    fNumberOfL1Bits       = findValueInSettings<double>("OTCICtoLpGBTecv_NumberOfL1Bits", 1e5);
    fNumberOfStubBits     = findValueInSettings<double>("OTCICtoLpGBTecv_NumberOfStubBits", 1e6);
    fDoMatchingInFirmware = true;
    fListOfLpGBTPhase     = convertStringToFloatList(findValueInSettings<std::string>("OTCICtoLpGBTecv_LpGBTPhase", "0-14"));
    fListOfCICStrength    = convertStringToFloatList(findValueInSettings<std::string>("OTCICtoLpGBTecv_CICStrength", "1, 3, 5"));
    fListOfClockPolarity  = convertStringToFloatList(findValueInSettings<std::string>("OTCICtoLpGBTecv_ClockPolarity", "0-1"));
    fListOfClockStrength  = convertStringToFloatList(findValueInSettings<std::string>("OTCICtoLpGBTecv_ClockStrength", "1, 4, 7"));

    // Error handle for incorrect LpGBTPhase input
    std::unordered_set<float> allowedLpGBTPhases{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
    std::unordered_set<float> phaseValues;
    for(float phase: fListOfLpGBTPhase)
    {
        if(phaseValues.find(phase) != phaseValues.end())
        {
            LOG(ERROR) << BOLDRED << "Error, repeat detected in OTCICtoLpGBTecv_LpGBTPhase parameter." << std::endl;
            throw Exception("Repeat detected in OTCICtoLpGBTecv_LpGBTPhase parameter");
        }
        if(allowedLpGBTPhases.find(phase) == allowedLpGBTPhases.end())
        {
            LOG(ERROR) << BOLDRED << "Error, " << phase
                       << " is not an allowed value for the OTCICtoLpGBTecv_LpGBTPhase parameter. The allowed values are: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, and 14." << std::endl;
            throw Exception("Unallowed value for the OTCICtoLpGBTecv_LpGBTPhase parameter");
        }
        phaseValues.insert(phase);
    }

    // Error handle for incorrect CICStrength input
    std::unordered_set<float> allowedCICStrengths{1, 2, 3, 4, 5};
    std::unordered_set<float> cicStrengthValues;
    for(float cicStrength: fListOfCICStrength)
    {
        if(cicStrengthValues.find(cicStrength) != cicStrengthValues.end())
        {
            LOG(ERROR) << BOLDRED << "Error, repeat detected in OTCICtoLpGBTecv_CICStrength parameter." << std::endl;
            throw Exception("Repeat detected in OTCICtoLpGBTecv_CICStrength parameter");
        }
        if(allowedCICStrengths.find(cicStrength) == allowedCICStrengths.end())
        {
            LOG(ERROR) << BOLDRED << "Error, " << cicStrength << " is not an allowed value for the OTCICtoLpGBTecv_CICStrength parameter. The allowed values are: 1, 2, 3, 4, and 5." << std::endl;
            throw Exception("Unallowed value for the OTCICtoLpGBTecv_CICStrength parameter");
        }
        cicStrengthValues.insert(cicStrength);
    }

    // Error handle for incorrect ClockPolarity input
    std::unordered_set<float> allowedClockPolarities{0, 1};
    std::unordered_set<float> clockPolarityValues;
    for(float clockPolarity: fListOfClockPolarity)
    {
        if(clockPolarityValues.find(clockPolarity) != clockPolarityValues.end())
        {
            LOG(ERROR) << BOLDRED << "Error, repeat detected in OTCICtoLpGBTecv_ClockPolarity parameter." << std::endl;
            throw Exception("Repeat detected in OTCICtoLpGBTecv_ClockPolarity parameter");
        }
        if(allowedClockPolarities.find(clockPolarity) == allowedClockPolarities.end())
        {
            LOG(ERROR) << BOLDRED << "Error, " << clockPolarity << " is not an allowed value for the OTCICtoLpGBTecv_ClockPolarity parameter. The allowed values are 0 and 1." << std::endl;
            throw Exception("Unallowed value for the OTCICtoLpGBTecv_ClockPolarity parameter");
        }
        clockPolarityValues.insert(clockPolarity);
    }

    // Error handle for incorrect ClockStrength input
    std::unordered_set<float> allowedClockStrengths{1, 2, 3, 4, 5, 6, 7};
    std::unordered_set<float> clockStrengthValues;
    for(float clockStrength: fListOfClockStrength)
    {
        if(clockStrengthValues.find(clockStrength) != clockStrengthValues.end())
        {
            LOG(ERROR) << BOLDRED << "Error, repeat detected in OTCICtoLpGBTecv_ClockStrength parameter." << std::endl;
            throw Exception("Repeat detected in OTCICtoLpGBTecv_ClockStrength parameter");
        }
        if(allowedClockStrengths.find(clockStrength) == allowedClockStrengths.end())
        {
            LOG(ERROR) << BOLDRED << "Error, " << clockStrength << " is not an allowed value for the OTCICtoLpGBTecv_ClockStrength parameter. The allowed values are: 1, 2, 3, 4, 5, 6, and 7."
                       << std::endl;
            throw Exception("Unallowed value for the OTCICtoLpGBTecv_ClockStrength parameter");
        }
        clockStrengthValues.insert(clockStrength);
    }

    setUpPatternMatching();

    ContainerFactory::copyAndInitBoard<PatternMatcher>(*fDetectorContainer, fTheBoardPatternMatcher);
    ContainerFactory::copyAndInitBoard<uint8_t>(*fDetectorContainer, fTheBoardNumberOfBytesInPattern);

    for(auto theBoard: *fDetectorContainer)
    {
        fTheBoardPatternMatcher.getObject(theBoard->getId())->getSummary<PatternMatcher>()  = createTheL1PatternMatcher(theBoard);
        fTheBoardNumberOfBytesInPattern.getObject(theBoard->getId())->getSummary<uint8_t>() = getNumberOfBytesInSinglePacket(theBoard->getFirstObject());
    }

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: plots are booked during initialization
    fDQMHistogramOTCICtoLpGBTecv.book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif
}

void OTCICtoLpGBTecv::ConfigureCalibration() {}

void OTCICtoLpGBTecv::Running()
{
    LOG(INFO) << BOLDMAGENTA << "Starting OTCICtoLpGBTecv measurement." << RESET;
    Initialise();
    runECV();
    LOG(INFO) << BOLDGREEN << "Done with OTCICtoLpGBTecv." << RESET;
    Reset();
}

void OTCICtoLpGBTecv::runECV()
{
    for(auto theBoard: *fDetectorContainer)
    {
        prepareFWForL1IntegrityTest(theBoard);
        static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(theBoard))->getTriggerInterface()->Start(true);

        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup) { prepareHybridForStubIntegrityTest(theHybrid); }
        }
    }

    for(uint8_t clockPolarity: fListOfClockPolarity)
    {
        LOG(INFO) << BOLDMAGENTA << "CLOCK POLARITY: " << +clockPolarity << RESET;
        for(uint8_t clockStrength: fListOfClockStrength)
        {
            LOG(INFO) << BOLDMAGENTA << "    CLOCK STRENGTH: " << +clockStrength << RESET;
            for(uint8_t cicStrength: fListOfCICStrength)
            {
                LOG(INFO) << BOLDMAGENTA << "        CIC STRENGTH: " << +cicStrength << RESET;

                for(auto phase: fListOfLpGBTPhase)
                {
                    std::string printout = "            RX PHASE: ";
                    int         i        = 0;
                    printout += (" " + std::to_string(+static_cast<int>(phase)));
                    if(i > 0) std::cout << "\x1b[A";
                    i++;
                    LOG(INFO) << BOLDMAGENTA << printout << RESET;

                    runECVPoint(clockPolarity, clockStrength, cicStrength, phase);
                } // LpGBT phase
            } // cic strenght
        } // clock strenght
    } // clock polarity
}

void OTCICtoLpGBTecv::runECVPoint(uint8_t clockPolarity, uint8_t clockStrength, uint8_t cicStrength, uint8_t phase)
{
    // set LpGBT and CIC parameters
    for(auto theBoard: *fDetectorContainer)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            static_cast<D19clpGBTInterface*>(flpGBTInterface)->setCICClockPolarityAndStrength(theOpticalGroup->flpGBT, clockPolarity, clockStrength, theOpticalGroup);
            std::map<uint8_t, std::vector<uint8_t>> theGroupsAndChannels = theOpticalGroup->getLpGBTrxGroupsAndChannels();
            auto&                                   clpGBT               = theOpticalGroup->flpGBT;
            flpGBTInterface->ConfigureAllRxPhase(clpGBT, phase, theGroupsAndChannels);

            for(auto theHybrid: *theOpticalGroup)
            {
                auto& cCic = static_cast<OuterTrackerHybrid*>(theHybrid)->fCic;
                fCicInterface->ConfigureDriveStrength(cCic, cicStrength);

                // reset the number of matches!!
                for(auto& theNumberOfBitsAndErrors:
                    fPatternMatchingBitErrorContainer.getHybrid(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId())->getSummary<std::vector<GenericDataArray<float, 2>>>())
                {
                    theNumberOfBitsAndErrors.at(0) = 0;
                    theNumberOfBitsAndErrors.at(1) = 0;
                }
            }
        }
    }

    // run stub integrity test
    for(auto theBoard: *fDetectorContainer)
    {
        D19cBackendAlignmentFWInterface* theAlignerInterface = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(theBoard))->getBackendAlignmentInterface();
        theAlignerInterface->setSuppressPrintout(true);
        runStubIntegrityTestFirmwareMatch(theBoard, true);
        theAlignerInterface->setSuppressPrintout(false);
    }

    DetectorDataContainer alignmentResultContainer;
    ContainerFactory::copyAndInitHybrid<bool>(*fDetectorContainer, alignmentResultContainer);

    // prepare and align L1
    for(auto theBoard: *fDetectorContainer)
    {
        D19cBackendAlignmentFWInterface* theAlignerInterface = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(theBoard))->getBackendAlignmentInterface();
        theAlignerInterface->setSuppressPrintout(true);
        // fBeBoardInterface->ChipReSync(theBoard);
        static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(theBoard))->getTriggerInterface()->Start(true);
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                alignmentResultContainer.getHybrid(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId())->getSummary<bool>() =
                    fPatternCheckerHelper->tryLineAlignment(theAlignerInterface, theHybrid, 0);
            }
        }
        theAlignerInterface->setSuppressPrintout(false);
        // fBeBoardInterface->Stop(theBoard);
    }

    // run L1 integrity test
    for(auto theBoard: *fDetectorContainer)
    {
        auto theFWInterface = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(theBoard));
        runL1IntegrityTest(theBoard,
                           theFWInterface,
                           fTheBoardNumberOfBytesInPattern.getObject(theBoard->getId())->getSummary<uint8_t>(),
                           fTheBoardPatternMatcher.getObject(theBoard->getId())->getSummary<PatternMatcher>(),
                           alignmentResultContainer.getBoard(theBoard->getId()));
    }

    auto    phaseIterator = std::find(fListOfLpGBTPhase.begin(), fListOfLpGBTPhase.end(), phase);
    uint8_t phaseIndex    = std::distance(fListOfLpGBTPhase.begin(), phaseIterator) + 1;
#ifdef __USE_ROOT__
    // Find the pClockStrength and pPhase indices

    fDQMHistogramOTCICtoLpGBTecv.fillEfficiency(clockPolarity, clockStrength, cicStrength, phaseIndex, fPatternMatchingBitErrorContainer);
#else
    if(fDQMStreamerEnabled)
    {
        // Find the pClockStrength and pPhase indices
        ContainerSerialization theECVlpGBTCICContainerSerialization("OTCICtoLpGBTecvEfficiencyHistogram");
        theECVlpGBTCICContainerSerialization.streamByOpticalGroupContainer(fDQMStreamer, fPatternMatchingBitErrorContainer, clockPolarity, clockStrength, cicStrength, phaseIndex);
    }
#endif
}

void OTCICtoLpGBTecv::Stop(void)
{
    LOG(INFO) << "Stopping OTCICtoLpGBTecv measurement.";
#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: processing the histograms
    fDQMHistogramOTCICtoLpGBTecv.process();
#endif
    SaveResults();
    closeFileHandler();
    LOG(INFO) << "OTCICtoLpGBTecv stopped.";
}

void OTCICtoLpGBTecv::Pause() {}

void OTCICtoLpGBTecv::Resume() {}

void OTCICtoLpGBTecv::Reset() { fRegisterHelper->restoreSnapshot(); }
