#include "CicFEAlignment.h"

// #ifdef __USE_ROOT__
#include "HWInterface/D19cDebugFWInterface.h"
#include "HWInterface/D19cFWInterface.h"
#include "HWInterface/TriggerInterface.h"
#include "Utils/CBCChannelGroupHandler.h"
#include "Utils/ContainerFactory.h"
#include "Utils/LpGBTalignmentResult.h"
#include "Utils/Occupancy.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

CicFEAlignment::CicFEAlignment() : OTTool() {}

CicFEAlignment::~CicFEAlignment() {}

void CicFEAlignment::Initialise()
{
    LOG(INFO) << BOLDMAGENTA << "CicFEAlignment::Initialise" << RESET;
    fSuccess = false;
    fWithMPA = false;
    // this is needed if you're going to use groups anywhere
    CBCChannelGroupHandler theChannelGroupHandler;
    theChannelGroupHandler.setChannelGroupParameters(16, 1, 2);
    setChannelGroupHandler(theChannelGroupHandler);

    DetectorDataContainer theOccupancyContainer;
    fDetectorDataContainer = &theOccupancyContainer;
    ContainerFactory::copyAndInitStructure<Occupancy>(*fDetectorContainer, *fDetectorDataContainer);
    LOG(INFO) << BOLDMAGENTA << "CicFEAlignment::Initialise" << RESET;
    // prepare common OTTool
    Prepare();
    SetName("CicFEAlignment");

    // initialize containers holding data from this tool
    ContainerFactory::copyAndInitChip<AlignmentValues>(*fDetectorContainer, fPhaseAlignmentValues);
    ContainerFactory::copyAndInitChip<AlignmentValues>(*fDetectorContainer, fWordAlignmentValues);
    for(auto cBoard: *fDetectorContainer)
    {
        auto& cPhaseAlignmentThisBoard = fPhaseAlignmentValues.getObject(cBoard->getId());
        auto& cWordAlignmentThisBoard  = fWordAlignmentValues.getObject(cBoard->getId());

        for(auto cOpticalGroup: *cBoard)
        {
            auto& cPhaseAlignmentThisOpticalGroup = cPhaseAlignmentThisBoard->getObject(cOpticalGroup->getId());
            auto& cWordAlignmentThisOpticalGroup  = cWordAlignmentThisBoard->getObject(cOpticalGroup->getId());

            for(auto cHybrid: *cOpticalGroup)
            {
                auto& cPhaseAlignmentThisHybrid = cPhaseAlignmentThisOpticalGroup->getObject(cHybrid->getId());
                auto& cWordAlignmentThisHybrid  = cWordAlignmentThisOpticalGroup->getObject(cHybrid->getId());
                for(auto cChip: *cHybrid)
                {
                    auto& cPhaseAlignmentThisChip = cPhaseAlignmentThisHybrid->getObject(cChip->getId());
                    auto& cWordAlignmentThisChip  = cWordAlignmentThisHybrid->getObject(cChip->getId());

                    auto& cPhaseAlVals = cPhaseAlignmentThisChip->getSummary<AlignmentValues>();
                    cPhaseAlVals.clear();
                    cPhaseAlVals.resize(6, 0);
                    auto& cWordAlignmentVals = cWordAlignmentThisChip->getSummary<AlignmentValues>();
                    cWordAlignmentVals.clear();
                    cWordAlignmentVals.resize(5, 0);
                }
            }
        }
    }

#ifdef __USE_ROOT__
    fDQMHistogrammer.book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif
}

