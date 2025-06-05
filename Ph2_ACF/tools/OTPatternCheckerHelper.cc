#include "tools/OTPatternCheckerHelper.h"
#include "HWInterface/D19cBERTinterface.h"
#include "HWInterface/D19cBackendAlignmentFWInterface.h"
#include "HWInterface/D19cFWInterface.h"
#include "System/RegisterHelper.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/GenericDataArray.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

std::string OTPatternCheckerHelper::fCalibrationDescription = "Pattern checker test";

OTPatternCheckerHelper::OTPatternCheckerHelper() : OTalignBoardDataWord() {}

OTPatternCheckerHelper::~OTPatternCheckerHelper() {}

void OTPatternCheckerHelper::Initialise(void)
{
    fRegisterHelper->takeSnapshot();
    // free the registers in case any
    prepareCalibration();

    fNumberOfBits = findValueInSettings<double>("OTPatternCheckerHelper_NumberOfBits", 1E10);

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: plots are booked during initialization
    fDQMHistogramOTPatternCheckerHelper.book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif
}

void OTPatternCheckerHelper::prepareCalibration()
{
    initializeContainers();
    fBroadcastAlignSetting = 0;
}

void OTPatternCheckerHelper::ConfigureCalibration() {}

void OTPatternCheckerHelper::Running()
{
    LOG(INFO) << "Starting OTPatternCheckerHelper measurement.";
    Initialise();
    patternCheckerTest();
    LOG(INFO) << "Done with OTPatternCheckerHelper.";
    Reset();
}

void OTPatternCheckerHelper::Stop(void)
{
    LOG(INFO) << "Stopping OTPatternCheckerHelper measurement.";
#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: processing the histograms
    fDQMHistogramOTPatternCheckerHelper.process();
#endif
    SaveResults();
    closeFileHandler();
    LOG(INFO) << "OTPatternCheckerHelper stopped.";
}

void OTPatternCheckerHelper::Pause() {}

void OTPatternCheckerHelper::Resume() {}

void OTPatternCheckerHelper::Reset() { fRegisterHelper->restoreSnapshot(); }

void OTPatternCheckerHelper::patternCheckerTest(BoardDataContainer*          theErrorBitContainer,
                                                uint8_t                      line,
                                                const std::vector<uint32_t>& pattern,
                                                const std::vector<uint32_t>& patternMask,
                                                float                        numberOfBits,
                                                bool                         runAlignment)

{
    BoardDataContainer                                      thePatternAndMaskContainer;
    std::pair<std::vector<uint32_t>, std::vector<uint32_t>> theInitialPatternAndMask{pattern, patternMask};
    ContainerFactory::copyAndInitHybrid<std::pair<std::vector<uint32_t>, std::vector<uint32_t>>>(
        *fDetectorContainer->getObject(theErrorBitContainer->getId()), thePatternAndMaskContainer, theInitialPatternAndMask);
    patternCheckerTest(theErrorBitContainer, line, thePatternAndMaskContainer, numberOfBits, runAlignment);
}

