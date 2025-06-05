#include "tools/OTBitErrorRateTest.h"
#include "HWDescription/VTRx.h"
#include "HWInterface/D19cBERTinterface.h"
#include "HWInterface/D19cBackendAlignmentFWInterface.h"
#include "HWInterface/D19cFWInterface.h"
#include "HWInterface/ExceptionHandler.h"
#include "HWInterface/VTRxInterface.h"
#include "System/RegisterHelper.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/GenericDataArray.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

std::string OTBitErrorRateTest::fCalibrationDescription = "Bit error rate test";

OTBitErrorRateTest::OTBitErrorRateTest() : OTalignBoardDataWord() {}

OTBitErrorRateTest::~OTBitErrorRateTest() {}

void OTBitErrorRateTest::Initialise(void)
{
    fRegisterHelper->takeSnapshot();
    // free the registers in case any
    initializeContainers();
    fBroadcastAlignSetting = 1;

    fNumberOfBits = findValueInSettings<double>("OTBitErrorRateTest_NumberOfBits", 1E10);

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: plots are booked during initialization
    fDQMHistogramOTBitErrorRateTest.book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif
}

void OTBitErrorRateTest::ConfigureCalibration() {}

void OTBitErrorRateTest::Running()
{
    Initialise();
    bitErrorRateTest();
    Reset();
}

void OTBitErrorRateTest::Stop(void)
{
    LOG(INFO) << "Stopping OTBitErrorRateTest measurement.";
#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: processing the histograms
    fDQMHistogramOTBitErrorRateTest.process();
#endif
    SaveResults();
    closeFileHandler();
    LOG(INFO) << "OTBitErrorRateTest stopped.";
}

void OTBitErrorRateTest::Pause() {}

void OTBitErrorRateTest::Resume() {}

void OTBitErrorRateTest::Reset() { fRegisterHelper->restoreSnapshot(); }