void CicFEAlignment::writeObjects()
{
#ifdef __USE_ROOT__
    this->SaveResults();
    fDQMHistogrammer.process();
    fResultFile->Flush();
#endif
}
// State machine control functions
bool CicFEAlignment::AlignInputs()
{
    LOG(INFO) << BOLDMAGENTA << __PRETTY_FUNCTION__ << RESET;
    // align CIC inputs - first phase
    bool cPhaseAligned = this->PhaseAlignment();
    if(!cPhaseAligned)
    {
        LOG(INFO) << BOLDRED << "FAILED " << BOLDBLUE << " phase alignment step on CIC input .. " << RESET;
        exit(FAILED_PHASE_ALIGNMENT);
    }
    LOG(INFO) << BOLDGREEN << "SUCCESSFUL " << BOLDBLUE << " phase alignment on CIC inputs... " << RESET;
    fSuccess = cPhaseAligned;

    // then word
    bool cWordAligned = this->WordAlignment();
    if(!cWordAligned)
    {
        LOG(INFO) << BOLDRED << "FAILED " << BOLDBLUE << "word alignment step on CIC input .. " << RESET;
        exit(FAILED_WORD_ALIGNMENT);
    }
    LOG(INFO) << BOLDGREEN << "SUCCESSFUL " << BOLDBLUE << " word alignment on CIC inputs... " << RESET;

    // bool cBxAligned = (fWithMPA) ? this->SetBx0Delay(fStubBxDelayPS) : this->SetBx0Delay(fStubBxDelay2S);
    // if(!cBxAligned)
    // {
    //     LOG(INFO) << BOLDRED << "FAILED " << BOLDBLUE << " to set Bx0 delay in CIC ... " << RESET;
    //     exit(FAILED_BX_ALIGNMENT);
    // }
    // LOG(INFO) << BOLDGREEN << "SUCCESSFUL " << BOLDBLUE << " setting of Bx0 delay in CIC ... " << RESET;
    fSuccess = (cPhaseAligned && cWordAligned);
    return fSuccess;
}
void CicFEAlignment::Running()
{
    Initialise();
    AlignInputs();
    Reset();
}
void CicFEAlignment::SetStubWindowOffsets(uint8_t pBendCode, int pBend)
{
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    // read bend LUT
                    ReadoutChip*         theChip   = static_cast<ReadoutChip*>(cChip);
                    std::vector<uint8_t> cBendLUT  = static_cast<CbcInterface*>(fReadoutChipInterface)->readLUT(theChip);
                    auto                 cIterator = std::find(cBendLUT.begin(), cBendLUT.end(), pBendCode);
                    if(cIterator != cBendLUT.end())
                    {
                        int     cPosition    = std::distance(cBendLUT.begin(), cIterator);
                        double  cBend_strips = -7. + 0.5 * cPosition;
                        uint8_t cOffsetCode  = static_cast<uint8_t>(std::abs(cBend_strips * 2)) | (std::signbit(-1 * cBend_strips) << 3);
                        fReadoutChipInterface->WriteChipReg(theChip, "CoincWind&Offset12", (cOffsetCode << 4) | (cOffsetCode << 0));
                        fReadoutChipInterface->WriteChipReg(theChip, "CoincWind&Offset34", (cOffsetCode << 4) | (cOffsetCode << 0));
                        LOG(DEBUG) << BOLDBLUE << "Bend code of " << std::bitset<4>(pBendCode) << " found for bend reg " << +cPosition << " which means " << cBend_strips << " strips [offset code "
                                   << std::bitset<4>(cOffsetCode) << "]." << RESET;
                    }
                    else
                    {
                        LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << " [" << __LINE__ << "] Bend code not available in the lookup table, aborting" << RESET;
                        abort();
                    }
                }
            }
        }
    }
}
bool CicFEAlignment::SetBx0Delay(uint8_t pDelay, uint8_t pStubPackageDelay)
{
    // configure Bx0 alignment patterns in CIC
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                OuterTrackerHybrid* theHybrid = static_cast<OuterTrackerHybrid*>(cHybrid);
                if(theHybrid->fCic != NULL)
                {
                    bool cConfigured = fCicInterface->ManualBx0Alignment(theHybrid->fCic, pDelay);
                    if(!cConfigured)
                    {
                        LOG(INFO) << BOLDRED << "Failed to manually set Bx0 delay in CIC..." << RESET;
                        exit(0);
                    }
                }
            }
        }
    }
    return true;
}

