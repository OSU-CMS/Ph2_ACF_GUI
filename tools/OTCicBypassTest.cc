#include "tools/OTCicBypassTest.h"
#include "HWInterface/D19cFWInterface.h"
#include "System/RegisterHelper.h"
#include "Utils/ContainerSerialization.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

std::string OTCicBypassTest::fCalibrationDescription = "Test CIC bypass feature by injecting stubs and forwarding CIC input to the ouput";

OTCicBypassTest::OTCicBypassTest() : Tool() {}

OTCicBypassTest::~OTCicBypassTest() {}

void OTCicBypassTest::Initialise(void)
{
    fRegisterHelper->takeSnapshot();
    // free the registers in case any
    fNumberOfIterations = 1;

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: plots are booked during initialization
    fDQMHistogramOTCicBypassTest.book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif
}

void OTCicBypassTest::ConfigureCalibration() {}

void OTCicBypassTest::Running()
{
    LOG(INFO) << "Starting OTCicBypassTest measurement.";
    Initialise();
    runCICbypassTest();
    LOG(INFO) << "Done with OTCicBypassTest.";
    Reset();
}

void OTCicBypassTest::Stop(void)
{
    LOG(INFO) << "Stopping OTCicBypassTest measurement.";
#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: processing the histograms
    fDQMHistogramOTCicBypassTest.process();
#endif
    SaveResults();
    closeFileHandler();
    LOG(INFO) << "OTCicBypassTest stopped.";
}

void OTCicBypassTest::Pause() {}

void OTCicBypassTest::Resume() {}

void OTCicBypassTest::Reset() { fRegisterHelper->restoreSnapshot(); }

void OTCicBypassTest::runCICbypassTest()
{
    LOG(INFO) << BOLDYELLOW << "OTCicBypassTest::runCICbypassTest ... start integrity test" << RESET;

    for(auto theBoard: *fDetectorContainer)
    {
        auto theFWinterface = static_cast<D19cFWInterface*>(fBeBoardInterface->getFirmwareInterface(theBoard));
        for(auto theOpticalGroup: *theBoard)
        {
            // bool isA2Smodule = theOpticalGroup->getFrontEndType() == FrontEndType::OuterTracker2S;
            uint8_t numberOfBytesInSinglePacket = (static_cast<D19clpGBTInterface*>(flpGBTInterface)->GetChipRate(theOpticalGroup->flpGBT) == 10) ? 2 : 1;
            for(auto theHybrid: *theOpticalGroup)
            {
                std::map<uint8_t, std::map<uint8_t, std::pair<uint8_t, uint8_t>>> phyPortAndChannelToChipAndLine;
                auto&                                                             cCic                = static_cast<OuterTrackerHybrid*>(theHybrid)->fCic;
                auto                                                              theChipToCICMapping = cCic->getMapping();

                // inject the same channels on all Readout chips
                for(auto theChip: *theHybrid)
                {
                    for(uint8_t stubLine = 0; stubLine < 5; ++stubLine)
                    {
                        std::pair<uint8_t, uint8_t> phyPortAndChannel = fCicInterface->fromChipStubToPhyPortAndChannel(cCic, theChipToCICMapping, theChip->getId() % 8, stubLine);
                        phyPortAndChannelToChipAndLine[phyPortAndChannel.first][phyPortAndChannel.second] = {theChip->getId(), stubLine};
                    }

                    if(theChip->getFrontEndType() == FrontEndType::CBC3) injectStubs2S(theChip);
                    if(theChip->getFrontEndType() == FrontEndType::MPA2) injectStubsPS(theChip);
                }

                fCicInterface->SelectOutput(cCic, false);
                fBeBoardInterface->WriteBoardReg(fDetectorContainer->getObject(theOpticalGroup->getBeBoardId()), "fc7_daq_cnfg.physical_interface_block.slvs_debug.hybrid_select", theHybrid->getId());
                fBeBoardInterface->WriteBoardReg(fDetectorContainer->getObject(theOpticalGroup->getBeBoardId()), "fc7_daq_cnfg.physical_interface_block.slvs_debug.chip_select", 0);
                for(uint phyPort = 0; phyPort < 12; ++phyPort)
                {
                    uint8_t registerValue = 0x10 + phyPort;
                    fCicInterface->WriteChipReg(cCic, "MUX_CTRL", registerValue);
                    for(size_t iteration = 0; iteration < fNumberOfIterations; ++iteration)
                    {
                        auto   lineOutputVector = theFWinterface->StubDebug(true, 4, false);
                        size_t cNlines          = 4;
                        for(size_t line = 0; line < cNlines; ++line)
                        {
                            LOG(INFO) << BOLDRED << "Phyport " << +phyPort << " line " << line << " -> " << getPatternPrintout(lineOutputVector.at(line), numberOfBytesInSinglePacket) << RESET;
                        }
                    }
                }
            }
        }
    }
}

