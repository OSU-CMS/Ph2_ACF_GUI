#include "tools/OTCICphaseAlignment.h"
#include "HWInterface/CbcInterface.h"
#include "HWInterface/D19cDebugFWInterface.h"
#include "HWInterface/D19cFWInterface.h"
#include "HWInterface/D19cL1ReadoutInterface.h"
#include "HWInterface/ExceptionHandler.h"
#include "HWInterface/TriggerInterface.h"
#include "System/RegisterHelper.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/GenericDataArray.h"
#include <sstream>

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

std::string OTCICphaseAlignment::fCalibrationDescription = "Run the CIC automatic procedure to find the sampling phase for CBC/MPA stub and L1 lines";

OTCICphaseAlignment::OTCICphaseAlignment() : Tool() {}

OTCICphaseAlignment::~OTCICphaseAlignment() {}

void OTCICphaseAlignment::Initialise(void)
{
    fRegisterHelper->takeSnapshot();
    fRegisterHelper->freeFrontEndRegister(FrontEndType::CIC2, "PHY_PORT_CONFIG");
    fRegisterHelper->freeFrontEndRegister(FrontEndType::CIC2, "^scPhaseSelectB[0-3]i[0-5]$");

    fNumberOfAlignmentIterations = findValueInSettings<double>("OTCICphaseAlignment_NumberOfAlignmentIterations", 100);
    fMinLockingSuccessRate       = findValueInSettings<double>("OTCICphaseAlignment_MinLockingSuccessRate", 1.);

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: plots are booked during initialization
    fDQMHistogramOTCICphaseAlignment.book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif
}

void OTCICphaseAlignment::ConfigureCalibration() {}

void OTCICphaseAlignment::Running()
{
    LOG(INFO) << "Starting OTCICphaseAlignment measurement.";
    Initialise();
    phaseAlignment();
    LOG(INFO) << "Done with OTCICphaseAlignment.";
    Reset();
}

void OTCICphaseAlignment::Stop(void)
{
    LOG(INFO) << "Stopping OTCICphaseAlignment measurement.";
#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: processing the histograms
    fDQMHistogramOTCICphaseAlignment.process();
#endif
    SaveResults();
    closeFileHandler();
    LOG(INFO) << "OTCICphaseAlignment stopped.";
}

void OTCICphaseAlignment::Pause() {}

void OTCICphaseAlignment::Resume() {}

void OTCICphaseAlignment::Reset() { fRegisterHelper->restoreSnapshot(); }