void CicFEAlignment::InputLineScan()
{
    // only for stubs ... for L1 line difficult to do this for 2S
    for(uint8_t cLineId = 0; cLineId < 5; cLineId++)
    {
        auto cPattern = GenManPatternOutLine(cLineId);
        ScanInputPhase(cLineId, cPattern, 0, 15);
    }
}
// manually inject pattern on one of the CIC input lines from a CBC
uint8_t CicFEAlignment::GenManPatternOutLine(uint8_t pOutLine)
{
    uint8_t              cPattern          = 0x8A;
    uint8_t              cBendCode_phAlign = cPattern & 0x0F;
    uint8_t              cBendPattern2S    = (cBendCode_phAlign << 4) | cBendCode_phAlign;
    uint8_t              cSyncPattern2S    = cPattern;
    std::vector<uint8_t> cStubs;
    if(pOutLine == 0)
    {
        cStubs.push_back(cPattern);
        cStubs.push_back(cPattern + 20), cStubs.push_back(cPattern + 40);
    }
    else if(pOutLine == 1)
    {
        cStubs.push_back(cPattern - 20);
        cStubs.push_back(cPattern), cStubs.push_back(cPattern + 20);
    }
    else if(pOutLine == 2)
    {
        cStubs.push_back(cPattern - 40);
        cStubs.push_back(cPattern - 20), cStubs.push_back(cPattern);
    }
    else
    {
        cStubs.push_back(0xA0);
        cStubs.push_back(0xAA), cStubs.push_back(0xCA);
    }

    std::vector<uint8_t> cExpectedPatterns{cStubs[0], cStubs[1], cStubs[2], cBendPattern2S, cSyncPattern2S};
    // make sure FE chips are sending expected pattern
    for(auto cBoard: *fDetectorContainer)
    {
        // generate alignment pattern on all stub lines
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                // configure Chips to produce phase alignment patterns
                for(auto cChip: *cHybrid)
                {
                    if(cChip->getFrontEndType() == FrontEndType::CBC3)
                    {
                        auto                                 cInterface = static_cast<CbcInterface*>(fReadoutChipInterface);
                        std::vector<uint8_t>                 cBendLUT   = cInterface->readLUT(static_cast<ReadoutChip*>(cChip));
                        auto                                 cIterator  = std::find(cBendLUT.begin(), cBendLUT.end(), cBendCode_phAlign);
                        std::vector<std::pair<uint8_t, int>> theStubAddressAndBendVector;
                        if(cIterator != cBendLUT.end())
                        {
                            int    cPosition    = std::distance(cBendLUT.begin(), cIterator);
                            double cBend_strips = -7. + 0.5 * cPosition;
                            for(const auto theStub: cStubs) theStubAddressAndBendVector.push_back({theStub, static_cast<int>(cBend_strips * 2)});
                            cInterface->injectStubs(static_cast<ReadoutChip*>(cChip), theStubAddressAndBendVector);
                        }
                        else
                        {
                            LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << " [" << __LINE__ << "] Bend code not available in the lookup table, aborting" << RESET;
                            abort();
                        }
                    }
                } // chip
            } // hybrid
        } // OG
    } // board

    return cExpectedPatterns[pOutLine];
}
void CicFEAlignment::ScanInputPhase(uint8_t pOutLine, uint8_t pPattern, uint8_t pStartScan, uint8_t pEndScan)
{
    LOG(INFO) << BOLDBLUE << "Scanning input phase on CIC input line#" << +pOutLine << " - expected pattern is " << std::bitset<8>(pPattern) << RESET;
    for(uint8_t cPhase = pStartScan; cPhase < 1 + pEndScan; cPhase++) { CheckCicInput(pOutLine, pPattern, cPhase); }
}
DetectorDataContainer CicFEAlignment::CheckCicInput(uint8_t pOutLine, uint8_t pPattern, uint8_t pPhase)
{
    DetectorDataContainer cErrorRate;
    ContainerFactory::copyAndInitChip<float>(*fDetectorContainer, cErrorRate);

    DetectorDataContainer cStubData, cLineErrors;
    ContainerFactory::copyAndInitChip<std::string>(*fDetectorContainer, cStubData);
    ContainerFactory::copyAndInitChip<uint32_t>(*fDetectorContainer, cLineErrors);
    CheckOutLine(pOutLine, pPattern, pPhase, cStubData, cLineErrors);
    for(auto cBoard: *fDetectorContainer)
    {
        auto& cStubDataThisBrd = cStubData.getObject(cBoard->getId());
        auto& cErrorsThisBrd   = cLineErrors.getObject(cBoard->getId());
        auto& cErrRateThisBrd  = cErrorRate.getObject(cBoard->getId());
        for(auto cOpticalGroup: *cBoard)
        {
            auto& cStubDataThisOpticalGroup   = cStubDataThisBrd->getObject(cOpticalGroup->getId());
            auto& cLineErrorsThisOpticalGroup = cErrorsThisBrd->getObject(cOpticalGroup->getId());
            auto& cErrRateThisOpticalGroup    = cErrRateThisBrd->getObject(cOpticalGroup->getId());
            for(auto cHybrid: *cOpticalGroup)
            {
                auto& cStubDataThisHybrid   = cStubDataThisOpticalGroup->getObject(cHybrid->getId());
                auto& cLineErrorsThisHybrid = cLineErrorsThisOpticalGroup->getObject(cHybrid->getId());
                auto& cErrRateThisHybrid    = cErrRateThisOpticalGroup->getObject(cHybrid->getId());
                for(auto cChip: *cHybrid)
                {
                    auto& cStubDataThisChip   = cStubDataThisHybrid->getObject(cChip->getId());
                    auto& cLineErrorsThisChip = cLineErrorsThisHybrid->getObject(cChip->getId());
                    auto& cErrRateThisChip    = cErrRateThisHybrid->getObject(cChip->getId());
                    auto& cData               = cStubDataThisChip->getSummary<std::string>();
                    auto& cErrorCount         = cLineErrorsThisChip->getSummary<uint32_t>();
                    auto& cErrRate            = cErrRateThisChip->getSummary<float>();
                    cErrRate                  = (float)cErrorCount / cData.length();
                    LOG(DEBUG) << BOLDBLUE << "Expected pattern is " << std::bitset<8>(pPattern) << RESET;
                    LOG(DEBUG) << BOLDBLUE << "Error rate on this line is " << cErrRate << " errors/bit" << RESET;
                    LOG(DEBUG) << BOLDBLUE << "For a sampling phase of " << +pPhase << " " << cErrorCount << " bit errors in the scoped  data : " << cData << " out of " << cData.length() << " bits."
                               << RESET;
                } // chip
            } // hybrid
        } // OG
    } // board

#ifdef __USE_ROOT__
    fDQMHistogrammer.fillManualPhaseScan(pPhase, pOutLine, cLineErrors, cStubData);
#endif
    return cErrorRate;
}
SlvsLineStatus CicFEAlignment::CheckPhyPort(const Hybrid* pHybrid, PhyPortCnfg pPhyPortCnfg, uint8_t pPhase, uint8_t pPattern)
{
    SlvsLineStatus cStatus;
    std::bitset<8> cExpectedPattern(pPattern);
    std::string    cPatternToMatch = cExpectedPattern.to_string();
    auto           cBoardId        = pHybrid->getBeBoardId();
    auto           cInterface      = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(fDetectorContainer->getObject(cBoardId)));

    auto  cBoardIter = std::find_if(fDetectorContainer->begin(), fDetectorContainer->end(), [&cBoardId](Ph2_HwDescription::BeBoard* x) { return x->getId() == cBoardId; });
    auto& cCic       = static_cast<const OuterTrackerHybrid*>(pHybrid)->fCic;
    // select slvs debug line in FC7
    fBeBoardInterface->WriteBoardReg((*cBoardIter), "fc7_daq_cnfg.physical_interface_block.slvs_debug.hybrid_select", pHybrid->getId());
    fBeBoardInterface->WriteBoardReg((*cBoardIter), "fc7_daq_cnfg.physical_interface_block.slvs_debug.chip_select", 0);
    // select phyPort in CIC mux
    fCicInterface->SelectMux(cCic, pPhyPortCnfg.first);
    // set phase tap for this phy port input
    fCicInterface->SetPhaseTap(cCic, pPhyPortCnfg.first, pPhyPortCnfg.second, pPhase);
    // interface to retrieve debug data
    D19cDebugFWInterface* cDebugInterface = cInterface->getDebugInterface();

    // read back data from stub debug
    // for now .. I need to do this twice
    // figure out why in theFW
    cDebugInterface->StubDebug(true, 6, false);
    // auto cLines        = cDebugInterface->StubDebug(true, 6, false).first;
    // cStatus.second     = cLines[pPhyPortCnfg.second];
    // cStatus.first      = 0;
    // auto        cFound = cStatus.second.find(cPatternToMatch);
    // std::string cPatternReceived;
    // if(cFound != std::string::npos)
    // {
    //     cPatternReceived = cStatus.second.substr(cFound, cStatus.second.length() - cFound) + cStatus.second.substr(0, cFound);
    //     LOG(DEBUG) << BOLDYELLOW << "Shifted str : " << cPatternReceived << " - bit shift is " << cFound << RESET;
    // }
    // else
    //     cPatternReceived = cStatus.second;

    // for(uint8_t cSize = 0; cSize < cPatternReceived.length(); cSize += 8)
    // {
    //     auto cSubStr = cPatternReceived.substr(cSize, 8);
    //     for(uint8_t cIndx = 0; cIndx < cSubStr.size(); cIndx++)
    //     {
    //         if(cSubStr[cIndx] != cPatternToMatch[cIndx]) cStatus.first++;
    //     }
    // }
    // cStatus.second = cPatternReceived;
    return cStatus;
}
void CicFEAlignment::CheckOutLine(uint8_t pOutLine, uint8_t pPattern, uint8_t pPhase, DetectorDataContainer& pLineData, DetectorDataContainer& pErrorCounter)
{
    // retreive data and compare
    std::bitset<8> cExpectedPattern(pPattern);
    std::string    cPatternToMatch = cExpectedPattern.to_string();
    // const unsigned int cNStubLinesFromFE=5;
    for(auto cBoard: *fDetectorContainer)
    {
        auto& cStubDataThisBrd = pLineData.getObject(cBoard->getId());
        auto& cErrorsThisBrd   = pErrorCounter.getObject(cBoard->getId());
        for(auto cOpticalGroup: *cBoard)
        {
            auto& cStubDataThisOpticalGroup   = cStubDataThisBrd->getObject(cOpticalGroup->getId());
            auto& cLineErrorsThisOpticalGroup = cErrorsThisBrd->getObject(cOpticalGroup->getId());
            for(auto cHybrid: *cOpticalGroup)
            {
                auto& cStubDataThisHybrid   = cStubDataThisOpticalGroup->getObject(cHybrid->getId());
                auto& cLineErrorsThisHybrid = cLineErrorsThisOpticalGroup->getObject(cHybrid->getId());
                auto& cCic                  = static_cast<OuterTrackerHybrid*>(cHybrid)->fCic;

                for(auto cChip: *cHybrid)
                {
                    auto& cStubDataThisChip   = cStubDataThisHybrid->getObject(cChip->getId());
                    auto& cLineErrorsThisChip = cLineErrorsThisHybrid->getObject(cChip->getId());
                    auto& cData               = cStubDataThisChip->getSummary<std::string>();
                    auto& cErrorCount         = cLineErrorsThisChip->getSummary<uint32_t>();

                    auto cPhyPortCnfg   = fCicInterface->GetPhyPortConfig(cCic, cChip->getId(), pOutLine);
                    auto cPhyPortStatus = CheckPhyPort(cHybrid, cPhyPortCnfg, pPhase, pPattern);
                    cData               = cPhyPortStatus.second;
                    cErrorCount         = cPhyPortStatus.first;
                } // chip
            } // hybrid
        } // OG
    } // board
}
void CicFEAlignment::SetStaticPhaseAlignment()
{
    LOG(INFO) << BOLDBLUE << "Setting CIC phase to static mode.." << RESET;
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                auto& cCic = static_cast<OuterTrackerHybrid*>(cHybrid)->fCic;
                fCicInterface->SetStaticPhaseAlignment(cCic);
            }
        }
    }
}
bool CicFEAlignment::CicLpGbtAlignment()
{
    // align CIC-lpGBT
    bool cOutAligned = true;
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            auto& clpGBT = cOpticalGroup->flpGBT;
            if(clpGBT == nullptr) continue;
            bool cWithCIC = false;
            for(auto cHybrid: *cOpticalGroup)
            {
                auto& cCic = static_cast<OuterTrackerHybrid*>(cHybrid)->fCic;
                if(cCic == NULL) continue;
                cWithCIC = true;
            }
            if(!cWithCIC) continue;
            cOutAligned = cOutAligned && CicLpGbtAlignment(cOpticalGroup);
        }
    }
    if(!cOutAligned)
    {
        LOG(INFO) << BOLDRED << "FAILED " << BOLDBLUE << " phase alignment step on CIC output [in lpGBT] .. " << RESET;
        exit(FAILED_PHASE_ALIGNMENT);
    }
    LOG(INFO) << BOLDGREEN << "SUCCESSFUL " << BOLDBLUE << " phase alignment on CIC outputs [in lpGBT]... " << RESET;
    return cOutAligned;
}
bool CicFEAlignment::CicLpGbtAlignment(const OpticalGroup* pOpticalGroup)
{
    auto cBoardId   = pOpticalGroup->getBeBoardId();
    auto cBoardIter = std::find_if(fDetectorContainer->begin(), fDetectorContainer->end(), [&cBoardId](Ph2_HwDescription::BeBoard* x) { return x->getId() == cBoardId; });
    // stop triggers to make sure that there are no L1 packets from the CIC
    fBeBoardInterface->Stop((*cBoardIter));

    LOG(INFO) << BOLDMAGENTA << "Aligning CIC-lpGBT data on OpticalGroup#" << +pOpticalGroup->getId() << RESET;
    auto& clpGBT = pOpticalGroup->flpGBT;
    // configure CICs to output alignment pattern on stub lines
    std::vector<uint8_t> cFeEnableRegs(0);
    for(auto cHybrid: *pOpticalGroup)
    {
        auto& cCic = static_cast<OuterTrackerHybrid*>(cHybrid)->fCic;
        // disable alignment output
        fCicInterface->SelectOutput(cCic, true);
        cFeEnableRegs.push_back(fCicInterface->ReadChipReg(cCic, "FE_ENABLE"));
        fCicInterface->EnableFEs(cCic, {0, 1, 2, 3, 4, 5, 6, 7}, false);
    }

    std::map<uint8_t, std::vector<uint8_t>> groupsAndChannels              = pOpticalGroup->getLpGBTrxGroupsAndChannels();
    auto                                    theOpticalGroupAlignmentResult = static_cast<D19clpGBTInterface*>(flpGBTInterface)->PhaseAlignRx(clpGBT, groupsAndChannels, 5);
    bool                                    isAligned                      = static_cast<D19clpGBTInterface*>(flpGBTInterface)->didAlignmentSucceded(theOpticalGroupAlignmentResult, 1, pOpticalGroup);

    // configure CICs to NOT output alignment pattern on stub lines
    size_t cIndx = 0;
    for(auto cHybrid: *pOpticalGroup)
    {
        auto& cCic = static_cast<OuterTrackerHybrid*>(cHybrid)->fCic;
        // disable alignment output
        fCicInterface->SelectOutput(cCic, false);
        fCicInterface->WriteChipReg(cCic, "FE_ENABLE", cFeEnableRegs[cIndx]);
        cIndx++;
    }
    return isAligned;
}

