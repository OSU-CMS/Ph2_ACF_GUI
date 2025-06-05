#include "HWInterface/DPInterface.h"
#include "HWInterface/BeBoardFWInterface.h"
#include <bitset>
using namespace Ph2_HwDescription;

namespace Ph2_HwInterface
{
DPInterface::DPInterface()
{
    fEmulatorRunning = false;
    fWait_us         = 1000;
}

DPInterface::~DPInterface() {}
void DPInterface::ResetFCMDBram(BeBoardFWInterface* pInterface)
{
    LOG(INFO) << BOLDBLUE << "Resetting FCMD BRAM from sw.." << RESET;
    for(uint16_t cBx = 0; cBx < 16383; cBx++)
    {
        // first data from MPAs
        std::vector<std::pair<std::string, uint32_t>> cRegs;
        cRegs.push_back({"fc7_daq_cnfg.fast_command_block.generic_fcmd_data", 0x00});
        cRegs.push_back({"fc7_daq_cnfg.fast_command_block.generic_fcmd_addr", cBx});
        pInterface->WriteStackReg(cRegs);
        // write enable
        cRegs.clear();
        std::this_thread::sleep_for(std::chrono::microseconds(fWait_us * 10));
        pInterface->WriteReg("fc7_daq_ctrl.fast_command_block.control.wr_en_generic", 0x1);
        std::this_thread::sleep_for(std::chrono::microseconds(fWait_us * 10));
        if(cBx % 1000 == 0) LOG(INFO) << BOLDBLUE << "\t... Bx..." << +cBx << RESET;
    }
}
void DPInterface::StartSyncPlaying(BeBoardFWInterface* pInterface)
{
    uint8_t cType = 0; // feh data player
    std::this_thread::sleep_for(std::chrono::microseconds(fWait_us * 10));
    bool cIsRunning = this->IsRunning(pInterface, cType);
    if(cIsRunning)
    {
        LOG(INFO) << BOLDGREEN << " Data Player [RUNNING] .. going to stop" << RESET;
        pInterface->WriteReg("fc7_daq_ctrl.fast_command_block.control.stop_generic", 0x1);
        std::this_thread::sleep_for(std::chrono::microseconds(fWait_us * 10));
    }
    pInterface->WriteReg("fc7_daq_ctrl.fast_command_block.control.start_generic", 0x1);
    std::this_thread::sleep_for(std::chrono::microseconds(fWait_us * 10));
}
void DPInterface::GenerateDummyData()
{
    size_t             cNtrials       = 10;
    size_t             cNEvents       = 1000;
    float              cTriggerRate   = 150e3;
    float              cMeanNtriggers = cTriggerRate * (cNEvents * 25e-9);
    constexpr uint8_t  cHeaderSize    = 18;
    constexpr uint8_t  cErBitsSize    = 2;
    constexpr uint8_t  cNRows         = 127;
    constexpr uint8_t  cNCols         = 15;
    constexpr uint32_t cSClstrSize    = 7 + 3 + 1;
    constexpr uint32_t cPClstrSize    = 7 + 3 + 4;

    LOG(INFO) << BOLDBLUE << "Mean trigger rate is " << +cTriggerRate << " Expect to see  " << +cMeanNtriggers << " in " << +cNEvents << " events." << RESET;

    /* initialize random seed: */
    std::srand(std::time(NULL));

    std::random_device cRndm{};
    std::mt19937       cGen{cRndm()};

    // poisson distributed
    std::poisson_distribution<>        cDistTriggers(cMeanNtriggers);
    std::uniform_int_distribution<int> cFlatDist(0, cNEvents);

    // normally distributed hits
    std::normal_distribution<> cDNSclusters{2, 1};
    std::normal_distribution<> cDNPclusters{5, 1};

    cMeanNtriggers = 0;
    int cNtrgsSnt  = 0;
    // now generate T1 and data
    fFastCommands.clear();
    fInputCICL1Data.clear();
    int cL1Counter = 0;
    for(size_t cIndx = 0; cIndx < cNtrials; cIndx++)
    {
        int cL1Id      = 0;
        int cNTriggers = std::round(cDistTriggers(cGen));
        LOG(INFO) << BOLDBLUE << "This batch of " << +cNEvents << " have " << +cNTriggers << " triggers" << RESET;
        // randomly choose N integers
        // between 0 and nevents
        std::vector<int> cTrgrdEvents(cNTriggers, 0);
        std::vector<int> cNPclusters(cNTriggers, 0);
        std::vector<int> cNSclusters(cNTriggers, 0);
        for(size_t i = 0; i < cTrgrdEvents.size(); i++)
        {
            cTrgrdEvents[i] = std::round(cFlatDist(cGen));
            cNtrgsSnt++;
        }
        // sort triggered events
        std::sort(cTrgrdEvents.begin(), cTrgrdEvents.end());
        for(size_t i = 0; i < cTrgrdEvents.size(); i++)
        {
            cNSclusters[i] = 10; // std::round(cDNSclusters(cGen));
            cNPclusters[i] = 10; // std::round(cDNPclusters(cGen));

            LOG(DEBUG) << BOLDBLUE << "\t.. event #" << +cTrgrdEvents[i] << " will have a trigger "
                       << " of an event with " << +cNSclusters[i] << " s clusters and " << +cNPclusters[i] << " p clusters. " << RESET;
        }

        for(size_t cBx = 0; cBx < cNEvents; cBx++)
        {
            // found in list of triggered events
            bool cTrgdEvnt = (std::find(cTrgrdEvents.begin(), cTrgrdEvents.end(), cBx) != cTrgrdEvents.end());
            // empty fast command frame
            // no data from MPA
            if(!cTrgdEvnt)
            {
                // just push back 0s for this Bx
                std::vector<uint8_t> cMPAdata(8, 0); // one per FE
                fInputCICL1Data.push_back(cMPAdata);
                fFastCommands.push_back(0xC1);
            }
            else
            {
                // data
                std::string cData = "";
                // header
                for(size_t cIndx = 0; cIndx < cHeaderSize; cIndx++) { cData += "1"; }
                cData += "0";
                // two error bits
                for(size_t cIndx = 0; cIndx < cErBitsSize; cIndx++) { cData += "0"; }
                // L1 Id
                std::bitset<9> cLvl1Id(1 + cL1Counter);
                cData += cLvl1Id.to_string();
                cData += "0";
                std::bitset<5> cNClustersStrips{(unsigned long)cNSclusters[cL1Id]};
                std::bitset<5> cNClustersPixels{(unsigned long)cNPclusters[cL1Id]};
                cData += cNClustersStrips.to_string();
                cData += cNClustersPixels.to_string();
                cData += "0";
                // now add the Strip clusters
                for(size_t cCluster = 0; cCluster < (size_t)cNSclusters[cL1Id]; cCluster++)
                {
                    int cStripId = std::rand() % cNRows;

                    uint8_t        cScluster     = cStripId;
                    uint8_t        cScluterWidth = std::rand() % 7; // should be gaussian
                    uint8_t        cMip          = 0;
                    std::bitset<7> cStripCluster{(unsigned long)cScluster};
                    std::bitset<3> cStripClusterWdth{(unsigned long)cScluterWidth};
                    std::bitset<1> cMIPBit{(unsigned long)cMip};
                    cData += cStripCluster.to_string();
                    cData += cStripClusterWdth.to_string();
                    cData += cMIPBit.to_string();
                }
                // now add the pixel clusters
                for(size_t cCluster = 0; cCluster < (size_t)cNPclusters[cL1Id]; cCluster++)
                {
                    uint8_t        cPcluster     = std::rand() % cNRows;
                    uint8_t        cPcluterWidth = std::rand() % 7; // should be gaussian
                    uint8_t        cPZposition   = std::rand() % cNCols;
                    std::bitset<7> cPxlCluster{(unsigned long)cPcluster};
                    std::bitset<3> cPxlClusterWdth{(unsigned long)cPcluterWidth};
                    std::bitset<4> cPxlClusterZ{(unsigned long)cPZposition};

                    cData += cPxlCluster.to_string();
                    cData += cPxlClusterWdth.to_string();
                    cData += cPxlClusterZ.to_string();
                }
                // trailer
                cData += "0";
                size_t cNbits = cData.length();
                // figure out when you start transmitting
                size_t cFrstBx = cBx + 15; // start 15 Bx after the L1 trigger has been sent
                size_t cLstBx  = cFrstBx + std::ceil(cNbits / 8.);
                LOG(DEBUG) << BOLDBLUE << "Event#" << +cBx << "L1 Id " << +cL1Id << " generate data packet with " << +cNSclusters[cL1Id] << " s clusters " << +cNPclusters[cL1Id] << " p clusters "
                           << " L1 data is " << cData << " length of L1 data is " << +cNbits << " expect to have " << (cSClstrSize)*cNSclusters[cL1Id] + (cPClstrSize)*cNPclusters[cL1Id]
                           << " bits including cluster information"
                           << " which will take " << cNbits / (8.) << " Bx to transmit the data "
                           << " which means between " << +cFrstBx << " and " << +cLstBx << " Bxs." << RESET;

                for(size_t cId = cBx; cId < cFrstBx; cId++)
                {
                    if(cId == cBx)
                        fFastCommands.push_back(0xC9);
                    else
                        fFastCommands.push_back(0xC1);
                    std::vector<uint8_t> cMPAdata(8, 0); // one per FE
                    fInputCICL1Data.push_back(cMPAdata);
                }
                size_t      cBitId = 0;
                std::string cWrd   = "";
                size_t      cNWrds = 0;
                for(size_t cId = cFrstBx; cId < cLstBx; cId++)
                {
                    for(size_t cBitIdx = 0; cBitIdx < 8; cBitIdx++)
                    {
                        cWrd += (cBitId < cData.length()) ? cData[cBitId] : '0';
                        cBitId++;
                        if(cWrd.length() == 8)
                        {
                            // push back into FE data vector
                            auto                 cByte = std::stoi(cWrd, nullptr, 2);
                            std::vector<uint8_t> cMPAdata(8, cByte);
                            fInputCICL1Data.push_back(cMPAdata);
                            fFastCommands.push_back(0xC1);
                            LOG(DEBUG) << BOLDBLUE << "\t.. Wrd#" << +cNWrds << " - " << cWrd << " - " << +cByte << RESET;
                            cWrd = "";
                            cNWrds++;
                        }
                    }
                }
                LOG(DEBUG) << BOLDBLUE << "\t.. have " << +fInputCICL1Data.size() << " bytes of MPA data "
                           << " and " << +fFastCommands.size() << " T1 cmds." << RESET;
                cL1Id++;
                cL1Counter++;
            }
        }
        cMeanNtriggers += cNTriggers / (float)(cNtrials);
    }
    fMaxBx = fFastCommands.size();
    LOG(INFO) << BOLDBLUE << "Mean triggers " << cMeanNtriggers << " in " << +fFastCommands.size() << " simulated events "
              << " with " << +cNtrgsSnt << " triggers sent in total " << RESET;
}
bool DPInterface::ReadL1Data(BeBoardFWInterface* pInterface)
{
    // output data
    fFastCommands.clear();
    fInputCICL1Data.clear();
    // std::vector<uint8_t> cFastCommands;
    // std::vector<std::vector<uint8_t>> cMPAData;

    std::string   cMPATriggeredOutputFile = "./PRINT_MPA_L1_OUT_TO_FILE.log";
    std::ifstream cMPATriggeredOutputStream;
    cMPATriggeredOutputStream.open(cMPATriggeredOutputFile);
    bool cSuccess = cMPATriggeredOutputStream.good();
    if(!cSuccess)
    {
        LOG(INFO) << BOLDRED << "Could not open files from MPA-CIC verification framework" << RESET;
        return false;
    }
    else
    {
        // boost::tokenizer<boost::char_separator<char>> cSeperator(", ");
        typedef boost::tokenizer<boost::char_separator<char>> cTokenizer;
        boost::char_separator<char>                           cSeparator(";", "", boost::keep_empty_tokens);
        boost::char_separator<char>                           cEqSeperaror("=", "", boost::keep_empty_tokens);

        uint                     cLineIndx   = 0;
        uint8_t                  cBitCounter = 0;
        std::string              cT1Cmd      = ""; // fast command
        std::vector<std::string> cL1Data(8, "");   // L1 data one per MPA
        for(std::string cMPATriggeredOutput; getline(cMPATriggeredOutputStream, cMPATriggeredOutput);)
        {
            // remove whitespace
            cMPATriggeredOutput.erase(std::remove(cMPATriggeredOutput.begin(), cMPATriggeredOutput.end(), ' '), cMPATriggeredOutput.end());
            if(cLineIndx >= 169544 && cLineIndx <= 169713 && cT1Cmd.length() == 8)
                LOG(INFO) << BOLDBLUE << "Line#" << +cLineIndx << " : "
                          << "\t.." << cMPATriggeredOutput << "\t.. fast command " << cT1Cmd << "\t.. FE0 data " << cL1Data[0] << "\t.. FE1 data " << cL1Data[1] << "\t.. FE2 data " << cL1Data[2]
                          << "\t.. FE3 data " << cL1Data[3] << "\t.. FE4 data " << cL1Data[4] << "\t.. FE5 data " << cL1Data[5] << "\t.. FE6 data " << cL1Data[6] << "\t.. FE7 data " << cL1Data[7]
                          << "\t.. Bx " << fFastCommands.size() << RESET;

            cTokenizer cTokens(cMPATriggeredOutput, cSeparator);
            // if line is empty .. go on
            if(cTokens.begin() == cTokens.end()) continue;

            // parse line
            auto cTokenIterator = cTokens.begin();
            // get Bx Id
            auto       cBxStr = *cTokenIterator;
            cTokenizer cBxTokens(cBxStr, cEqSeperaror);
            cTokenIterator++;
            auto       cT1Str = *cTokenIterator;
            cTokenizer cT1Tokens(cT1Str, cEqSeperaror);
            cTokenIterator++;
            auto       cL1Str = *cTokenIterator;
            cTokenizer cL1DataTokens(cL1Str, cEqSeperaror);

            auto cT1TokensIterator = cT1Tokens.begin();
            cT1TokensIterator++;

            auto cL1DataTokensIterator = cL1DataTokens.begin();
            cL1DataTokensIterator++;

            if(cBitCounter % 8 != 0 || cBitCounter == 0)
            {
                cT1Cmd += (*cT1TokensIterator).c_str();
                for(uint8_t cHybridIndex = 0; cHybridIndex < 8; cHybridIndex++) cL1Data[cHybridIndex] += (*cL1DataTokensIterator).c_str()[7 - cHybridIndex];
            }
            else
            {
                // push into fast command vector
                std::bitset<8> cFastCommand(cT1Cmd);
                fFastCommands.push_back(cFastCommand.to_ulong());
                cT1Cmd = "";
                cT1Cmd += (*cT1TokensIterator).c_str();
                // push into data vector
                std::vector<uint8_t> cDataFromMPAs(0);
                for(uint8_t cHybridIndex = 0; cHybridIndex < 8; cHybridIndex++)
                {
                    std::bitset<8> cData(cL1Data[cHybridIndex]);
                    cDataFromMPAs.push_back(cData.to_ulong());
                    cL1Data[cHybridIndex] = "";
                    cL1Data[cHybridIndex] += (*cL1DataTokensIterator).c_str()[7 - cHybridIndex];
                }
                fInputCICL1Data.push_back(cDataFromMPAs);
            }
            cBitCounter++;
            cLineIndx++;
        }
    }

    size_t cNBxs = fFastCommands.size();
    LOG(INFO) << BOLDBLUE << "Read back data for " << +cNBxs << " BXs from file ... " << RESET;
    if(cNBxs < fMaxBx)
        this->SetMaxNBx(cNBxs);
    else
        this->SetMaxNBx(fMaxBx);

    return true;
}
uint16_t DPInterface::LoadL1Data(BeBoardFWInterface* pInterface)
{
    fNTriggers           = 0;
    uint8_t cHybridIndex = 0; // choose HybridId 0 from verification frameword
    size_t  cNBxs        = fMaxBx;
    LOG(INFO) << BOLDBLUE << "Configure CIC data player BRAM for L1 and T1 lines  for " << +cNBxs << " clock cycles." << RESET;
    // configure number of patterns to  play
    pInterface->WriteReg("fc7_daq_cnfg.fast_command_block.generic_fcmd_series.number_of_cmds_to_play", cNBxs);
    // write pattern to L1 and fast command BRAMs
    uint8_t cLine = 0;
    for(uint16_t cBx = 1; cBx <= cNBxs; cBx++)
    {
        // first data from MPAs
        char    cBuffer[100];
        uint8_t cBaseIndex = (cLine / 2) * 2;
        std::sprintf(cBuffer, "fc7_daq_cnfg.physical_interface_block.mpa_to_cic_data_player_line_%01d_%01d.line%d_pattern_7_to_0", cBaseIndex, cBaseIndex + 1, cLine);
        std::string                                   cReg0(cBuffer);
        std::vector<std::pair<std::string, uint32_t>> cRegs;
        char                                          cTmpBuffer[106];
        std::sprintf(cTmpBuffer, "fc7_daq_cnfg.physical_interface_block.mpa_to_cic_data_player_address_BRAM_line_%01d_%01d.line%01d_address", cBaseIndex, cBaseIndex + 1, cLine);
        std::string cRegAddr(cTmpBuffer);
        // MPA data
        cRegs.push_back({cReg0, fInputCICL1Data[cBx - 1][cHybridIndex]});
        // MPA data BRAM address
        cRegs.push_back({cRegAddr, cBx});

        // fast command BRAM data and address
        // bram only takes the fcmd code (so not the header and not the trailer)
        uint8_t cCode = (fFastCommands[cBx - 1] & (0xF << 1)) >> 1;
        LOG(DEBUG) << BOLDBLUE << "Fast command from verification framework is " << std::bitset<8>(fFastCommands[cBx - 1]) << " writing " << std::bitset<4>(cCode)
                   << " to generic fast command data player in address  " << (cBx) << " FE0 data " << std::bitset<8>(fInputCICL1Data[cBx - 1][cHybridIndex]) << RESET;

        cRegs.push_back({"fc7_daq_cnfg.fast_command_block.generic_fcmd_data", cCode});
        cRegs.push_back({"fc7_daq_cnfg.fast_command_block.generic_fcmd_addr", cBx});
        pInterface->WriteStackReg(cRegs);
        // write enable
        cRegs.clear();
        cRegs.push_back({"fc7_daq_ctrl.physical_interface_block.mpa_to_cic_data_player_BRAM.write_enable", (1 << cLine)});
        cRegs.push_back({"fc7_daq_ctrl.fast_command_block.control.wr_en_generic", 0x1});
        pInterface->WriteStackReg(cRegs);
        cRegs.clear();

        // back to 0
        cRegs.push_back({"fc7_daq_cnfg.fast_command_block.generic_fcmd_data", 0x00});
        cRegs.push_back({"fc7_daq_cnfg.fast_command_block.generic_fcmd_addr", 0x00});
        cRegs.push_back({cReg0, 0x00});
        cRegs.push_back({cRegAddr, 0x00});
        pInterface->WriteStackReg(cRegs);
        cRegs.clear();
        fNTriggers += (cCode == 0x04) ? 1 : 0;
    }
    pInterface->WriteReg("fc7_daq_cnfg.fast_command_block.generic_fcmd_series.number_of_cmds_to_play", cNBxs);
    pInterface->WriteReg("fc7_daq_cnfg.fast_command_block.generic_fcmd_series.number_of_series_to_play", 0x1);

    // check
    // this->CheckFcmdBRAM(pInterface);
    return fNTriggers;
}
void DPInterface::CheckFcmdBRAM(BeBoardFWInterface* pInterface)
{
    for(uint16_t cBx = 0; cBx < fMaxBx; cBx++)
    {
        // configure address and data
        pInterface->WriteReg("fc7_daq_cnfg.fast_command_block.generic_fcmd_addr", cBx);
        auto cDebug   = pInterface->ReadReg("fc7_daq_stat.fast_command_block.generic_fcmd_debug");
        auto cAddress = (cDebug & 0x3FFF);
        auto cData    = (cDebug & (0xF << 14)) >> 14;
        LOG(INFO) << BOLDBLUE << "Bx" << +cBx << " BRAM contains " << std::bitset<32>(cDebug) << " so write T1 " << std::bitset<4>(cData) << " to BRAM address " << +cAddress << RESET;
    }
}
uint16_t DPInterface::SendTriggers(BeBoardFWInterface* pInterface)
{
    fNTriggers = 0;
    // // make sure the block is in the idle state
    // make sure the block is reset
    pInterface->WriteReg("fc7_daq_ctrl.fast_command_block.control.reset", 0x1);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    //
    pInterface->WriteReg("fc7_daq_ctrl.fast_command_block.control.start_generic", 0x0);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // configure number of patterns to  play
    // write pattern to L1 and fast command BRAMs
    pInterface->WriteReg("fc7_daq_cnfg.fast_command_block.generic_fcmd_addr", 0x00);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::vector<std::pair<std::string, uint32_t>> cRegs;
    for(uint16_t cBx = 1; cBx <= fMaxBx; cBx++)
    {
        uint8_t cCode = ((cBx + 1) % 100 == 0) ? 0x4 : 0x0;
        fNTriggers += (cCode == 0x4) ? 1 : 0;

        // configure address and data
        std::vector<std::pair<std::string, uint32_t>> cRegs;
        cRegs.push_back({"fc7_daq_cnfg.fast_command_block.generic_fcmd_data", (cCode & 0x0F)});
        cRegs.push_back({"fc7_daq_cnfg.fast_command_block.generic_fcmd_addr", cBx});
        pInterface->WriteStackReg(cRegs);
        cRegs.clear();

        // write enable
        pInterface->WriteReg("fc7_daq_ctrl.fast_command_block.control.wr_en_generic", 0x1);
        // auto cDebug = pInterface->ReadReg("fc7_daq_stat.fast_command_block.generic_fcmd_debug");
        // LOG(DEBUG) << BOLDBLUE << "Bx" << +cBx << " -- fast command code " << std::bitset<4>(cCode) << " BRAM debug contains " << std::bitset<32>(cDebug) << RESET;

        // back to 0
        cRegs.push_back({"fc7_daq_cnfg.fast_command_block.generic_fcmd_data", 0x00});
        cRegs.push_back({"fc7_daq_cnfg.fast_command_block.generic_fcmd_addr", 0x00});
        pInterface->WriteStackReg(cRegs);
        cRegs.clear();
    }

    //
    pInterface->WriteReg("fc7_daq_cnfg.fast_command_block.generic_fcmd_series.number_of_cmds_to_play", fMaxBx);
    pInterface->WriteReg("fc7_daq_cnfg.fast_command_block.generic_fcmd_series.number_of_series_to_play", 0x1);

    // pInterface->WriteReg( "fc7_daq_ctrl.fast_command_block.control.start_generic", 0x1 ) ;
    // uint32_t cGenFCMDdone = 0;
    // uint32_t cNTriggers=0;
    // do
    // {
    //     cGenFCMDdone = pInterface->ReadReg ("fc7_daq_stat.fast_command_block.general.generic_fcmd_done");
    //     cNTriggers = pInterface->ReadReg("fc7_daq_stat.fast_command_block.trigger_in_counter");
    //     LOG (INFO) << BOLDBLUE << "After sending start signal : Done bit of FCMD Player is "
    //         << +cGenFCMDdone
    //         << "\n\t\t\t # of triggers sent : " << +cNTriggers << RESET;
    //     std::this_thread::sleep_for (std::chrono::milliseconds (100) );
    // }while( cGenFCMDdone != 0x01 );
    // LOG (INFO) << BOLDBLUE << "uDTC sent " << +cNTriggers << " triggers when I've asked for "
    //   << +fNTriggers
    //   << RESET;

    // this->CheckFcmdBRAM(pInterface);
    return fNTriggers;
}
void DPInterface::ConfigureLineBRAM(BeBoardFWInterface* pInterface, uint8_t pLine, uint8_t pPattern, uint16_t pNumberOfClks)
{
    LOG(INFO) << BOLDBLUE << "Configure CIC data player BRAM for line " << +pLine << RESET;
    LOG(INFO) << BOLDBLUE << "Playing... " << std::bitset<8>(pPattern) << " for " << +pNumberOfClks << " clock cycles." << RESET;
    // write the same pattern to all BRAMs
    // first - configure pattern
    char    cBuffer[100];
    uint8_t cBaseIndex = (pLine / 2) * 2;
    std::sprintf(cBuffer, "fc7_daq_cnfg.physical_interface_block.mpa_to_cic_data_player_line_%01d_%01d.line%d_pattern_7_to_0", cBaseIndex, cBaseIndex + 1, pLine);
    std::string cReg0(cBuffer);
    pInterface->WriteReg(cReg0, pPattern);
    std::this_thread::sleep_for(std::chrono::microseconds(fWait_us * 10));
    // now configure how many clock cycles to play this pattern for
    // i.e. write this constant pattern to the BRAM for pNumberOfClks
    // addresses
    // address 0 is reserved
    for(uint16_t cIndx = 0; cIndx < pNumberOfClks; cIndx++)
    {
        if((cIndx) % 100 == 0) LOG(INFO) << BOLDBLUE << "\t... writing entry#" << +cIndx << RESET;
        char cTmpBuffer[106];
        std::sprintf(cTmpBuffer, "fc7_daq_cnfg.physical_interface_block.mpa_to_cic_data_player_address_BRAM_line_%01d_%01d.line%01d_address", cBaseIndex, cBaseIndex + 1, pLine);
        // select address
        std::string cRegAddr(cTmpBuffer);
        pInterface->WriteReg(cRegAddr, cIndx + 1);
        std::this_thread::sleep_for(std::chrono::microseconds(fWait_us * 10));
        // write enable
        pInterface->WriteReg("fc7_daq_ctrl.physical_interface_block.mpa_to_cic_data_player_BRAM.write_enable", (1 << pLine));
        std::this_thread::sleep_for(std::chrono::microseconds(fWait_us * 10));
    }
    // also write this number of patterns to fcmd generic
    // player
    pInterface->WriteReg("fc7_daq_cnfg.fast_command_block.generic_fcmd_series.number_of_cmds_to_play", pNumberOfClks);
    pInterface->WriteReg("fc7_daq_cnfg.fast_command_block.generic_fcmd_series.number_of_series_to_play", 0x1);
}
void DPInterface::ConstPatternBRAMs(BeBoardFWInterface* pInterface, std::vector<uint8_t> pPatterns, uint16_t pNumberOfClks)
{
    // configure a constant pattern on N lines
    uint8_t                                       cLine = 0;
    std::vector<std::pair<std::string, uint32_t>> cPtrnCnfgRegs;
    for(auto cPattern: pPatterns)
    {
        LOG(INFO) << BOLDBLUE << "Configure CIC data player BRAM for line " << +cLine << "to play... " << std::bitset<8>(cPattern) << " for " << +pNumberOfClks << " clock cycles." << RESET;
        // write the same pattern to all BRAMs
        // first - configure pattern
        // on all lines
        char    cBuffer[100];
        uint8_t cBaseIndex = (cLine / 2) * 2;
        std::sprintf(cBuffer, "fc7_daq_cnfg.physical_interface_block.mpa_to_cic_data_player_line_%01d_%01d.line%d_pattern_7_to_0", cBaseIndex, cBaseIndex + 1, cLine);
        std::string cReg0(cBuffer);
        cPtrnCnfgRegs.push_back({cReg0, cPattern});
        cLine++;
    }
    pInterface->WriteStackReg(cPtrnCnfgRegs);

    // now configure how many clock cycles to play this pattern for
    // i.e. write this constant pattern to the BRAM for pNumberOfClks
    // addresses
    // address 0 is reserved
    for(uint16_t cBx = 0; cBx < pNumberOfClks; cBx++)
    {
        if((cBx) % 100 == 0) LOG(INFO) << BOLDBLUE << "\t... writing entry#" << +cBx << RESET;

        std::vector<std::pair<std::string, uint32_t>> cBramAddrs;
        uint8_t                                       cEnableReg = 0;
        for(uint8_t cLineIndx = 0; cLineIndx < cLine; cLineIndx++)
        {
            uint8_t cBaseIndex = (cLineIndx / 2) * 2;
            char    cTmpBuffer[106];
            std::sprintf(cTmpBuffer, "fc7_daq_cnfg.physical_interface_block.mpa_to_cic_data_player_address_BRAM_line_%01d_%01d.line%01d_address", cBaseIndex, cBaseIndex + 1, cLineIndx);
            // select address
            std::string cRegAddr(cTmpBuffer);
            cEnableReg = (cEnableReg) | (1 << cLineIndx);
            cBramAddrs.push_back({cTmpBuffer, cBx + 1});
        }
        pInterface->WriteStackReg(cBramAddrs);
        // write enable
        pInterface->WriteReg("fc7_daq_ctrl.physical_interface_block.mpa_to_cic_data_player_BRAM.write_enable", cEnableReg);
    }

    // zero
    // 0 data and address
    uint8_t                                       cEnableReg = 0;
    std::vector<std::pair<std::string, uint32_t>> cRegs;
    for(uint8_t cLineIndx = 0; cLineIndx < cLine; cLineIndx++)
    {
        uint8_t cBaseIndex = (cLineIndx / 2) * 2;
        char    cTmpBuffer[106];
        std::sprintf(cTmpBuffer, "fc7_daq_cnfg.physical_interface_block.mpa_to_cic_data_player_address_BRAM_line_%01d_%01d.line%01d_address", cBaseIndex, cBaseIndex + 1, cLineIndx);
        // select address
        std::string cRegAddr(cTmpBuffer);
        cEnableReg = (cEnableReg) | (1 << cLineIndx);
        cRegs.push_back({cTmpBuffer, 0x00});

        char cBuffer[100];
        std::sprintf(cBuffer, "fc7_daq_cnfg.physical_interface_block.mpa_to_cic_data_player_line_%01d_%01d.line%d_pattern_7_to_0", cBaseIndex, cBaseIndex + 1, cLineIndx);
        std::string cPatternReg(cBuffer);
        cRegs.push_back({cPatternReg, 0x00});
    }
    pInterface->WriteStackReg(cRegs);
    pInterface->WriteReg("fc7_daq_ctrl.physical_interface_block.mpa_to_cic_data_player_BRAM.write_enable", cEnableReg);
    cRegs.clear();

    pInterface->WriteReg("fc7_daq_cnfg.fast_command_block.generic_fcmd_series.number_of_cmds_to_play", pNumberOfClks);
    pInterface->WriteReg("fc7_daq_cnfg.fast_command_block.generic_fcmd_series.number_of_series_to_play", 0x1);
}
void DPInterface::Configure(BeBoardFWInterface* pInterface, uint32_t pPattern, uint16_t pFrequency)
{
    std::vector<uint8_t> pPatterns;

    for(int i = 0; i < 5; i++) pPatterns.push_back(pPattern);

    if(pFrequency == 320)
        LOG(INFO) << BOLDBLUE << std::bitset<8>(pPattern) << " pattern to use in PS ROH data player." << RESET;
    else
        LOG(INFO) << BOLDBLUE << std::bitset<16>(pPattern) << " pattern to use in PS ROH data player." << RESET;

    if(pFrequency == 320) // ConstPatternBRAMs(pInterface, pPatterns, 1);
        pInterface->WriteReg("fc7_daq_cnfg.physical_interface_block.data_player.pattern_320MHz", pPattern);
    else // ConstPatternBRAMs(pInterface, pPatterns, 1);
        pInterface->WriteReg("fc7_daq_cnfg.physical_interface_block.data_player.pattern_640MHz", pPattern);
    std::this_thread::sleep_for(std::chrono::microseconds(fWait_us * 10));
}
void DPInterface::ConfigureLine(BeBoardFWInterface* pInterface, uint16_t pPattern, uint8_t pLine)
{
    char    cBuffer[76];
    uint8_t cBaseIndex = (pLine / 2) * 2;
    std::sprintf(cBuffer, "fc7_daq_cnfg.physical_interface_block.mpa_to_cic_data_player_line_%01d_%01d", cBaseIndex, cBaseIndex + 1);

    char cBuffer1[30];
    std::sprintf(cBuffer1, "line%01d_pattern_7_to_0", pLine);

    std::string cReg0(cBuffer1);
    std::string cRegister0(cBuffer);
    cRegister0 += "." + cReg0;
    uint8_t cValue_Reg0 = (pPattern & (0xFF << 0)) >> 0;
    pInterface->WriteReg(cRegister0, cValue_Reg0);
    std::this_thread::sleep_for(std::chrono::microseconds(fWait_us * 10));
    uint8_t cReg0Val = pInterface->ReadReg(cRegister0);
    LOG(INFO) << BOLDGREEN << cRegister0 << ": " << std::bitset<8>(cReg0Val) << RESET;
}

void DPInterface::CheckNPatterns(BeBoardFWInterface* pInterface)
{
    auto cInput = pInterface->ReadReg("fc7_daq_stat.physical_interface_block.mpa_to_cic_cntr");
    LOG(INFO) << BOLDGREEN << " Data Player sent : " << +cInput << " patterns." << RESET;
}
void DPInterface::Start(BeBoardFWInterface* pInterface, uint8_t pType)
{
    pInterface->WriteReg("fc7_daq_ctrl.physical_interface_block.data_player.start_data_player", 0x01);

    std::this_thread::sleep_for(std::chrono::microseconds(fWait_us * 10));
    bool cIsRunning = this->IsRunning(pInterface, pType);
    if(cIsRunning) { LOG(INFO) << BOLDGREEN << " Data Player [STARTED]" << RESET; }
    else { LOG(INFO) << BOLDRED << " Data Player [START ERROR]" << RESET; }
}

bool DPInterface::IsRunning(BeBoardFWInterface* pInterface, uint8_t pType)
{
    std::string cRegName = (pType == 0) ? "fc7_daq_stat.physical_interface_block.data_player.stat_feh_data_player" : "fc7_daq_stat.physical_interface_block.data_player.stat_roh_data_player";
    // std::string cRegName = "fc7_daq_stat.physical_interface_block.data_player.stat_roh_data_player";
    fEmulatorRunning = (pInterface->ReadReg(cRegName) == 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    return fEmulatorRunning;
}
void DPInterface::Stop(BeBoardFWInterface* pInterface)
{
    pInterface->WriteReg("fc7_daq_ctrl.physical_interface_block.data_player.stop_data_player", 0x01);
    std::this_thread::sleep_for(std::chrono::microseconds(fWait_us * 10));
    LOG(INFO) << BOLDBLUE << "FE data player for PS ROH stopped." << RESET;
}

} // namespace Ph2_HwInterface