void OTCICphaseAlignment::phaseAlignment()
{
    LOG(INFO) << BOLDBLUE << "Starting CIC automated phase alignment procedure" << RESET;
    std::string theQueryFunction = "skipSSAQuery";
    auto        theSkipSSAquery  = [](const ChipContainer* theReadoutChip)
    {
        if(static_cast<const ReadoutChip*>(theReadoutChip)->getFrontEndType() == FrontEndType::SSA2) return false;
        return true;
    };
    fDetectorContainer->addReadoutChipQueryFunction(theSkipSSAquery, theQueryFunction);

    DetectorDataContainer thePhaseHistogramContainer;
    ContainerFactory::copyAndInitHybrid<GenericDataArray<float, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS, 16>>(*fDetectorContainer, thePhaseHistogramContainer);

    DetectorDataContainer theBestPhaseContainer;
    ContainerFactory::copyAndInitHybrid<GenericDataArray<uint8_t, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS>>(*fDetectorContainer, theBestPhaseContainer);

    DetectorDataContainer theLockingEfficiencyContainer;
    ContainerFactory::copyAndInitHybrid<GenericDataArray<float, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS>>(*fDetectorContainer, theLockingEfficiencyContainer);

    for(auto theBoard: *fDetectorContainer)
    {
        fBeBoardInterface->WriteBoardReg(theBoard, "fc7_daq_cnfg.fast_command_block.user_trigger_frequency", 100); // too low trigger rate does not work
        // all modules must be of the same type
        bool isPSmodule = theBoard->getFirstObject()->getFrontEndType() == FrontEndType::OuterTrackerPS;
        // generate alignment pattern on all stub lines
        LOG(INFO) << BOLDBLUE << "Generating Patterns needed for phase alignment of CIC inputs." << RESET;

        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                auto& cCic = static_cast<OuterTrackerHybrid*>(theHybrid)->fCic;
                fCicInterface->SetAutomaticPhaseAlignment(cCic, true);
                // configure Chips to produce phase alignment patterns
            }
        }

        auto thePhaseHistogramBoardDataContainer    = thePhaseHistogramContainer.getObject(theBoard->getId());
        auto theLockingEfficiencyBoardDataContainer = theLockingEfficiencyContainer.getObject(theBoard->getId());
        auto theBestPhaseBoardDataContainer         = theBestPhaseContainer.getObject(theBoard->getId());
        if(isPSmodule)
            AlignAllCICinputsPS(theBoard, thePhaseHistogramBoardDataContainer, theLockingEfficiencyBoardDataContainer, theBestPhaseBoardDataContainer);
        else
            AlignAllCICinputs2S(theBoard, thePhaseHistogramBoardDataContainer, theLockingEfficiencyBoardDataContainer, theBestPhaseBoardDataContainer);

        // normalize efficiency histogram
        for(auto opticalGroup: *theLockingEfficiencyBoardDataContainer)
        {
            for(auto hybrid: *opticalGroup)
            {
                auto& theLockingEfficiency = hybrid->getSummary<GenericDataArray<float, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS>>();
                for(uint8_t frontEnd = 0; frontEnd < NUMBER_OF_CIC_PORTS; ++frontEnd)
                {
                    for(uint8_t line = 0; line < NUMBER_OF_LINES_PER_CIC_PORTS; ++line) { theLockingEfficiency.at(frontEnd).at(line) /= fNumberOfAlignmentIterations; }
                }
            }
        }

        // normalize phase histogram
        for(auto opticalGroup: *thePhaseHistogramBoardDataContainer)
        {
            for(auto hybrid: *opticalGroup)
            {
                auto& thePhaseHistogram = hybrid->getSummary<GenericDataArray<float, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS, 16>>();
                for(uint8_t frontEnd = 0; frontEnd < NUMBER_OF_CIC_PORTS; ++frontEnd)
                {
                    for(uint8_t line = 0; line < NUMBER_OF_LINES_PER_CIC_PORTS; ++line)
                    {
                        for(uint8_t phase = 0; phase < 16; ++phase) thePhaseHistogram.at(frontEnd).at(line).at(phase) /= fNumberOfAlignmentIterations;
                    }
                }
            }
        }

        // check alignment
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                // enable automatic phase aligner
                const auto& theLockingEfficiency = theLockingEfficiencyBoardDataContainer->getObject(theOpticalGroup->getId())
                                                       ->getObject(theHybrid->getId())
                                                       ->getSummary<GenericDataArray<float, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS>>();
                bool cLocked = true;
                for(auto theChip: *theHybrid)
                {
                    for(uint8_t line = 0; line < NUMBER_OF_LINES_PER_CIC_PORTS; ++line)
                    {
                        if(theLockingEfficiency.at(theChip->getId() % 8).at(line) < fMinLockingSuccessRate)
                        {
                            std::stringstream errorMessage;
                            errorMessage << "OTCICphaseAlignment::phaseAlignment - Error in aligning CIC on ";
                            if(line == 0)
                                errorMessage << "L1 line";
                            else
                                errorMessage << "Stub line " << +(line - 1);
                            errorMessage << " - locking efficiency = " << theLockingEfficiency.at(theChip->getId() % 8).at(line) << " less then minimum requited (" << fMinLockingSuccessRate << ")";
                            errorMessage << " - Chip  " << +theChip->getId() << " Hybrid " << +theHybrid->getId() << " OpticalGroup " << +theOpticalGroup->getId() << " BeBoard " << +theBoard->getId();
                            LOG(ERROR) << BOLDRED << errorMessage.str() << RESET;
                            cLocked = false;
                        }
                    }
                }
                std::stringstream message;
                message << BOLDBLUE << "Phase aligner on CIC" << +theHybrid->getId();
                if(cLocked)
                    message << BOLDGREEN << " LOCKED ";
                else
                    message << BOLDRED << " FAILED to LOCK ";
                message << BOLDBLUE << " ... storing values and switching to static phase " << RESET;
                LOG(INFO) << message.str();
                if(!cLocked)
                {
                    LOG(INFO) << BOLDRED << "FAILED to lock CIC inputs on Board id " << +theBoard->getId() << " OpticalGroup id" << +theOpticalGroup->getId() << " Hybrid id" << +theHybrid->getId()
                              << " --- OpticalGroup will be disabled" << RESET;
                    ExceptionHandler::getInstance()->disableOpticalGroup(theBoard->getId(), theOpticalGroup->getId());
                    continue;
                }
            } // CICs
        } // OG

        // Find and set best phases
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                auto& thePhaseHistogram = thePhaseHistogramContainer.getObject(theBoard->getId())
                                              ->getObject(theOpticalGroup->getId())
                                              ->getObject(theHybrid->getId())
                                              ->getSummary<GenericDataArray<float, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS, 16>>();
                auto& theBestPhase = theBestPhaseContainer.getObject(theBoard->getId())
                                         ->getObject(theOpticalGroup->getId())
                                         ->getObject(theHybrid->getId())
                                         ->getSummary<GenericDataArray<uint8_t, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS>>();
                for(uint8_t frontEnd = 0; frontEnd < NUMBER_OF_CIC_PORTS; ++frontEnd)
                {
                    for(uint8_t line = 0; line < NUMBER_OF_LINES_PER_CIC_PORTS; ++line)
                    {
                        float   bestPhaseEfficiency = 0;
                        uint8_t bestPhase           = 15;
                        for(uint8_t phase = 0; phase < 16; ++phase)
                        {
                            auto theCurrentEfficiency = thePhaseHistogram.at(frontEnd).at(line).at(phase);
                            if(theCurrentEfficiency > bestPhaseEfficiency)
                            {
                                bestPhase           = phase;
                                bestPhaseEfficiency = theCurrentEfficiency;
                            }
                        }
                        theBestPhase.at(frontEnd).at(line) = bestPhase;
                    }
                }
                auto& cCic = static_cast<OuterTrackerHybrid*>(theHybrid)->fCic;
                fCicInterface->SetStaticPhaseAlignment(cCic);
                fCicInterface->writeAllTaps(cCic, theBestPhase);
            }
        }
    }

