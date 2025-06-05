#include "tools/OTChipToCICecv.h"
#include "System/RegisterHelper.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/GenericDataArray.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

std::string OTChipToCICecv::fCalibrationDescription = "Run electric chain validation test between CBC/MPA and CIC";

OTChipToCICecv::OTChipToCICecv() : OTalignLpGBTinputsForBypass() {}

OTChipToCICecv::~OTChipToCICecv() {}

void OTChipToCICecv::Initialise(void)
{
    fRegisterHelper->takeSnapshot();
    // free the registers in case any

    fNumberOfTestedBits      = findValueInSettings<double>("OTChipToCICecv_NumberOfTestedBits", 1e6);
    fNumberOfTestedBitsL12S  = findValueInSettings<double>("OTChipToCICecv_NumberOfTestedBitsL12S", 1e5);
    fListOfCBCslvsCurrents   = convertStringToFloatList(findValueInSettings<std::string>("OTChipToCICecv_ListOfCBCslvsCurrents", "0, 8, 14"));
    fShiftRegisterPatternMPA = findValueInSettings<double>("OTChipToCICecv_MPAshiftRegisterPattern", 0xAA);
    fListOfMPAslvsCurrents   = convertStringToFloatList(findValueInSettings<std::string>("OTChipToCICecv_ListOfMPAslvsCurrents", "1, 4, 7"));

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: plots are booked during initialization
    fDQMHistogramOTChipToCICecv.book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif

    preparePatternChecker();
}

void OTChipToCICecv::ConfigureCalibration() {}

void OTChipToCICecv::Running()
{
    LOG(INFO) << "Starting OTChipToCICecv measurement.";
    Initialise();
    runOTChiptoCICecv();
    LOG(INFO) << "Done with OTChipToCICecv.";
    Reset();
}

void OTChipToCICecv::Stop(void)
{
    LOG(INFO) << "Stopping OTChipToCICecv measurement.";
#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: processing the histograms
    fDQMHistogramOTChipToCICecv.process();
#endif
    SaveResults();
    closeFileHandler();
    LOG(INFO) << "OTChipToCICecv stopped.";
}

void OTChipToCICecv::Pause() {}

void OTChipToCICecv::Resume() {}

void OTChipToCICecv::Reset() { fRegisterHelper->restoreSnapshot(); }

void OTChipToCICecv::runOTChiptoCICecv()
{
    bool                isPS              = fDetectorContainer->getFirstObject()->getFirstObject()->getFrontEndType() == FrontEndType::OuterTrackerPS;
    uint8_t             numberOfCICphases = 15;
    std::vector<float>* theListOfSlvsCurrents;
    if(isPS)
        theListOfSlvsCurrents = &fListOfMPAslvsCurrents;
    else
        theListOfSlvsCurrents = &fListOfCBCslvsCurrents;

    std::map<uint8_t, std::map<uint8_t, std::map<uint8_t, DetectorDataContainer>>> errorRateContainerMap;
    for(uint8_t phyPort = 0; phyPort < 12; ++phyPort)
    {
        for(uint8_t cicPhase = 0; cicPhase < numberOfCICphases; ++cicPhase)
        {
            if(cicPhase == 2 || cicPhase == 3) continue;
            for(uint8_t slvsCurrent: *theListOfSlvsCurrents)
            {
                ContainerFactory::copyAndInitHybrid<GenericDataArray<float, 4, 2>>(*fDetectorContainer, errorRateContainerMap[cicPhase][slvsCurrent][phyPort]);
            }
        }
    }

    for(auto* theBoard: *fDetectorContainer)
    {
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

            for(uint8_t cicPhase = 0; cicPhase < 15; ++cicPhase)
            {
                if(cicPhase == 2 || cicPhase == 3) continue;
                LOG(INFO) << BOLDGREEN << "Measuring CIC Phase " << +cicPhase << RESET;

                setCICPhase(theBoard, cicPhase, phyPort);

                for(uint8_t slvsCurrent: *theListOfSlvsCurrents)
                {
                    setSlvsChipCurrent(theBoard, slvsCurrent);
                    runPatternMatching(theBoard, errorRateContainerMap[cicPhase][slvsCurrent], isPS, phyPort);
                }
            }
        }
    }

    for(uint8_t cicPhase = 0; cicPhase < numberOfCICphases; ++cicPhase)
    {
        if(cicPhase == 2 || cicPhase == 3) continue;
        for(uint8_t slvsCurrent: *theListOfSlvsCurrents)
        {
            DetectorDataContainer matchingEfficiencyContainer;
            ContainerFactory::copyAndInitChip<GenericDataArray<float, 6, 2>>(*fDetectorContainer, matchingEfficiencyContainer);

            for(auto theBoard: *fDetectorContainer)
            {
                for(auto theOpticalGroup: *theBoard)
                {
                    for(auto theHybrid: *theOpticalGroup)
                    {
                        auto& theCic = static_cast<OuterTrackerHybrid*>(theHybrid)->fCic;
                        for(uint8_t phyPort = 0; phyPort < 12; ++phyPort)
                        {
                            const auto& theLinePatternMatching = errorRateContainerMap[cicPhase][slvsCurrent][phyPort]
                                                                     .getHybrid(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId())
                                                                     ->getSummary<GenericDataArray<float, 4, 2>>();
                            for(uint8_t line = 0; line < NUMBER_OF_LINES_PER_CIC_PHY_PORTS; ++line)
                            {
                                auto chipIdAndLine = fCicInterface->fromPhyPortAndChanneltoChipIdAndLine(theCic, phyPort, line);

                                try
                                {
                                    matchingEfficiencyContainer.getChip(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId(), chipIdAndLine.first + (isPS ? 8 : 0))
                                        ->getSummary<GenericDataArray<float, 6, 2>>()
                                        .at(chipIdAndLine.second) = theLinePatternMatching.at(line);
                                }
                                catch(const std::exception& e)
                                {
                                    // do nothing, the chip was not enabled
                                }
                            }
                        }
                    }
                }

#ifdef __USE_ROOT__
                fDQMHistogramOTChipToCICecv.fillPhaseScanMatchingEfficiency(matchingEfficiencyContainer, cicPhase, slvsCurrent);
#else
                if(fDQMStreamerEnabled)
                {
                    ContainerSerialization thePhaseScanMatchingEfficiencySerialization("OTChipToCICecvPhaseScanMatchingEfficiency");
                    thePhaseScanMatchingEfficiencySerialization.streamByHybridContainer(fDQMStreamer, matchingEfficiencyContainer, cicPhase, slvsCurrent);
                }
#endif
            }
        }
    }
}