void OTCicBypassTest::injectStubs2S(Ph2_HwDescription::ReadoutChip* theCBC)
{
    LOG(INFO) << BOLDBLUE << "            injecting stubs on CBC Id " << +theCBC->getId() << RESET;

    fReadoutChipInterface->WriteChipReg(theCBC, "PtCut", 14);
    fReadoutChipInterface->WriteChipReg(theCBC, "ClusterCut", 4);
    static_cast<CbcInterface*>(fReadoutChipInterface)->selectLogicMode(theCBC, "Sampled", true, true);

    std::vector<std::pair<std::string, uint16_t>> theRegisterVector;
    theRegisterVector.push_back({"Bend7", fBendingAndCode.at(0)}); // bendind = 0 will ouput 5
    theRegisterVector.push_back({"Bend8", fBendingAndCode.at(2)}); // bendind = 2 will ouput A
    theRegisterVector.push_back({"Bend9", fBendingAndCode.at(4)}); // bendind = 4 will ouput F
    theRegisterVector.push_back({"CoincWind&Offset12", 0x00});     // set stub window offset to 0
    theRegisterVector.push_back({"CoincWind&Offset34", 0x00});     // set stub window offset to 0
    fReadoutChipInterface->WriteChipMultReg(theCBC, theRegisterVector);

    // inject stubs on CBC to CIC stub lines 0 (first stub address) lines 1 (second stub address), line 3 (first and second stub bend)
    std::vector<std::pair<uint8_t, int>> stubSeedAndBendingVector{{0x0A, 0}, {0xA0, 2}, {0xAA, 4}};
    // std::vector<std::pair<uint8_t, int>> stubSeedAndBendingVector{};
    static_cast<CbcInterface*>(fReadoutChipInterface)->injectStubs(theCBC, stubSeedAndBendingVector);
}

void OTCicBypassTest::injectStubsPS(Ph2_HwDescription::ReadoutChip* theMPA)
{
    fReadoutChipInterface->WriteChipReg(theMPA, "LFSR_data", 0xAA);
    static_cast<PSInterface*>(fReadoutChipInterface)->fTheMPA2Interface->WriteChipRegBits(theMPA, "Control_1", 0x2, "Mask", 0x03);

    // fReadoutChipInterface->WriteChipReg(theMPA, "StubMode", 2); // Use pixel mode to exclude possible SSA communication issues
    // fReadoutChipInterface->WriteChipReg(theMPA, "StubWindow", 8);
    // fReadoutChipInterface->WriteChipReg(theMPA, "CodeM10", 0x0); // bending = 0 will ouput 0

    // // col, row, cluster size
    // std::vector<Cluster> theClusterList{Cluster(0xA, 0x55, 1)};
    // static_cast<PSInterface*>(fReadoutChipInterface)->injectNoiseClusters(theMPA, theClusterList);
}