#ifdef __USE_ROOT__
    fDQMHistogramOTCICphaseAlignment.fillPhaseHistogramResults(thePhaseHistogramContainer);
    fDQMHistogramOTCICphaseAlignment.fillBestPhaseResults(theBestPhaseContainer);
    fDQMHistogramOTCICphaseAlignment.fillLockingEfficiencyResults(theLockingEfficiencyContainer);
#else
    if(fDQMStreamerEnabled)
    {
        ContainerSerialization thePhaseHistogramContainerSerialization("OTCICphaseAlignmentPhaseHistogram");
        thePhaseHistogramContainerSerialization.streamByOpticalGroupContainer(fDQMStreamer, thePhaseHistogramContainer);

        ContainerSerialization theBestPhaseContainerSerialization("OTCICphaseAlignmentBestPhase");
        theBestPhaseContainerSerialization.streamByOpticalGroupContainer(fDQMStreamer, theBestPhaseContainer);

        ContainerSerialization theLockingEfficiencyContainerSerialization("OTCICphaseAlignmentLockingEfficiency");
        theLockingEfficiencyContainerSerialization.streamByOpticalGroupContainer(fDQMStreamer, theLockingEfficiencyContainer);
    }
#endif

    fDetectorContainer->removeReadoutChipQueryFunction(theQueryFunction);
}