void OTBitErrorRateTest::bitErrorRateTestPerLine(Ph2_HwDescription::BeBoard* theBoard,
                                                 BoardDataContainer*         theBertContainer,
                                                 BoardDataContainer*         theFECContainer,
                                                 BoardDataContainer*         thePhaseClockDelayContainer,
                                                 float                       numberOfBits,
                                                 uint8_t                     lineNumber)
{
    bool is10Gmodule = flpGBTInterface->GetChipRate(theBoard->getFirstObject()->flpGBT) == 10;

    uint16_t iteration                 = 0;
    uint16_t maximumNumberOfIterations = 10;
    bool     allAligned                = false;
    while(iteration < maximumNumberOfIterations)
    {
        allAligned = true;
        for(auto theOpticalGroup: *theBoard)
        {
            if(!static_cast<D19clpGBTInterface*>(flpGBTInterface)->enablePRBS(theOpticalGroup, thePhaseClockDelayContainer->getOpticalGroup(theOpticalGroup->getId())->getSummary<uint16_t>()))
            {
                LOG(WARNING) << WARNING_FORMAT << "Failed to align LpGBT on Board " << theOpticalGroup->getBeBoardId() << " OpticalGroup " << theOpticalGroup->getId() << ", retrying "
                             << maximumNumberOfIterations - iteration << " more times" << RESET;
                allAligned = false;
            }
        }
        if(allAligned) break;
        ++iteration;
    }

    if(!allAligned) { LOG(ERROR) << ERROR_FORMAT << "Failed to align LpGBT on Board " << theBoard->getId() << " after " << maximumNumberOfIterations << "trials" << RESET; }

    D19cBackendAlignmentFWInterface* theAlignerInterface = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(theBoard))->getBackendAlignmentInterface();

    for(auto theOpticalGroup: *theBoard)
    {
        for(auto theHybrid: *theOpticalGroup)
        {
            theAlignerInterface->enableAlignmentOnPRBS(theHybrid->getId());
            if(!tryLineAlignment(theAlignerInterface, theHybrid, lineNumber))
            {
                if(!fSuppressErrorPrintout)
                    LOG(ERROR) << ERROR_FORMAT << "Failed to align OpticalGroup " << theOpticalGroup->getId() << " Hybrid " << theHybrid->getId() << " line " << +lineNumber << RESET;
            }
            theAlignerInterface->disableAlignmentOnPRBS(theHybrid->getId());
        }
    }

    D19cBERTinterface* theBERTinterface = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(theBoard))->getBERTinterface();

    theBERTinterface->setUsePRBS(true);
    theBERTinterface->setSuppressErrorPrintout(fSuppressErrorPrintout);

    fBeBoardInterface->WriteBoardReg(theBoard, "fc7_daq_cnfg.physical_interface_block.lpgbt_fec_config.fec_err_cnt_en_bit", 1);
    fBeBoardInterface->WriteBoardReg(theBoard, "fc7_daq_cnfg.physical_interface_block.lpgbt_fec_config.fec_err_cnt_rst_bit", 1);
    fBeBoardInterface->WriteBoardReg(theBoard, "fc7_daq_cnfg.physical_interface_block.lpgbt_fec_config.fec_err_cnt_rst_bit", 0);

    BoardDataContainer bertResultsBoardContainer = theBERTinterface->runBERTonSingleLine(theBoard, lineNumber, is10Gmodule, numberOfBits);

    for(auto theOpticalGroup: bertResultsBoardContainer)
    {
        for(auto theHybrid: *theOpticalGroup)
        {
            const auto& receivedBERTresultsVector                                                                                  = theHybrid->getSummary<GenericDataArray<uint64_t, 2>>();
            theBertContainer->getHybrid(theOpticalGroup->getId(), theHybrid->getId())->getSummary<GenericDataArray<uint64_t, 2>>() = receivedBERTresultsVector;
        }
    }
    if(theFECContainer != nullptr)
    {
        for(auto theOpticalGroup: *theFECContainer)
        {
            fBeBoardInterface->WriteBoardReg(theBoard, "fc7_daq_cnfg.physical_interface_block.lpgbt_fec_config.fec_err_cnt_sel_offset", theOpticalGroup->getId());
            auto theFECcounter                      = fBeBoardInterface->ReadBoardReg(theBoard, "fc7_daq_stat.physical_interface_block.lpgbt_fec_counter");
            theOpticalGroup->getSummary<uint32_t>() = theFECcounter;
        }
    }
}

