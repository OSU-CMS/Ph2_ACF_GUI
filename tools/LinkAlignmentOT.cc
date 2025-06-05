#include "tools/LinkAlignmentOT.h"

#include "HWInterface/D19cBackendAlignmentFWInterface.h"
#include "HWInterface/D19cDebugFWInterface.h"
#include "HWInterface/D19cFWInterface.h"
#include "HWInterface/ExceptionHandler.h"
#include "Utils/CBCChannelGroupHandler.h"
#include "Utils/ContainerFactory.h"
#include "Utils/LpGBTalignmentResult.h"

// #include "boost/format.hpp"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

LinkAlignmentOT::LinkAlignmentOT() : OTTool() {}
LinkAlignmentOT::~LinkAlignmentOT() {}

// Processing
void LinkAlignmentOT::AlignStubPackage()
{
    LOG(INFO) << BOLDYELLOW << "Will execute Sarahs align stub package function" << RESET;
    for(const auto cBoard: *fDetectorContainer) { AlignStubPackage(cBoard); } // align stubs
}
bool LinkAlignmentOT::Align()
{
    LOG(INFO) << BOLDYELLOW << "LinkAlignmentOT::Align ..." << RESET;
    for(const auto cBoard: *fDetectorContainer)
    {
        // force trigger source to be internal triggers
        LOG(INFO) << BOLDYELLOW << "Forcing trigger source to internal triggers" << RESET;
        fBeBoardInterface->WriteBoardReg(cBoard, "fc7_daq_cnfg.fast_command_block.trigger_source", 3);
        for(auto cOpticalGroup: *cBoard)
        {
            AlignLpGBTInputs(cOpticalGroup);

            auto& clpGBT = cOpticalGroup->flpGBT;
            if((cOpticalGroup->getFrontEndType() == FrontEndType::OuterTrackerPS) && static_cast<D19clpGBTInterface*>(flpGBTInterface)->GetChipRate(clpGBT) == 10)
            {
                // FIXME it is not clear why but it seems to help finishing the reconfigure step.
                LOG(INFO) << BOLDRED << "ATTENTION!!! Adding Phase Alignment for PS 10G ..." << RESET;
                PhaseAlignBEdata(cOpticalGroup);
            }

            WordAlignBEdata(cOpticalGroup);
        }
        // check that word alignment of L1 data worked
        LOG(INFO) << BOLDYELLOW << "LinkAlignmentOT::Align ... trying to readout L1 data.. " << RESET;
        ReadNEvents(cBoard, 10);
    } // align BE

    AlignStubPackage();
    fSuccess = true;
    return fSuccess;
}

// Initialization function
void LinkAlignmentOT::Initialise()
{
    // prepare common OTTool
    Prepare();
    SetName("LinkAlignmentOT");

    // list of board registers that can be modified by this tool
    std::vector<std::string> cBrdRegsToKeep{"fc7_daq_cnfg.physical_interface_block.stubs.stub_package_delay"};
    SetBrdRegstoPerserve(cBrdRegsToKeep);

    // no Chip registers to perserve

    // initialize containers that hold values found by this tool
    ContainerFactory::copyAndInitHybrid<std::vector<uint8_t>>(*fDetectorContainer, fBeSamplingDelay);
    ContainerFactory::copyAndInitHybrid<std::vector<uint8_t>>(*fDetectorContainer, fBeBitSlip);
    ContainerFactory::copyAndInitHybrid<uint8_t>(*fDetectorContainer, fLpGBTSamplingDelay);
    for(auto cBoard: *fDetectorContainer)
    {
        auto& cBeSamplingDelay = fBeSamplingDelay.getObject(cBoard->getId());
        auto& cBeBitSlip       = fBeBitSlip.getObject(cBoard->getId());
        auto& cLinkSampling    = fLpGBTSamplingDelay.getObject(cBoard->getId());

        for(auto cOpticalGroup: *cBoard)
        {
            auto&  cBeSamplingDelayOG = cBeSamplingDelay->getObject(cOpticalGroup->getId());
            auto&  cBeBitSlipOG       = cBeBitSlip->getObject(cOpticalGroup->getId());
            auto&  cLinkDelayOG       = cLinkSampling->getObject(cOpticalGroup->getId());
            size_t cNlines            = (cOpticalGroup->getFrontEndType() == FrontEndType::OuterTrackerPS) ? 7 : 6;
            for(auto cHybrid: *cOpticalGroup)
            {
                auto& cBeSamplingDelayHybrd = cBeSamplingDelayOG->getObject(cHybrid->getId());
                auto& cBeBitSlipHybrd       = cBeBitSlipOG->getObject(cHybrid->getId());
                auto& cThisBeSamplingDelay  = cBeSamplingDelayHybrd->getSummary<std::vector<uint8_t>>();
                auto& cThisBeBitSlip        = cBeBitSlipHybrd->getSummary<std::vector<uint8_t>>();
                auto& cLinkDelay            = cLinkDelayOG->getObject(cHybrid->getId())->getSummary<uint8_t>();
                cLinkDelay                  = 0;
                for(size_t cLineId = 0; cLineId < cNlines; cLineId++)
                {
                    cThisBeSamplingDelay.push_back(0);
                    cThisBeBitSlip.push_back(0);
                }
            }
        }
    }
}

// Align lpGBT inputs
bool LinkAlignmentOT::AlignLpGBTInputs(const OpticalGroup* pOpticalGroup)
{
    LOG(INFO) << BOLDYELLOW << "LinkAlignmentOT::AlignLpGBTInputs ..." << RESET;
    auto cBoardId   = pOpticalGroup->getBeBoardId();
    auto cBoardIter = std::find_if(fDetectorContainer->begin(), fDetectorContainer->end(), [&cBoardId](Ph2_HwDescription::BeBoard* x) { return x->getId() == cBoardId; });
    // stop triggers to make sure that there are no L1 packets from the CIC
    fBeBoardInterface->Stop((*cBoardIter));

    LOG(INFO) << BOLDMAGENTA << "Aligning CIC-lpGBT data on OpticalGroup#" << +pOpticalGroup->getId() << RESET;
    auto& clpGBT = pOpticalGroup->flpGBT;
    if(clpGBT == nullptr) return true;

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

        auto& cLinkSampling = fLpGBTSamplingDelay.getObject((*cBoardIter)->getId())->getObject(pOpticalGroup->getId())->getObject(cHybrid->getId())->getSummary<uint8_t>();
        cLinkSampling       = 0;
    }
    return isAligned;
}

