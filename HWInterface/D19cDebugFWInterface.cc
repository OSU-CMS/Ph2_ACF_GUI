#include "HWInterface/D19cDebugFWInterface.h"
#include "HWInterface/RegManager.h"
#include "Utils/ConsoleColor.h"
#include "Utils/easylogging++.h"
#include <bitset>
#include <thread>

using namespace Ph2_HwInterface;

namespace Ph2_HwInterface
{
D19cDebugFWInterface::D19cDebugFWInterface(RegManager* theRegManager) : fTheRegManager(theRegManager) {}

D19cDebugFWInterface::~D19cDebugFWInterface() {}

std::vector<uint32_t> D19cDebugFWInterface::L1ADebug(uint8_t pWait_ms, bool pPrint)
{
    if(pPrint) LOG(INFO) << BOLDBLUE << "D19cDebugFWInterface::L1ADebug ...." << RESET;
    // enable initial fast reset
    fTheRegManager->WriteReg("fc7_daq_cnfg.fast_command_block.misc.initial_fast_reset_enable", 0);

    fTheRegManager->WriteReg("fc7_daq_cnfg.physical_interface_block.slvs_debug.auto_l1_capture", 1);
    // fTheRegManager->WriteReg("fc7_daq_cnfg.physical_interface_block.slvs_debug.slvs_debug_l1_delay", 1);

    // disable back-pressure
    fTheRegManager->WriteReg("fc7_daq_cnfg.fast_command_block.misc.backpressure_enable", 0);

    int  iteration = 0;
    bool dataReady = false;
    while(!dataReady && iteration++ < 100)
    {
        fTheRegManager->WriteReg("fc7_daq_ctrl.fast_command_block.control.stop_trigger", 0x1);
        // reset trigger
        fTheRegManager->WriteReg("fc7_daq_ctrl.fast_command_block.control.reset", 0x1);
        // load new trigger configuration
        fTheRegManager->WriteReg("fc7_daq_ctrl.fast_command_block.control.load_config", 0x1);
        // std::cout<< __PRETTY_FUNCTION__ << " [" << __LINE__ << "] enable = " << +fTheRegManager->ReadReg("fc7_daq_cnfg.sync_block.enable") << std::endl;
        // std::cout<< __PRETTY_FUNCTION__ << " [" << __LINE__ << "] is_bc0_enable = " << +fTheRegManager->ReadReg("fc7_daq_cnfg.sync_block.is_bc0_enable") << std::endl;
        // std::cout<< __PRETTY_FUNCTION__ << " [" << __LINE__ << "] is_resync_enable = " << +fTheRegManager->ReadReg("fc7_daq_cnfg.sync_block.is_resync_enable") << std::endl;
        // std::cout<< __PRETTY_FUNCTION__ << " [" << __LINE__ << "] delay = " << +fTheRegManager->ReadReg("fc7_daq_cnfg.sync_block.delay") << std::endl;
        // std::cout<< __PRETTY_FUNCTION__ << " [" << __LINE__ << "] num_triggers = " << +fTheRegManager->ReadReg("fc7_daq_cnfg.sync_block.num_triggers") << std::endl;
        fTheRegManager->WriteReg("fc7_daq_ctrl.fast_command_block.control.start_trigger", 0x1);
        if(pPrint) LOG(INFO) << BOLDBLUE << "Started triggers ...." << RESET;
        // wait until you've received at least one trigger
        auto cNTriggersRxd = fTheRegManager->ReadReg("fc7_daq_stat.fast_command_block.trigger_in_counter");
        auto cStartTime = std::chrono::high_resolution_clock::now(), cEndTime = cStartTime;
        auto cDuration = std::chrono::duration_cast<std::chrono::microseconds>(cEndTime - cStartTime).count();
        do {
            cEndTime      = std::chrono::high_resolution_clock::now();
            cDuration     = std::chrono::duration_cast<std::chrono::microseconds>(cEndTime - cStartTime).count();
            cNTriggersRxd = fTheRegManager->ReadReg("fc7_daq_stat.fast_command_block.trigger_in_counter");
            if(pPrint) LOG(INFO) << BOLDMAGENTA << "Trigger in counter is " << +cNTriggersRxd << " waited for " << cDuration << " us so far" << RESET;
        } while(cNTriggersRxd < 3 && cDuration < pWait_ms * 1e3);
        fTheRegManager->WriteReg("fc7_daq_ctrl.fast_command_block.control.stop_trigger", 0x1);
        fTotalNumberOfTriggers += fTheRegManager->ReadReg("fc7_daq_stat.fast_command_block.trigger_in_counter");

        // LOG(DEBUG) << BOLDMAGENTA << "First header found after " << fTheRegManager->ReadReg("fc7_daq_stat.physical_interface_block.slvs_debug.first_header_delay") << " clock cycles." << RESET;

        std::this_thread::sleep_for(std::chrono::microseconds(10));
        dataReady = fTheRegManager->ReadReg("fc7_daq_stat.physical_interface_block.slvs_debug.slvs_debug_l1_ready") == 1;
    }
    // if(iteration > 1) std::cout << __PRETTY_FUNCTION__ << " [" << __LINE__ << "] iteration = " << iteration << std::endl;
    // if(!dataReady) std::cout << __PRETTY_FUNCTION__ << " [" << __LINE__ << "] Data not ready!!!!!!!!" << std::endl;

    auto cWords = fTheRegManager->ReadBlockReg("fc7_daq_stat.physical_interface_block.l1a_debug", 50);
    if(pPrint)
    {
        std::string cBuffer   = "";
        size_t      cLineIndx = 0;
        for(auto cWord: cWords)
        {
            auto                     cString = std::bitset<32>(cWord).to_string();
            std::vector<std::string> cOutputWords(0);
            for(size_t cIndex = 0; cIndex < 4; cIndex++) { cOutputWords.push_back(cString.substr(cIndex * 8, 8)); }
            std::string cOutput = "";
            for(auto cIt = cOutputWords.end() - 1; cIt >= cOutputWords.begin(); cIt--)
            {
                cOutput += *cIt + " ";
                cBuffer += *cIt;
            }
            LOG(INFO) << BOLDBLUE << "#" << +cLineIndx << ":" << cOutput << RESET;
            cLineIndx++;
        }
    }

    return cWords;
}

std::vector<std::vector<uint32_t>> D19cDebugFWInterface::StubDebug(bool pWithTestPulse, uint8_t pNlines, bool pPrint)
{
    // LOG(DEBUG) << BOLDBLUE << "D19cDebugFWInterface::StubDebug ...." << RESET;

    uint8_t cReSync   = 0;
    uint8_t cCalPulse = 0;
    uint8_t cL1A      = 0;
    uint8_t cBC0      = 0;
    uint8_t cDuration = 0;
    if(pWithTestPulse) { cCalPulse = 1; }
    else
        cL1A = 1;

    uint32_t encode_resync    = cReSync << 16;
    uint32_t encode_cal_pulse = cCalPulse << 17;
    uint32_t encode_l1a       = cL1A << 18;
    uint32_t encode_bc0       = cBC0 << 19;
    uint32_t encode_duration  = cDuration << 28;
    uint32_t final_command    = encode_resync + encode_l1a + encode_cal_pulse + encode_bc0 + encode_duration;
    fTheRegManager->WriteReg("fc7_daq_ctrl.fast_command_block.control", final_command);

    auto cWords = fTheRegManager->ReadBlockReg("fc7_daq_stat.physical_interface_block.stub_debug", 80);
    // LOG(DEBUG) << BOLDBLUE << "Captured stub debug  ...." << RESET;

    std::vector<std::vector<uint32_t>> lineWordVector(pNlines);

    std::vector<std::string> cLines(0);
    size_t                   cLine = 0;
    do {
        // std::cout<< __PRETTY_FUNCTION__ << " [" << __LINE__ << "] Line " << cLine << " words " << std::hex;
        std::vector<std::string> cOutputWords(0);
        for(size_t cIndex = 0; cIndex < 10; cIndex++)
        {
            auto cWord = cWords.at(cLine * 10 + cIndex);
            // std::cout << cWord << " ";
            lineWordVector.at(cLine).push_back(cWord);
            auto cString = std::bitset<32>(cWord).to_string();
            for(size_t cOffset = 0; cOffset < 4; cOffset++) { cOutputWords.push_back(cString.substr(cOffset * 8, 8)); }
        }
        // std::cout << std::dec << std::endl;

        std::string cOutput_wSpace = "";
        std::string cOutput        = "";
        for(auto cIt = cOutputWords.end() - 1; cIt >= cOutputWords.begin(); cIt--)
        {
            cOutput_wSpace += *cIt + " ";
            cOutput += *cIt;
        }
        if(pPrint) LOG(INFO) << BOLDBLUE << "Line " << +cLine << " : " << cOutput_wSpace << RESET;
        cLines.push_back(cOutput);
        // cStrLength = cOutput.length();
        cLine++;
    } while(cLine < pNlines);
    return lineWordVector;
}
std::vector<std::string> D19cDebugFWInterface::ScopeStubLines(bool pWithTestPulse)
{
    LOG(INFO) << BOLDBLUE << "D19cDebugFWInterface::ScopeStubLines ...." << RESET;
    uint8_t cReSync   = 0;
    uint8_t cCalPulse = 0;
    uint8_t cL1A      = 0;
    uint8_t cBC0      = 0;
    uint8_t cDuration = 0;
    if(pWithTestPulse) { cCalPulse = 1; }
    else { cL1A = 1; }
    uint32_t encode_resync    = cReSync << 16;
    uint32_t encode_cal_pulse = cCalPulse << 17;
    uint32_t encode_l1a       = cL1A << 18;
    uint32_t encode_bc0       = cBC0 << 19;
    uint32_t encode_duration  = cDuration << 28;
    uint32_t final_command    = encode_resync + encode_l1a + encode_cal_pulse + encode_bc0 + encode_duration;
    fTheRegManager->WriteReg("fc7_daq_ctrl.fast_command_block.control", final_command);

    auto                     cWords = fTheRegManager->ReadBlockReg("fc7_daq_stat.physical_interface_block.stub_debug", 80);
    std::vector<std::string> cLines(0);
    size_t                   cLine   = 0;
    size_t                   cNlines = 6;
    // int cStrLength=0;
    do {
        std::vector<std::string> cOutputWords(0);
        for(size_t cIndex = 0; cIndex < cNlines; cIndex++)
        {
            auto cWord   = cWords.at(cLine * 10 + cIndex);
            auto cString = std::bitset<32>(cWord).to_string();
            for(size_t cOffset = 0; cOffset < 4; cOffset++) { cOutputWords.push_back(cString.substr(cOffset * 8, 8)); }
        }

        std::string cOutput_wSpace = "";
        std::string cOutput        = "";
        for(auto cIt = cOutputWords.end() - 1; cIt >= cOutputWords.begin(); cIt--)
        {
            cOutput_wSpace += *cIt + " ";
            cOutput += *cIt;
        }
        // LOG(DEBUG) << BOLDBLUE << "Line " << +cLine << " : " << cOutput_wSpace << RESET;
        cLines.push_back(cOutput);
        cLine++;
    } while(cLine < cNlines);
    return cLines;
}
} // namespace Ph2_HwInterface