void OTCICphaseAlignment::AlignAllCICinputsPS(BeBoard*            theBoard,
                                              BoardDataContainer* thePhaseHistogramBoardDataContainer,
                                              BoardDataContainer* theLockingEfficiencyBoardDataContainer,
                                              BoardDataContainer* theBestPhaseBoardDataContainer)
{
    // Set all MPAs to procude the alignment pattern
    for(auto theOpticalGroup: *theBoard)
    {
        for(auto theHybrid: *theOpticalGroup)
        {
            for(auto theReadoutChip: *theHybrid) { fReadoutChipInterface->producePhaseAlignmentPattern(theReadoutChip); }
        }
    }

    for(size_t iterationNumber = 0; iterationNumber < fNumberOfAlignmentIterations; ++iterationNumber)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                auto& cCic = static_cast<OuterTrackerHybrid*>(theHybrid)->fCic;
                fCicInterface->ResetPhaseAligner(cCic);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                auto& thePhaseHistogram = thePhaseHistogramBoardDataContainer->getObject(theOpticalGroup->getId())
                                              ->getObject(theHybrid->getId())
                                              ->getSummary<GenericDataArray<float, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS, 16>>();
                auto& theLockingEfficiency = theLockingEfficiencyBoardDataContainer->getObject(theOpticalGroup->getId())
                                                 ->getObject(theHybrid->getId())
                                                 ->getSummary<GenericDataArray<float, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS>>();
                auto lockingSuccess = fCicInterface->getLineLocked(cCic);
                auto optimalPhases  = fCicInterface->getAllOptimalTaps(cCic);
                for(uint8_t frontEnd = 0; frontEnd < NUMBER_OF_CIC_PORTS; ++frontEnd)
                {
                    for(uint8_t line = 0; line < NUMBER_OF_LINES_PER_CIC_PORTS; ++line)
                    {
                        if(lockingSuccess.at(frontEnd).at(line)) theLockingEfficiency.at(frontEnd).at(line)++;
                        thePhaseHistogram.at(frontEnd).at(line).at(optimalPhases.at(frontEnd).at(line))++;
                    }
                }
            }
        }
    }
}