bool LinkAlignmentOT::WordAlignBEdata(const OpticalGroup* pOpticalGroup, bool pDisableUnsresponsiveHybrids)
{
    fStubDebug      = true;
    bool cAligned   = false;
    auto cBoardId   = pOpticalGroup->getBeBoardId();
    auto cBoardIter = std::find_if(fDetectorContainer->begin(), fDetectorContainer->end(), [&cBoardId](Ph2_HwDescription::BeBoard* x) { return x->getId() == cBoardId; });
    LOG(INFO) << BOLDYELLOW << "LinkAlignmentOT::WordAlignBEdata for an OG " << RESET;
    auto cInterface = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(fDetectorContainer->getObject(cBoardId)));

    D19cDebugFWInterface*            cDebugInterface   = cInterface->getDebugInterface();
    D19cBackendAlignmentFWInterface* cAlignerInterface = cInterface->getBackendAlignmentInterface();
    LOG(INFO) << BOLDYELLOW << "LinkAlignmentOT::WordAlignBEdata after debug interface " << RESET;

    auto& cBeBitSlip   = fBeBitSlip.getObject((*cBoardIter)->getId());
    auto& cBeBitSlipOG = cBeBitSlip->getObject(pOpticalGroup->getId());

    // configure CICs to output alignment pattern on L1 lines
    std::vector<uint8_t> cFeEnableRegs(0);
    for(auto cHybrid: *pOpticalGroup)
    {
        auto& cCic = static_cast<OuterTrackerHybrid*>(cHybrid)->fCic;
        // disable alignment output
        fCicInterface->SelectOutput(cCic, true);
        cFeEnableRegs.push_back(fCicInterface->ReadChipReg(cCic, "FE_ENABLE"));
        fCicInterface->EnableFEs(cCic, {0, 1, 2, 3, 4, 5, 6, 7}, false);
    }
    // stop triggers to make sure that there are no L1 packets from the CIC
    fBeBoardInterface->Stop((*cBoardIter));

    // align stub lines in the BE
    size_t cNlines = (pOpticalGroup->getFrontEndType() == FrontEndType::OuterTrackerPS) ? 6 : 5;
    LOG(INFO) << BOLDMAGENTA << "LinkAlignmentOT::WordAlignBEdata ... word alignment on " << +cNlines << "/6 lines stub from CIC.." << RESET;
    for(size_t cLineId = 1; cLineId <= cNlines; cLineId++)
    {
        for(auto cHybrid: *pOpticalGroup)
        {
            auto& cBeBitSlipHybrd = cBeBitSlipOG->getObject(cHybrid->getId());
            auto& cThisBeBitSlip  = cBeBitSlipHybrd->getSummary<std::vector<uint8_t>>();

            LOG(INFO) << BOLDMAGENTA << "Aligning Stub line#" << +cLineId << " on Hybrid#" << +cHybrid->getId() << RESET;
            auto alignmentResult    = cAlignerInterface->alignWord(cHybrid->getId(), cLineId);
            bool cAligned           = alignmentResult.fWordAlignmentSuccess;
            cThisBeBitSlip[cLineId] = alignmentResult.fBitslip;

            if(!cAligned)
            {
                if(((cHybrid->getId() % 2) == 0) & ((cLineId - 1) == 4) & (pOpticalGroup->getFrontEndType() == FrontEndType::OuterTracker2S))
                {
                    continue;
                } // CIC_OUT_4_R will always fail for kick-off SEH, ignore here to keep allowing noise measurements
                LOG(INFO) << BOLDRED << "Could not word align-BE data in LinkAlignmentOT on Board id " << +cBoardId << " OpticalGroup id" << +pOpticalGroup->getId() << " Hybrid id"
                          << +cHybrid->getId() << " stub line " << +(cLineId - 1) << " --- Hybrid will be disabled" << RESET;
                if(pDisableUnsresponsiveHybrids) ExceptionHandler::getInstance()->disableHybrid(cBoardId, pOpticalGroup->getId(), cHybrid->getId());
                continue;
            }
            if(cThisBeBitSlip[cLineId] == 0 && !fAllowZeroBitslip)
            {
                size_t cMaxAttempts = 10;
                size_t cIter        = 0;
                do {
                    alignmentResult         = cAlignerInterface->alignWord(cHybrid->getId(), cLineId);
                    cAligned                = alignmentResult.fWordAlignmentSuccess;
                    cThisBeBitSlip[cLineId] = alignmentResult.fBitslip;
                    cIter++;
                } while(cIter < cMaxAttempts && cThisBeBitSlip[cLineId] == 0);
                if(cThisBeBitSlip[cLineId] == 0)
                {
                    LOG(INFO) << BOLDRED << "Bitslip of 0 found for BE-stub data on Board id " << +cBoardId << " OpticalGroup id" << +pOpticalGroup->getId() << " Hybrid id" << +cHybrid->getId()
                              << " stub line " << +(cLineId - 1) << " --- Hybrid will be disabled" << RESET;
                    if(pDisableUnsresponsiveHybrids) ExceptionHandler::getInstance()->disableHybrid(cBoardId, pOpticalGroup->getId(), cHybrid->getId());
                    continue;
                }
            }
        }
    }
    // check for 0 bit slips
    for(auto cHybrid: *pOpticalGroup)
    {
        auto&                cBeBitSlipHybrd = cBeBitSlipOG->getObject(cHybrid->getId());
        auto&                cThisBeBitSlip  = cBeBitSlipHybrd->getSummary<std::vector<uint8_t>>();
        std::vector<uint8_t> cBitSlipHist(15, 0);
        for(auto cItem: cThisBeBitSlip) cBitSlipHist[cItem]++;
        auto cMode = std::max_element(cBitSlipHist.begin(), cBitSlipHist.end()) - cBitSlipHist.begin();
        LOG(INFO) << BOLDMAGENTA << "Hybrid#" << +cHybrid->getId() << " most frequent bitslip is " << +cMode << RESET;
        // now if any line has a bit-slip that isn't the mode.. set it to the mode
        // for( size_t cLineId =1 ; cLineId <= cNlines; cLineId++)
        // {
        //     if(cThisBeBitSlip[cLineId] != cMode ){
        //         fBeBoardInterface->WriteBoardReg((*cBoardIter), "fc7_daq_cnfg.physical_interface_block.slvs_debug.hybrid_select", cHybrid->getId());
        //         fBeBoardInterface->WriteBoardReg((*cBoardIter), "fc7_daq_cnfg.physical_interface_block.slvs_debug.chip_select", 0);
        //         for( uint8_t cBitSlip=0; cBitSlip < 8; cBitSlip++)
        //         {
        //             LOG (INFO) << BOLDMAGENTA << "Manually setting BitSlip on Line#" << +cLineId << " to " << +cBitSlip << RESET;
        //             cTuner.SetLineMode(cInterface, cHybrid->getId(), 0, cLineId, 0);
        //             cTuner.SetLineMode(cInterface, cHybrid->getId(), 0, cLineId, 2, 0, cMode, 0, 0);
        //             cDebugInterface->StubDebug( true, cNlines );
        //         }
        //     }
        // }
    }
    if(fStubDebug)
    {
        for(auto cHybrid: *pOpticalGroup)
        {
            LOG(INFO) << BOLDMAGENTA << "Stub debug output - hybrid#" << +cHybrid->getId() << RESET;
            fBeBoardInterface->WriteBoardReg((*cBoardIter), "fc7_daq_cnfg.physical_interface_block.slvs_debug.hybrid_select", cHybrid->getId());
            fBeBoardInterface->WriteBoardReg((*cBoardIter), "fc7_daq_cnfg.physical_interface_block.slvs_debug.chip_select", 0);
            for(size_t cIter = 0; cIter < 1; cIter++)
            {
                LOG(INFO) << BOLDYELLOW << "Debug capture Iteration#" << cIter << RESET;
                cDebugInterface->StubDebug(true, cNlines);
            }
        }
    }

    // disable stub output
    for(auto cHybrid: *pOpticalGroup)
    {
        auto& cCic = static_cast<OuterTrackerHybrid*>(cHybrid)->fCic;
        // disable alignment output
        fCicInterface->SelectOutput(cCic, false);
    }
    // align L1 data in the BE
    fL1Debug = true;
    LOG(INFO) << BOLDMAGENTA << "LinkAlignmentOT::WordAlignBEdata ... word alignment on L1 lines from CIC.." << RESET;
    cAligned = L1WordAlignment(pOpticalGroup, fL1Debug);

    size_t cIndx = 0;
    // re-confiure enabled FEs
    for(auto cHybrid: *pOpticalGroup)
    {
        auto& cCic = static_cast<OuterTrackerHybrid*>(cHybrid)->fCic;
        fCicInterface->WriteChipReg(cCic, "FE_ENABLE", cFeEnableRegs[cIndx]);
        cIndx++;
    }
    LOG(INFO) << BOLDYELLOW << "Reached end of WordAlignBEData" << RESET;
    return cAligned;
}