void OTBitErrorRateTest::bitErrorRateTest(uint8_t line)
{
    LOG(INFO) << BOLDBLUE << "Running BERT phase scan on line " << +line << RESET;

    bool     is10Gmodule  = flpGBTInterface->GetChipRate(fDetectorContainer->getFirstObject()->getFirstObject()->flpGBT) == 10;
    uint16_t maximumPhase = is10Gmodule ? 32 : 64;

    std::map<uint16_t, DetectorDataContainer> thePhaseScanContainer;
    for(uint16_t phase = 0; phase < maximumPhase; ++phase) { ContainerFactory::copyAndInitHybrid<GenericDataArray<uint64_t, 2>>(*fDetectorContainer, thePhaseScanContainer[phase]); }

    DetectorDataContainer theComulativeCountainer;
    ContainerFactory::copyAndInitOpticalGroup<std::map<uint16_t, GenericDataArray<float, 2>>>(*fDetectorContainer, theComulativeCountainer);

    DetectorDataContainer theBERTcounterCountainer;
    ContainerFactory::copyAndInitHybrid<GenericDataArray<uint64_t, 2>>(*fDetectorContainer, theBERTcounterCountainer);

    DetectorDataContainer theFECcounterCountainer;
    ContainerFactory::copyAndInitOpticalGroup<uint32_t>(*fDetectorContainer, theFECcounterCountainer);

    DetectorDataContainer theBestPhaseCountainer;
    ContainerFactory::copyAndInitOpticalGroup<uint16_t>(*fDetectorContainer, theBestPhaseCountainer);

    fSuppressErrorPrintout = true;
    for(uint16_t phase = 0; phase < maximumPhase; ++phase)
    {
        DetectorDataContainer thePhaseCountainer;
        ContainerFactory::copyAndInitOpticalGroup<uint16_t>(*fDetectorContainer, thePhaseCountainer, phase);

        for(auto theBoard: *fDetectorContainer)
        {
            bitErrorRateTestPerLine(theBoard, thePhaseScanContainer[phase].getBoard(theBoard->getId()), nullptr, thePhaseCountainer.getBoard(theBoard->getId()), 1e6, line);

            for(auto theOpticalGroup: *theBoard)
            {
                auto& theMapElement = theComulativeCountainer.getOpticalGroup(theBoard->getId(), theOpticalGroup->getId())->getSummary<std::map<uint16_t, GenericDataArray<float, 2>>>()[phase];
                theMapElement.at(0) = 0;
                theMapElement.at(1) = 0;
                for(auto theHybrid: *theOpticalGroup)
                {
                    const auto& theHybridElement = thePhaseScanContainer[phase].getHybrid(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId())->getSummary<GenericDataArray<uint64_t, 2>>();
                    theMapElement.at(0) += theHybridElement.at(0);
                    theMapElement.at(1) += theHybridElement.at(1);
                }
                theMapElement.at(1) = theMapElement.at(0) == 0 ? 1. : theMapElement.at(1) / theMapElement.at(0);
            }
        }
    }
    fSuppressErrorPrintout = false;

    // find mimumum BERT
    DetectorDataContainer theBERTcounterMinimum;
    float                 theMinimum = 1.;
    ContainerFactory::copyAndInitOpticalGroup<float>(*fDetectorContainer, theBERTcounterMinimum, theMinimum);

    for(auto theBoard: theComulativeCountainer)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            auto& theOpticalGroupMinimum = theBERTcounterMinimum.getOpticalGroup(theBoard->getId(), theOpticalGroup->getId())->getSummary<float>();
            for(auto cumulativeErrorRate: theOpticalGroup->getSummary<std::map<uint16_t, GenericDataArray<float, 2>>>())
            {
                if(cumulativeErrorRate.second.at(1) < theOpticalGroupMinimum) theOpticalGroupMinimum = cumulativeErrorRate.second.at(1);
            }
        }
    }

    // find minimum sequences
    DetectorDataContainer theBERTcounterMinimumSequences;
    ContainerFactory::copyAndInitOpticalGroup<std::vector<std::pair<uint16_t, uint16_t>>>(*fDetectorContainer, theBERTcounterMinimumSequences);
    for(auto theBoard: theComulativeCountainer)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            bool  minimumFound           = false;
            auto  theOpticalGroupMinimum = theBERTcounterMinimum.getOpticalGroup(theBoard->getId(), theOpticalGroup->getId())->getSummary<float>();
            auto& minimumPhaseRanges     = theBERTcounterMinimumSequences.getOpticalGroup(theBoard->getId(), theOpticalGroup->getId())->getSummary<std::vector<std::pair<uint16_t, uint16_t>>>();
            for(auto cumulativeErrorRate: theOpticalGroup->getSummary<std::map<uint16_t, GenericDataArray<float, 2>>>())
            {
                if(cumulativeErrorRate.second.at(1) == theOpticalGroupMinimum)
                {
                    if(!minimumFound)
                    {
                        minimumFound = true;
                        minimumPhaseRanges.push_back({cumulativeErrorRate.first, cumulativeErrorRate.first});
                    }
                    else { minimumPhaseRanges.back().second = cumulativeErrorRate.first; }
                }
                else
                    minimumFound = false;
            }
        }
    }

    // find longest minimum sequences
    for(auto theBoard: theBERTcounterMinimumSequences)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            const auto& minimumPhaseRanges   = theBERTcounterMinimumSequences.getOpticalGroup(theBoard->getId(), theOpticalGroup->getId())->getSummary<std::vector<std::pair<uint16_t, uint16_t>>>();
            uint16_t    longestSequenceRange = 0;
            uint16_t    longestSequenceIndex = 0;
            for(size_t index = 0; index < minimumPhaseRanges.size(); ++index)
            {
                uint16_t sequenceRange = minimumPhaseRanges.at(index).second - minimumPhaseRanges.at(index).first;
                if(sequenceRange > longestSequenceRange)
                {
                    longestSequenceRange = sequenceRange;
                    longestSequenceIndex = index;
                }
            }

            theBestPhaseCountainer.getOpticalGroup(theBoard->getId(), theOpticalGroup->getId())->getSummary<uint16_t>() = minimumPhaseRanges.at(longestSequenceIndex).first + longestSequenceRange / 2;
        }
    }

    for(auto theBoard: *fDetectorContainer)
    {
        bitErrorRateTestPerLine(theBoard,
                                theBERTcounterCountainer.getBoard(theBoard->getId()),
                                theFECcounterCountainer.getBoard(theBoard->getId()),
                                theBestPhaseCountainer.getBoard(theBoard->getId()),
                                fNumberOfBits,
                                line);
    }

