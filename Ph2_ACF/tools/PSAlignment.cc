#include "tools/PSAlignment.h"

#include "HWInterface/D19cFWInterface.h"
#include "HWInterface/L1ReadoutInterface.h"
#include "Utils/CBCChannelGroupHandler.h"
#include "Utils/ContainerFactory.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

PSAlignment::PSAlignment() : OTTool() {}

PSAlignment::~PSAlignment() {}

void PSAlignment::Initialise()
{
    // prepare common OTTool
    Prepare();
    SetName("PSAlignment");

    // list of chip registers that can be modified by this tool
    std::vector<std::string> cRegsMod; //{"LatencyRx320", "LatencyRx40", "RetimePix", "EdgeSelTrig", "EdgeSelT1Raw"};
    for(size_t cIndx = 0; cIndx <= 5; cIndx++)
    {
        std::stringstream cRegName;
        cRegName << "OutSetting_" << +cIndx;
        cRegsMod.push_back(cRegName.str());
    }

    cRegsMod.push_back("ReadoutMode");
    cRegsMod.push_back("Control_1");
    cRegsMod.push_back("LatencyRx320");
    cRegsMod.push_back("LatencyRx40");
    cRegsMod.push_back("MemoryControl_1_ALL");
    cRegsMod.push_back("MemoryControl_2_ALL");
    cRegsMod.push_back("EdgeSelT1Raw");
    cRegsMod.push_back("EdgeSelTrig");
    SetChipRegstoPerserve(FrontEndType::MPA2, cRegsMod);
    cRegsMod.clear();
    cRegsMod.push_back("ReadoutMode");
    cRegsMod.push_back("control_1");
    cRegsMod.push_back("control_3");
    SetChipRegstoPerserve(FrontEndType::SSA2, cRegsMod);

    // data containers to hold alignment parameters
    ContainerFactory::copyAndInitChip<std::vector<MPAInputAlignment>>(*fDetectorContainer, fAlParsContainer);
    ContainerFactory::copyAndInitChip<std::vector<MPAInputAlignment>>(*fDetectorContainer, fL1AlParsContainer);
    ContainerFactory::copyAndInitChip<std::vector<MPAInputAlignment>>(*fDetectorContainer, fStubAlParsContainer);

    for(auto cBoard: *fDetectorContainer)
    {
        auto& cAlParsThisBoard     = fAlParsContainer.getObject(cBoard->getId());
        auto& cL1AlParsThisBoard   = fL1AlParsContainer.getObject(cBoard->getId());
        auto& cStubAlParsThisBoard = fStubAlParsContainer.getObject(cBoard->getId());
        for(auto cOpticalGroup: *cBoard)
        {
            auto& cAlParsThisOG     = cAlParsThisBoard->getObject(cOpticalGroup->getId());
            auto& cL1AlParsThisOG   = cL1AlParsThisBoard->getObject(cOpticalGroup->getId());
            auto& cStubAlParsThisOG = cStubAlParsThisBoard->getObject(cOpticalGroup->getId());
            for(auto cHybrid: *cOpticalGroup)
            {
                auto& cAlParsThisHybrd     = cAlParsThisOG->getObject(cHybrid->getId());
                auto& cL1AlParsThisHybrd   = cL1AlParsThisOG->getObject(cHybrid->getId());
                auto& cStubAlParsThisHybrd = cStubAlParsThisOG->getObject(cHybrid->getId());
                for(auto cChip: *cHybrid)
                {
                    if(cChip->getFrontEndType() == FrontEndType::MPA2)
                    {
                        PSv2                  = true;
                        auto& cAlParsThisChip = cAlParsThisHybrd->getObject(cChip->getId());
                        cAlParsThisChip->getSummary<std::vector<MPAInputAlignment>>().clear();
                        auto& cL1AlParsThisChip = cL1AlParsThisHybrd->getObject(cChip->getId());
                        cL1AlParsThisChip->getSummary<std::vector<MPAInputAlignment>>().clear();
                        auto& cStubAlParsThisChip = cStubAlParsThisHybrd->getObject(cChip->getId());
                        cStubAlParsThisChip->getSummary<std::vector<MPAInputAlignment>>().clear();
                    }
                }
            }
        }
    }

    // make sure ReadoutMode is set correctly
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    if(cChip->getFrontEndType() == FrontEndType::CBC3) continue;

                    uint16_t cReadoutMode = 0x0;
                    if(cChip->getFrontEndType() == FrontEndType::MPA2) // Something must be fixed somewhere! If not done like this RetimePix is overwritten and stubs lost!
                    {
                        ChipRegMask cMask;
                        cMask.fNbits    = 2;
                        cMask.fBitShift = 0;
                        cChip->setRegBits("Control_1", cMask, cReadoutMode);
                        cReadoutMode = cChip->getReg("Control_1");
                    }
                    fReadoutChipInterface->WriteChipReg(cChip, "ReadoutMode", cReadoutMode);
                }
            }
        }
    }
}
void PSAlignment::MapMPAOutputs(std::string pSetupType)
{
    LOG(INFO) << BOLDBLUE << "PSAlignment: mapping MPA outputs " << RESET;
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalReadout: *cBoard)
        {
            for(auto cHybrid: *cOpticalReadout)
            {
                // map MPA outputs
                for(auto cChip: *cHybrid) // for each chip (makes sense)
                {
                    if(cChip->getFrontEndType() != FrontEndType::MPA2) continue;

                    // mapping for PS module
                    // mapping for probe station/etc. can be different
                    if(pSetupType.find("PSModule") != std::string::npos)
                    {
                        std::vector<int> cMappedTo{1, 2, 3, 4, 5, 0};
                        for(size_t cIndx = 0; cIndx < cMappedTo.size(); cIndx++)
                        {
                            LOG(DEBUG) << BOLDBLUE << "Configuring MPA output register [mapping between output bits and output pads] .... Output# " << +cIndx << RESET;
                            std::ostringstream cRegName;
                            cRegName << "OutSetting_" << cIndx;
                            fReadoutChipInterface->WriteChipReg(cChip, cRegName.str(), cMappedTo[cIndx]);
                        }
                    }
                } // chip
            } // hybrid
        } // optical group
    }
}
void PSAlignment::ConfigureDefaultAlignmentParameters(std::string pSetupType)
{
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalReadout: *cBoard)
        {
            for(auto cHybrid: *cOpticalReadout)
            {
                // map MPA outputs
                for(auto cChip: *cHybrid) // for each chip (makes sense)
                {
                    if(cChip->getFrontEndType() != FrontEndType::MPA2) continue;

                    // mapping for PS module
                    // mapping for probe station/etc. can be different
                    if(pSetupType.find("PSModule") != std::string::npos)
                    {
                        // These are now set in the XML because they are different for differen PS versions!!!
                        // fReadoutChipInterface->WriteChipReg(cChip, "RetimePix", 0x4);
                        // fReadoutChipInterface->WriteChipReg(cChip, "LatencyRx320", 0x36); // different for PSv2 /2.1
                        // fReadoutChipInterface->WriteChipReg(cChip, "LatencyRx40", 0x02);
                        // fReadoutChipInterface->WriteChipReg(cChip, "EdgeSelTrig", 0x00);
                        // fReadoutChipInterface->WriteChipReg(cChip, "EdgeSelT1Raw", 0x00); // different for PSv2 /2.1
                    }
                } // chip
            } // hybrid
        } // optical group
    }

    // make sure that we save the values at the end

    std::vector<std::string> cRegsMod{"LatencyRx320", "LatencyRx40", "RetimePix", "EdgeSelTrig", "EdgeSelT1Raw", "Control_1"};
    for(size_t cIndx = 0; cIndx <= 5; cIndx++)
    {
        std::stringstream cRegName;
        cRegName << "OutSetting_" << +cIndx;
        cRegsMod.push_back(cRegName.str());
    }
    cRegsMod.push_back("ReadoutMode");
    SetChipRegstoPerserve(FrontEndType::MPA2, cRegsMod);
    cRegsMod.clear();
    cRegsMod.push_back("ReadoutMode");
    SetChipRegstoPerserve(FrontEndType::SSA2, cRegsMod);
}

bool PSAlignment::AlignStubInputs(BeBoard* pBoard)
{
    bool     cPhaseFound = true;
    uint32_t cNevents    = 10;
    LOG(INFO) << BOLDBLUE << "Aligning MPA stub inputs.." << RESET;
    uint8_t cRow        = 9; // random pixel to activate -- could be configurable
    uint8_t cCol        = 45;
    auto    cStubOffset = pBoard->getStubOffset();
    // check trigger source
    // and reload
    uint16_t cTriggerSrc = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_cnfg.fast_command_block.trigger_source");
    LOG(DEBUG) << BOLDBLUE << "Trigger source is set to " << +cTriggerSrc << RESET;
    cTriggerSrc = (cTriggerSrc == 6) ? cTriggerSrc : 6;
    std::vector<std::pair<std::string, uint32_t>> cRegVec;
    cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.trigger_source", cTriggerSrc});
    cRegVec.push_back({"fc7_daq_ctrl.fast_command_block.control.load_config", 0x1});
    fBeBoardInterface->WriteBoardMultReg(pBoard, cRegVec);
    uint16_t cDelay   = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_cnfg.fast_command_block.test_pulse.delay_after_test_pulse");
    uint16_t cLatency = cDelay - 1;
    if(PSv2) cLatency -= 2;
    LOG(INFO) << BOLDMAGENTA << "Expect correct L1 latency to be " << +cLatency << RESET;

    // configure SSA to inject digitally
    for(auto cOpticalReadout: *pBoard)
    {
        for(auto cHybrid: *cOpticalReadout)
        {
            // configure SSA to inject digitally
            for(auto cChip: *cHybrid) // for each chip (makes sense)
            {
                if(cChip->getFrontEndType() == FrontEndType::MPA2)
                {
                    fReadoutChipInterface->WriteChipReg(cChip, "ENFLAGS_ALL", 0x0);
                    fReadoutChipInterface->WriteChipReg(cChip, "TriggerLatency", cLatency);
                    fReadoutChipInterface->WriteChipReg(cChip, "Threshold", 150);
                    fReadoutChipInterface->WriteChipReg(cChip, "ENFLAGS_C" + std::to_string(cCol) + "_R" + std::to_string(cRow), 0x37);
                    fReadoutChipInterface->WriteChipReg(cChip, "DigitalSync_C" + std::to_string(cCol) + "_R" + std::to_string(cRow), 0x01); // enable 1 pix
                    fReadoutChipInterface->WriteChipReg(cChip, "EdgeSelTrig", 0x0);
                }
                if(cChip->getFrontEndType() == FrontEndType::SSA2)
                {
                    fReadoutChipInterface->WriteChipReg(cChip, "TriggerLatency", cLatency + 1);
                    fReadoutChipInterface->WriteChipReg(cChip, "ENFLAGS_ALL", 0x0);
                    fReadoutChipInterface->WriteChipReg(cChip, "Threshold", 150);
                    fReadoutChipInterface->WriteChipReg(cChip, "DigCalibPattern_L_ALL", 0x01);
                    fReadoutChipInterface->WriteChipReg(cChip, "DigCalibPattern_H_ALL", 0x01);
                    fReadoutChipInterface->WriteChipReg(cChip, "DigitalSync_S" + std::to_string(cCol), 0x1);
                }
            } // chip
        } // hybrid
    } // optica]l group

    for(auto cOpticalReadout: *pBoard)
    {
        for(auto cHybrid: *cOpticalReadout)
        {
            for(auto cChip: *cHybrid)
            {
                if(cChip->getFrontEndType() != FrontEndType::MPA2) continue;

                // uint8_t  cMode = 3;
                uint8_t cStubWindow = 8;
                // static_cast<PSInterface*>(fReadoutChipInterface)->Activate_ps(cChip, 2);
                // text parsed reimplementation
                fReadoutChipInterface->WriteChipReg(cChip, "StubMode", 0);
                fReadoutChipInterface->WriteChipReg(cChip, "StubWindow", cStubWindow);
            } // chip
        } // hybrid
    } // optica]l group

    // scan phase and check stubs
    // for each chip on a hybrid

    bool cCurPhaseFound = true;
    // not sure why cChipId loop, why not just all on hybrid
    for(auto cOpticalReadout: *pBoard)
    {
        for(auto cHybrid: *cOpticalReadout)
        {
            // configure SSA to inject digitally
            for(auto cChip: *cHybrid) // for each chip (makes sense)
            {
                if(cChip->getFrontEndType() != FrontEndType::MPA2) continue;

                uint8_t cChipId = cChip->getId();
                if(!cCurPhaseFound) cPhaseFound = false; // all chips need to be tuned
                cCurPhaseFound = false;
                for(uint8_t cPhase = 0; cPhase < 8; cPhase++)
                {
                    if(cCurPhaseFound == true) break; // break loops if chip phase already found
                    for(uint8_t cRetime = 0; cRetime < 8; cRetime++)
                    {
                        if(cCurPhaseFound == true) break;
                        for(auto cOpticalReadout: *pBoard)
                        {
                            for(auto cHybrid: *cOpticalReadout)
                            {
                                // configure SSA to inject digitally
                                for(auto cChip: *cHybrid) // for each chip (makes sense)
                                {
                                    if(cChip->getFrontEndType() != FrontEndType::MPA2) continue;
                                    if(cChip->getId() != cChipId) continue;
                                    fReadoutChipInterface->WriteChipReg(cChip, "StubInputPhase", cPhase);
                                    fReadoutChipInterface->WriteChipReg(cChip, "RetimePix", cRetime);
                                } // chip
                            } // hybrid
                        } // optical group
                        std::cout << "Writing common_stubdata_delay " << +cLatency << "," << +cStubOffset << "," << +cRetime << std::endl;

                        size_t writeslat = cLatency - (cStubOffset + cRetime); // stub latency
                        std::cout << "Writing common_stubdata_delay " << writeslat << std::endl;
                        fBeBoardInterface->WriteBoardReg(pBoard, "fc7_daq_cnfg.readout_block.global.common_stubdata_delay", writeslat);

                        // now read data
                        LOG(INFO) << BOLDBLUE << "Setting stub input sampling phase, and pixel retime for MPA#" << +cChipId << " on hybrid to " << +cPhase << " and " << +cRetime << RESET;
                        ReadNEvents(pBoard, cNevents);
                        const std::vector<Event*>& cEvents = this->GetEvents();
                        LOG(INFO) << BOLDBLUE << "Checking phase by reading back " << +cEvents.size() << " events from the FC7 ..." << RESET;
                        uint32_t MatchNStubtot = 0;
                        for(auto& ev: cEvents)
                        {
                            for(auto cOpticalReadout: *pBoard)
                            {
                                for(auto cHybrid: *cOpticalReadout)
                                {
                                    for(auto cChip: *cHybrid)
                                    {
                                        if(cChip->getFrontEndType() != FrontEndType::MPA2) continue;
                                        if(cChip->getId() != cChipId) continue;
                                        auto stubs = static_cast<D19cCic2Event*>(ev)->StubVector(cHybrid->getId(), cChip->getId());
                                        for(auto& st: stubs)
                                        {
                                            if((2 * cCol) == st.getPosition() and (cRow - 1) == st.getRow()) MatchNStubtot += 1; // Match row and column
                                            std::cout << "getPosition " << +st.getPosition() << std::endl;
                                            std::cout << "getBend " << +st.getBend() << std::endl;
                                            std::cout << "getRow " << +st.getRow() << std::endl;
                                        }
                                    }
                                }
                            }
                        }
                        std::cout << "MatchNStubtot " << MatchNStubtot << std::endl;
                        if(MatchNStubtot > (cNevents - 1))
                        {
                            LOG(INFO) << BOLDGREEN << "-----" << RESET;
                            LOG(INFO) << BOLDGREEN << "Stub input sampling phase and pixel retime for MPA#" << +cChipId << " completed." << RESET;
                            LOG(INFO) << BOLDGREEN << "Phase:" << +cPhase << " Retime:" << +cRetime << RESET;
                            LOG(INFO) << BOLDGREEN << "-----" << RESET;
                            cCurPhaseFound = true;
                        }

                    } // retime loop
                } // phase loop
            }
        }
    }

    return cPhaseFound;
}