// Phase align L1 + stub data in the backend
bool LinkAlignmentOT::PhaseAlignBEdata(const BeBoard* pBoard)
{
    bool cAligned = true;
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            cAligned = PhaseAlignBEdata(cOpticalGroup);
            if(!cAligned)
            {
                LOG(INFO) << BOLDRED << "Could not phase align-BE data in LinkAlignmentOT on Board id " << +pBoard->getId() << " OpticalGroup id" << +cOpticalGroup->getId()
                          << " --- OpticalGroup will be disabled" << RESET;
                ExceptionHandler::getInstance()->disableOpticalGroup(pBoard->getId(), cOpticalGroup->getId());
                continue;
            }
        }
    }
    return cAligned;
}
bool LinkAlignmentOT::PhaseAlignBEdata(const OpticalGroup* pOpticalGroup)
{
    bool cAligned   = true;
    auto cBoardId   = pOpticalGroup->getBeBoardId();
    auto cBoardIter = std::find_if(fDetectorContainer->begin(), fDetectorContainer->end(), [&cBoardId](Ph2_HwDescription::BeBoard* x) { return x->getId() == cBoardId; });
    auto cInterface = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(fDetectorContainer->getObject(cBoardId)));

    D19cBackendAlignmentFWInterface* cAlignerInterface = cInterface->getBackendAlignmentInterface();

    auto& cBeSamplingDelay   = fBeSamplingDelay.getObject((*cBoardIter)->getId());
    auto& cBeSamplingDelayOG = cBeSamplingDelay->getObject(pOpticalGroup->getId());

    // configure CICs to output alignment pattern on L1 lines
    std::vector<uint8_t> cFeEnableRegs(0);
    for(auto cHybrid: *pOpticalGroup)
    {
        auto& cCic = static_cast<OuterTrackerHybrid*>(cHybrid)->fCic;
        // disable alignment output
        fCicInterface->SelectOutput(cCic, true);
        cFeEnableRegs.push_back(fCicInterface->ReadChipReg(cCic, "FE_ENABLE"));
        fCicInterface->EnableFEs(cCic, {0, 1, 2, 3, 4, 5, 6, 7}, false);
    }
    // stop triggers to make sure that there are no L1 packets from the CIC
    fBeBoardInterface->Stop((*cBoardIter));

    // align stub lines in the BE
    size_t cNlines = (pOpticalGroup->getFrontEndType() == FrontEndType::OuterTrackerPS) ? 6 : 5;
    LOG(INFO) << BOLDMAGENTA << "LinkAlignmentOT::WordAlignBEdata ... word alignment on " << +cNlines << "/6 lines stub from CIC.." << RESET;
    for(size_t cLineId = 0; cLineId <= cNlines; cLineId++)
    {
        for(auto cHybrid: *pOpticalGroup)
        {
            auto& cBeSamplingDelayHybrd = cBeSamplingDelayOG->getObject(cHybrid->getId());
            auto& cThisBeSamplingDelay  = cBeSamplingDelayHybrd->getSummary<std::vector<uint8_t>>();

            if(cLineId > 0)
                LOG(INFO) << BOLDMAGENTA << "Setting sampling delay on Stub line#" << +cLineId << " on Hybrid#" << +cHybrid->getId() << RESET;
            else
                LOG(INFO) << BOLDMAGENTA << "Setting sampling delay on L1A line on Hybrid#" << +cHybrid->getId() << RESET;

            auto theAlignmentResults      = cAlignerInterface->tunePhase(cHybrid->getId(), cLineId);
            cAligned                      = theAlignmentResults.fPhaseAlignmentSuccess;
            cThisBeSamplingDelay[cLineId] = theAlignmentResults.fDelay;
            if(!cAligned) return cAligned;
        }
    }

    // configure CICs to NOT output alignment pattern on stub lines
    // and re-configure FE enable register
    size_t cIndx = 0;
    for(auto cHybrid: *pOpticalGroup)
    {
        auto& cCic = static_cast<OuterTrackerHybrid*>(cHybrid)->fCic;
        // disable alignment output
        fCicInterface->SelectOutput(cCic, false);
        fCicInterface->WriteChipReg(cCic, "FE_ENABLE", cFeEnableRegs[cIndx]);
        cIndx++;
    }
    return cAligned;
}
bool LinkAlignmentOT::L1WordAlignment(const OpticalGroup* pOpticalGroup, bool pScope, uint8_t pSkipLine)
{
    auto cBoardId   = pOpticalGroup->getBeBoardId();
    auto cBoardIter = std::find_if(fDetectorContainer->begin(), fDetectorContainer->end(), [&cBoardId](Ph2_HwDescription::BeBoard* x) { return x->getId() == cBoardId; });
    LOG(INFO) << BOLDYELLOW << "LinkAlignmentOT::L1WordAlignment " << RESET;

    auto                             cInterface        = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(fDetectorContainer->getObject(cBoardId)));
    D19cBackendAlignmentFWInterface* cAlignerInterface = cInterface->getBackendAlignmentInterface();
    D19cDebugFWInterface*            cDebugInterface   = cInterface->getDebugInterface();

    bool cSuccess = true;

    // configure triggers
    // make sure you're only sending one trigger at a time
    std::vector<std::pair<std::string, uint32_t>> cVecReg;
    cVecReg.clear();
    std::vector<std::string> cFcmdRegs{"misc.trigger_multiplicity", "user_trigger_frequency", "trigger_source", "misc.backpressure_enable", "triggers_to_accept"};
    std::vector<uint16_t>    cFcmdRegVals{0, 100, 3, 0, 0};
    std::vector<uint8_t>     cFcmdRegOrigVals(0);
    for(size_t cIndx = 0; cIndx < cFcmdRegs.size(); cIndx++)
    {
        std::string cRegName = "fc7_daq_cnfg.fast_command_block." + cFcmdRegs[cIndx];
        cVecReg.push_back({cRegName, cFcmdRegVals[cIndx]});
    }
    cVecReg.push_back({"fc7_daq_ctrl.fast_command_block.control.load_config", 0x1});
    cVecReg.push_back({"fc7_daq_cnfg.tlu_block.tlu_enabled", 0x0});
    cVecReg.push_back({"fc7_daq_cnfg.readout_block.global.data_handshake_enable", 0x1});
    fBeBoardInterface->WriteBoardMultReg(*cBoardIter, cVecReg);

    auto& cBeBitSlip   = fBeBitSlip.getObject((*cBoardIter)->getId());
    auto& cBeBitSlipOG = cBeBitSlip->getObject(pOpticalGroup->getId());

    LOG(INFO) << BOLDBLUE << "Aligning the back-end to properly decode L1A data coming from the front-end objects." << RESET;
    fBeBoardInterface->ChipReSync(*cBoardIter);
    fBeBoardInterface->Start(*cBoardIter);
    // back-end tuning on l1 lines

    for(auto cHybrid: *pOpticalGroup)
    {
        if(cHybrid->getId() + 1 == pSkipLine) // only scan line 1 or 2  if pSkipLine is set pSkilLine == 0 means no line should be skipped
            continue;

        auto& cCic = static_cast<OuterTrackerHybrid*>(cHybrid)->fCic;
        if(cCic == nullptr)
        {
            LOG(INFO) << BOLDYELLOW << " No CIC to use for L1 Word alignment..." << RESET;
            continue;
        }

        auto& cBeBitSlipHybrd = cBeBitSlipOG->getObject(cHybrid->getId());
        auto& cThisBeBitSlip  = cBeBitSlipHybrd->getSummary<std::vector<uint8_t>>();

        uint8_t cLineId         = 0;
        auto    alignmentResult = cAlignerInterface->alignWord(cHybrid->getId(), cLineId);
        cSuccess                = alignmentResult.fWordAlignmentSuccess;
        cThisBeBitSlip[cLineId] = alignmentResult.fBitslip;
    }
    fBeBoardInterface->Stop(*cBoardIter);

    for(auto cHybrid: *pOpticalGroup)
    {
        auto& cCic = static_cast<OuterTrackerHybrid*>(cHybrid)->fCic;
        if(cCic == nullptr || !pScope) continue;

        // select lines for slvs debug
        fBeBoardInterface->WriteBoardReg(*cBoardIter, "fc7_daq_cnfg.physical_interface_block.slvs_debug.hybrid_select", cHybrid->getId());
        fBeBoardInterface->WriteBoardReg(*cBoardIter, "fc7_daq_cnfg.physical_interface_block.slvs_debug.chip_select", 0);
        cDebugInterface->L1ADebug();
    }

    return cSuccess;
}
void LinkAlignmentOT::AlignStubPackage(BeBoard* pBoard)
{
    size_t cTriggerMult  = 0;
    size_t cDelayAfterTP = 300;
    // set board and get interface
    auto cInterface = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(pBoard));
    // make sure you're only sending one trigger at a time here
    LOG(INFO) << GREEN << "Trying to align CIC stub decoder in the back-end" << RESET;
    // sparsification of
    bool                 cSparsified = pBoard->getSparsification();
    std::vector<uint8_t> cFeEnableRegs(0);
    // disable FEs for all hybrids
    if(cSparsified)
        LOG(INFO) << BOLDMAGENTA << "LinkAlignmentOT::AlignStubPackage Sparsification on " << RESET;
    else
        LOG(INFO) << BOLDMAGENTA << "LinkAlignmentOT::AlignStubPackage Sparsification off " << RESET;

    for(auto cOpticalGroup: *pBoard)
    {
        for(auto cHybrid: *cOpticalGroup)
        {
            auto& cCic = static_cast<OuterTrackerHybrid*>(cHybrid)->fCic;
            cFeEnableRegs.push_back(fCicInterface->ReadChipReg(cCic, "FE_ENABLE"));
            // disable all FEs. . not needed here
            fCicInterface->EnableFEs(cCic, {0, 1, 2, 3, 4, 5, 6, 7}, false);
        }
    }
    // check trigger source
    // and reload
    uint16_t cTriggerSrc         = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_cnfg.fast_command_block.trigger_source");
    uint16_t cOriginalTPdelay    = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_cnfg.fast_command_block.test_pulse.delay_after_test_pulse");
    uint16_t cOriginalResetEn    = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_cnfg.fast_command_block.test_pulse.en_fast_reset");
    uint16_t cOriginalTriggerSrc = cTriggerSrc;
    uint16_t cOrignalTriggerMult = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_cnfg.fast_command_block.misc.trigger_multiplicity");
    uint8_t  cOriginalTLUconfig  = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_cnfg.tlu_block.tlu_enabled");

    std::vector<std::pair<std::string, uint32_t>> cRegVec;
    cTriggerSrc = (cTriggerSrc == 6) ? cTriggerSrc : 6;
    cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.trigger_source", cTriggerSrc});
    cRegVec.push_back({"fc7_daq_ctrl.fast_command_block.control.load_config", 0x1});
    cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.misc.trigger_multiplicity", cTriggerMult});
    cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.test_pulse.delay_after_test_pulse", cDelayAfterTP});
    cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.test_pulse.en_fast_reset", 1});
    cRegVec.push_back({"fc7_daq_cnfg.tlu_block.tlu_enabled", 0x0});
    fBeBoardInterface->WriteBoardMultReg(pBoard, cRegVec);

    // first lets figure out how many hybrids are enabled
    auto cEnableMask = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_cnfg.global.hybrid_enable");
    // and select one hybrid from each link
    uint32_t cNewMask = 0x00;
    for(auto cOpticalGroup: *pBoard)
    {
        bool cFirstOnLink = true;
        for(auto cHybrid: *cOpticalGroup)
        {
            if(!cFirstOnLink) continue;
            LOG(INFO) << BOLDMAGENTA << "\t\t..Hybrid#" << +cHybrid->getId() << " on Link#" << +cOpticalGroup->getId() << RESET;
            cNewMask     = cNewMask | (1 << cHybrid->getId());
            cFirstOnLink = false;
        }
    }
    LOG(INFO) << BOLDBLUE << "LinkAlignmentOT::AlignStubPackage setting hybrid enable register to " << std::bitset<32>(cNewMask) << RESET;

    bool cSkip = false;
    // Two final delay variables according to the registers
    uint32_t cFinalDelayOGs_link0_link9   = 0;
    uint32_t cFinalDelayOGs_link10_link11 = 0;
    if(!cSkip)
    {
        // Loop over all OG, stub package delay can be different for each OG
        for(auto cOpticalGroup: *pBoard)
        {
            // Get register name dependent of OG
            std::string cRegNameHybrid0;
            std::string cRegNameHybrid1;
            if(cOpticalGroup->getId() < 10)
            {
                cRegNameHybrid0 = "fc7_daq_cnfg.physical_interface_block.stubs_package_delay_hybrid0_link0_link9";
                cRegNameHybrid1 = "fc7_daq_cnfg.physical_interface_block.stubs_package_delay_hybrid1_link0_link9";
            }
            else
            {
                cRegNameHybrid0 = "fc7_daq_cnfg.physical_interface_block.stubs_package_delay_hybrid0_link10_link11";
                cRegNameHybrid1 = "fc7_daq_cnfg.physical_interface_block.stubs_package_delay_hybrid1_link10_link11";
            }

            // gethybrid IDs
            std::vector<uint8_t>                    cHybridIds(0);
            std::map<uint8_t, std::vector<uint8_t>> cHybridIdsMap;
            auto                                    cIter = cHybridIdsMap.find(cOpticalGroup->getId());
            if(cIter == cHybridIdsMap.end())
            {
                std::vector<uint8_t> cDummy;
                cDummy.clear();
                cHybridIdsMap[cOpticalGroup->getId()] = cDummy;
                cIter                                 = cHybridIdsMap.find(cOpticalGroup->getId());
            }
            bool cFirstOnLink = true;
            for(auto cHybrid: *cOpticalGroup)
            {
                if(!cFirstOnLink) continue;
                cHybridIds.push_back(cHybrid->getId());
                cIter->second.push_back(cHybrid->getId());
                cFirstOnLink = false;
            }

            // unique ids for each hybrid
            bool    cCorrectDelay = false;
            uint8_t cFinalDelayOG = 0;
            // now try and find correct package delay
            uint16_t cMaxBxCounter = 3564;
            uint32_t cNevents      = 10;

            LOG(DEBUG) << cMaxBxCounter << RESET;
            size_t cAttempt = 0;
            do {
                LOG(INFO) << BOLDMAGENTA << "Package delay alignment attempt#" << +cAttempt << RESET;
                for(uint8_t cPackageDelay = 0; cPackageDelay < 8; cPackageDelay++)
                {
                    if(cCorrectDelay) continue;

                    LOG(INFO) << BOLDMAGENTA << "Trying a stub package delay set to " << +cPackageDelay << ".. check BxIds in SW" << RESET;
                    // Get register value according to OG and write it to the register
                    uint32_t cRegValue;
                    if(cOpticalGroup->getId() < 10)
                        cRegValue = (cPackageDelay << cOpticalGroup->getId() % 10 * 3) + cFinalDelayOGs_link0_link9;
                    else
                        cRegValue = (cPackageDelay << cOpticalGroup->getId() % 10 * 3) + cFinalDelayOGs_link10_link11;

                    LOG(INFO) << BOLDYELLOW << "OG#" << cOpticalGroup->getId() << "\t.. Package delay of " << +cPackageDelay << " -- reg value " << std::bitset<32>(cRegValue) << RESET;
                    fBeBoardInterface->WriteBoardReg(pBoard, cRegNameHybrid0, cRegValue);
                    fBeBoardInterface->WriteBoardReg(pBoard, cRegNameHybrid1, cRegValue);
                    cInterface->Bx0Alignment();

                    ReadNEvents(pBoard, cNevents);
                    const std::vector<Event*>& cEvents = this->GetEvents();
                    LOG(DEBUG) << BOLDBLUE << "Read back " << +cEvents.size() << " events from the FC7 ..." << RESET;

                    // fill map of BxIds for this hybrid
                    std::map<uint8_t, std::vector<int>> cBxIds;
                    for(auto& cEvent: cEvents)
                    {
                        for(auto cId: cHybridIds)
                        {
                            auto cIter = cBxIds.find(cId);
                            if(cIter == cBxIds.end())
                            {
                                std::vector<int> cDummy;
                                cDummy.clear();
                                cBxIds[cId] = cDummy;
                                cIter       = cBxIds.find(cId);
                            }
                            cIter->second.push_back(cEvent->BxId(cId));
                            LOG(INFO) << BOLDYELLOW << "Event#" << +cEvent->GetEventCount() << "\t.. Hybrid#" << +cId << " BxId is " << cEvent->BxId(cId) << RESET;
                        }
                    }

                    // check that BxIds are synchronous across single links
                    std::vector<uint8_t> cIdsToCompare(0);
                    for(auto cIter: cHybridIdsMap)
                    {
                        LOG(INFO) << BOLDBLUE << "\t..Checking Sync for hybrids on Link#" << +cIter.first << RESET;
                        bool cSyncThisLink = true;  // if there's only one hybrid by definition you are in sync
                        if(cIter.second.size() > 1) // either 1 or 2 hybrids per link
                        {
                            // check if the two hybrids are synchronous
                            LOG(INFO) << BOLDYELLOW << "\t.. checking sync between " << +cIter.second[0] << " and " << +cIter.second[1] << RESET;
                            auto& cBxIdsFirst  = cBxIds[cIter.second[0]];
                            auto& cBxIdsSecond = cBxIds[cIter.second[1]];
                            cSyncThisLink      = (cBxIdsFirst == cBxIdsSecond);
                            if(cSyncThisLink)
                                LOG(INFO) << BOLDGREEN << "Sync on Link#" << +cIter.first << " between Hybrid#" << +cIter.second[0] << " and Hybrid#" << +cIter.second[1] << RESET;
                            else
                                LOG(INFO) << BOLDRED << "No Sync on Link#" << +cIter.first << " between Hybrid#" << +cIter.second[0] << " and Hybrid#" << +cIter.second[1] << RESET;
                        }
                        // if in sync.. add first hybrid id to list
                        if(cSyncThisLink) { cIdsToCompare.push_back(cIter.second[0]); }
                        else
                            LOG(INFO) << BOLDRED << "\t..FAILED sync on Link#" << +cIter.first << " between Hybrid#" << +cIter.second[0] << " and Hybrid#" << +cIter.second[1] << RESET;
                    }
                    // if all the links are synchronous then..
                    // check which package delay has to be used for which OG
                    // The condition is that the package delay read out has to be larger than 8
                    if(cIdsToCompare.size() == cHybridIdsMap.size())
                    {
                        std::vector<uint16_t> cPairsCompared;
                        std::vector<uint8_t>  cMatchesFound;
                        // compare ids from all links
                        for(auto cIdFirst: cIdsToCompare)
                        {
                            for(auto cIdSecond: cIdsToCompare)
                            {
                                if(cIdFirst == cIdSecond) continue;
                                uint16_t cPairId = (std::max(cIdFirst, cIdSecond) << 8) | std::min(cIdFirst, cIdSecond);
                                if(std::find(cPairsCompared.begin(), cPairsCompared.end(), cPairId) != cPairsCompared.end()) continue;

                                uint8_t cMatchFound = (cBxIds[cIdFirst] == cBxIds[cIdSecond]);
                                if(cMatchFound)
                                {
                                    LOG(INFO) << BOLDGREEN << "\t\t..BxIds from Hybrid#" << +cIdFirst << " and " << +cIdSecond << " are identical.. next will check the difference" << RESET;
                                }
                                else
                                    LOG(INFO) << BOLDRED << "\t\t..BxIds from Hybrid#" << +cIdFirst << " and " << +cIdSecond << " DO NOT match.. " << RESET;
                                cMatchesFound.push_back(cMatchFound);
                                cPairsCompared.push_back(cPairId);
                            }
                        }
                        if(cIdsToCompare.size() == 1)
                        {
                            uint16_t cPairId = 0xFF << 8 | cIdsToCompare[0];
                            cPairsCompared.push_back(cPairId);
                            cMatchesFound.push_back(1);
                        }
                        // for those that match.. check BxId difference
                        std::vector<uint8_t> cFoundDelays(0);
                        // std::vector<uint8_t> cGoodBxId(0);
                        for(size_t cIndx = 0; cIndx < cMatchesFound.size(); cIndx++)
                        {
                            if(cMatchesFound[cIndx] == 0) continue;
                            uint8_t              cFirst = cPairsCompared[cIndx] & 0xFF;
                            uint8_t              cScnd  = (cPairsCompared[cIndx] >> 8) & 0xFF;
                            std::vector<uint8_t> cIdsToCheck(0);
                            cIdsToCheck.push_back(cFirst);
                            // 0xFF marks the case where there is no second hybrid to c
                            // compare against
                            if(cScnd != 0xFF) cIdsToCheck.push_back(cScnd);
                            // std::vector<uint8_t> cIdsToCheck{ cFirst, cScnd};
                            size_t cNFound = 0;
                            for(auto cIdToCheck: cIdsToCheck)
                            {
                                std::vector<int> cBxDifferences(0);
                                size_t           cNRollOvers = 0;
                                size_t           cCounter    = 0;
                                uint8_t          cGoodBxIds  = 0;
                                for(auto cBxId: cBxIds[cIdToCheck])
                                {
                                    if(cBxId > 8) cGoodBxIds++;
                                    if(cCounter > 0)
                                    {
                                        auto cPreviousBxId = cBxIds[cIdToCheck][cCounter - 1];
                                        int  cBxDifference = (cNRollOvers)*cMaxBxCounter + (cPreviousBxId % cMaxBxCounter);
                                        cNRollOvers += ((cPreviousBxId >= 2500) && (cPreviousBxId < cMaxBxCounter)) && (cBxId < cPreviousBxId) ? 1 : 0;
                                        cBxDifference = (cNRollOvers)*cMaxBxCounter + (cBxId % cMaxBxCounter) - cBxDifference;
                                        if(cBxId > (int)cDelayAfterTP)
                                        {
                                            LOG(INFO) << BOLDGREEN << "\t\t\t\t.. Diff#" << cCounter << " : " << cBxDifference << " [ BxID = " << cBxIds[cIdToCheck][cCounter] << " ]" << RESET;
                                        }
                                        else
                                            LOG(INFO) << BOLDRED << "\t\t\t\t.. Diff#" << cCounter << " : " << cBxDifference << " [ BxID = " << cBxIds[cIdToCheck][cCounter] << " ]" << RESET;
                                        cBxDifferences.push_back(cBxDifference);
                                    }
                                    cCounter++;
                                }
                                if(std::adjacent_find(cBxDifferences.begin(), cBxDifferences.end(), std::not_equal_to<int>()) == cBxDifferences.end())
                                {
                                    if(cGoodBxIds == cBxIds[cIdToCheck].size())
                                    {
                                        LOG(INFO) << BOLDGREEN << "\t\t\t..Constant BxId difference of " << +cBxDifferences[0] << " 40 MHz clks on Hybrid#" << +cIdToCheck << RESET;
                                        cNFound++;
                                    }
                                    else
                                        LOG(INFO) << BOLDRED << "\t\t\t..Constant BxId difference of " << +cBxDifferences[0] << " 40 MHz clks on Hybrid#" << +cIdToCheck << RESET;
                                }
                            }
                            cFoundDelays.push_back((cNFound == cIdsToCheck.size()) ? 1 : 0);
                        }
                        auto cNFound = std::accumulate(cFoundDelays.begin(), cFoundDelays.end(), 0);
                        if((size_t)cNFound == cMatchesFound.size() && cNFound != 0)
                        {
                            LOG(INFO) << BOLDGREEN << "All hybrids match for a package delay of " << +cPackageDelay << RESET;
                            cCorrectDelay = true;
                            cFinalDelayOG = cPackageDelay;
                            if(cOpticalGroup->getId() < 10)
                                cFinalDelayOGs_link0_link9 = cRegValue;
                            else
                                cFinalDelayOGs_link10_link11 = cRegValue;
                        }
                        else
                            LOG(INFO) << BOLDRED << "For a package delay of " << +cPackageDelay << " found " << +cNFound << "/" << cMatchesFound.size()
                                      << " pairs of hybrids with a constant difference in BxIds" << RESET;
                    } // Ids are synchronous across each link
                    else
                        LOG(INFO) << BOLDRED << "For a package delay of " << +cPackageDelay << " DE-SYNC in one of the links..." << RESET;
                } // pkg delay
                cAttempt++;
            } while(cAttempt < 1 && !cCorrectDelay);
            LOG(INFO) << BOLDGREEN << "Optimal package delay of OG#" << cOpticalGroup->getId() << " is: " << +cFinalDelayOG << RESET;
            if(cOpticalGroup->getId() < 10)
                LOG(INFO) << BOLDGREEN << "Optimal package delay all OG until OG#" << cOpticalGroup->getId() << " is: " << std::bitset<32>(cFinalDelayOGs_link0_link9) << RESET;
            else
                LOG(INFO) << BOLDGREEN << "Optimal package delay all OG until OG#" << cOpticalGroup->getId() << " is: " << std::bitset<32>(cFinalDelayOGs_link10_link11) << RESET;

        } // OG
    }
    // set everything back to original values .. except for the trigger source
    // like I wasn't here
    // reset fast command registers
    LOG(INFO) << BOLDMAGENTA << "LinkAlignmentOT::FindPackageDelay Resetting BeBoards regs back to their original values" << RESET;
    cRegVec.clear();
    cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.trigger_source", 3});
    cRegVec.push_back({"fc7_daq_ctrl.fast_command_block.control.load_config", 0x1});
    cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.misc.trigger_multiplicity", cOrignalTriggerMult});
    cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.test_pulse.delay_after_test_pulse", cOriginalTPdelay});
    cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.test_pulse.en_fast_reset", cOriginalResetEn});
    cRegVec.push_back({"fc7_daq_cnfg.tlu_block.tlu_enabled", cOriginalTLUconfig});
    fBeBoardInterface->WriteBoardMultReg(pBoard, cRegVec);

    // reconfigure sparsification + FEs enabled in this CIC
    LOG(INFO) << BOLDMAGENTA << "LinkAlignmentOT::FindPackageDelay Resetting Sparsification" << RESET;
    fBeBoardInterface->WriteBoardReg(pBoard, "fc7_daq_cnfg.physical_interface_block.cic.2s_sparsified_enable", (int)cSparsified);
    size_t cIndx = 0;
    for(auto cOpticalGroup: *pBoard)
    {
        for(auto cHybrid: *cOpticalGroup)
        {
            auto& cCic = static_cast<OuterTrackerHybrid*>(cHybrid)->fCic;
            fCicInterface->SetSparsification(cCic, cSparsified);
            fCicInterface->WriteChipReg(cCic, "FE_ENABLE", cFeEnableRegs[cIndx]);
            cIndx++;
        }
    }
    fBeBoardInterface->WriteBoardReg(pBoard, "fc7_daq_cnfg.global.hybrid_enable", cEnableMask);
    // and check
    // make sure you do this with internal triggers
    ReadNEvents(pBoard, 10);
    const std::vector<Event*>& cEvents     = this->GetEvents();
    int                        cEventCount = 0;
    for(auto& cEvent: cEvents)
    {
        for(auto cOpticalGroup: *pBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                auto cBx = (int)cEvent->BxId(cHybrid->getId());
                LOG(INFO) << BOLDGREEN << "Event#" << cEventCount << " Link#" << +cOpticalGroup->getId() << " Hybrid#" << +cHybrid->getId() << " BxId " << cBx << RESET;
            }
        }
        cEventCount++;
    }
    fBeBoardInterface->WriteBoardReg(pBoard, "fc7_daq_cnfg.fast_command_block.trigger_source", cOriginalTriggerSrc);
    LOG(INFO) << BOLDMAGENTA << "Found package delay for OG#0 to OG#9 to be " << +cFinalDelayOGs_link0_link9 << " binary " << std::bitset<32>(cFinalDelayOGs_link0_link9) << RESET;
    LOG(INFO) << BOLDMAGENTA << "Found package delay for OG#10 to OG#11 to be " << +cFinalDelayOGs_link10_link11 << " binary " << std::bitset<32>(cFinalDelayOGs_link10_link11) << RESET;

    // set everything back to original values .. like I wasn't here
    // reset fast command registers
    LOG(INFO) << BOLDMAGENTA << "LinkAlignmentOT::FindPackageDelay Resetting BeBoards regs back to their original values" << RESET;
    cRegVec.clear();
    cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.trigger_source", cOriginalTriggerSrc});
    cRegVec.push_back({"fc7_daq_ctrl.fast_command_block.control.load_config", 0x1});
    cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.misc.trigger_multiplicity", cOrignalTriggerMult});
    cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.test_pulse.delay_after_test_pulse", cOriginalTPdelay});
    cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.test_pulse.en_fast_reset", cOriginalResetEn});
    cRegVec.push_back({"fc7_daq_cnfg.tlu_block.tlu_enabled", cOriginalTLUconfig});
    fBeBoardInterface->WriteBoardMultReg(pBoard, cRegVec);
}
bool LinkAlignmentOT::AlignStubPackage(const OpticalGroup* pOpticalGroup)
{
    auto cBoardId   = pOpticalGroup->getBeBoardId();
    auto cBoardIter = std::find_if(fDetectorContainer->begin(), fDetectorContainer->end(), [&cBoardId](Ph2_HwDescription::BeBoard* x) { return x->getId() == cBoardId; });
    auto cInterface = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(fDetectorContainer->getObject(cBoardId)));

    // make sure you're only sending one trigger at a time here
    LOG(INFO) << GREEN << "Trying to align CIC stub decoder in the back-end" << RESET;
    // sparsification of
    bool                 cSparsified   = (*cBoardIter)->getSparsification();
    uint32_t             cNevents      = 10;
    uint16_t             cMaxBxCounter = 3564;
    bool                 cCorrectDelay = false;
    std::vector<uint8_t> cFeEnableRegs(0);
    // disable FEs for all hybrids
    if(cSparsified)
        LOG(INFO) << BOLDMAGENTA << "LinkAlignmentOT::AlignStubPackage Sparsification on " << RESET;
    else
        LOG(INFO) << BOLDMAGENTA << "LinkAlignmentOT::AlignStubPackage Sparsification off " << RESET;

    for(auto cHybrid: *pOpticalGroup)
    {
        auto& cCic = static_cast<OuterTrackerHybrid*>(cHybrid)->fCic;
        cFeEnableRegs.push_back(fCicInterface->ReadChipReg(cCic, "FE_ENABLE"));
        // disable all FEs. . not needed here
        fCicInterface->EnableFEs(cCic, {0, 1, 2, 3, 4, 5, 6, 7}, false);
    }

    // check trigger source
    // and reload
    uint16_t cTriggerSrc         = fBeBoardInterface->ReadBoardReg((*cBoardIter), "fc7_daq_cnfg.fast_command_block.trigger_source");
    uint16_t cOriginalTriggerSrc = cTriggerSrc;
    uint16_t cOrignalTriggerMult = fBeBoardInterface->ReadBoardReg((*cBoardIter), "fc7_daq_cnfg.fast_command_block.misc.trigger_multiplicity");
    uint8_t  cOriginalTLUconfig  = fBeBoardInterface->ReadBoardReg((*cBoardIter), "fc7_daq_cnfg.tlu_block.tlu_enabled");
    cTriggerSrc                  = (cTriggerSrc == 6) ? cTriggerSrc : 6;
    LOG(INFO) << BOLDBLUE << "Trigger source is set to " << +cTriggerSrc << RESET;
    std::vector<std::pair<std::string, uint32_t>> cRegVec;
    cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.trigger_source", cTriggerSrc});
    cRegVec.push_back({"fc7_daq_ctrl.fast_command_block.control.load_config", 0x1});
    cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.misc.trigger_multiplicity", 0x0});
    cRegVec.push_back({"fc7_daq_cnfg.tlu_block.tlu_enabled", 0x0});
    fBeBoardInterface->WriteBoardMultReg((*cBoardIter), cRegVec);

    // now try and find correct package delay
    auto cOriginalDelay = fBeBoardInterface->ReadBoardReg((*cBoardIter), "fc7_daq_cnfg.physical_interface_block.stubs.stub_package_delay");
    LOG(INFO) << BOLDBLUE << "Original package delay is " << +cOriginalDelay << RESET;
    uint8_t cPackageDelay = 7;
    uint8_t cFinalDelay   = cPackageDelay;
    for(cPackageDelay = 0; cPackageDelay < 8; cPackageDelay++)
    {
        if(cCorrectDelay) continue;

        LOG(INFO) << BOLDMAGENTA << "Trying a stub package delay set to " << +cPackageDelay << ".. check BxIds in SW" << RESET;
        fBeBoardInterface->WriteBoardReg((*cBoardIter), "fc7_daq_cnfg.physical_interface_block.stubs.stub_package_delay", cPackageDelay);
        cInterface->Bx0Alignment();

        // check stubs
        // 2 events should be enough
        // LOG(DEBUG) << BOLDMAGENTA << "Requesting " << +cNevents << " events from the board " << RESET;
        ReadNEvents((*cBoardIter), cNevents);
        const std::vector<Event*>& cEventsWithStubs = this->GetEvents();
        LOG(DEBUG) << BOLDBLUE << "Read back " << +cEventsWithStubs.size() << " events from the FC7 ..." << RESET;

        // now ... check for incrementing BxIds
        int              cNRollOvers = 0;
        std::vector<int> cBxIds(0);
        std::vector<int> cBxDifferences(0); // I think by injecting this way this number should always be the same ..
        for(auto& cEvent: cEventsWithStubs)
        {
            auto cHybrid = pOpticalGroup->getFirstObject();
            auto cBx     = (int)cEvent->BxId(cHybrid->getId());
            if(cBxIds.size() > 0)
            {
                int cBxDifference = (cNRollOvers)*cMaxBxCounter + (cBxIds[cBxIds.size() - 1] % cMaxBxCounter);
                cNRollOvers += ((cBxIds[cBxIds.size() - 1] >= 2500) && (cBxIds[cBxIds.size() - 1] < cMaxBxCounter)) && (cBx < cBxIds[cBxIds.size() - 1]) ? 1 : 0;
                cBxDifference = (cNRollOvers)*cMaxBxCounter + (cBx % cMaxBxCounter) - cBxDifference;
                cBxDifferences.push_back(cBxDifference);
                // LOG(INFO) << BOLDBLUE << "\t.....BxDifference is " << +cBxDifference << RESET;
            }
            cBxIds.push_back(cBx);
            LOG(DEBUG) << BOLDBLUE << "Hybrid " << +cHybrid->getId() << " BxID " << +cBx << RESET;
        } // events
        // figure out the differences between the bxIds
        auto cFirstDifference = cBxDifferences[0];
        std::adjacent_difference(cBxDifferences.begin(), cBxDifferences.end(), cBxDifferences.begin());
        cBxDifferences.erase(cBxDifferences.begin()); // erase the first element
        for(auto cDifference: cBxDifferences) LOG(DEBUG) << BOLDBLUE << "\t..." << +cDifference << RESET;
        // all elements are equal
        if(cFirstDifference != 0 && std::equal(cBxDifferences.begin() + 1, cBxDifferences.end(), cBxDifferences.begin()))
        {
            LOG(DEBUG) << BOLDGREEN << "Found differences between bxIds to always be the same : " << +cFirstDifference << RESET;
            LOG(INFO) << BOLDGREEN << "Going to fix the manual package delay to " << +cPackageDelay << RESET;
            cFinalDelay   = cPackageDelay;
            cCorrectDelay = true;
        }
        else
            LOG(DEBUG) << BOLDRED << "Found differences between bxIds to be different from one another." << RESET;

    } // pkg delay

    // set everything back to original values .. like I wasn't here
    // reset fast command registers
    LOG(INFO) << BOLDMAGENTA << "LinkAlignmentOT::FindPackageDelay Resetting BeBoards regs back to their original values" << RESET;
    cRegVec.clear();
    cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.trigger_source", cOriginalTriggerSrc});
    cRegVec.push_back({"fc7_daq_ctrl.fast_command_block.control.load_config", 0x1});
    cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.misc.trigger_multiplicity", cOrignalTriggerMult});
    cRegVec.push_back({"fc7_daq_cnfg.tlu_block.tlu_enabled", cOriginalTLUconfig});
    fBeBoardInterface->WriteBoardMultReg((*cBoardIter), cRegVec);

    // reconfigure sparsification + FEs enabled in this CIC
    LOG(INFO) << BOLDMAGENTA << "LinkAlignmentOT::FindPackageDelay Resetting Sparsification" << RESET;
    fBeBoardInterface->WriteBoardReg((*cBoardIter), "fc7_daq_cnfg.physical_interface_block.cic.2s_sparsified_enable", (int)cSparsified);
    size_t cIndx = 0;

    for(auto cHybrid: *pOpticalGroup)
    {
        auto& cCic = static_cast<OuterTrackerHybrid*>(cHybrid)->fCic;
        fCicInterface->SetSparsification(cCic, cSparsified);
        fCicInterface->WriteChipReg(cCic, "FE_ENABLE", cFeEnableRegs[cIndx]);
        cIndx++;
    }
    LOG(INFO) << BOLDMAGENTA << "Found package delay to be " << +cFinalDelay << RESET;
    return cCorrectDelay;
}
// State machine control functions
void LinkAlignmentOT::Running()
{
    Initialise();
    try
    {
        this->Align();
    }
    catch(const std::exception& e)
    {
        fSuccess = false;
        LOG(INFO) << BOLDRED << "LinkAlignmentOT failed" << RESET;
        throw std::runtime_error(std::string("Could not align link in the BE"));
    }
    fSuccess = true;
    Reset();
}

void LinkAlignmentOT::Stop() {}

void LinkAlignmentOT::Pause() {}

void LinkAlignmentOT::Resume() {}
