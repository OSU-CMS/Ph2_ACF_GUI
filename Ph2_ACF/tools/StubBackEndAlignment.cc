#include "tools/StubBackEndAlignment.h"

#include "HWInterface/D19cFWInterface.h"
#include "Utils/CBCChannelGroupHandler.h"
#include "Utils/ContainerFactory.h"
#include "boost/format.hpp"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

StubBackEndAlignment::StubBackEndAlignment() : OTTool() {}
StubBackEndAlignment::~StubBackEndAlignment() {}

void StubBackEndAlignment::Initialise()
{
    // prepare common OTTool
    Prepare();
    SetName("StubBackEndAlignment");

    // list of board registers that can be modified by this tool
    std::vector<std::string> cBrdRegsToKeep{"fc7_daq_cnfg.physical_interface_block.stubs.stub_package_delay"};
    SetBrdRegstoPerserve(cBrdRegsToKeep);

    // list of board registers that can be modified by this tool
    for(auto cBoard: *fDetectorContainer)
    {
        LOG(INFO) << BOLDYELLOW << "Package delay on BeBoard#" << +cBoard->getId() << " set to "
                  << fBeBoardInterface->ReadBoardReg(cBoard, "fc7_daq_cnfg.physical_interface_block.stubs.stub_package_delay") << RESET;
    }
}
bool StubBackEndAlignment::FindPackageDelay(BeBoard* pBoard)
{
    // make sure you're only sending one trigger at a time here
    LOG(INFO) << GREEN << "Trying to align CIC decoder in the back-end" << RESET;
    // sparsification of
    bool                 cSparsified   = pBoard->getSparsification();
    uint32_t             cNevents      = 10;
    uint16_t             cMaxBxCounter = 3564;
    bool                 cCorrectDelay = false;
    std::vector<uint8_t> cFeEnableRegs(0);
    for(auto cOpticalGroup: *pBoard)
    {
        if(cSparsified)
            LOG(INFO) << BOLDMAGENTA << "StubBackEndAlignment::FindPackageDelay Sparsification on " << RESET;
        else
            LOG(INFO) << BOLDMAGENTA << "StubBackEndAlignment::FindPackageDelay Sparsification off " << RESET;

        for(auto cHybrid: *cOpticalGroup)
        {
            auto& cCic = static_cast<OuterTrackerHybrid*>(cHybrid)->fCic;
            cFeEnableRegs.push_back(fCicInterface->ReadChipReg(cCic, "FE_ENABLE"));
            // disable all FEs. . not needed here
            fCicInterface->EnableFEs(cCic, {0, 1, 2, 3, 4, 5, 6, 7}, false);
        }
    } // disable FEs for all hybrids

    // check trigger source
    // and reload
    uint16_t cTriggerSrc         = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_cnfg.fast_command_block.trigger_source");
    uint16_t cOriginalTriggerSrc = cTriggerSrc;
    uint16_t cOrignalTriggerMult = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_cnfg.fast_command_block.misc.trigger_multiplicity");
    uint8_t  cOriginalTLUconfig  = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_cnfg.tlu_block.tlu_enabled");
    cTriggerSrc                  = (cTriggerSrc == 6) ? cTriggerSrc : 6;
    LOG(INFO) << BOLDBLUE << "Trigger source is set to " << +cTriggerSrc << RESET;
    std::vector<std::pair<std::string, uint32_t>> cRegVec;
    cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.trigger_source", cTriggerSrc});
    cRegVec.push_back({"fc7_daq_ctrl.fast_command_block.control.load_config", 0x1});
    cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.misc.trigger_multiplicity", 0x0});
    cRegVec.push_back({"fc7_daq_cnfg.tlu_block.tlu_enabled", 0x0});
    fBeBoardInterface->WriteBoardMultReg(pBoard, cRegVec);

    // now try and find correct package delay
    auto cOriginalDelay = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_cnfg.physical_interface_block.stubs.stub_package_delay");
    LOG(INFO) << BOLDBLUE << "Original package delay is " << +cOriginalDelay << RESET;
    uint8_t cPackageDelay = 7;
    uint8_t cFinalDelay   = cPackageDelay;
    for(cPackageDelay = 0; cPackageDelay < 8; cPackageDelay++)
    {
        if(cCorrectDelay) continue;

        LOG(INFO) << BOLDMAGENTA << "Package delay set to " << +cPackageDelay << RESET;
        fBeBoardInterface->WriteBoardReg(pBoard, "fc7_daq_cnfg.physical_interface_block.stubs.stub_package_delay", cPackageDelay);
        (static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(pBoard)))->Bx0Alignment();

        // check stubs
        // 2 events should be enough
        // LOG(DEBUG) << BOLDMAGENTA << "Requesting " << +cNevents << " events from the board " << RESET;
        ReadNEvents(pBoard, cNevents);
        const std::vector<Event*>& cEventsWithStubs = this->GetEvents();
        LOG(INFO) << BOLDBLUE << "Read back " << +cEventsWithStubs.size() << " events from the FC7 ..." << RESET;

        // now ... check for incrementing BxIds
        int              cNRollOvers = 0;
        std::vector<int> cBxIds(0);
        std::vector<int> cBxDifferences(0); // I think by injecting this way this number should always be the same ..
        for(auto& cEvent: cEventsWithStubs)
        {
            for(auto cOpticalGroup: *pBoard)
            {
                for(auto cHybrid: *cOpticalGroup)
                {
                    if(cHybrid->getId() > 0) continue;

                    auto cBx = (int)cEvent->BxId(cHybrid->getId());
                    if(cBxIds.size() > 0)
                    {
                        int cBxDifference = (cNRollOvers)*cMaxBxCounter + (cBxIds[cBxIds.size() - 1] % cMaxBxCounter);
                        cNRollOvers += ((cBxIds[cBxIds.size() - 1] >= 2500) && (cBxIds[cBxIds.size() - 1] < cMaxBxCounter)) && (cBx < cBxIds[cBxIds.size() - 1]) ? 1 : 0;
                        cBxDifference = (cNRollOvers)*cMaxBxCounter + (cBx % cMaxBxCounter) - cBxDifference;
                        cBxDifferences.push_back(cBxDifference);
                        // LOG(INFO) << BOLDBLUE << "\t.....BxDifference is " << +cBxDifference << RESET;
                    }
                    cBxIds.push_back(cBx);
                    LOG(INFO) << BOLDBLUE << "Hybrid " << +cHybrid->getId() << " BxID " << +cBx << RESET;

                } // hybrids or CICs
            } // OGs
        } // events
        // figure out the differences between the bxIds
        auto cFirstDifference = cBxDifferences[0];
        std::adjacent_difference(cBxDifferences.begin(), cBxDifferences.end(), cBxDifferences.begin());
        cBxDifferences.erase(cBxDifferences.begin()); // erase the first element
        for(auto cDifference: cBxDifferences) LOG(DEBUG) << BOLDBLUE << "\t..." << +cDifference << RESET;
        // all elements are equal
        if(cFirstDifference != 0 && std::equal(cBxDifferences.begin() + 1, cBxDifferences.end(), cBxDifferences.begin()))
        {
            LOG(INFO) << BOLDGREEN << "Found differences between bxIds to always be the same : " << +cFirstDifference << RESET;
            LOG(INFO) << BOLDGREEN << "Going to fix the manual package delay to " << +cPackageDelay << RESET;
            cFinalDelay   = cPackageDelay;
            cCorrectDelay = true;
        }
        else
            LOG(INFO) << BOLDRED << "Found differences between bxIds to be different from one another." << RESET;

    } // pkg delay

    // set everything back to original values .. like I wasn't here
    // reset fast command registers
    LOG(INFO) << BOLDMAGENTA << "StubBackEndAlignment::FindPackageDelay Resetting BeBoards regs back to their original values" << RESET;
    cRegVec.clear();
    cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.trigger_source", cOriginalTriggerSrc});
    cRegVec.push_back({"fc7_daq_ctrl.fast_command_block.control.load_config", 0x1});
    cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.misc.trigger_multiplicity", cOrignalTriggerMult});
    cRegVec.push_back({"fc7_daq_cnfg.tlu_block.tlu_enabled", cOriginalTLUconfig});
    fBeBoardInterface->WriteBoardMultReg(pBoard, cRegVec);

    // reconfigure sparsification + FEs enabled in this CIC
    LOG(INFO) << BOLDMAGENTA << "StubBackEndAlignment::FindPackageDelay Resetting Sparsification" << RESET;
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
    LOG(INFO) << BOLDMAGENTA << "Found package delay to be " << +cFinalDelay << RESET;
    return cCorrectDelay;
}
bool StubBackEndAlignment::FindStubLatency(BeBoard* pBoard)
{
    uint32_t cNevents = 10;
    LOG(INFO) << BOLDMAGENTA << "StubAlignmentThreshold" << RESET;
    // auto     cSetting   = fSettingsMap.find("StubAlignmentThreshold");
    uint32_t cThreshold = 120;
    LOG(INFO) << BOLDMAGENTA << "StubAlignmentScanStart" << RESET;
    // auto     cSetting1  = fSettingsMap.find("StubAlignmentScanStart");
    uint32_t cScanStart = 100;
    LOG(INFO) << BOLDMAGENTA << "DNEN" << RESET;

    // sparsification of
    bool cSparsified = pBoard->getSparsification();
    if(cSparsified)
        LOG(INFO) << BOLDMAGENTA << "StubBackEndAlignment::FindStubLatency Sparsification on " << RESET;
    else
        LOG(INFO) << BOLDMAGENTA << "StubBackEndAlignment::FindStubLatency Sparsification off " << RESET;

    LOG(INFO) << GREEN << "Trying to find stub latency finding in the back-end" << RESET;
    fBeBoardInterface->WriteBoardReg(pBoard, "fc7_daq_cnfg.physical_interface_block.cic.2s_sparsified_enable", (int)cSparsified);

    // read back original masks
    bool cWithPS = false;
    for(auto cOpticalGroup: *pBoard) { cWithPS = cWithPS || (cOpticalGroup->getFrontEndType() == FrontEndType::OuterTrackerPS); }

    for(auto cOpticalGroup: *pBoard) // TODO: Need a PSv2 Flag
    {
        for(auto cHybrid: *cOpticalGroup)
        {
            for(auto cChip: *cHybrid)
            {
                if(cChip->getFrontEndType() == FrontEndType::MPA2)
                {
                    cWithPS = false;
                    break;
                }
            }
        }
    }

    // reconfigure fast commands
    // fast command config
    // if PS module want trigger multiplicty to be 3
    uint8_t cMult = (cWithPS) ? 2 : 0;
    // uint8_t                  cMult            = 0;
    uint8_t                  cTriggerSource   = 6;
    uint16_t                 cDelayAfterReset = 100;
    uint16_t                 cDelayAfterTP    = 200;
    uint16_t                 cDelayTillNext   = 400;
    std::vector<std::string> cFcmdRegs{"trigger_source", "test_pulse.delay_after_fast_reset", "test_pulse.delay_after_test_pulse", "test_pulse.delay_before_next_pulse", "misc.trigger_multiplicity"};
    std::vector<uint16_t>    cFcmdRegVals{cTriggerSource, cDelayAfterReset, cDelayAfterTP, cDelayTillNext, cMult};
    std::vector<uint16_t>    cFcmdRegOrigVals(cFcmdRegs.size(), 0);
    std::vector<std::pair<std::string, uint32_t>> cRegVec;
    cRegVec.clear();
    for(size_t cIndx = 0; cIndx < cFcmdRegs.size(); cIndx++)
    {
        std::string cRegName    = "fc7_daq_cnfg.fast_command_block." + cFcmdRegs[cIndx];
        cFcmdRegOrigVals[cIndx] = fBeBoardInterface->ReadBoardReg(pBoard, cRegName);
        cRegVec.push_back({cRegName, cFcmdRegVals[cIndx]});
    }
    cRegVec.push_back({"fc7_daq_ctrl.fast_command_block.control.load_config", 0x1});
    fBeBoardInterface->WriteBoardMultReg(pBoard, cRegVec);
    uint8_t cOriginalTLUconfig = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_cnfg.tlu_block.tlu_enabled");
    fBeBoardInterface->WriteBoardReg(pBoard, "fc7_daq_cnfg.tlu_block.tlu_enabled", 0);

    // expected spacing between TP and L1A
    auto                  cDelay          = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_cnfg.fast_command_block.test_pulse.delay_after_test_pulse");
    size_t                cNinjectedHits  = 0;
    size_t                cNinjectedStubs = 0;
    int                   cReTime         = 0;
    uint8_t               cPSrow          = 10;
    uint16_t              cPSmode         = 2; // (0) pixel-strip, (1) strip-strip, (2) pixel-pixel, (3) strip-pixel
    uint8_t               cStubWindow     = 1; // stub window in half pixels (1)
    uint8_t               cMode           = cPSmode;
    std::vector<uint32_t> cPixelIds(0); // these will be used to generate stubs
    std::vector<uint8_t>  cRows{cPSrow};
    uint8_t               cMaxStubs = 4;
    // int    cChipId         = 7;
    for(auto cOpticalReadout: *pBoard)
    {
        for(auto cHybrid: *cOpticalReadout)
        {
            // for 2S - do with ananlogue injection into CBC
            size_t cNinjectedStubsThisHybrid = 0;
            for(auto cChip: *cHybrid) // for each chip (makes sense)
            {
                if(cChip->getFrontEndType() != FrontEndType::CBC3) continue;
                bool cWithNoise = false;
                // inject stubs with TP
                uint8_t                              cSeed = 60; // 2 + (uint8_t)(cChip->getId()*2);
                std::vector<std::pair<uint8_t, int>> cSeeds{{cSeed, 0}};
                // make sure we are within the limits of the CIC
                // only inject 3 stubs here
                if(cNinjectedStubsThisHybrid > cMaxStubs) { cSeeds.clear(); }

                size_t cNhits = 0;
                for(const auto& theSeedAndBend: cSeeds)
                    for(size_t cIndx = 0; cIndx < cSeeds.size(); cIndx += 1)
                    {
                        auto cHitList = (static_cast<CbcInterface*>(fReadoutChipInterface))->stubInjectionPattern(cChip, theSeedAndBend.first, theSeedAndBend.second);
                        cNinjectedHits += cHitList.size();
                        cNhits += cHitList.size();
                    }

                cNinjectedStubs += cSeeds.size();
                cNinjectedStubsThisHybrid += cSeeds.size();
                (static_cast<CbcInterface*>(fReadoutChipInterface))->injectStubs(cChip, cSeeds, cWithNoise);
                fReadoutChipInterface->WriteChipReg(cChip, "Threshold", cThreshold);
                // // enable stub logic
                // // make sure OR mode is used
                // static_cast<CbcInterface*>(fReadoutChipInterface)->selectLogicMode(cChip, "OR", true, true);
                // // set PtCut to maximum
                // fReadoutChipInterface->WriteChipReg(cChip, "PtCut", 14);
                // // no cluster cut
                // fReadoutChipInterface->WriteChipReg(cChip, "ClusterCut", 4);
                LOG(INFO) << BOLDMAGENTA << "Injecting " << +cNhits << " hits and " << +cSeeds.size() << " stubs in CBC#" << +cChip->getId() << " on hybrid#" << +cHybrid->getId() << RESET;
            } // 2S chips  - CBCs

            // for PS - first digital injection in MPAs
            for(auto cChip: *cHybrid) // for each chip (makes sense)
            {
                if(cChip->getFrontEndType() != FrontEndType::MPA2) continue;

                std::vector<Injection> cInjections(0);
                for(size_t cIndx = 0; cIndx < cRows.size(); cIndx++)
                {
                    Injection cInjection;
                    cInjection.fColumn = 2 * (int)cChip->getId() % 8 + 1;
                    cInjection.fRow    = cRows[cIndx];
                    cInjections.push_back(cInjection);
                    uint32_t cPixelId = (uint32_t)(cInjection.fColumn) * 120 + (uint32_t)cInjection.fRow;
                    cPixelIds.push_back(cPixelId);
                } // create injection patterns
                cNinjectedHits += cRows.size();
                cNinjectedStubs += cInjections.size();
                // activate stub mode
                fReadoutChipInterface->WriteChipReg(cChip, "StubMode", cMode);
                fReadoutChipInterface->WriteChipReg(cChip, "StubWindow", cStubWindow);
                cReTime = fReadoutChipInterface->ReadChipReg(cChip, "RetimePix");

                (static_cast<PSInterface*>(fReadoutChipInterface))->digiInjection(cChip, cInjections, 0x01);
            } // PS chips  - MPAs

            // for PS - digital injection in SSAs
            for(auto cChip: *cHybrid) // for each chip (makes sense)
            {
                if(cChip->getFrontEndType() != FrontEndType::SSA2) continue;

                // uint8_t cPattern = cDistributeInj ? (1 << (7 - cChip->getId())) : (0x1 << 0);
                // disable all SSAs when doing this - why?
                fReadoutChipInterface->WriteChipReg(cChip, "ENFLAGS_ALL", 0x0);

                if(cPSmode == 2) continue;
                fReadoutChipInterface->WriteChipReg(cChip, "DigCalibPattern_L_ALL", (0x1 << 0));
                fReadoutChipInterface->WriteChipReg(cChip, "DigCalibPattern_H_ALL", (0x0 << 0));
                fReadoutChipInterface->WriteChipReg(cChip, "CalPulse_duration", 0x01);
                for(auto cRow: cRows)
                {
                    LOG(DEBUG) << BOLDMAGENTA << "\t...Injecting in"
                               << " strip#" << +cRow << " in SSA#" << +cChip->getId() << RESET;
                    fReadoutChipInterface->WriteChipReg(cChip, "ENFLAGS_S" + std::to_string(cRow), 0x9);
                }
                cNinjectedHits += cRows.size();
            } // PS chips  - MPAs

        } // Hybrid
    } // OG

    // find correct hit latency
    bool     cFoundCorrectHitLatency = false;
    uint16_t cHitLatency             = 0;
    int      cExpectedOffset         = -6;
    float    cFraction               = (cWithPS) ? 0.5 * (1.0 / (1 + cMult)) : 0.5;
    for(int cOffset = cExpectedOffset; cOffset < cExpectedOffset + 20; cOffset++)
    {
        if(cFoundCorrectHitLatency) continue;

        int cLatency = cDelay + cOffset;
        if(cLatency < 0) continue;

        LOG(INFO) << BOLDGREEN << "Hit Latency of " << +cLatency << " - offset of " << +cOffset << RESET;

        for(auto cOpticalReadout: *pBoard)
        {
            for(auto cHybrid: *cOpticalReadout)
            {
                for(auto cChip: *cHybrid) // for each chip (makes sense)
                {
                    if(cChip->getFrontEndType() == FrontEndType::SSA2)
                        fReadoutChipInterface->WriteChipReg(cChip, "TriggerLatency", (uint16_t)cLatency + 1);
                    else
                        fReadoutChipInterface->WriteChipReg(cChip, "TriggerLatency", (uint16_t)cLatency);
                } // Chip - only MPAs and CBCs for this test since I'm eihter in p=p mode or 2S
            } // hybrid
        } // OG

        // send a ReSync since the latency was changed
        fBeBoardInterface->ChipReSync(pBoard);
        // look at data
        ReadNEvents(pBoard, cNevents);
        const std::vector<Event*>& cEvents         = this->GetEvents();
        size_t                     cNEventsMatched = 0;
        // one of these triggers should match
        for(size_t cTriggerId = 0; cTriggerId < (size_t)(cMult + 1); cTriggerId++)
        {
            if(cFoundCorrectHitLatency) continue;
            LOG(INFO) << BOLDBLUE << "Checking readout for match in number of hits ... looking at Trigger#" << +cTriggerId << " in a burst of " << (1 + cMult) << RESET;
            auto cEventIter   = cEvents.begin() + cTriggerId;
            cNEventsMatched   = 0;
            size_t cAllEvents = 0;
            do {
                if(cEventIter >= cEvents.end()) break;
                size_t cNHits = 0;
                for(auto cOpticalReadout: *pBoard)
                {
                    for(auto cHybrid: *cOpticalReadout)
                    {
                        size_t cNHitsPerHybrid = 0;
                        for(auto cChip: *cHybrid)
                        {
                            if(cChip->getFrontEndType() == FrontEndType::SSA2) continue;

                            if(cChip->getFrontEndType() == FrontEndType::MPA2)
                            {
                                auto cPclus = static_cast<D19cCic2Event*>(*cEventIter)->GetPixelClusters(cChip->getHybridId(), cChip->getId());
                                auto cSclus = static_cast<D19cCic2Event*>(*cEventIter)->GetStripClusters(cChip->getHybridId(), cChip->getId());
                                LOG(DEBUG) << BOLDBLUE << "Trigger#" << +cTriggerId << " in a burst of " << (1 + cMult) << " MPA" << +cChip->getId() << " found " << cSclus.size() << " S clusters and "
                                           << cPclus.size() << " P clusters in L1 data." << RESET;
                            }
                            size_t cNHitsThisFE = (*cEventIter)->GetHits(cHybrid->getId(), cChip->getId()).size();
                            cNHitsPerHybrid += cNHitsThisFE;
                            cNHits += cNHitsThisFE;
                            // if( cNHitsThisFE > 0 )
                            LOG(DEBUG) << BOLDBLUE << "\t.. Chip" << +cChip->getId() << " found " << +cNHitsThisFE << " hits .." << RESET;
                        }
                        LOG(DEBUG) << BOLDMAGENTA << "Trigger#" << +cTriggerId << " in a burst of " << (1 + cMult) << " found " << +cNHitsPerHybrid << " hits in Hybrid#" << +cHybrid->getId() << RESET;
                    }
                }
                cNEventsMatched += (cNHits == cNinjectedHits) ? 1 : 0;
                cAllEvents++;
                cEventIter += (1 + cMult);
            } while(cEventIter < cEvents.end());
            if(cNEventsMatched == cAllEvents && cAllEvents > 0)
            {
                LOG(INFO) << BOLDGREEN << ".... matched number of hits in all events." << RESET;
                cFoundCorrectHitLatency = true;
            }
        } // event loop
        if(cFoundCorrectHitLatency)
        {
            cHitLatency = cLatency;
            LOG(INFO) << BOLDGREEN << "For a latency of " << +cLatency << " found " << +cNEventsMatched << " out of " << +cEvents.size() << " events with the correct number of hits for all FEs."
                      << RESET;
        }
        else
            LOG(INFO) << BOLDRED << "For a latency of " << +cLatency << " found " << +cNEventsMatched << " out of " << +cEvents.size() << " events with the correct number of hits for all FEs."
                      << RESET;
    }

    bool cFoundCorrectStubLatency = cFoundCorrectHitLatency;
    int  cCorrectOffset           = 0;
    if(cFoundCorrectStubLatency)
    {
        cFoundCorrectStubLatency = false;
        LOG(INFO) << BOLDGREEN << "Searching for correct stub  latency .. hit latency set to " << +cHitLatency << RESET;
        // // now scan stub latency
        auto cOriginalStubDelay = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_cnfg.readout_block.global.common_stubdata_delay");
        LOG(INFO) << BOLDMAGENTA << "Original stub delay set to " << +cOriginalStubDelay << RESET;
        for(int cOffset = cScanStart; cOffset >= 20; cOffset--)
        {
            if(cFoundCorrectStubLatency) continue;
            int cStubLatency = cHitLatency - cOffset;
            LOG(INFO) << BOLDGREEN << "Stub Latency of " << +cStubLatency << RESET;

            fBeBoardInterface->WriteBoardReg(pBoard, "fc7_daq_cnfg.readout_block.global.common_stubdata_delay", cStubLatency);
            ReadNEvents(pBoard, cNevents);
            const std::vector<Event*>& cEvents      = this->GetEvents();
            size_t                     cNStubsFound = 0;
            for(auto cEvent: cEvents)
            {
                for(auto cOpticalGroup: *pBoard)
                {
                    for(auto cHybrid: *cOpticalGroup)
                    {
                        auto   cBx              = (int)cEvent->BxId(cHybrid->getId());
                        size_t cNstubsThisHybrd = 0;
                        for(auto cChip: *cHybrid)
                        {
                            if(cChip->getFrontEndType() == FrontEndType::SSA2) continue;
                            if(cEvent->GetHits(cHybrid->getId(), cChip->getId()).size() == 0) continue;

                            auto cStubs = static_cast<D19cCic2Event*>(cEvent)->StubVector(cHybrid->getId(), cChip->getId());
                            cNstubsThisHybrd += cStubs.size();
                            cNStubsFound += cStubs.size();
                        } // Chips
                        if(cNstubsThisHybrd > 0)
                            LOG(INFO) << BOLDMAGENTA << "Event#" << +cEvent->GetEventCount() << " found " << +cNstubsThisHybrd << " stubs in CIC#" << +cHybrid->getId() << " BxId is " << +cBx << RESET;
                    } // hybrids
                } // OGs
            } // events
            cFoundCorrectStubLatency = (cNStubsFound > cFraction * cNinjectedStubs * cEvents.size());
            if(cFoundCorrectStubLatency)
            {
                cCorrectOffset = cOffset;
                LOG(INFO) << BOLDGREEN << "For a stub latency of " << +cStubLatency << " found " << +cNStubsFound << " stubs out of " << +cNinjectedStubs * cEvents.size() << " expected." << RESET;
            }
        } // offset scan
    }

    // adding this here in preparation for stub decoding
    // I think this should belong to the board.. need to fix
    if(cFoundCorrectStubLatency)
    {
        LOG(INFO) << BOLDMAGENTA << "Stub offset set to " << cCorrectOffset - cReTime << " clock cycles." << RESET;
        pBoard->setStubOffset(cCorrectOffset - cReTime);
        // verification step
        // print to screen for now
        bool cConfirm = false;
        if(cConfirm)
        {
            ReadNEvents(pBoard, cNevents);
            const std::vector<Event*>& cEvents = this->GetEvents();
            for(auto cEvent: cEvents)
            {
                for(auto cOpticalGroup: *pBoard)
                {
                    for(auto cHybrid: *cOpticalGroup)
                    {
                        for(auto cChip: *cHybrid)
                        {
                            if(cChip->getFrontEndType() == FrontEndType::SSA2) continue;

                            auto cStubs = static_cast<D19cCic2Event*>(cEvent)->StubVector(cHybrid->getId(), cChip->getId());
                            auto cHits  = cEvent->GetHits(cHybrid->getId(), cChip->getId());
                            if((int)(cHits.size()) > 0 && (int)cStubs.size() > 0)
                            {
                                LOG(INFO) << BOLDGREEN << "Event#" << +cEvent->GetEventCount() << " ... found " << cHits.size() << " hits in FE#" << +cChip->getId() << " and " << +cStubs.size()
                                          << " stubs." << RESET;
                            }
                        } // Chips
                    } // hybrids
                } // OGs
            } // events
        }
    }

    // reconfigure sparsification
    // this->enableTestPulse(false);
    LOG(INFO) << BOLDMAGENTA << "BackEndAlignment::FindStubLatency Resetting Sparsification" << RESET;
    fBeBoardInterface->WriteBoardReg(pBoard, "fc7_daq_cnfg.physical_interface_block.cic.2s_sparsified_enable", (int)cSparsified);
    for(auto cOpticalGroup: *pBoard)
    {
        for(auto cHybrid: *cOpticalGroup)
        {
            auto& cCic = static_cast<OuterTrackerHybrid*>(cHybrid)->fCic;
            fCicInterface->SetSparsification(cCic, cSparsified);
        } // hybrids
    } // OG
    // set everything back to original values .. like I wasn't here
    // reset fast command registers
    LOG(INFO) << BOLDMAGENTA << "BackEndAlignment::FindStubLatency Resetting BeBoards regs back to their original values" << RESET;
    for(size_t cIndx = 0; cIndx < cFcmdRegs.size(); cIndx++)
    {
        std::string cRegName = "fc7_daq_cnfg.fast_command_block." + cFcmdRegs[cIndx];
        cRegVec.push_back({cRegName, cFcmdRegOrigVals[cIndx]});
    }
    cRegVec.push_back({"fc7_daq_ctrl.fast_command_block.control.load_config", 0x1});
    fBeBoardInterface->WriteBoardMultReg(pBoard, cRegVec);
    fBeBoardInterface->WriteBoardReg(pBoard, "fc7_daq_cnfg.tlu_block.tlu_enabled", cOriginalTLUconfig);
    return cFoundCorrectStubLatency;
}
bool StubBackEndAlignment::FindPackageDelay()
{
    bool cAligned = true;
    for(auto cBoard: *fDetectorContainer)
    {
        // only find package delay if there is a CIC
        if(fWithCIC)
        {
            cAligned = cAligned && FindPackageDelay(cBoard);
            if(!cAligned) continue;
        }
    }
    return cAligned;
}
bool StubBackEndAlignment::FindStubLatency()
{
    bool cAligned = true;
    for(auto cBoard: *fDetectorContainer)
    {
        // only find package delay if there is a CIC
        if(fWithCIC)
        {
            cAligned = cAligned && FindStubLatency(cBoard);
            if(!cAligned) continue;
        }
    }
    return cAligned;
}

bool StubBackEndAlignment::Align()
{
    LOG(INFO) << BOLDBLUE << "Starting back-end alignment procedure .... " << RESET;
    bool cAligned = FindPackageDelay();
    if(!cAligned) return cAligned;

    cAligned = FindStubLatency();
    if(!cAligned) return cAligned;

    return cAligned;
}
// State machine control functions
void StubBackEndAlignment::Running()
{
    Initialise();
    fSuccess = FindStubLatency();
    if(!fSuccess)
    {
        LOG(ERROR) << BOLDRED << "Failed to align stubs in the back-end" << RESET;
        Destroy();
        exit(FAILED_STUBBACKEND_ALIGNMENT);
    }
    Reset();
}

void StubBackEndAlignment::Stop() {}

void StubBackEndAlignment::Pause() {}

void StubBackEndAlignment::Resume() {}