bool CicFEAlignment::PhaseAlignment(uint16_t pWait_us, uint32_t pNTriggers)
{
    bool cDebug   = false;
    bool cAligned = true;
    LOG(INFO) << BOLDBLUE << "Starting CIC automated phase alignment procedure for CBCs .... " << RESET;

    for(auto cBoard: *fDetectorContainer)
    {
        bool cWithCBC = false;
        // generate alignment pattern on all stub lines
        LOG(INFO) << BOLDBLUE << "Generating Patterns needed for phase alignment of CIC inputs." << RESET;
        auto cInterface = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(cBoard));
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                auto& cCic = static_cast<OuterTrackerHybrid*>(cHybrid)->fCic;
                fCicInterface->SetAutomaticPhaseAlignment(cCic, true);
                // configure Chips to produce phase alignment patterns
                for(auto cChip: *cHybrid)
                {
                    if(cChip->getFrontEndType() == FrontEndType::CBC3) cWithCBC = true;
                    fReadoutChipInterface->producePhaseAlignmentPattern(cChip, 10);
                }
            }
        }
        // send N triggers on L1 lines
        if(cWithCBC)
        {
            LOG(INFO) << BOLDBLUE << "Sending triggers to FEs to align L1 output from CBCs.." << RESET;
            uint16_t cTriggerSrc = fBeBoardInterface->ReadBoardReg(cBoard, "fc7_daq_cnfg.fast_command_block.trigger_source");
            // if external or async triggers are used then revert to internal here
            bool                                          cReconfigureTrigger = (cTriggerSrc == 4 || cTriggerSrc || 5 || cTriggerSrc == 10);
            std::vector<std::pair<std::string, uint32_t>> cRegVec;
            if(cReconfigureTrigger)
            {
                uint16_t cSrc = 3;
                if(cTriggerSrc != cSrc)
                {
                    LOG(INFO) << BOLDBLUE << "\t.. Changing trigger source is set to " << +cSrc << RESET;
                    cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.trigger_source", cSrc});
                }
                cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.triggers_to_accept", pNTriggers});
                cRegVec.push_back({"fc7_daq_ctrl.fast_command_block.control.load_config", 0x1});
                fBeBoardInterface->WriteBoardMultReg(cBoard, cRegVec);
            }
            auto cTriggerInterface = cInterface->getTriggerInterface();
            cTriggerInterface->SendNTriggers(pNTriggers);

            // set trigger source back
            if(cReconfigureTrigger)
            {
                LOG(INFO) << BOLDBLUE << "\t.. Changing trigger source back to " << +cTriggerSrc << RESET;
                std::vector<std::pair<std::string, uint32_t>> cRegVec;
                cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.trigger_source", cTriggerSrc});
                cRegVec.push_back({"fc7_daq_ctrl.fast_command_block.control.load_config", 0x1});
                fBeBoardInterface->WriteBoardMultReg(cBoard, cRegVec);
            }
        } // in the CBC case you need to send triggers to get alignment data on L1 line
        // check alignment
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                // enable automatic phase aligner
                auto& cCic    = static_cast<OuterTrackerHybrid*>(cHybrid)->fCic;
                bool  cLocked = fCicInterface->CheckPhaseAlignerLock(cCic);
                // if locked .. switch to automatic phase aligner mode with best values
                if(cLocked)
                {
                    LOG(INFO) << BOLDBLUE << "Phase aligner on CIC" << +cHybrid->getId() << BOLDGREEN << " LOCKED " << BOLDBLUE << " ... storing values and switching to static phase " << RESET;
                }
                else
                    LOG(INFO) << BOLDBLUE << "Phase aligner on CIC" << +cHybrid->getId() << BOLDRED << " FAILED to LOCK " << BOLDBLUE << " ... storing values and switching to static phase " << RESET;
                cAligned = cAligned && cLocked;
            } // CICs
        } // OG
    }
    if(cAligned) this->SetStaticPhaseAlignment();

    // check
    for(auto cBoard: *fDetectorContainer)
    {
        if(!cDebug) continue;

        auto cInterface = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(cBoard));

        D19cDebugFWInterface* cDebugInterface = cInterface->getDebugInterface();
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                auto& cCic = static_cast<OuterTrackerHybrid*>(cHybrid)->fCic;
                for(uint8_t cPhyPort = 0; cPhyPort < 12; cPhyPort++)
                {
                    fCicInterface->SelectMux(cCic, cPhyPort);
                    cDebugInterface->StubDebug(true, 4);
                }
                fCicInterface->ControlMux(cCic, 0);
            }
        }
    }

    return cAligned;
}
bool CicFEAlignment::WordAlignment(uint32_t pWait_us)
{
    LOG(INFO) << BOLDBLUE << "Starting CIC automated word alignment procedure .... " << RESET;

    bool cAligned = true;
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            std::vector<uint8_t> cWordAligned(0);
            for(auto cHybrid: *cOpticalGroup)
            {
                auto& cCic = static_cast<OuterTrackerHybrid*>(cHybrid)->fCic;
                if(cCic == NULL) continue;

                // configure word alignment pattern on CBCs
                std::vector<uint8_t> cAlignmentPatterns = fReadoutChipInterface->getWordAlignmentPatterns();
                for(auto cChip: *cHybrid) { fReadoutChipInterface->produceWordAlignmentPattern(cChip); }
                bool cSuccessAlign          = fCicInterface->AutomatedWordAlignment(cCic, cAlignmentPatterns);
                auto theWordAlignmentValues = fCicInterface->retrieveExternalWordAlignmentValues(cCic);
                cSuccessAlign               = cSuccessAlign && fCicInterface->ConfigureExternalWordAlignment(cCic, theWordAlignmentValues);
                cWordAligned.push_back(cSuccessAlign ? 1 : 0);
            } // hybrid - configure word alignment patterns

            cAligned     = true;
            size_t cIndx = 0;
            for(auto cHybrid: *cOpticalGroup)
            {
                auto& cCic = static_cast<OuterTrackerHybrid*>(cHybrid)->fCic;
                if(cCic == NULL) continue;

                cAligned = cAligned && cWordAligned[cIndx];
                fCicInterface->SetStaticWordAlignment(cCic);
                // check status
                if(cWordAligned[cIndx]) { LOG(INFO) << BOLDBLUE << "Automated word alignment procedure " << BOLDGREEN << " SUCCEEDED!" << RESET; }
                else { LOG(INFO) << BOLDRED << "Automated word alignment procedure " << BOLDRED << " FAILED!" << RESET; }
                cIndx++;
            }
        }
    }

    return cAligned;
}
void CicFEAlignment::Stop()
{
    dumpConfigFiles();
    // Destroy();
}

void CicFEAlignment::Pause() {}

void CicFEAlignment::Resume() {}

// #endif