void OTChipToCICecv::setCICPhase(BeBoard* theBoard, uint8_t phase, uint8_t phyPort)
{
    for(auto theOpticalGroup: *theBoard)
    {
        for(auto theHybrid: *theOpticalGroup)
        {
            auto& theCic = static_cast<OuterTrackerHybrid*>(theHybrid)->fCic;
            // choosing best LpGBT phase for bypassing CIC
            for(uint8_t line = 0; line < 4; ++line)
            {
                auto bestPhase       = theCic->getLpGBTphaseForCICbypass(phyPort, line);
                auto groupAndChannel = theOpticalGroup->getGroupAndChannel(theHybrid->getId(), line + 1); // stub lines start from 1, line 0 is L1
                flpGBTInterface->ConfigureRxPhase(theOpticalGroup->flpGBT, groupAndChannel.first, groupAndChannel.second, bestPhase);
            }

            std::vector<std::pair<std::string, uint16_t>> cicPhaseRegisterVector;
            for(uint8_t channel = 0; channel < 4; ++channel)
            {
                std::stringstream cicPhaseRegisterName;
                cicPhaseRegisterName << "scPhaseSelectB" << +channel << "i" << +(phyPort / 2);
                cicPhaseRegisterVector.push_back({cicPhaseRegisterName.str(), phase | phase << 4});
            }
            fCicInterface->WriteChipMultReg(theCic, cicPhaseRegisterVector);
        }
    }
}

void OTChipToCICecv::setSlvsChipCurrent(BeBoard* theBoard, uint8_t slvsCurrent)
{
    for(auto theOpticalGroup: *theBoard)
    {
        for(auto theHybrid: *theOpticalGroup)
        {
            for(auto theChip: *theHybrid)
            {
                if(theChip->getFrontEndType() == FrontEndType::CBC3)
                {
                    uint8_t defaultBetaMult     = fReadoutChipInterface->ReadChipReg(theChip, "BetaMult&SLVS") & 0xF0;
                    uint8_t BetaMultAndSLVSbyte = (defaultBetaMult | (uint8_t(slvsCurrent) & 0xF));
                    // setting CBC strength
                    fReadoutChipInterface->WriteChipReg(theChip, "BetaMult&SLVS", BetaMultAndSLVSbyte);
                }
                else if(theChip->getFrontEndType() == FrontEndType::MPA2)
                {
                    auto thePSinterface = static_cast<PSInterface*>(fReadoutChipInterface)->fTheMPA2Interface;
                    thePSinterface->WriteChipRegBits(theChip, "ConfSLVS", slvsCurrent, "Mask", 0x07);
                }
            }
        }
    }
}