void OTCICphaseAlignment::AlignAllCICinputs2S(BeBoard*            theBoard,
                                              BoardDataContainer* thePhaseHistogramBoardDataContainer,
                                              BoardDataContainer* theLockingEfficiencyBoardDataContainer,
                                              BoardDataContainer* theBestPhaseBoardDataContainer)
{
    // Align L1 line
    LOG(INFO) << BOLDYELLOW << "Aligning CIC phases on CBC L1 lines" << RESET;

    uint32_t pNTriggers = 500;

    auto cInterface = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(theBoard));

    std::vector<std::pair<std::string, uint32_t>> cRegVec;
    cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.trigger_source", 6});
    cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.triggers_to_accept", pNTriggers});
    cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.test_pulse.en_fast_reset", 1});
    cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.test_pulse.en_test_pulse", 0});
    cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.test_pulse.en_l1a", 1});
    cRegVec.push_back({"fc7_daq_ctrl.fast_command_block.control.load_config", 0x1});
    fBeBoardInterface->WriteBoardMultReg(theBoard, cRegVec);

    for(auto theOpticalGroup: *theBoard)
    {
        for(auto theHybrid: *theOpticalGroup)
        {
            for(auto theReadoutChip: *theHybrid) { static_cast<CbcInterface*>(fReadoutChipInterface)->produceL1phaseAlignmentPattern(theReadoutChip); }
        }
    }

    for(size_t iterationNumber = 0; iterationNumber < fNumberOfAlignmentIterations; ++iterationNumber)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                auto& cCic = static_cast<OuterTrackerHybrid*>(theHybrid)->fCic;
                fCicInterface->ResetPhaseAligner(cCic);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        cInterface->getL1ReadoutInterface()->ResetReadout();
        cInterface->getTriggerInterface()->SendNTriggers(pNTriggers);

        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                auto& cCic              = static_cast<OuterTrackerHybrid*>(theHybrid)->fCic;
                auto& thePhaseHistogram = thePhaseHistogramBoardDataContainer->getObject(theOpticalGroup->getId())
                                              ->getObject(theHybrid->getId())
                                              ->getSummary<GenericDataArray<float, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS, 16>>();
                auto& theLockingEfficiency = theLockingEfficiencyBoardDataContainer->getObject(theOpticalGroup->getId())
                                                 ->getObject(theHybrid->getId())
                                                 ->getSummary<GenericDataArray<float, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS>>();
                auto lockingSuccess = fCicInterface->getLineLocked(cCic);
                auto optimalPhases  = fCicInterface->getAllOptimalTaps(cCic);
                for(uint8_t frontEnd = 0; frontEnd < NUMBER_OF_CIC_PORTS; ++frontEnd)
                {
                    if(lockingSuccess.at(frontEnd).at(0)) theLockingEfficiency.at(frontEnd).at(0)++;
                    thePhaseHistogram.at(frontEnd).at(0).at(optimalPhases.at(frontEnd).at(0))++;
                }
            }
        }
    }

    // Align Stub line 0
    LOG(INFO) << BOLDYELLOW << "Aligning CIC phases on CBC Stub 0 lines" << RESET;
    for(auto theOpticalGroup: *theBoard)
    {
        for(auto theHybrid: *theOpticalGroup)
        {
            for(auto theReadoutChip: *theHybrid) { static_cast<CbcInterface*>(fReadoutChipInterface)->produceStubLine0PhaseAlignmentPattern(theReadoutChip); }
        }
    }

    for(size_t iterationNumber = 0; iterationNumber < fNumberOfAlignmentIterations; ++iterationNumber)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                auto& cCic = static_cast<OuterTrackerHybrid*>(theHybrid)->fCic;
                fCicInterface->ResetPhaseAligner(cCic);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                auto& thePhaseHistogram = thePhaseHistogramBoardDataContainer->getObject(theOpticalGroup->getId())
                                              ->getObject(theHybrid->getId())
                                              ->getSummary<GenericDataArray<float, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS, 16>>();
                auto& theLockingEfficiency = theLockingEfficiencyBoardDataContainer->getObject(theOpticalGroup->getId())
                                                 ->getObject(theHybrid->getId())
                                                 ->getSummary<GenericDataArray<float, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS>>();
                auto lockingSuccess = fCicInterface->getLineLocked(cCic);
                auto optimalPhases  = fCicInterface->getAllOptimalTaps(cCic);
                for(uint8_t frontEnd = 0; frontEnd < NUMBER_OF_CIC_PORTS; ++frontEnd)
                {
                    if(lockingSuccess.at(frontEnd).at(1)) theLockingEfficiency.at(frontEnd).at(1)++;
                    thePhaseHistogram.at(frontEnd).at(1).at(optimalPhases.at(frontEnd).at(1))++;
                }
            }
        }
    }

    // Align Stub line 1-2-3-4
    LOG(INFO) << BOLDYELLOW << "Aligning CIC phases on CBC Stub 1, 2, 3 and 4 lines" << RESET;
    for(auto theOpticalGroup: *theBoard)
    {
        for(auto theHybrid: *theOpticalGroup)
        {
            for(auto theReadoutChip: *theHybrid) { static_cast<CbcInterface*>(fReadoutChipInterface)->produceStubLines1To4PhaseAlignmentPattern(theReadoutChip); }
        }
    }

    for(size_t iterationNumber = 0; iterationNumber < fNumberOfAlignmentIterations; ++iterationNumber)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                auto& cCic = static_cast<OuterTrackerHybrid*>(theHybrid)->fCic;
                fCicInterface->ResetPhaseAligner(cCic);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                auto& thePhaseHistogram = thePhaseHistogramBoardDataContainer->getObject(theOpticalGroup->getId())
                                              ->getObject(theHybrid->getId())
                                              ->getSummary<GenericDataArray<float, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS, 16>>();
                auto& theLockingEfficiency = theLockingEfficiencyBoardDataContainer->getObject(theOpticalGroup->getId())
                                                 ->getObject(theHybrid->getId())
                                                 ->getSummary<GenericDataArray<float, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS>>();
                auto lockingSuccess = fCicInterface->getLineLocked(cCic);
                auto optimalPhases  = fCicInterface->getAllOptimalTaps(cCic);
                for(uint8_t frontEnd = 0; frontEnd < NUMBER_OF_CIC_PORTS; ++frontEnd)
                {
                    for(uint8_t line = 2; line < NUMBER_OF_LINES_PER_CIC_PORTS; ++line)
                    {
                        if(lockingSuccess.at(frontEnd).at(line)) theLockingEfficiency.at(frontEnd).at(line)++;
                        thePhaseHistogram.at(frontEnd).at(line).at(optimalPhases.at(frontEnd).at(line))++;
                    }
                }
            }
        }
    }
}