#ifdef __USE_ROOT__
    for(uint16_t phase = 0; phase < maximumPhase; ++phase) { fDQMHistogramOTBitErrorRateTest.fillErrorCounterPhaseScan(thePhaseScanContainer[phase], phase, line); }
    fDQMHistogramOTBitErrorRateTest.fillBERTbestPhase(theBestPhaseCountainer, line);
    fDQMHistogramOTBitErrorRateTest.fillErrorCounter(theBERTcounterCountainer, line);
    fDQMHistogramOTBitErrorRateTest.fillFECcounter(theFECcounterCountainer, line);
#else
    if(fDQMStreamerEnabled)
    {
        for(uint16_t phase = 0; phase < maximumPhase; ++phase)
        {
            ContainerSerialization theErrorCounterSerializationPhaseScan("OTBitErrorRateTestErrorCounterPhaseScan");
            theErrorCounterSerializationPhaseScan.streamByOpticalGroupContainer(fDQMStreamer, thePhaseScanContainer[phase], phase, line);
        }

        ContainerSerialization theBestPhaseSerialization("OTBitErrorRateTestBestPhase");
        theBestPhaseSerialization.streamByOpticalGroupContainer(fDQMStreamer, theBestPhaseCountainer, line);

        ContainerSerialization theErrorCounterSerialization("OTBitErrorRateTestErrorCounter");
        theErrorCounterSerialization.streamByOpticalGroupContainer(fDQMStreamer, theBERTcounterCountainer, line);

        ContainerSerialization theFECcounterSerialization("OTBitErrorRateTestFECcounter");
        theFECcounterSerialization.streamByOpticalGroupContainer(fDQMStreamer, theFECcounterCountainer, line);
    }
#endif
}

void OTBitErrorRateTest::bitErrorRateTest()
{
    // std::vector<std::pair<std::string, uint16_t>> theRegisterValues;
    // theRegisterValues.push_back({"CH1BIAS", 40});
    // theRegisterValues.push_back({"CH1MOD", 18 | 0x80});
    // for(auto theBoard: *fDetectorContainer)
    // {
    //     for(auto theOpticalGroup: *theBoard)
    //     {
    //         fVTRxInterface->WriteChipMultReg(theOpticalGroup->fVTRx, theRegisterValues, false);
    //     }
    // }
    uint8_t numberOfLines = fDetectorContainer->getFirstObject()->getFirstObject()->getFrontEndType() == FrontEndType::OuterTrackerPS ? 7 : 6;
    for(uint8_t line = 0; line < numberOfLines; ++line) { bitErrorRateTest(line); }
}
