#include "tools/OTMPAtoCICecv.h"
#include "HWInterface/D19cFWInterface.h"
#include "System/RegisterHelper.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/GenericDataArray.h"
#include <algorithm>
#include <bitset>

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

std::string OTMPAtoCICecv::fCalibrationDescription = "Run electric chain validation test between MPA and CIC";

OTMPAtoCICecv::OTMPAtoCICecv() : Tool() {}

OTMPAtoCICecv::~OTMPAtoCICecv() {}

void OTMPAtoCICecv::Initialise(void)
{
    fRegisterHelper->takeSnapshot();
    // free the registers in case any

    fNumberOfIterations    = findValueInSettings<double>("OTMPAtoCICecv_NumberOfIterations", 1000);
    fShiftRegisterPattern  = findValueInSettings<double>("OTMPAtoCICecv_ShiftRegisterPattern", 0xAA);
    fListOfMPAslvsCurrents = convertStringToFloatList(findValueInSettings<std::string>("OTMPAtoCICecv_ListOfMPAslvsCurrents", "1, 4, 7"));

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: plots are booked during initialization
    fDQMHistogramOTMPAtoCICecv.book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif
}

void OTMPAtoCICecv::ConfigureCalibration() {}

void OTMPAtoCICecv::Running()
{
    LOG(INFO) << "Starting OTMPAtoCICecv measurement.";
    Initialise();
    setMPAshiftRegister();
    runElectricChainValidation();
    LOG(INFO) << "Done with OTMPAtoCICecv.";
    Reset();
}

void OTMPAtoCICecv::Stop(void)
{
    LOG(INFO) << "Stopping OTMPAtoCICecv measurement.";
#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: processing the histograms
    fDQMHistogramOTMPAtoCICecv.process();
#endif
    SaveResults();
    closeFileHandler();
    LOG(INFO) << "OTMPAtoCICecv stopped.";
}

void OTMPAtoCICecv::Pause() {}

void OTMPAtoCICecv::Resume() {}

void OTMPAtoCICecv::Reset() { fRegisterHelper->restoreSnapshot(); }

void OTMPAtoCICecv::setMPAshiftRegister()
{
    auto thePSinterface = static_cast<PSInterface*>(fReadoutChipInterface)->fTheMPA2Interface;

    auto        MPAqueryFunction          = [](const ChipContainer* theChip) { return (static_cast<const ReadoutChip*>(theChip)->getFrontEndType() == FrontEndType::MPA2); };
    std::string theMPAqueryFunctionString = "MPAqueryFunction";
    fDetectorContainer->addReadoutChipQueryFunction(MPAqueryFunction, theMPAqueryFunctionString);

    setSameDac("LFSR_data", fShiftRegisterPattern);

    for(auto theBoard: *fDetectorContainer)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                for(auto theMPA: *theHybrid)
                {
                    thePSinterface->WriteChipRegBits(theMPA, "Control_1", 0x2, "Mask", 0x03); // Enable shift register
                }
            }
        }
    }
    fDetectorContainer->removeReadoutChipQueryFunction(theMPAqueryFunctionString);
}