void OTPatternCheckerHelper::patternCheckerTest(BoardDataContainer* theErrorBitContainer,
                                                uint8_t             line,
                                                BoardDataContainer& thePatternAndMaskContainer,
                                                float               numberOfBits,
                                                bool                runAlignment,
                                                BoardDataContainer* theAlignmentPatternAndMaskContainer)
{
    auto theBoard = fDetectorContainer->getObject(theErrorBitContainer->getId());

    bool is10Gmodule = flpGBTInterface->GetChipRate(theBoard->getFirstObject()->flpGBT) == 10;

    if(runAlignment)
    {
        D19cBackendAlignmentFWInterface* theAlignerInterface = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(theBoard))->getBackendAlignmentInterface();
        theAlignerInterface->setSuppressErrorPrintout(fSuppressErrorPrintout);

        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                if(theAlignmentPatternAndMaskContainer == nullptr)
                {
                    const auto& thePatternAndMask =
                        thePatternAndMaskContainer.getHybrid(theOpticalGroup->getId(), theHybrid->getId())->getSummary<std::pair<std::vector<uint32_t>, std::vector<uint32_t>>>();
                    theAlignerInterface->enableAlignmentOnCustomPattern(theHybrid->getId(), (thePatternAndMask.first.at(0) >> 16 & 0xffff), (thePatternAndMask.second.at(0) >> 16 & 0xffff));
                }
                else
                {
                    const auto& thePatternAndMask = theAlignmentPatternAndMaskContainer->getHybrid(theOpticalGroup->getId(), theHybrid->getId())->getSummary<std::pair<uint16_t, uint16_t>>();
                    theAlignerInterface->enableAlignmentOnCustomPattern(theHybrid->getId(), thePatternAndMask.first, thePatternAndMask.second);
                }
                bool success = tryLineAlignment(theAlignerInterface, theHybrid, line);
                theAlignerInterface->disableAlignmentOnCustomPattern(theHybrid->getId());
                if(!success)
                {
                    if(!fSuppressErrorPrintout)
                        LOG(ERROR) << ERROR_FORMAT << "Failed to align OpticalGroup " << theOpticalGroup->getId() << " Hybrid " << theHybrid->getId() << " line " << +line << RESET;
                }
            }
        }
    }

    D19cBERTinterface* theBERTinterface = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(theBoard))->getBERTinterface();

    theBERTinterface->setUsePRBS(false);
    theBERTinterface->setSuppressErrorPrintout(fSuppressErrorPrintout);

    for(auto theOpticalGroup: *theBoard)
    {
        for(auto theHybrid: *theOpticalGroup)
        {
            const auto& thePatternAndMask = thePatternAndMaskContainer.getHybrid(theOpticalGroup->getId(), theHybrid->getId())->getSummary<std::pair<std::vector<uint32_t>, std::vector<uint32_t>>>();
            theBERTinterface->setCheckedPattern(theHybrid->getId(), thePatternAndMask.first);
            theBERTinterface->setCheckedPatternMask(theHybrid->getId(), thePatternAndMask.second);
        }
    }

    BoardDataContainer bertResultsBoardContainer = theBERTinterface->runBERTonSingleLine(theBoard, line, is10Gmodule, numberOfBits);

    for(auto theOpticalGroup: bertResultsBoardContainer)
    {
        for(auto theHybrid: *theOpticalGroup)
        {
            auto receivedBERTresultsVector = theHybrid->getSummary<GenericDataArray<uint64_t, 2>>();
            if(receivedBERTresultsVector.at(0) == 0)
            {
                receivedBERTresultsVector.at(0) = numberOfBits;
                receivedBERTresultsVector.at(1) = numberOfBits;
            }
            theErrorBitContainer->getHybrid(theOpticalGroup->getId(), theHybrid->getId())->getSummary<GenericDataArray<uint64_t, 2>>() = receivedBERTresultsVector;
        }
    }
}

void OTPatternCheckerHelper::patternCheckerTest()
{
    for(auto theBoard: *fDetectorContainer)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            LOG(INFO) << BOLDMAGENTA << "    Optical Group " << +theOpticalGroup->getId() << RESET;
            for(auto theHybrid: *theOpticalGroup)
            {
                LOG(INFO) << BOLDMAGENTA << "        Hybrid " << +theHybrid->getId() << RESET;
                auto& cCic = static_cast<OuterTrackerHybrid*>(theHybrid)->fCic;
                fCicInterface->SelectOutput(cCic, true);
            }
        }
    }

    uint8_t numberOfLines = fDetectorContainer->getFirstObject()->getFirstObject()->getFrontEndType() == FrontEndType::OuterTrackerPS ? 7 : 6;
    for(uint8_t line = 1; line < numberOfLines; ++line)
    {
        DetectorDataContainer thePatternCounterCountainer;
        ContainerFactory::copyAndInitHybrid<GenericDataArray<uint64_t, 2>>(*fDetectorContainer, thePatternCounterCountainer);
        for(auto theBoard: thePatternCounterCountainer)
        {
            std::vector<uint32_t> pattern{0xeaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa};
            std::vector<uint32_t> patternMask{0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff};
            if(static_cast<D19clpGBTInterface*>(flpGBTInterface)->GetChipRate(fDetectorContainer->getObject(theBoard->getId())->getFirstObject()->flpGBT) == 5) pattern.at(2) = 0xeaaaaaaa;
            patternCheckerTest(theBoard, line, pattern, patternMask, fNumberOfBits, true);
        }

#ifdef __USE_ROOT__
        fDQMHistogramOTPatternCheckerHelper.fillErrorCounter(thePatternCounterCountainer, line);
#else
        if(fDQMStreamerEnabled)
        {
            ContainerSerialization theErrorCounterSerialization("OTPatternCheckerHelperErrorCounter");
            theErrorCounterSerialization.streamByOpticalGroupContainer(fDQMStreamer, thePatternCounterCountainer, line);
        }
#endif
    }
}