bool PSAlignment::AlignL1Inputs(BeBoard* pBoard)
{
    bool cPhaseFound = true;
    LOG(INFO) << BOLDBLUE << "Aligning MPA L1 inputs.." << RESET;
    uint32_t cNevents = 10;

    // check trigger source
    // and reload
    uint8_t  cRow        = 9; // random pixel to activate -- could be configurable
    uint8_t  cCol        = 45;
    uint16_t cTriggerSrc = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_cnfg.fast_command_block.trigger_source");
    LOG(DEBUG) << BOLDBLUE << "Trigger source is set to " << +cTriggerSrc << RESET;
    cTriggerSrc = (cTriggerSrc == 6) ? cTriggerSrc : 6;
    std::vector<std::pair<std::string, uint32_t>> cRegVec;
    cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.trigger_source", cTriggerSrc});
    cRegVec.push_back({"fc7_daq_ctrl.fast_command_block.control.load_config", 0x1});
    fBeBoardInterface->WriteBoardMultReg(pBoard, cRegVec);

    uint16_t cDelay   = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_cnfg.fast_command_block.test_pulse.delay_after_test_pulse");
    uint16_t cLatency = cDelay - 1; // for MPA2! to genneralize if function used
    if(PSv2) cLatency -= 2;
    LOG(INFO) << BOLDMAGENTA << "Expect correct L1 latency to be " << +cLatency << RESET;

    for(auto cOpticalReadout: *pBoard)
    {
        for(auto cHybrid: *cOpticalReadout)
        {
            for(auto cChip: *cHybrid)
            {
                // make sure L1 latency is configured
                if(cChip->getFrontEndType() == FrontEndType::MPA2)
                {
                    fReadoutChipInterface->WriteChipReg(cChip, "InjectedCharge", 0x0);
                    fReadoutChipInterface->WriteChipReg(cChip, "ENFLAGS_ALL", 0x0);
                    fReadoutChipInterface->WriteChipReg(cChip, "TriggerLatency", cLatency);
                    fReadoutChipInterface->WriteChipReg(cChip, "ENFLAGS_C" + std::to_string(cCol) + "_R" + std::to_string(cRow), 0x37);
                    fReadoutChipInterface->WriteChipReg(cChip, "DigitalSync_C" + std::to_string(cCol) + "_R" + std::to_string(cRow), 0x01); // enable 1 pix
                    fReadoutChipInterface->WriteChipReg(cChip, "DigPattern_ALL", 0xFF);
                }

                if(cChip->getFrontEndType() == FrontEndType::SSA2)
                {
                    fReadoutChipInterface->WriteChipReg(cChip, "TriggerLatency", cLatency + 1);
                    fReadoutChipInterface->WriteChipReg(cChip, "DigCalibPattern_L_ALL", 0x01);
                    fReadoutChipInterface->WriteChipReg(cChip, "DigCalibPattern_H_ALL", 0x01);
                    fReadoutChipInterface->WriteChipReg(cChip, "ENFLAGS_ALL", 0x0);
                    fReadoutChipInterface->WriteChipReg(cChip, "ENFLAGS_S" + std::to_string(cCol), 0x9);
                    fReadoutChipInterface->WriteChipReg(cChip, "Threshold", 150);
                }

            } // chip
        } // hybrid
    } // optica]l group

    // scan phase and check L1
    bool cCurPhaseFound = true;
    for(auto cOpticalReadout: *pBoard)
    {
        for(auto cHybrid: *cOpticalReadout)
        {
            // configure SSA to inject digitally
            for(auto cChip: *cHybrid) // for each chip (makes sense)
            {
                if(cChip->getFrontEndType() != FrontEndType::MPA2) continue;

                uint8_t cChipId = cChip->getId();

                if(!cCurPhaseFound) cPhaseFound = false;
                cCurPhaseFound = false;
                for(uint8_t cPhase = 3; cPhase < 8; cPhase++)
                {
                    if(cCurPhaseFound == true) break;
                    for(uint8_t cES = 3; cES < 4; cES++)
                    {
                        if(cCurPhaseFound == true) break;
                        for(uint8_t cWord = 0; cWord < 5; cWord++)
                        {
                            if(cCurPhaseFound == true) break;
                            for(auto cOpticalReadout: *pBoard)
                            {
                                for(auto cHybrid: *cOpticalReadout)
                                {
                                    for(auto cChip: *cHybrid)
                                    {
                                        if(cChip->getFrontEndType() != FrontEndType::MPA2) continue;
                                        if(cChip->getId() != cChipId) continue;
                                        fReadoutChipInterface->WriteChipReg(cChip, "L1InputPhase", cPhase);
                                        fReadoutChipInterface->WriteChipReg(cChip, "LatencyRx40", cWord);
                                        fReadoutChipInterface->WriteChipReg(cChip, "EdgeSelT1Raw", cES);
                                    } // chip
                                } // hybrid
                            } // optical group

                            // now read data
                            LOG(INFO) << BOLDBLUE << "Setting L1 input sampling phase and word for MPA#" << +cChipId << " on hybrid to " << +cPhase << " and " << +cWord << RESET;
                            ReadNEvents(pBoard, cNevents);
                            const std::vector<Event*>& cEvents = this->GetEvents();
                            LOG(INFO) << BOLDBLUE << "Checking phase by reading back " << +cEvents.size() << " events from the FC7 ..." << RESET;
                            uint32_t MatchNPclustot = 0;
                            uint32_t MatchNSclustot = 0;
                            for(auto& ev: cEvents)
                            {
                                for(auto cOpticalReadout: *pBoard)
                                {
                                    for(auto cHybrid: *cOpticalReadout)
                                    {
                                        for(auto cChip: *cHybrid)
                                        {
                                            if(cChip->getFrontEndType() != FrontEndType::MPA2) continue;
                                            if(cChip->getId() != cChipId) continue;

                                            auto Pclus = static_cast<D19cCic2Event*>(ev)->GetPixelClusters(cHybrid->getId(), cChip->getId());
                                            auto Sclus = static_cast<D19cCic2Event*>(ev)->GetStripClusters(cHybrid->getId(), cChip->getId());

                                            for(auto& pc: Pclus)
                                            {
                                                // std::cout << "-------------------------------PIXELS-------------------------------"<< std::endl;
                                                // std::cout << "fAddress "<<+pc.fAddress<<std::endl;
                                                // std::cout << "fWidth "<<+pc.fWidth<< std::endl;
                                                // std::cout << "fZpos "<<+pc.fZpos << std::endl;
                                                if((cCol) == pc.fAddress and (cRow - 1) == pc.fZpos)
                                                {
                                                    // std::cout << "PIXPASS"<< std::endl;

                                                    MatchNPclustot += 1;
                                                }
                                            }
                                            for(auto& sc: Sclus)
                                            {
                                                // std::cout << "-------------------------------STRIPS-------------------------------"<< std::endl;
                                                // std::cout << "fAddress? "<<+sc.fAddress<<std::endl;
                                                // std::cout << "fWidth "<<+sc.fWidth<< std::endl;
                                                // std::cout << "fMip "<<+sc.fMip << std::endl<< std::endl;
                                                if((cCol) == sc.fAddress and sc.fMip == 1)
                                                {
                                                    // std::cout << "STRIPPASS"<< std::endl;
                                                    MatchNSclustot += 1;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            std::cout << "MatchNPclustot " << MatchNPclustot << " MatchNSclustot " << MatchNSclustot << std::endl;
                            if((MatchNPclustot > (cNevents - 2)) and (MatchNSclustot > (cNevents - 2)))
                            {
                                LOG(INFO) << BOLDGREEN << "-----" << RESET;
                                LOG(INFO) << BOLDGREEN << "Phase and Word alignment for MPA#" << +cChipId << " completed." << RESET;
                                LOG(INFO) << BOLDGREEN << "Phase:" << +cPhase << " Word:" << +cWord << " Edge Sel:" << +cES << RESET;
                                LOG(INFO) << BOLDGREEN << "-----" << RESET;
                                cCurPhaseFound = true;
                            }
                        } // word loop
                    } // ES loop
                } // phase loop
            } // chip id loop [up-to 8 chips per hybrid ]
        }
    }
    return cPhaseFound;
}
std::vector<std::pair<uint8_t, uint8_t>> PSAlignment::AlignL1(ReadoutChip* pChip, std::vector<Injection> pInjections, uint8_t pEdgeSelRaw)
{
    struct
    {
        bool operator()(PixelClusterPS a, PixelClusterPS b) const { return a.fAddress < b.fAddress; }
    } customSortPclus;
    struct
    {
        bool operator()(StripClusterPS a, StripClusterPS b) const { return a.fAddress < b.fAddress; }
    } customSortSclus;

    // setting edge select for raw input
    uint8_t           cLineId = 8;
    std::stringstream cRegName;
    cRegName << "SelectEdgeL" << +cLineId;
    fReadoutChipInterface->WriteChipReg(pChip, cRegName.str(), pEdgeSelRaw);

    std::vector<std::pair<uint8_t, uint8_t>> cGoodCombinations;
    cGoodCombinations.clear();
    LOG(DEBUG) << BOLDMAGENTA << "PSAlignment::AlignL1  - aligning L1 and stub data for SSA-MPA pair#" << +pChip->getId() % 8 << RESET;
    auto     cBoardId     = pChip->getBeBoardId();
    auto     cBoardIter   = std::find_if(fDetectorContainer->begin(), fDetectorContainer->end(), [&cBoardId](Ph2_HwDescription::BeBoard* x) { return x->getId() == cBoardId; });
    auto     cTriggerMult = fBeBoardInterface->ReadBoardReg(*cBoardIter, "fc7_daq_cnfg.fast_command_block.misc.trigger_multiplicity");
    uint32_t cNevents     = 10;

    uint8_t cStartPhaseL1 = 3; // to-do - set from xml
    uint8_t cStopPhaseL1  = 8; // to-do - set from xml
    bool    cOnlyFirst    = false;
    for(uint8_t cPhase = cStartPhaseL1; cPhase < cStopPhaseL1; cPhase++)
    {
        if(cOnlyFirst && cGoodCombinations.size() > 0) continue; // for now .. only the first one
        for(uint8_t cWord = 0; cWord < 8; cWord++)               // goes to 16... for now scan to 8
        {
            if(cOnlyFirst && cGoodCombinations.size() > 0) continue; // for now .. only the first one
            fReadoutChipInterface->WriteChipReg(pChip, "L1InputPhase", cPhase);
            fReadoutChipInterface->WriteChipReg(pChip, "LatencyRx40", cWord);

            ReadNEvents(*cBoardIter, cNevents);
            const std::vector<Event*>& cEvents = this->GetEvents();
            if(cEvents.size() == 0) continue;
            float cMatchingCount = (cTriggerMult == 0) ? (cEvents.size() - 1) : cEvents.size() / (1 + cTriggerMult);

            LOG(DEBUG) << BOLDBLUE << "\tSetting L1 input sampling phase MPAs to " << +cPhase << " and Rx40 delay to " << +cWord << RESET;
            // start at the beginning + trigger id in burst
            for(size_t cTriggerId = 0; cTriggerId < cTriggerMult + 1; cTriggerId++)
            {
                size_t cMatchedEvents = 0;
                auto   cEventIter     = cEvents.begin() + cTriggerId;
                do {
                    if(cEventIter >= cEvents.end()) break;
                    bool cNmatch = true;
                    bool cFmatch = true;
                    auto cPclus  = static_cast<D19cCic2Event*>(*cEventIter)->GetPixelClusters(pChip->getHybridId(), pChip->getId());
                    auto cSclus  = static_cast<D19cCic2Event*>(*cEventIter)->GetStripClusters(pChip->getHybridId(), pChip->getId());
                    LOG(DEBUG) << BOLDMAGENTA << "\t\t found " << cPclus.size() << " P clusters and " << cSclus.size() << " S clusters" << RESET;
                    // sort P clusters by row
                    std::sort(cPclus.begin(), cPclus.end(), customSortPclus);
                    // sort S clusters by row
                    std::sort(cSclus.begin(), cSclus.end(), customSortSclus);
                    cNmatch = cNmatch && (cSclus.size() == (pInjections.size()) && cPclus.size() == pInjections.size());
                    cFmatch = cNmatch;
                    if(cNmatch)
                    {
                        std::vector<StripClusterPS> cMtchdSclstrs;
                        std::vector<PixelClusterPS> cMtchdPclstrs;
                        // Check P-clusters
                        for(auto cInjection: pInjections)
                        {
                            bool cMatchFound = false;
                            for(auto cPcluster: cPclus)
                            {
                                if(cMatchFound) continue;
                                cMatchFound = (cPcluster.fAddress == cInjection.fRow) && (cPcluster.fZpos == cInjection.fColumn);
                                if(cMatchFound)
                                {
                                    PixelClusterPS cMtchdPclstr;
                                    cMtchdPclstr.fAddress = cPcluster.fAddress;
                                    cMtchdPclstr.fZpos    = cPcluster.fZpos;
                                    cMtchdPclstrs.push_back(cMtchdPclstr);
                                }
                            }
                            cFmatch = cFmatch && cMatchFound;
                        }

                        // check S-clusters
                        for(auto cInjection: pInjections)
                        {
                            bool cMatchFound = false;
                            for(auto cScluster: cSclus)
                            {
                                if(cMatchFound) continue;
                                cMatchFound = (cScluster.fAddress == cInjection.fRow);
                                if(cMatchFound)
                                {
                                    StripClusterPS cMtchdSclstr;
                                    cMtchdSclstr.fAddress = cInjection.fRow;
                                    cMtchdSclstrs.push_back(cMtchdSclstr);
                                }
                            }
                            cFmatch = cFmatch && cMatchFound;
                        }

                        if(cFmatch)
                        {
                            LOG(INFO) << BOLDGREEN << "\t\t Trigger#" << +cTriggerId << " Event#" << (*cEventIter)->GetEventCount() << " in a burst of " << (1 + cTriggerMult) << " MPA"
                                      << +pChip->getId() << " found " << cSclus.size() << " matched S clusters and " << cPclus.size() << " matched P clusters in L1 data.  L1 input sampling phase is  "
                                      << +cPhase << " Rx40 delay is " << +cWord << RESET;
                            for(uint8_t cMatchId = 0; cMatchId < cMtchdSclstrs.size(); cMatchId++)
                            {
                                LOG(INFO) << BOLDGREEN << "\t\t\t Exact match found " << BOLDYELLOW << " S-cluster in row " << +cMtchdSclstrs[cMatchId].fAddress << " P-cluster in row "
                                          << +cMtchdPclstrs[cMatchId].fAddress << " column " << +cMtchdPclstrs[cMatchId].fZpos << RESET;
                            }
                        }
                    }
                    // if( cNmatch && cFmatch )
                    //     LOG (INFO) << BOLDGREEN << "Event#" << (*cEventIter)->GetEventCount() << " Trigger#" << +cTriggerId
                    //         << " in a burst of " << (1+ cTriggerMult)
                    //         << " number of S-clusters match is " << +cNmatch
                    //         << " full S-cluster match is " << +cFmatch
                    //         << RESET;
                    cMatchedEvents += (cFmatch && cNmatch) ? 1 : 0;
                    cEventIter += (1 + cTriggerMult);
                } while(cEventIter < cEvents.end());
                if(cMatchedEvents >= cMatchingCount) // to allow for single triggers
                {
                    std::pair<uint8_t, uint8_t> cComb;
                    cComb.first  = cPhase;
                    cComb.second = cWord;
                    cGoodCombinations.push_back(cComb);
                    LOG(INFO) << BOLDGREEN << "All events match for, LatencyRx320 of " << +cComb.first << " , LatencyRx40 [first]" << +(cComb.second & 0x3) << " LatencyRx40 [re-start] "
                              << +(cComb.second >> 2) << " full matching of S-clusters in MPA data" << RESET;
                }
            }
        }
    }
    return cGoodCombinations;
}
std::vector<std::pair<uint8_t, uint8_t>> PSAlignment::AlignStubs(ReadoutChip* pChip, std::vector<Injection> pInjections, uint16_t pLatency)
{
    struct
    {
        bool operator()(PixelClusterPS a, PixelClusterPS b) const { return a.fAddress < b.fAddress; }
    } customSortPclus;
    struct
    {
        bool operator()(StripClusterPS a, StripClusterPS b) const { return a.fAddress < b.fAddress; }
    } customSortSclus;
    // struct
    // {
    //     bool operator()(EventStub a, EventStub b) const { return a.fPosition < b.fPosition; }
    // } customSortStubs;

    std::vector<std::pair<uint8_t, uint8_t>> cGoodCombinationsStubs;
    cGoodCombinationsStubs.clear();
    LOG(DEBUG) << BOLDMAGENTA << "PSAlignment::AlignChip  - aligning L1 and stub data for SSA-MPA pair#" << +pChip->getId() % 8 << RESET;
    auto     cBoardId     = pChip->getBeBoardId();
    auto     cBoardIter   = std::find_if(fDetectorContainer->begin(), fDetectorContainer->end(), [&cBoardId](Ph2_HwDescription::BeBoard* x) { return x->getId() == cBoardId; });
    auto     cTriggerMult = fBeBoardInterface->ReadBoardReg(*cBoardIter, "fc7_daq_cnfg.fast_command_block.misc.trigger_multiplicity");
    uint32_t cNevents     = 10;

    // auto     cSetting                = fSettingsMap.find("MinStubPhase");
    uint32_t cStartPhase = 3;
    // cSetting                         = fSettingsMap.find("MaxStubPhase");
    uint32_t              cEndPhase  = 8;
    bool                  cOnlyFirst = true;
    bool                  cCheckL1   = true;
    std::vector<uint16_t> cStubOffsets(0);

    for(uint8_t cRetime = 8; cRetime >= 4; cRetime--) // to-do - add range to xml
    {
        if(cOnlyFirst && cGoodCombinationsStubs.size() > 0) continue;
        for(uint8_t cPhase = cStartPhase; cPhase < cEndPhase; cPhase++) // to-do - add range to xml
        {
            if(cOnlyFirst && cGoodCombinationsStubs.size() > 0) continue;
            bool cGoodStubDelayFound = false;
            for(int cStubAddDelay = -10; cStubAddDelay <= 10; cStubAddDelay++) // to-do - add range to xml
            {
                if(cGoodStubDelayFound) continue;
                auto   cStubOffset = (*cBoardIter)->getStubOffset() + cStubAddDelay;
                size_t cStubDelay  = pLatency - cStubOffset - cRetime; // stub latency
                fReadoutChipInterface->WriteChipReg(pChip, "StubInputPhase", cPhase);
                fReadoutChipInterface->WriteChipReg(pChip, "RetimePix", cRetime);
                LOG(INFO) << BOLDMAGENTA << "StubInputPhase is " << +cPhase << " ReTime pix is " << +cRetime << " [ Stub offset is " << cStubOffset << " Stub Latency is " << cStubDelay << " ]"
                          << RESET;
                fBeBoardInterface->WriteBoardReg((*cBoardIter), "fc7_daq_cnfg.readout_block.global.common_stubdata_delay", cStubDelay);
                ReadNEvents((*cBoardIter), cNevents);
                const std::vector<Event*>& cEvents = this->GetEvents();
                if(cEvents.size() == 0) continue;
                float cMatchingCount = (cTriggerMult == 0) ? (cEvents.size() - 1) : cEvents.size() / (float)(1 + cTriggerMult);

                LOG(DEBUG) << BOLDMAGENTA << "Writing common_stubdata_delay " << cStubDelay << " [ReTime pix is set to " << +cRetime << " ]\t\t LatencyRx320 for stubs of " << +cPhase << " ReTime of "
                           << +cRetime << RESET;
                for(size_t cTriggerId = 0; cTriggerId < (1 + cTriggerMult); cTriggerId++)
                {
                    size_t cMatchedEvents   = 0;
                    auto   cEventIter       = cEvents.begin() + cTriggerId;
                    size_t cMatchedEventsL1 = 0;
                    size_t cNchecked        = 0;
                    do {
                        if(cEventIter >= cEvents.end()) break;
                        auto cPclus = static_cast<D19cCic2Event*>(*cEventIter)->GetPixelClusters(pChip->getHybridId(), pChip->getId());
                        auto cSclus = static_cast<D19cCic2Event*>(*cEventIter)->GetStripClusters(pChip->getHybridId(), pChip->getId());
                        // sort P clusters by row
                        std::sort(cPclus.begin(), cPclus.end(), customSortPclus);
                        // sort S clusters by row
                        std::sort(cSclus.begin(), cSclus.end(), customSortSclus);
                        auto cStubs = static_cast<D19cCic2Event*>(*cEventIter)->StubVector(pChip->getHybridId(), pChip->getId());
                        cMatchedEventsL1 += (cPclus.size() == pInjections.size() && cSclus.size() == pInjections.size()) ? 1 : 0;
                        bool cNmatch = true;
                        if(cCheckL1)
                        {
                            cNmatch = (cPclus.size() == pInjections.size() && cSclus.size() == pInjections.size());
                            LOG(DEBUG) << BOLDGREEN << "Trigger#" << +cTriggerId << " in a burst of " << (1 + cTriggerMult) << " MPA" << +pChip->getId() << " found "
                                       << " correct numnber of S and P clusters." << RESET;
                        }
                        if(cStubs.size() == pInjections.size() && cNmatch)
                            LOG(DEBUG) << BOLDGREEN << "Trigger#" << +cTriggerId << " Event#" << (*cEventIter)->GetEventCount() << " in a burst of " << (1 + cTriggerMult) << " MPA" << +pChip->getId()
                                       << " found " << cSclus.size() << " S clusters and " << cPclus.size() << " P clusters in L1 data from MPA#" << +pChip->getId() << " also have " << +cStubs.size()
                                       << " stbs." << RESET;
                        // print stubs if they are there
                        if(cNmatch)
                        {
                            size_t cStubCntr = 0;
                            // sort stubs by position
                            // std::sort(cStubs.begin(), cStubs.end(), customSortStubs);
                            size_t cMatchedStubSeeds = 0;
                            for(auto cInjection: pInjections)
                            {
                                bool    cMatchedStub      = false;
                                uint8_t cMatchedStubIndex = 0;
                                for(auto cStub: cStubs)
                                {
                                    // LOG(INFO) << BOLDGREEN << "\t\tUNMATCHED Stub#" << " Position " << +cStub.getPosition() << " - Row "<< +cStub.getRow() << " - Bend " << +cStub.getBend() <<
                                    // RESET;
                                    if(cMatchedStub) continue;
                                    cMatchedStub = (2 * cInjection.fRow == cStub.getPosition() && cInjection.fColumn == cStub.getRow());
                                    if(!cMatchedStub) cMatchedStubIndex++;
                                    cStubCntr++;
                                }
                                if(cMatchedStub)
                                    LOG(INFO) << BOLDGREEN << "\t\tStub#" << +cMatchedStubIndex << " Position " << +cStubs[cMatchedStubIndex].getPosition() << " - Row "
                                              << +cStubs[cMatchedStubIndex].getRow() << " - Bend " << +cStubs[cMatchedStubIndex].getBend() << RESET;
                                // else
                                //     LOG(INFO) << BOLDRED << "\t\tStub#" << +cStubCntr << " Position " << +cStubs[cMatchedStubIndex].getPosition()
                                //         << " - Row " << +cStubs[cMatchedStubIndex].getRow()
                                //         << " - Bend " << +cStubs[cMatchedStubIndex].getBend()
                                //         << RESET;

                                cMatchedStubSeeds += (cMatchedStub) ? 1 : 0;
                            }
                            cNmatch = cNmatch && (cMatchedStubSeeds == std::min(pInjections.size(), size_t(5)));
                        }
                        // update counters
                        cEventIter += (1 + cTriggerMult);
                        cMatchedEvents += (cNmatch) ? 1 : 0;
                        cNchecked++;
                    } while(cEventIter < cEvents.end());
                    if(cMatchedEvents > 0)
                        LOG(INFO) << BOLDMAGENTA << cMatchedEvents << " out of " << cNevents << " matched for stubs " << cMatchedEventsL1 << " events matched for L1A data "
                                  << ". Found " << cEvents.size() << " events in the readout ]. In total " << cNchecked << " events were checked" << RESET;
                    if(cMatchedEvents >= 0.5 * cMatchingCount) // to allow for a trigger multiplicity of 1
                    {
                        cStubOffsets.push_back((pLatency - cRetime) - cStubDelay);
                        std::pair<uint8_t, uint8_t> cComb;
                        cComb.first  = cPhase;
                        cComb.second = cRetime;
                        cGoodCombinationsStubs.push_back(cComb);
                        LOG(DEBUG) << BOLDGREEN << "All events stubs match for, LatencyRx320 of " << +cComb.first << " , ReTimePix " << +cComb.second << " full matching of stubs in CIC data .."
                                   << " - stub offset is " << +((pLatency - cRetime) - cStubDelay) << RESET;
                        cGoodStubDelayFound = true;
                    }
                }
            }
        }
    }
    size_t cIndx = 0;
    for(auto cCombStbs: cGoodCombinationsStubs)
    {
        LOG(DEBUG) << BOLDMAGENTA << "\t.. Stub data : LatencyRx320 of " << +cCombStbs.first << " , ReTimePix " << +cCombStbs.second << " full matching of stub data in MPA" << RESET;
        fReadoutChipInterface->WriteChipReg(pChip, "StubInputPhase", cCombStbs.first);
        fReadoutChipInterface->WriteChipReg(pChip, "RetimePix", cCombStbs.second);
        (*cBoardIter)->setStubOffset(cStubOffsets[cIndx]);
        cIndx++;
    }

    return cGoodCombinationsStubs;
}
std::vector<std::pair<uint8_t, uint8_t>> PSAlignment::AlignChip(ReadoutChip* pChip, std::vector<Injection> pInjections, uint16_t pLatency)
{
    std::vector<std::pair<uint8_t, uint8_t>> cGoodCombinations;
    cGoodCombinations.clear();
    LOG(INFO) << BOLDMAGENTA << "PSAlignment::AlignChip  - aligning L1 and stub data for SSA-MPA pair#" << +pChip->getId() % 8 << RESET;
    auto     cBoardId     = pChip->getBeBoardId();
    auto     cBoardIter   = std::find_if(fDetectorContainer->begin(), fDetectorContainer->end(), [&cBoardId](Ph2_HwDescription::BeBoard* x) { return x->getId() == cBoardId; });
    auto     cTriggerMult = fBeBoardInterface->ReadBoardReg(*cBoardIter, "fc7_daq_cnfg.fast_command_block.misc.trigger_multiplicity");
    uint32_t cNevents     = 10;

    uint8_t cStartPhaseL1 = 2; // to-do : set from xml
    uint8_t cStopPhaseL1  = 5; // to-do : set from xml
    bool    cOnlyFirst    = true;
    for(uint8_t cPhase = cStartPhaseL1; cPhase < cStopPhaseL1; cPhase++)
    {
        if(cOnlyFirst && cGoodCombinations.size() > 0) continue; // for now .. only the first one
        for(uint8_t cWord = 0; cWord < 16; cWord++)
        {
            if(cOnlyFirst && cGoodCombinations.size() > 0) continue; // for now .. only the first one
            fReadoutChipInterface->WriteChipReg(pChip, "L1InputPhase", cPhase);
            fReadoutChipInterface->WriteChipReg(pChip, "LatencyRx40", cWord);

            ReadNEvents(*cBoardIter, cNevents);
            const std::vector<Event*>& cEvents = this->GetEvents();
            if(cEvents.size() == 0) continue;

            LOG(DEBUG) << BOLDBLUE << "Setting L1 input sampling phase MPAs to " << +cPhase << " and Rx40 delay to " << +cWord << " - going to read " << +cNevents << RESET;
            // start at the beginning + trigger id in burst
            for(size_t cTriggerId = 0; cTriggerId < cTriggerMult + 1; cTriggerId++)
            {
                size_t cMatchedEvents = 0;
                auto   cEventIter     = cEvents.begin() + cTriggerId;
                do {
                    if(cEventIter >= cEvents.end()) break;
                    bool cNmatch = true;
                    bool cFmatch = true;
                    auto cPclus  = static_cast<D19cCic2Event*>(*cEventIter)->GetPixelClusters(pChip->getHybridId(), pChip->getId());
                    auto cSclus  = static_cast<D19cCic2Event*>(*cEventIter)->GetStripClusters(pChip->getHybridId(), pChip->getId());
                    cNmatch      = cNmatch && (cSclus.size() == pInjections.size() && cPclus.size() == pInjections.size());
                    cFmatch      = cNmatch;
                    if(cNmatch)
                    {
                        LOG(DEBUG) << BOLDBLUE << "Trigger#" << +cTriggerId << " in a burst of " << (1 + cTriggerMult) << " MPA" << +pChip->getId() << " found " << cSclus.size() << " S clusters and "
                                   << cPclus.size() << " P clusters in L1 data from MPA#" << +pChip->getId() << RESET;
                        for(size_t cIndx = 0; cIndx < pInjections.size(); cIndx++)
                        {
                            cFmatch = cFmatch && (cPclus[cIndx].fAddress == cSclus[cIndx].fAddress);
                            if((cPclus[cIndx].fAddress == cSclus[cIndx].fAddress))
                                LOG(DEBUG) << BOLDGREEN << "Exact match found " << BOLDYELLOW << " P-cluster in row " << +cPclus[cIndx].fAddress << " column " << +cPclus[cIndx].fZpos << " width is "
                                           << +cPclus[cIndx].fWidth << BOLDCYAN << " S-cluster in row " << +cSclus[cIndx].fAddress << " column " << (0) << " width is " << +cSclus[cIndx].fWidth
                                           << RESET;
                            else
                                LOG(DEBUG) << BOLDRED << "Exact match not found " << BOLDYELLOW << " P-cluster in row " << +cPclus[cIndx].fAddress << " column " << +cPclus[cIndx].fZpos << " width is "
                                           << +cPclus[cIndx].fWidth << BOLDCYAN << " S-cluster in row " << +cSclus[cIndx].fAddress << " column " << (0) << " width is " << +cSclus[cIndx].fWidth
                                           << RESET;
                        }
                    }
                    // if( cNmatch && cFmatch )
                    //     LOG (INFO) << BOLDGREEN << "Event#" << (*cEventIter)->GetEventCount() << " Trigger#" << +cTriggerId
                    //         << " in a burst of " << (1+ cTriggerMult)
                    //         << " number of S-clusters match is " << +cNmatch
                    //         << " full S-cluster match is " << +cFmatch
                    //         << RESET;
                    cMatchedEvents += (cFmatch && cNmatch) ? 1 : 0;
                    cEventIter += (1 + cTriggerMult);
                } while(cEventIter < cEvents.end());
                if(cMatchedEvents == cNevents)
                {
                    std::pair<uint8_t, uint8_t> cComb;
                    cComb.first  = cPhase;
                    cComb.second = cWord;
                    cGoodCombinations.push_back(cComb);
                    LOG(DEBUG) << BOLDGREEN << "All events match for, LatencyRx320 of " << +cComb.first << " , LatencyRx40 [first]" << +(cComb.second & 0x3) << " LatencyRx40 [re-start] "
                               << +(cComb.second >> 2) << " full matching of S-clusters in MPA data" << RESET;
                }
            }
        }
    }

    size_t cL1CombIndx = 0;
    for(auto cComb: cGoodCombinations)
    {
        if(cL1CombIndx > 0 && cOnlyFirst) continue;
        // now run stub alignment procedure
        LOG(INFO) << BOLDYELLOW << "Scanning Stub alignment parameters for L1 alignmnet parameters.." << RESET;
        fReadoutChipInterface->WriteChipReg(pChip, "L1InputPhase", cComb.first);
        fReadoutChipInterface->WriteChipReg(pChip, "LatencyRx40", cComb.second);
        fReadoutChipInterface->WriteChipReg(pChip, "StubMode", 0);
        fReadoutChipInterface->WriteChipReg(pChip, "StubWindow", 1);

        // I think that the sampling phase or the stub line should be about the same
        // as the L1 line
        // if the lines between SSAs and MPAs on the hybrid are matched
        // and I think they are
        uint8_t                                  cStartPhase = (cComb.first == 0) ? cComb.first : cComb.first - 1;
        uint8_t                                  cEndPhase   = cComb.first + 2;
        std::vector<std::pair<uint8_t, uint8_t>> cGoodCombinationsStubs;
        cGoodCombinationsStubs.clear();
        int cGoodStubDelay = 0;
        for(int cStubAddDelay = 0; cStubAddDelay <= 5; cStubAddDelay++)
        {
            if(cGoodCombinationsStubs.size() > 0) continue;
            LOG(INFO) << BOLDMAGENTA << "Additional stub data delay of " << cStubAddDelay << RESET;
            for(uint8_t cPhase = cStartPhase; cPhase < cEndPhase; cPhase++)
            {
                if(cGoodCombinationsStubs.size() > 0) continue;
                for(uint8_t cRetime = 0; cRetime < 8; cRetime++)
                {
                    if(cGoodCombinationsStubs.size() > 0) continue;
                    auto   cStubOffset = (*cBoardIter)->getStubOffset() + cStubAddDelay;
                    size_t cStubDelay  = pLatency - cStubOffset - cRetime; // stub latency
                    fReadoutChipInterface->WriteChipReg(pChip, "RetimePix", cRetime);
                    fReadoutChipInterface->WriteChipReg(pChip, "StubInputPhase", cPhase);

                    // LOG (INFO) << BOLDYELLOW  << "Writing common_stubdata_delay " << cStubDelay << " [ReTime pix is set to " << +cRetime << " ]" << std::endl;
                    fBeBoardInterface->WriteBoardReg((*cBoardIter), "fc7_daq_cnfg.readout_block.global.common_stubdata_delay", cStubDelay);
                    ReadNEvents((*cBoardIter), cNevents);
                    const std::vector<Event*>& cEvents = this->GetEvents();

                    LOG(INFO) << BOLDMAGENTA << "LatencyRx320 for stubs of " << +cPhase << " ReTime of " << +cRetime << RESET;
                    for(size_t cTriggerId = 0; cTriggerId < (1 + cTriggerMult); cTriggerId++)
                    {
                        size_t cMatchedEvents = 0;
                        auto   cEventIter     = cEvents.begin() + cTriggerId;
                        do {
                            if(cEventIter >= cEvents.end()) break;
                            bool cNmatch = true;
                            auto cPclus  = static_cast<D19cCic2Event*>(*cEventIter)->GetPixelClusters(pChip->getHybridId(), pChip->getId());
                            auto cSclus  = static_cast<D19cCic2Event*>(*cEventIter)->GetStripClusters(pChip->getHybridId(), pChip->getId());
                            auto cStubs  = static_cast<D19cCic2Event*>(*cEventIter)->StubVector(pChip->getHybridId(), pChip->getId());
                            cNmatch      = cNmatch && (cStubs.size() == pInjections.size() && cPclus.size() == pInjections.size() && cSclus.size() == pInjections.size());
                            if(cStubs.size() != 0 && cNmatch)
                                LOG(INFO) << BOLDBLUE << "Trigger#" << +cTriggerId << " in a burst of " << (1 + cTriggerMult) << " MPA" << +pChip->getId() << " found " << cSclus.size()
                                          << " S clusters and " << cPclus.size() << " P clusters in L1 data from MPA#" << +pChip->getId() << " also have " << +cStubs.size() << " stbs." << RESET;
                            if(cStubs.size() == pInjections.size())
                            {
                                size_t cStubCntr = 0;
                                for(auto cStub: cStubs)
                                {
                                    LOG(INFO) << BOLDCYAN << "\t\tStub#" << +cStubCntr << " Position " << +cStub.getPosition() << " - Row " << +cStub.getRow() << " - Bend " << +cStub.getBend()
                                              << RESET;
                                    cStubCntr++;
                                }
                            }
                            cEventIter += (1 + cTriggerMult);
                            cMatchedEvents += (cNmatch) ? 1 : 0;
                        } while(cEventIter < cEvents.end());
                        if(cMatchedEvents == cNevents)
                        {
                            cGoodStubDelay = cStubAddDelay;
                            std::pair<uint8_t, uint8_t> cComb;
                            cComb.first  = cPhase;
                            cComb.second = cRetime;
                            cGoodCombinationsStubs.push_back(cComb);
                            LOG(INFO) << BOLDGREEN << "All events stubs match for, LatencyRx320 of " << +cComb.first << " , ReTimePix " << +cComb.second << " full matching of S-clusters in MPA data"
                                      << RESET;
                        }
                    }
                }
            }
        }
        (*cBoardIter)->setStubOffset((*cBoardIter)->getStubOffset() + cGoodStubDelay);

        LOG(INFO) << BOLDMAGENTA << "Summary of SSA-MPA data alignment" << RESET;
        LOG(INFO) << BOLDMAGENTA << "LatencyRx320 of " << +cComb.first << " , LatencyRx40 " << +cComb.second << " full matching of S-clusters in MPA data" << RESET;
        for(auto cCombStbs: cGoodCombinationsStubs)
        {
            LOG(DEBUG) << BOLDMAGENTA << "\t.. Stub data : LatencyRx320 of " << +cCombStbs.first << " , ReTimePix " << +cCombStbs.second << " full matching of stub data in MPA" << RESET;
            fReadoutChipInterface->WriteChipReg(pChip, "StubInputPhase", cCombStbs.first);
            fReadoutChipInterface->WriteChipReg(pChip, "RetimePix", cCombStbs.second);
        }
        cL1CombIndx++;
    }
    return cGoodCombinations;
}
// want
bool PSAlignment::FindLatency(BeBoard* pBoard, uint8_t pChipId, std::vector<Injection> pInjections, uint8_t pEdgeSelT1)
{
    // find correct hit latency
    uint32_t cNevents       = 10;
    uint16_t cHitLatency    = 0;
    uint16_t cDelay         = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_cnfg.fast_command_block.test_pulse.delay_after_test_pulse");
    auto     cTriggerMult   = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_cnfg.fast_command_block.misc.trigger_multiplicity");
    int      cOptimalOffset = -1 + (2 * (cTriggerMult > 1)); // want triggered event to be in trigger#2 of the burst
    if(PSv2) cOptimalOffset -= 2;

    bool cFoundCorrectHitLatency = false;
    for(int cOffset = (cOptimalOffset - 5); cOffset < cOptimalOffset + 5; cOffset++)
    {
        if(cFoundCorrectHitLatency) continue;

        cHitLatency     = cDelay + cOffset;
        bool cChipFound = false;
        for(auto cOpticalReadout: *pBoard)
        {
            for(auto cHybrid: *cOpticalReadout)
            {
                std::vector<uint8_t> cChipIds(0);
                for(auto cChip: *cHybrid) // for each chip (makes sense)
                {
                    cChipIds.push_back(cChip->getId());
                }
                cChipFound = (std::find(cChipIds.begin(), cChipIds.end(), pChipId) != cChipIds.end());
                if(!cChipFound) continue;

                for(auto cChip: *cHybrid) // for each chip (makes sense)
                {
                    if(cChip->getId() % 8 != pChipId) continue;
                    if(cChip->getFrontEndType() == FrontEndType::SSA2)
                        fReadoutChipInterface->WriteChipReg(cChip, "TriggerLatency", (uint16_t)cHitLatency + 1);
                    else
                    {
                        fReadoutChipInterface->WriteChipReg(cChip, "TriggerLatency", (uint16_t)cHitLatency);
                        fReadoutChipInterface->WriteChipReg(cChip, "SelectEdgeT1", pEdgeSelT1);
                    }
                } // Chip - only MPAs and CBCs for this test since I'm eihter in p=p mode or 2S
            } // hybrid
        } // OG
        if(!cChipFound) continue;

        // send a ReSync since the latency was changed
        LOG(INFO) << BOLDGREEN << "Checking hit Latency of " << +cHitLatency << " - offset of " << +cOffset << " - Chip#" << +pChipId << RESET;
        fBeBoardInterface->ChipReSync(pBoard);
        // look at data
        ReadNEvents(pBoard, cNevents);
        const std::vector<Event*>& cEvents        = this->GetEvents();
        float                      cMatchingCount = (cTriggerMult == 0) ? (cEvents.size() - 1) : cEvents.size() / (float)(1 + cTriggerMult);
        // size_t                     cNEventsMatched = 0;
        // one of these triggers should match
        for(size_t cTriggerId = 0; cTriggerId < (size_t)(cTriggerMult + 1); cTriggerId++)
        {
            if(cFoundCorrectHitLatency) continue;
            LOG(DEBUG) << BOLDBLUE << "Checking readout for match in number of hits ... looking at Trigger#" << +cTriggerId << " in a burst of " << (1 + cTriggerMult) << RESET;
            auto cEventIter = cEvents.begin() + cTriggerId;
            // cNEventsMatched=0;
            size_t cAllEvents     = 0;
            size_t cMatchedEvents = 0;
            do {
                if(cEventIter >= cEvents.end()) break;
                bool cAllEventsMatched = true;
                for(auto cOpticalReadout: *pBoard)
                {
                    for(auto cHybrid: *cOpticalReadout)
                    {
                        std::vector<uint8_t> cChipIds(0);
                        for(auto cChip: *cHybrid) // for each chip (makes sense)
                        {
                            cChipIds.push_back(cChip->getId());
                        }
                        bool cChipFound = (std::find(cChipIds.begin(), cChipIds.end(), pChipId) != cChipIds.end());
                        if(!cChipFound) continue;

                        for(auto cChip: *cHybrid)
                        {
                            if(cChip->getFrontEndType() == FrontEndType::SSA2) continue;
                            if(cChip->getId() % 8 != pChipId) continue;

                            auto cPclusters = static_cast<D19cCic2Event*>(*cEventIter)->GetPixelClusters(cChip->getHybridId(), cChip->getId());
                            for(auto cInjection: pInjections)
                            {
                                bool cMatchFound = false;
                                for(auto cPcluster: cPclusters)
                                {
                                    if(cMatchFound) continue;
                                    cMatchFound = (cPcluster.fAddress == cInjection.fRow) && (cPcluster.fZpos == cInjection.fColumn);
                                    if(cMatchFound)
                                    {
                                        LOG(INFO) << BOLDGREEN << "\t\t Trigger#" << +cTriggerId << " Event#" << (*cEventIter)->GetEventCount() << " in a burst of " << (1 + cTriggerMult) << " MPA"
                                                  << +cChip->getId() << " found " << cPclusters.size() << " P clusters."
                                                  << " Match for injection in " << +cInjection.fRow << " , " << +cInjection.fColumn << RESET;
                                    }
                                    else
                                        LOG(DEBUG) << BOLDRED << "\t\t Trigger#" << +cTriggerId << " Event#" << (*cEventIter)->GetEventCount() << " in a burst of " << (1 + cTriggerMult) << " MPA"
                                                   << +cChip->getId() << " found " << cPclusters.size() << " P clusters."
                                                   << " not a match for injection in " << +cInjection.fRow << " , " << +cInjection.fColumn << " -- " << +cPcluster.fAddress << " , " << +cPcluster.fZpos
                                                   << RESET;
                                }
                                cAllEventsMatched = cAllEventsMatched && cMatchFound;
                            }
                        } // chip
                    }
                }
                cAllEvents++;
                cMatchedEvents += (cAllEventsMatched) ? 1 : 0;
                cEventIter += (1 + cTriggerMult);
            } while(cEventIter < cEvents.end());
            if(cMatchedEvents >= cMatchingCount)
            {
                LOG(INFO) << BOLDGREEN << " All events matched." << RESET;
                cFoundCorrectHitLatency = true;
            }
            else
                LOG(DEBUG) << BOLDRED << " Not all events match." << RESET;
        } // event loop
    }
    if(cFoundCorrectHitLatency)
        fOptimalLatency = cHitLatency;
    else
        fOptimalLatency = 0;
    return cFoundCorrectHitLatency;
}

bool PSAlignment::AlignInputs(BeBoard* pBoard, uint8_t pChipId)
{
    if(pBoard->getStubOffset() == 0) pBoard->setStubOffset(75);
    LOG(INFO) << BOLDBLUE << "Aligning MPA inputs for both L1 and Stub data for SSA-MPA pair #" << +pChipId << RESET;

    // check trigger source
    // and reload
    Injection              cInjection;
    std::vector<Injection> cInjections;
    // in principle here I would like to make sure all lines are aligned
    cInjection.fRow    = 10;
    cInjection.fColumn = 2;
    cInjections.push_back(cInjection); // 0
    cInjection.fRow    = 20;
    cInjection.fColumn = 3;
    cInjections.push_back(cInjection); // 1
    cInjection.fRow    = 30;
    cInjection.fColumn = 4;
    cInjections.push_back(cInjection); // 2
    cInjection.fRow    = 40;
    cInjection.fColumn = 5;
    cInjections.push_back(cInjection); // 3
    cInjection.fRow    = 50;
    cInjection.fColumn = 6;
    cInjections.push_back(cInjection); // 4
    cInjection.fRow    = 60;
    cInjection.fColumn = 7;
    cInjections.push_back(cInjection); // 5
    cInjection.fRow    = 70;
    cInjection.fColumn = 8;
    cInjections.push_back(cInjection); // 6
    cInjection.fRow    = 80;
    cInjection.fColumn = 9;
    cInjections.push_back(cInjection); // 7

    // check trigger source
    // and reload
    uint16_t cTriggerSrc         = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_cnfg.fast_command_block.trigger_source");
    uint16_t cOriginalTriggerSrc = cTriggerSrc;
    uint8_t  cOriginalTLUconfig  = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_cnfg.tlu_block.tlu_enabled");
    cTriggerSrc                  = (cTriggerSrc == 6) ? cTriggerSrc : 6;
    LOG(DEBUG) << BOLDBLUE << "Trigger source is set to " << +cTriggerSrc << RESET;
    std::vector<std::pair<std::string, uint32_t>> cRegVec;
    cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.trigger_source", cTriggerSrc});
    cRegVec.push_back({"fc7_daq_ctrl.fast_command_block.control.load_config", 0x1});
    cRegVec.push_back({"fc7_daq_cnfg.tlu_block.tlu_enabled", 0x0});
    fBeBoardInterface->WriteBoardMultReg(pBoard, cRegVec);

    auto     cTriggerMult   = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_cnfg.fast_command_block.misc.trigger_multiplicity");
    uint16_t cDelay         = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_cnfg.fast_command_block.test_pulse.delay_after_test_pulse");
    int      cOptimalOffset = -1 + (2 * (cTriggerMult > 1)); // want triggered event to be in trigger#2 of the burst
    if(PSv2) cOptimalOffset -= 2;
    // int      cOptimalOffset = 1 + (2*(cTriggerMult > 1)); // want triggered event to be in trigger#2 of the burst
    // int      cOptimalOffset = 1 + (cTriggerMult > 1); // want triggered event to be in trigger#2 of the burst
    uint16_t cLatency = cDelay + cOptimalOffset;
    LOG(DEBUG) << BOLDMAGENTA << "Expect correct latency to be " << +cLatency << RESET;

    // inject pixel clusters
    InjectPattern(pBoard, cInjections, pChipId);

    // scan alignment parameters - first L1A
    std::vector<uint8_t> cEdgeSelsRaw{0, 1}; //,0};
    // first scan the edge select for the T1 commands
    auto&   cL1AlParsThisBoard = fL1AlParsContainer.getObject(pBoard->getId());
    auto    cOriginlStubOffset = pBoard->getStubOffset();
    bool    cScanEdgeT1        = true;
    uint8_t cEdgeSelT1         = 1;
    do {
        LOG(INFO) << BOLDYELLOW << "Edge-select for T1 input is " << +cEdgeSelT1 << RESET;
        // need to find the correct hit latency
        // try 5 times
        uint8_t cLatencyAttempt = 0;
        bool    cLatencyFound   = false;
        do {
            LOG(INFO) << BOLDBLUE << "Latency Scan PSAlignment Attempt#" << +cLatencyAttempt << RESET;
            cLatencyFound = FindLatency(pBoard, pChipId, cInjections, cEdgeSelT1);
            cLatencyAttempt++;
        } while(!cLatencyFound && cLatencyAttempt < 2);

        // if the correct hit latency could not be found
        // skip to the next edge
        if(cLatencyAttempt >= 2 && !cLatencyFound)
        {
            if(cEdgeSelT1 == 1)
            {
                LOG(INFO) << BOLDRED << "FAILED to find L1 latency... going to the next edge  ... " << RESET;
                cEdgeSelT1 = 0;
            }
            else
            {
                cScanEdgeT1 = false;
                continue;
            }
        }
        else
            cScanEdgeT1 = false;

        // then the edge for the raw input
        for(auto cEdgeSelRaw: cEdgeSelsRaw)
        {
            LOG(INFO) << BOLDYELLOW << "\t..Edge-select for Raw input is " << +cEdgeSelRaw << RESET;

            for(auto cOpticalReadout: *pBoard)
            {
                auto& cL1AlParsThisOG = cL1AlParsThisBoard->getObject(cOpticalReadout->getId());
                for(auto cHybrid: *cOpticalReadout)
                {
                    auto& cL1AlParsThisHybrid = cL1AlParsThisOG->getObject(cHybrid->getId());
                    for(auto cChip: *cHybrid)
                    {
                        if(cChip->getFrontEndType() != FrontEndType::MPA2) continue;
                        if(cChip->getId() % 8 != pChipId) continue;

                        auto cL1AlignmentPars = this->AlignL1(cChip, cInjections, cEdgeSelRaw);
                        if(cL1AlignmentPars.size() > 0)
                        {
                            LOG(INFO) << BOLDYELLOW << "\t\tFound " << +cL1AlignmentPars.size() << " combinations of alignment parameters for L1 data from SSA" << RESET;
                        }
                        else
                            LOG(INFO) << BOLDYELLOW << "\t\t no alignment parameters found for L1 data from SSA" << RESET;

                        auto& cL1AlParsThisChip = cL1AlParsThisHybrid->getObject(cChip->getId());
                        auto& cMPAInL1Pars      = cL1AlParsThisChip->getSummary<std::vector<MPAInputAlignment>>();
                        for(auto cPar: cL1AlignmentPars)
                        {
                            MPAInputAlignment cPars;
                            cPars.fL1InputPhase   = cPar.first;
                            cPars.fLatencyRx40    = cPar.second;
                            cPars.fEdgeSelT1      = cEdgeSelT1;
                            cPars.fEdgeSelL1      = cEdgeSelRaw;
                            cPars.fStubInputPhase = 0;
                            cPars.fRetimePix      = 0;
                            cPars.fStubOffset     = pBoard->getStubOffset();
                            cMPAInL1Pars.push_back(cPars);
                            LOG(DEBUG) << BOLDYELLOW << "\t\t L1Pars [ InputPhase = " << +cPars.fL1InputPhase << " LatencyRx40 " << +cPars.fLatencyRx40 << " ]" << RESET;
                        }
                    }
                }
            }
        }
    } while(cScanEdgeT1);

    // check that at least one L1 alignment parameter was found for each chip
    bool cOneFoundForAll = true;
    for(auto cOpticalReadout: *pBoard)
    {
        auto& cL1AlParsThisOG = cL1AlParsThisBoard->getObject(cOpticalReadout->getId());
        for(auto cHybrid: *cOpticalReadout)
        {
            auto& cL1AlParsThisHybrid = cL1AlParsThisOG->getObject(cHybrid->getId());
            for(auto cChip: *cHybrid)
            {
                if(cChip->getFrontEndType() != FrontEndType::MPA2) continue;
                if(cChip->getId() % 8 != pChipId) continue;

                auto& cL1AlParsThisChip = cL1AlParsThisHybrid->getObject(cChip->getId());
                auto& cMPAInL1Pars      = cL1AlParsThisChip->getSummary<std::vector<MPAInputAlignment>>();
                cOneFoundForAll         = cOneFoundForAll && cMPAInL1Pars.size() > 0;
            }
        }
    }
    fBeBoardInterface->ChipReSync(pBoard);

    // now scan stub alignment parameters
    // for this.. enough to have L1 alignment pars
    // set to 'first good'
    auto& cStubAlParsThisBoard = fStubAlParsContainer.getObject(pBoard->getId());
    for(auto cOpticalReadout: *pBoard)
    {
        auto& cAlParsThisOG   = cStubAlParsThisBoard->getObject(cOpticalReadout->getId());
        auto& cL1AlParsThisOG = cL1AlParsThisBoard->getObject(cOpticalReadout->getId());
        for(auto cHybrid: *cOpticalReadout)
        {
            auto& cAlParsThisHybrid   = cAlParsThisOG->getObject(cHybrid->getId());
            auto& cL1AlParsThisHybrid = cL1AlParsThisOG->getObject(cHybrid->getId());
            for(auto cChip: *cHybrid)
            {
                if(cChip->getId() % 8 != pChipId) continue;

                auto& cAlParsThisChip = cAlParsThisHybrid->getObject(cChip->getId());
                auto& cMPAPars        = cAlParsThisChip->getSummary<std::vector<MPAInputAlignment>>();

                auto& cL1AlParsThisChip = cL1AlParsThisHybrid->getObject(cChip->getId());
                auto& cMPAParsL1        = cL1AlParsThisChip->getSummary<std::vector<MPAInputAlignment>>();
                // assume all lines must use the same edge
                size_t cL1ParIndx = 0;
                for(auto cL1Par: cMPAParsL1)
                {
                    if(cL1ParIndx > 0) continue;
                    MPAInputAlignment cPars;
                    cPars.fL1InputPhase = cL1Par.fL1InputPhase;
                    cPars.fLatencyRx40  = cL1Par.fLatencyRx40;
                    cPars.fEdgeSelT1    = cL1Par.fEdgeSelT1;
                    cPars.fEdgeSelL1    = cL1Par.fEdgeSelL1;
                    // configure L1A inputs
                    ConfigureRawInputs(cChip, cPars, 0);
                    // send a ReSync
                    fBeBoardInterface->ChipReSync(pBoard);
                    bool    cScanEdgeStubs = true;
                    uint8_t cEdgeSelStub   = 0;
                    do {
                        pBoard->setStubOffset(cOriginlStubOffset);
                        cPars.fEdgeSelStubs = cEdgeSelStub;

                        cPars.fStubInputPhase = 0;
                        cPars.fRetimePix      = 0;
                        cPars.fStubOffset     = cOriginlStubOffset;
                        cPars.fEdgeSelT1      = cEdgeSelStub; // says Davide
                        LOG(INFO) << BOLDCYAN << "L1Pars [ InputPhase = " << +cPars.fL1InputPhase << " LatencyRx40 " << +cPars.fLatencyRx40 << " EdgeSelT1 " << +cPars.fEdgeSelT1 << " EdgeSelL1 "
                                  << +cPars.fEdgeSelL1 << " EdgeSelStubs " << +cEdgeSelStub << " ]" << RESET;

                        // configure stub inputs
                        ConfigureStubInputs(cChip, cPars, 0);
                        auto cStubAlignmentPars = this->AlignStubs(cChip, cInjections, cLatency);

                        if(cStubAlignmentPars.size() > 0)
                        {
                            cScanEdgeStubs = false;
                            LOG(INFO) << BOLDCYAN << "Edge-select for Stub inputs will be set to " << +cEdgeSelStub << "; Found " << +cStubAlignmentPars.size()
                                      << " combinations of alignment parameters for stub data from SSA" << RESET;
                        }
                        else
                        {
                            if(cEdgeSelStub == 0)
                            {
                                LOG(INFO) << BOLDRED << "FAILED to find Stub alignment parameters .. going to the next edge  ... " << RESET;
                                cEdgeSelStub = 1;
                            }
                            else
                            {
                                cScanEdgeStubs = false;
                                continue;
                            }
                        }
                        for(auto cPar: cStubAlignmentPars)
                        {
                            cPars.fStubInputPhase = cPar.first;
                            cPars.fRetimePix      = cPar.second;
                            cPars.fStubOffset     = pBoard->getStubOffset();
                            LOG(INFO) << BOLDCYAN << "\t\t L1Pars [ InputPhase = " << +cPars.fL1InputPhase << " LatencyRx40 " << +cPars.fLatencyRx40 << " EdgeSelT1 " << +cPars.fEdgeSelT1
                                      << " EdgeSelL1 " << +cPars.fEdgeSelL1 << " ]"
                                      << " StubPars [ InputPhase " << +cPars.fStubInputPhase << " ReTimePix " << +cPars.fRetimePix << " StubOffset " << +cPars.fStubOffset << "]" << RESET;
                            cMPAPars.push_back(cPars);
                        }
                        pBoard->setStubOffset(cOriginlStubOffset);
                        // LOG (INFO) << BOLDCYAN << "Stub offset is " << +pBoard->getStubOffset() << RESET;
                    } while(cScanEdgeStubs); // stub edges
                    cL1ParIndx++;
                } // L1A pars
            } // chips
        } // hybrids
    } // links
    // check that at least one stub alignment parameter was found for each chip
    for(auto cOpticalReadout: *pBoard)
    {
        auto& cAlParsThisOG = cStubAlParsThisBoard->getObject(cOpticalReadout->getId());
        for(auto cHybrid: *cOpticalReadout)
        {
            auto& cAlParsThisHybrid = cAlParsThisOG->getObject(cHybrid->getId());
            for(auto cChip: *cHybrid)
            {
                if(cChip->getFrontEndType() != FrontEndType::MPA2) continue;
                if(cChip->getId() % 8 != pChipId) continue;

                auto& cAlParsThisChip = cAlParsThisHybrid->getObject(cChip->getId());
                auto& cMPAPars        = cAlParsThisChip->getSummary<std::vector<MPAInputAlignment>>();
                cOneFoundForAll       = cOneFoundForAll && cMPAPars.size() > 0;
            }
        }
    }
    // send a Resync to this board
    fBeBoardInterface->ChipReSync(pBoard);

    // now push back all valid alignment parameters for all
    auto& cAlParsThisBoard = fAlParsContainer.getObject(pBoard->getId());
    for(auto cOpticalReadout: *pBoard)
    {
        auto& cStubAlParsThisOG = cStubAlParsThisBoard->getObject(cOpticalReadout->getId());
        auto& cL1AlParsThisOG   = cL1AlParsThisBoard->getObject(cOpticalReadout->getId());
        auto& cAlParsThisOG     = cAlParsThisBoard->getObject(cOpticalReadout->getId());
        for(auto cHybrid: *cOpticalReadout)
        {
            auto& cStubAlParsThisHybrid = cStubAlParsThisOG->getObject(cHybrid->getId());
            auto& cL1AlParsThisHybrid   = cL1AlParsThisOG->getObject(cHybrid->getId());
            auto& cAlParsThisHybrid     = cAlParsThisOG->getObject(cHybrid->getId());
            for(auto cChip: *cHybrid)
            {
                if(cChip->getFrontEndType() != FrontEndType::MPA2) continue;
                if(cChip->getId() % 8 != pChipId) continue;

                auto& cL1lParsThisChip = cL1AlParsThisHybrid->getObject(cChip->getId());
                auto& cL1MPAPars       = cL1lParsThisChip->getSummary<std::vector<MPAInputAlignment>>();

                auto& cStubParsThisChip = cStubAlParsThisHybrid->getObject(cChip->getId());
                auto& cStubMPAPars      = cStubParsThisChip->getSummary<std::vector<MPAInputAlignment>>();

                auto& cParsThisChip = cAlParsThisHybrid->getObject(cChip->getId());
                auto& cPars         = cParsThisChip->getSummary<std::vector<MPAInputAlignment>>();

                for(auto cL1Par: cL1MPAPars)
                {
                    for(auto cStubPar: cStubMPAPars)
                    {
                        MPAInputAlignment cPar;
                        // L1A pars
                        cPar.fL1InputPhase = cL1Par.fL1InputPhase;
                        cPar.fLatencyRx40  = cL1Par.fLatencyRx40;
                        cPar.fEdgeSelT1    = cL1Par.fEdgeSelT1;
                        cPar.fEdgeSelL1    = cL1Par.fEdgeSelL1;
                        // Stub pars
                        cPar.fStubInputPhase = cStubPar.fStubInputPhase;
                        cPar.fRetimePix      = cStubPar.fRetimePix;
                        cPar.fEdgeSelStubs   = cStubPar.fEdgeSelStubs;
                        cPar.fStubOffset     = cStubPar.fStubOffset;
                        //
                        cPars.push_back(cPar);
                        PrintAlignmentParameters(cPar);
                    }
                }
            }
        }
    }

    // set stub offset back to what it was
    pBoard->setStubOffset(cOriginlStubOffset);

    // set everything back to original values .. like I wasn't here
    // reset fast command registers
    cRegVec.clear();
    cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.trigger_source", cOriginalTriggerSrc});
    cRegVec.push_back({"fc7_daq_ctrl.fast_command_block.control.load_config", 0x1});
    cRegVec.push_back({"fc7_daq_cnfg.tlu_block.tlu_enabled", cOriginalTLUconfig});
    fBeBoardInterface->WriteBoardMultReg(pBoard, cRegVec);

    return cOneFoundForAll;
}
bool PSAlignment::CheckL1Data(ClusterCollection<PixelClusterPS, 32> pPClusters, ClusterCollection<StripClusterPS, 32> pSClusters, std::vector<Injection> pInjections)
{
    bool cFmatch = true;

    struct
    {
        bool operator()(PixelClusterPS a, PixelClusterPS b) const { return a.fAddress < b.fAddress; }
    } customSortPclus;
    struct
    {
        bool operator()(StripClusterPS a, StripClusterPS b) const { return a.fAddress < b.fAddress; }
    } customSortSclus;

    // sort P clusters by row
    std::sort(pPClusters.begin(), pPClusters.end(), customSortPclus);
    // sort S clusters by row
    std::sort(pSClusters.begin(), pSClusters.end(), customSortSclus);

    std::vector<StripClusterPS> cMtchdSclstrs;
    std::vector<PixelClusterPS> cMtchdPclstrs;
    // Check P-clusters
    for(auto cInjection: pInjections)
    {
        bool cMatchFound = false;
        for(auto cPcluster: pPClusters)
        {
            if(cMatchFound) continue;
            cMatchFound = (cPcluster.fAddress == cInjection.fRow) && (cPcluster.fZpos == cInjection.fColumn);
            if(cMatchFound)
            {
                PixelClusterPS cMtchdPclstr;
                cMtchdPclstr.fAddress = cPcluster.fAddress;
                cMtchdPclstr.fZpos    = cPcluster.fZpos;
                cMtchdPclstrs.push_back(cMtchdPclstr);
            }
        }
        cFmatch = cFmatch && cMatchFound;
    }

    // check S-clusters
    for(auto cInjection: pInjections)
    {
        bool cMatchFound = false;
        for(auto cScluster: pSClusters)
        {
            if(cMatchFound) continue;
            cMatchFound = (cScluster.fAddress == cInjection.fRow);
            if(cMatchFound)
            {
                StripClusterPS cMtchdSclstr;
                cMtchdSclstr.fAddress = cInjection.fRow;
                cMtchdSclstrs.push_back(cMtchdSclstr);
            }
        }
        cFmatch = cFmatch && cMatchFound;
    }

    LOG(DEBUG) << BOLDGREEN << "\t\t Found " << cMtchdSclstrs.size() << " matched S clusters and " << cMtchdPclstrs.size() << " matched P clusters in L1 data" << RESET;
    for(uint8_t cMatchId = 0; cMatchId < cMtchdPclstrs.size(); cMatchId++)
    {
        LOG(DEBUG) << BOLDGREEN << "\t\t\t Exact match found for injection - P cluster " << BOLDYELLOW << +cMtchdPclstrs[cMatchId].fAddress << " column " << +cMtchdPclstrs[cMatchId].fZpos << RESET;
    }
    for(uint8_t cMatchId = 0; cMatchId < cMtchdSclstrs.size(); cMatchId++)
    {
        LOG(DEBUG) << BOLDGREEN << "\t\t\t Exact match found for injection - S cluster " << BOLDYELLOW << " S-cluster in row " << +cMtchdSclstrs[cMatchId].fAddress << RESET;
    }
    return cFmatch;
}
void PSAlignment::Validate(BeBoard* pBoard, std::vector<Injection> pInjections, uint8_t pEdgeSelT1)
{
    LOG(INFO) << BOLDBLUE << "Validating SSA-MPA pair alignment procedure .. edge-select T1 is " << +pEdgeSelT1 << RESET;
    // inject pattern
    InjectPattern(pBoard, pInjections);

    // configure trigger source for TP injections
    uint16_t cTriggerSrc = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_cnfg.fast_command_block.trigger_source");
    cTriggerSrc          = (cTriggerSrc == 6) ? cTriggerSrc : 6;
    std::vector<std::pair<std::string, uint32_t>> cRegVec;
    cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.trigger_source", cTriggerSrc});
    cRegVec.push_back({"fc7_daq_ctrl.fast_command_block.control.load_config", 0x1});
    // cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.test_pulse.delay_after_test_pulse",200});
    cRegVec.push_back({"fc7_daq_cnfg.tlu_block.tlu_enabled", 0x0});
    fBeBoardInterface->WriteBoardMultReg(pBoard, cRegVec);

    // find latency on all Chips
    std::vector<uint16_t> cLatencyBins(512, 0);
    for(size_t cChipId = 0; cChipId < 8; cChipId++)
    {
        if(FindLatency(pBoard, cChipId, pInjections, pEdgeSelT1)) { cLatencyBins[fOptimalLatency]++; }
    }
    auto cModeLatency = std::max_element(cLatencyBins.begin(), cLatencyBins.end()) - cLatencyBins.begin();

    LOG(INFO) << BOLDBLUE << "Most frequently found latency no this board is " << cModeLatency << RESET;
    size_t cNevents = 10;
    LOG(INFO) << BOLDRED << "1" << RESET;
    auto cTriggerMult = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_cnfg.fast_command_block.misc.trigger_multiplicity");
    for(size_t cTriggerId = 0; cTriggerId < (1 + cTriggerMult); cTriggerId++)
    {
        bool cGoodStubDelay = false;
        for(int cStubAddDelay = -10; cStubAddDelay <= 10; cStubAddDelay++) // to-do - add range to xml
        {
            if(cGoodStubDelay) continue;
            LOG(INFO) << BOLDRED << "2" << RESET;
            auto cStubOffset = pBoard->getStubOffset() + cStubAddDelay;
            LOG(INFO) << BOLDRED << "3 " << cModeLatency << " " << cStubOffset << " " << 6 << RESET;
            size_t cStubDelay = cModeLatency - cStubOffset - 6; // stub latency
            LOG(INFO) << BOLDRED << "4 " << cStubDelay << RESET;

            fBeBoardInterface->WriteBoardReg(pBoard, "fc7_daq_cnfg.readout_block.global.common_stubdata_delay", cStubDelay);
            LOG(INFO) << BOLDRED << "5" << RESET;
            ReadNEvents(pBoard, cNevents);
            const std::vector<Event*>& cEvents = this->GetEvents();
            if(cEvents.size() == 0) continue;
            LOG(INFO) << BOLDBLUE << "Checking full match for a stub latnecy of " << cStubDelay << RESET;

            bool cAllMatched = true;
            LOG(INFO) << BOLDBLUE << "\t... Trigger#" << +cTriggerId << " in a burst of " << (1 + cTriggerMult) << RESET;
            for(auto cOpticalReadout: *pBoard)
            {
                for(auto cHybrid: *cOpticalReadout)
                {
                    for(auto cChip: *cHybrid) // for each chip (makes sense)
                    {
                        if(cChip->getFrontEndType() != FrontEndType::MPA2) continue;
                        cAllMatched = cAllMatched && CheckFullMatch(cChip, cEvents, pInjections, cTriggerId, cTriggerMult);
                    }
                }
            }
            if(cAllMatched)
            {
                LOG(INFO) << BOLDGREEN << "\t\t..All Match... can stop the scan..." << RESET;
                cGoodStubDelay = true;
            }
        }
        if(cGoodStubDelay)
        {
            auto cDelay = fBeBoardInterface->ReadBoardReg(pBoard, "fc7_daq_cnfg.readout_block.global.common_stubdata_delay");
            LOG(INFO) << BOLDGREEN << " For trigger#" << +cTriggerId << " in a burst of " << (1 + cTriggerMult) << " good stub delay is " << cDelay << RESET;
        }
        else
            LOG(INFO) << BOLDRED << " No value of stub delay makes events in trigger#" << +cTriggerId << " in a burst of " << (1 + cTriggerMult) << " match." << RESET;
    }
}
bool PSAlignment::CheckFullMatch(ReadoutChip* pChip, const std::vector<Event*>& pEvents, std::vector<Injection> pInjections, uint8_t pTriggerId, size_t pTriggerMult)
{
    if(pChip->getFrontEndType() != FrontEndType::MPA2) return true;

    bool cCheckL1 = true;
    struct
    {
        bool operator()(PixelClusterPS a, PixelClusterPS b) const { return a.fAddress < b.fAddress; }
    } customSortPclus;
    struct
    {
        bool operator()(StripClusterPS a, StripClusterPS b) const { return a.fAddress < b.fAddress; }
    } customSortSclus;
    // struct
    // {
    //     bool operator()(EventStub a, EventStub b) const { return a.fPosition < b.fPosition; }
    // } customSortStubs;

    float  cMatchingCount   = (pTriggerMult == 0) ? (pEvents.size() - 1) : pEvents.size() / (float)(1 + pTriggerMult);
    size_t cMatchedEvents   = 0;
    auto   cEventIter       = pEvents.begin() + pTriggerId;
    size_t cMatchedEventsL1 = 0;
    size_t cNchecked        = 0;
    do {
        if(cEventIter >= pEvents.end()) break;
        auto cPclus = static_cast<D19cCic2Event*>(*cEventIter)->GetPixelClusters(pChip->getHybridId(), pChip->getId());
        auto cSclus = static_cast<D19cCic2Event*>(*cEventIter)->GetStripClusters(pChip->getHybridId(), pChip->getId());
        // sort P clusters by row
        std::sort(cPclus.begin(), cPclus.end(), customSortPclus);
        // sort S clusters by row
        std::sort(cSclus.begin(), cSclus.end(), customSortSclus);
        auto cStubs = static_cast<D19cCic2Event*>(*cEventIter)->StubVector(pChip->getHybridId(), pChip->getId());
        LOG(DEBUG) << BOLDBLUE << "\t\t..Read-back " << +cStubs.size() << " stubs from MPA#" << +pChip->getId() << RESET;
        cMatchedEventsL1 += (cPclus.size() == pInjections.size() && cSclus.size() == pInjections.size()) ? 1 : 0;
        bool cNmatch = true;
        if(cCheckL1)
        {
            cNmatch = CheckL1Data(cPclus, cSclus, pInjections);
            // cNmatch = (cPclus.size() == pInjections.size() && cSclus.size() == pInjections.size());
            LOG(DEBUG) << BOLDGREEN << "Trigger#" << +pTriggerId << " in a burst of " << (1 + pTriggerMult) << " MPA" << +pChip->getId() << " found "
                       << " correct numnber of S and P clusters." << RESET;
        }
        if(cStubs.size() == pInjections.size() && cNmatch)
            LOG(DEBUG) << BOLDGREEN << "Trigger#" << +pTriggerId << " Event#" << (*cEventIter)->GetEventCount() << " in a burst of " << (1 + pTriggerMult) << " MPA" << +pChip->getId() << " found "
                       << cSclus.size() << " S clusters and " << cPclus.size() << " P clusters in L1 data from MPA#" << +pChip->getId() << " also have " << +cStubs.size() << " stbs." << RESET;
        // print stubs if they are there
        if(cNmatch)
        {
            size_t cStubCntr = 0;
            // sort stubs by position
            // std::sort(cStubs.begin(), cStubs.end(), customSortStubs);
            size_t cMatchedStubSeeds = 0;
            for(auto cInjection: pInjections)
            {
                bool    cMatchedStub      = false;
                uint8_t cMatchedStubIndex = 0;
                for(auto cStub: cStubs)
                {
                    if(cMatchedStub) continue;
                    cMatchedStub = (2 * cInjection.fRow == cStub.getPosition() && cInjection.fColumn == cStub.getRow());
                    if(cMatchedStub)
                        LOG(INFO) << BOLDGREEN << "\t\tStub#" << +cMatchedStubIndex << " Position " << +cStub.getPosition() << " - Row " << +cStub.getRow() << " - Bend " << +cStub.getBend() << RESET;
                    if(!cMatchedStub) cMatchedStubIndex++;
                    cStubCntr++;
                }
                if(cMatchedStub)
                    LOG(DEBUG) << BOLDGREEN << "\t\tStub#" << +cMatchedStubIndex << " Position " << +cStubs[cMatchedStubIndex].getPosition() << " - Row " << +cStubs[cMatchedStubIndex].getRow()
                               << " - Bend " << +cStubs[cMatchedStubIndex].getBend() << RESET;
                else
                    LOG(DEBUG) << BOLDRED << "\t\tNo match found" << RESET;

                cMatchedStubSeeds += (cMatchedStub) ? 1 : 0;
            }
            cNmatch = cNmatch && (cMatchedStubSeeds == pInjections.size());
        }
        // update counters
        cEventIter += (1 + pTriggerMult);
        cMatchedEvents += (cNmatch) ? 1 : 0;
        cNchecked++;
    } while(cEventIter < pEvents.end());
    bool cMatchFound = cMatchedEvents >= cMatchingCount;
    if(cMatchFound) // to allow for single triggers
        LOG(INFO) << BOLDGREEN << "RCO#" << +pChip->getId() << "  " << cMatchedEvents << " matched for stubs " << cMatchedEventsL1 << " events matched for L1A data "
                  << ". Found " << pEvents.size() << " events in the readout ]. In total " << cNchecked << " events were checked" << RESET;
    else
        LOG(DEBUG) << BOLDRED << cMatchedEvents << " matched for stubs " << cMatchedEventsL1 << " events matched for L1A data "
                   << ". Found " << pEvents.size() << " events in the readout ]. In total " << cNchecked << " events were checked" << RESET;
    return cMatchFound;
}
bool PSAlignment::Align()
{
    LOG(INFO) << BOLDBLUE << "Starting MPA-SSA alignment procedure .... " << RESET;
    // not sure I need this here .. lets check
    // auto     cSetting       = fSettingsMap.find("TxDrive");
    uint32_t cTxDriveStr = 4;
    // cSetting                = fSettingsMap.find("PreEmph");
    uint32_t cTxPreEmphMode = 1;
    // configure TxDrive for lpGBT
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            auto& clpGBT = cOpticalGroup->flpGBT;
            if(clpGBT == nullptr) return true;

            // Tx Groups and Channels
            std::vector<uint8_t> cTxGroups = {0, 1, 2, 3}, cTxChannels = {0};
            uint8_t              cTxPreEmphStr = 4, cTxPreEmphWidth = 0, cTxInvert = 0;
            LOG(INFO) << BOLDGREEN << "Setting TxDrive on lpGBT to " << cTxDriveStr << RESET;
            for(const auto& cGroup: cTxGroups)
            {
                cTxInvert = (cGroup % 2 == 0) ? 1 : 0;
                for(const auto& cChannel: cTxChannels) flpGBTInterface->ConfigureTxChannel(clpGBT, {cGroup}, {cChannel}, cTxDriveStr, cTxPreEmphMode, cTxPreEmphStr, cTxPreEmphWidth, cTxInvert);
            }
        }
    }

    bool cl1Aligned   = true;
    bool cStubAligned = true;
    for(auto cBoard: *fDetectorContainer)
    {
        fBeBoardInterface->ChipReSync(cBoard);
        auto cInterface          = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(cBoard));
        auto cL1ReadoutInterface = cInterface->getL1ReadoutInterface();
        cL1ReadoutInterface->ResetReadout();

        bool cWithMPA = false;
        bool cWithSSA = false;
        for(auto cOpticalReadout: *cBoard)
        {
            for(auto cHybrid: *cOpticalReadout)
            {
                for(auto cChip: *cHybrid)
                {
                    cWithMPA = cWithMPA || (cChip->getFrontEndType() == FrontEndType::MPA2);
                    cWithSSA = cWithMPA || (cChip->getFrontEndType() == FrontEndType::SSA2);
                }
            }
        }
        if(!(cWithSSA && cWithMPA))
        {
            LOG(INFO) << BOLDBLUE << "Not performing SSA-MPA L1 alignment... no PS chips!" << RESET;
            continue;
        }
        // // potentially we have 8 chips possible
        // cl1Aligned = cl1Aligned && this->AlignL1Inputs(cBoard);
        // cStubAligned = cStubAligned && this->AlignStubInputs(cBoard);
        // LOG(INFO) << BOLDBLUE << "L1 alignemnt " << RESET;
        // cl1Aligned ? LOG(INFO) << BOLDGREEN << "Succeeded" << RESET : LOG(INFO) << BOLDRED << "Failed" << RESET;

        uint8_t cMaxChips = 8;
        for(int cChipId = 0; cChipId < cMaxChips; cChipId++)
        // for(int cChipId = cMaxChips; cChipId >=0; cChipId--)
        {
            // check if chip id is there
            bool cChipFound = false;
            for(auto cOpticalReadout: *cBoard)
            {
                if(cChipFound) break;
                for(auto cHybrid: *cOpticalReadout)
                {
                    if(cChipFound) break;
                    for(auto cChip: *cHybrid)
                    {
                        if(cChipFound) break;
                        if(cChip->getFrontEndType() != FrontEndType::MPA2) continue;
                        if(cChip->getId() % cMaxChips == cChipId) cChipFound = true;
                    }
                }
            }
            if(!cChipFound) continue;

            cl1Aligned = cl1Aligned && this->AlignInputs(cBoard, cChipId);
            cl1Aligned ? LOG(INFO) << BOLDGREEN << "Succeeded" << RESET : LOG(INFO) << BOLDRED << "Failed" << RESET;
            this->Reset();
        }
    }

    // now set alignment parameter to..
    // the first one found
    bool cAllAligned = true;
    for(auto cBoard: *fDetectorContainer)
    {
        auto& cAlParsThisBoard = fAlParsContainer.getObject(cBoard->getId());
        for(auto cOpticalReadout: *cBoard)
        {
            auto& cAlParsThisOG = cAlParsThisBoard->getObject(cOpticalReadout->getId());
            for(auto cHybrid: *cOpticalReadout)
            {
                auto& cAlParsThisHybrid = cAlParsThisOG->getObject(cHybrid->getId());
                for(auto cChip: *cHybrid)
                {
                    if(cChip->getFrontEndType() != FrontEndType::MPA2) continue;
                    auto& cParsThisChip = cAlParsThisHybrid->getObject(cChip->getId());
                    auto& cPars         = cParsThisChip->getSummary<std::vector<MPAInputAlignment>>();
                    if(cPars.size() == 0)
                    {
                        cAllAligned = false;
                        continue;
                    }

                    cAllAligned            = cAllAligned && true;
                    MPAInputAlignment cPar = cPars[0];
                    ConfigureAllInputs(cChip, cPar);
                }
            }
        }
    }

    // if aligned then make sure list of MPA
    // registers to save contains alignment
    // parameters
    if(cAllAligned)
    {
        LOG(INFO) << BOLDBLUE << "All SSA-MPA pairs are aligned.. will save values.." << RESET;
        std::vector<std::string> cRegsMod{"LatencyRx320", "LatencyRx40", "RetimePix", "EdgeSelTrig", "EdgeSelT1Raw", "Control_1"};
        for(size_t cIndx = 0; cIndx <= 5; cIndx++)
        {
            std::stringstream cRegName;
            cRegName << "OutSetting_" << +cIndx;
            cRegsMod.push_back(cRegName.str());
        }
        cRegsMod.push_back("ReadoutMode");
        SetChipRegstoPerserve(FrontEndType::MPA2, cRegsMod);
        cRegsMod.clear();
        cRegsMod.push_back("ReadoutMode");
        SetChipRegstoPerserve(FrontEndType::SSA2, cRegsMod);

        return cStubAligned && cl1Aligned; // for some reaso validate fails... todo

        // check trigger source
        // and reload
        Injection              cInjection;
        std::vector<Injection> cInjections;
        // in principle here I would like to make sure all lines are aligned
        // for now I will do just four
        cInjection.fRow    = 10;
        cInjection.fColumn = 2;
        cInjections.push_back(cInjection); // 0
        std::vector<uint8_t> cEdgeSelsT1{0, 1};

        for(auto cBoard: *fDetectorContainer)
        {
            for(auto cEdgeSelT1: cEdgeSelsT1)
            {
                for(size_t cAttempt = 0; cAttempt < 1; cAttempt++)
                {
                    LOG(INFO) << BOLDBLUE << "Validation attempt#" << cAttempt << RESET;
                    Validate(cBoard, cInjections, cEdgeSelT1);
                    this->Reset();
                }
            }
        }
    }

    return cStubAligned && cl1Aligned;
}
void PSAlignment::writeObjects() {}
// State machine control functions
void PSAlignment::Running()
{
    Initialise();
    MapMPAOutputs();
    ConfigureDefaultAlignmentParameters();
    Reset();
}

void PSAlignment::Stop() { dumpConfigFiles(); }

void PSAlignment::Pause() {}

void PSAlignment::Resume() {}