void OTMPAtoCICecv::runElectricChainValidation()
{
    auto        MPAqueryFunction          = [](const ChipContainer* theChip) { return (static_cast<const ReadoutChip*>(theChip)->getFrontEndType() == FrontEndType::MPA2); };
    std::string theMPAqueryFunctionString = "MPAqueryFunction";
    fDetectorContainer->addReadoutChipQueryFunction(MPAqueryFunction, theMPAqueryFunctionString);

    LOG(INFO) << BOLDYELLOW << "OTMPAtoCICecv::runElectricChainValidation ... start electric chain validation test" << RESET;

    auto thePSinterface = static_cast<PSInterface*>(fReadoutChipInterface)->fTheMPA2Interface;

    for(uint8_t slvsCurrent: fListOfMPAslvsCurrents)
    {
        LOG(INFO) << BOLDGREEN << "    Measuring slvs current " << +slvsCurrent << RESET;
        for(auto theBoard: *fDetectorContainer)
        {
            for(auto theOpticalGroup: *theBoard)
            {
                for(auto theHybrid: *theOpticalGroup)
                {
                    for(auto theMPA: *theHybrid)
                    {
                        thePSinterface->WriteChipRegBits(theMPA, "ConfSLVS", slvsCurrent, "Mask", 0x07); // set slvs current
                    }
                }
            }
        }

        for(uint8_t phase = 0; phase < 15; ++phase)
        {
            if(phase == 2 || phase == 3) continue;
            LOG(INFO) << BOLDGREEN << "        Measuring phase " << +phase << RESET;
            DetectorDataContainer theMatchingEfficiencyContainer;
            ContainerFactory::copyAndInitChip<GenericDataArray<float, 6>>(*fDetectorContainer, theMatchingEfficiencyContainer);

            for(auto theBoard: *fDetectorContainer)
            {
                auto theFWinterface = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(theBoard));
                for(auto theOpticalGroup: *theBoard)
                {
                    auto possiblePatternList = getPossiblePatterns(fShiftRegisterPattern, static_cast<D19clpGBTInterface*>(flpGBTInterface)->GetChipRate(theOpticalGroup->flpGBT) == 10);
                    for(auto theHybrid: *theOpticalGroup)
                    {
                        auto theCic = static_cast<OuterTrackerHybrid*>(theHybrid)->fCic;

                        std::vector<std::pair<std::string, uint16_t>> phaseRegisterVector;
                        for(uint8_t phyPortPair = 0; phyPortPair < 6; ++phyPortPair)
                        {
                            for(uint8_t channel = 0; channel < 4; ++channel)
                            {
                                std::stringstream phaseRegisterName;
                                phaseRegisterName << "scPhaseSelectB" << +channel << "i" << +(phyPortPair);
                                phaseRegisterVector.push_back({phaseRegisterName.str(), phase | phase << 4});
                            }
                        }
                        fCicInterface->WriteChipMultReg(theCic, phaseRegisterVector);

                        for(uint8_t phyPort = 0; phyPort < 12; ++phyPort)
                        {
                            for(uint8_t line = 0; line < 4; ++line)
                            {
                                auto bestPhase       = theCic->getLpGBTphaseForCICbypass(phyPort, line);
                                auto groupAndChannel = theOpticalGroup->getGroupAndChannel(theHybrid->getId(), line + 1); // stub lines start from 1, line 0 is L1
                                flpGBTInterface->ConfigureRxPhase(theOpticalGroup->flpGBT, groupAndChannel.first, groupAndChannel.second, bestPhase);
                            }
                            auto phyPortDataVector = readCICbypassOutput(theHybrid, theFWinterface, phyPort);
                            for(size_t line = 0; line < 4; ++line)
                            {
                                float matchingEfficiency = countMatchingBits(phyPortDataVector.at(line), possiblePatternList);
                                auto  chipIdAndLine      = fCicInterface->fromPhyPortAndChanneltoChipIdAndLine(theCic, phyPort, line);
                                // if(matchingEfficiency < 1 && matchingEfficiency>0.95)
                                // {
                                //     size_t patternSize = 10;
                                //     size_t numberOfPatterns = phyPortDataVector.at(line).size() / patternSize;
                                //     for(size_t patternCounter = 0; patternCounter<numberOfPatterns; ++patternCounter)
                                //     {
                                //         auto first = phyPortDataVector.at(line).begin() + patternCounter * patternSize;
                                //         auto last  = first + patternSize;
                                //         std::vector<uint32_t> patternVector(first, last);
                                //         bool matching = true;
                                //         for(auto word : patternVector)
                                //         {
                                //             if(word != 0x33333333 && word != 0x66666666 && word != 0x99999999 && word != 0xCCCCCCCC)
                                //             {
                                //                 matching = false;
                                //                 break;
                                //             }
                                //         }
                                //         if(!matching) std::cout<< __PRETTY_FUNCTION__ << " [" << __LINE__ << "] Iteration = " << patternCounter << " data = " << getPatternPrintout(patternVector, 2)
                                //         << std::endl;
                                //     }
                                // }

                                try // Handle disable chip
                                {
                                    theMatchingEfficiencyContainer.getChip(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId(), chipIdAndLine.first + 8)
                                        ->getSummary<GenericDataArray<float, 6>>()
                                        .at(chipIdAndLine.second) = matchingEfficiency;
                                }
                                catch(const std::exception& e)
                                {
                                    continue;
                                }
                            }
                        }
                    }
                }
            }

#ifdef __USE_ROOT__
            fDQMHistogramOTMPAtoCICecv.fillPhaseScanMatchingEfficiency(theMatchingEfficiencyContainer, phase, slvsCurrent);
#else
            if(fDQMStreamerEnabled)
            {
                ContainerSerialization thePhaseScanMatchingEfficiencySerialization("OTMPAtoCICecvPhaseScanMatchingEfficiency");
                thePhaseScanMatchingEfficiencySerialization.streamByHybridContainer(fDQMStreamer, theMatchingEfficiencyContainer, phase, slvsCurrent);
            }
#endif
        }
    }

    fDetectorContainer->removeReadoutChipQueryFunction(theMPAqueryFunctionString);
}

std::vector<std::vector<uint32_t>> OTMPAtoCICecv::readCICbypassOutput(Hybrid* theHybrid, D19cFWInterface* theFWinterface, uint8_t phyPort)
{
    size_t                             cNlines = 4;
    std::vector<std::vector<uint32_t>> phyPortDataVector(cNlines);

    auto theCic = static_cast<OuterTrackerHybrid*>(theHybrid)->fCic;
    fCicInterface->SelectOutput(theCic, false);
    uint8_t registerValue = 0x10 + phyPort;
    fCicInterface->WriteChipReg(theCic, "MUX_CTRL", registerValue);
    fBeBoardInterface->WriteBoardReg(fDetectorContainer->getObject(theHybrid->getBeBoardId()), "fc7_daq_cnfg.physical_interface_block.slvs_debug.hybrid_select", theHybrid->getId());
    fBeBoardInterface->WriteBoardReg(fDetectorContainer->getObject(theHybrid->getBeBoardId()), "fc7_daq_cnfg.physical_interface_block.slvs_debug.chip_select", 0);

    for(size_t iteration = 0; iteration < fNumberOfIterations; iteration++)
    {
        auto lineOutputVector = theFWinterface->StubDebug(true, cNlines, false);
        for(size_t line = 0; line < cNlines; ++line) { phyPortDataVector.at(line).insert(phyPortDataVector.at(line).end(), lineOutputVector.at(line).begin(), lineOutputVector.at(line).end()); }
    }

    return phyPortDataVector;
}
