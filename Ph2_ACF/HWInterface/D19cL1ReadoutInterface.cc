#include "HWInterface/D19cL1ReadoutInterface.h"
#include "HWInterface/RegManager.h"
#include "HWInterface/TriggerInterface.h"
#include "Utils/ConsoleColor.h"
#include "Utils/easylogging++.h"
#include <bitset>
#include <thread>

using namespace Ph2_HwDescription;

namespace Ph2_HwInterface
{
D19cL1ReadoutInterface::D19cL1ReadoutInterface(RegManager* theRegManager) : L1ReadoutInterface(theRegManager)
{
    // handshake should always be off for this readout mode
    fHandshake = fTheRegManager->ReadReg("fc7_daq_cnfg.readout_block.global.data_handshake_enable");
}

D19cL1ReadoutInterface::~D19cL1ReadoutInterface() {}

bool D19cL1ReadoutInterface::ResetReadout()
{
    LOG(DEBUG) << BOLDBLUE << "D19cL1ReadoutInterface Resetting readout..." << RESET;
    fTheRegManager->WriteReg("fc7_daq_ctrl.readout_block.control.readout_reset", 0x1);
    std::this_thread::sleep_for(std::chrono::microseconds(fWait_us));
    fTheRegManager->WriteReg("fc7_daq_ctrl.readout_block.control.readout_reset", 0x0);
    std::this_thread::sleep_for(std::chrono::microseconds(fWait_us));

    LOG(DEBUG) << BOLDBLUE << "Resetting DDR3 " << RESET;
    fDDR3Offset              = 0;
    auto     cDDR3Calibrated = (fTheRegManager->ReadReg("fc7_daq_stat.ddr3_block.init_calib_done") == 1);
    uint16_t cAttempts       = 0;
    uint16_t cMaxAttempts    = 1000;

    while(!cDDR3Calibrated && (cAttempts < cMaxAttempts))
    {
        LOG(DEBUG) << "Waiting for DDR3 to finish initial calibration";
        std::this_thread::sleep_for(std::chrono::microseconds(fWait_us));
        cDDR3Calibrated = (fTheRegManager->ReadReg("fc7_daq_stat.ddr3_block.init_calib_done") == 1);
        cAttempts++;
    }
    return cDDR3Calibrated;
}

void D19cL1ReadoutInterface::CountFwEvents()
{
    fNReadoutEvents = 0;
    if(fData.size() == 0) return;

    std::vector<uint32_t> cValidData(0);
    cValidData.clear();
    auto   cEventIterator = fData.begin();
    bool   cFoundEmpty    = false;
    size_t cOffset        = 0;
    size_t cCorr          = 0;
    do {
        // check event header
        uint32_t cFirstWord = *cEventIterator;
        uint32_t cHeader    = ((0xFFFF << 16) & cFirstWord) >> 16;
        if(cHeader != 0xFFFF)
        {
            if(!cFoundEmpty && cFirstWord == 0)
            {
                cFoundEmpty = true;
                cCorr       = cOffset; // how far from start
            }
            LOG(INFO) << BOLDMAGENTA << "Event header " << std::bitset<16>(cHeader) << " not EXPECTED" << RESET;
            cEventIterator = fData.end();
        }
        else
        {
            uint32_t cEventSize = (0x0000FFFF & (*cEventIterator)) * 4; // event size is given in 128 bit words
            // uint32_t cDummyCount = (0xFF & (*(cEventIterator + 1))) * 4;
            // LOG(DEBUG) << BOLDMAGENTA << "Valid event header .. copying over "
            //           << " event is made up of " << +cEventSize << " 32 bit words "
            //           << " of which " << +cDummyCount << " are dummy words." << RESET;
            // for(size_t cIndx = 0; cIndx < cEventSize; cIndx++) LOG(DEBUG) << BOLDYELLOW << "\t..." << std::bitset<32>(*(cEventIterator + cIndx)) << RESET;
            std::copy(fData.begin() + cOffset, fData.begin() + cOffset + cEventSize, std::back_inserter(cValidData));
            cEventIterator += cEventSize;
            cOffset += cEventSize;
            fNReadoutEvents++;
        }
    } while(cEventIterator < fData.end());
    cCorr = (cFoundEmpty) ? fData.size() - cCorr : cCorr;
    fData.clear();
    // LOG (INFO) << BOLDMAGENTA << "Original data vector has " << cOriginalDataSize
    //     << " 32-bit words..  found empty word in position "
    //     << cCorr
    //     << " from end of vector"
    //     << RESET;
    // adjust offset to first empty slot in DDR3
    if(cFoundEmpty)
    {
        LOG(INFO) << BOLDMAGENTA << "Found an empty event .. all 0s .. resetting DDR3 offset" << RESET;
        // fDDR3Offset = (cFoundEmpty) ? cFoundEmpty - cOffset : fDDR3Offset;
    }
    if(cValidData.size() == 0) return;

    std::move(cValidData.begin(), cValidData.end(), std::back_inserter(fData));
}

bool D19cL1ReadoutInterface::WaitForReadout()
{
    bool cFailed    = true;
    auto cNWords    = fTheRegManager->ReadReg("fc7_daq_stat.readout_block.general.words_cnt");
    auto cStartTime = std::chrono::high_resolution_clock::now(), cEndTime = cStartTime;
    auto cDuration = std::chrono::duration_cast<std::chrono::microseconds>(cEndTime - cStartTime).count();
    if(!fWaitForReadoutReq) // wait until words in the readout have stopped inreasing
    {
        // LOG (INFO) << BOLDMAGENTA << "D19cL1ReadoutInterface::WaitForReadout Now checking words from the FC7" << RESET;
        uint32_t cNWordsPrev    = cNWords;
        bool     cStopIncrement = false;
        do {
            std::this_thread::sleep_for(std::chrono::microseconds(fWait_us));
            cNWords        = fTheRegManager->ReadReg("fc7_daq_stat.readout_block.general.words_cnt");
            cStopIncrement = (cNWords == cNWordsPrev);
            cEndTime       = std::chrono::high_resolution_clock::now();
            cDuration      = std::chrono::duration_cast<std::chrono::microseconds>(cEndTime - cStartTime).count();

        } while(!cStopIncrement && cDuration < fTimeout_us);
        cFailed = (cNWords == 0);
        if(cFailed) LOG(ERROR) << BOLDRED << "D19cL1ReadoutInterface::WaitForReadout no words in the readout ..[ReadoutAttempt#" << fReadoutAttempt << "]" << RESET;
    }
    else // send triggers until the readout request flag is '1'
    {
        cFailed = !CheckReadoutReq();
        if(cFailed) LOG(ERROR) << BOLDRED << "D19cL1ReadoutInterface::WaitForReadout readout request not fullfilled ..[ReadoutAttempt#" << fReadoutAttempt << "]" << RESET;
    }
    return !cFailed;
}
bool D19cL1ReadoutInterface::WaitForNTriggers()
{
    // fTriggerInterface->ResetTriggerFSM();

    // wait for trigger state machine to send all triggers
    // auto cTriggerSource = this->fTheRegManager->ReadReg("fc7_daq_cnfg.fast_command_block.trigger_source"); // trigger source
    // LOG(DEBUG) << BOLDYELLOW << "D19cL1ReadoutInterface::WaitForNTriggers After resetting trigger FSM.. trigger source is " << cTriggerSource << RESET;
    // LOG(DEBUG) << BOLDYELLOW << "D19cL1ReadoutInterface::WaitForNTriggers Running Trigger FSM ..." << RESET;
    bool cSuccess = fTriggerInterface->RunTriggerFSM();
    if(!cSuccess) LOG(ERROR) << BOLDRED << "D19cL1ReadoutInterface timed-out while waiting for trigger FSM...[ReadoutAttempt#" << fReadoutAttempt << "]" << RESET;
    return cSuccess;
}

bool D19cL1ReadoutInterface::WaitForData() { return true; }
bool D19cL1ReadoutInterface::CheckReadoutReq()
{
    uint32_t cIterations = 0;
    auto     cStartTime = std::chrono::high_resolution_clock::now(), cEndTime = cStartTime;
    auto     cDuration   = std::chrono::duration_cast<std::chrono::microseconds>(cEndTime - cStartTime).count();
    auto     cReadoutReq = fTheRegManager->ReadReg("fc7_daq_stat.readout_block.general.readout_req");
    do {
        std::this_thread::sleep_for(std::chrono::microseconds(fWait_us));
        cReadoutReq = fTheRegManager->ReadReg("fc7_daq_stat.readout_block.general.readout_req");
        // LOG(DEBUG) << BOLDYELLOW << "D19cL1ReadoutInterface::CheckReadoutReq ReadoutReq is " << +cReadoutReq << RESET;
        cEndTime  = std::chrono::high_resolution_clock::now();
        cDuration = std::chrono::duration_cast<std::chrono::microseconds>(cEndTime - cStartTime).count();
        cIterations++;
    } while(cReadoutReq == 0 && cDuration < fTimeout_us);
    if(cReadoutReq == 0)
    {
        LOG(ERROR) << BOLDRED << "Readout request 0 [i.e words missing in the readout] ...[ReadoutAttempt#" << fReadoutAttempt << "]" << RESET;
        LOG(ERROR) << BOLDRED << "Read request FSM status = 0x" << std::hex << fTheRegManager->ReadReg("fc7_daq_stat.readout_block.general.fsm_status") << std::dec << RESET;
    }
    // else
    //     LOG(DEBUG) << BOLDGREEN << "ReadoutReq fullfilled.... " << RESET;
    return (cReadoutReq == 1);
}

bool D19cL1ReadoutInterface::CheckBuffers()
{
    // read all the words
    if(fTheRegManager->ReadReg("fc7_daq_stat.ddr3_block.is_ddr3_type"))
    {
        // readout_req high when buffer is almost full
        uint32_t cReadoutReq = fTheRegManager->ReadReg("fc7_daq_stat.readout_block.general.readout_req");
        return (cReadoutReq == 1); // DD3 almost full ? check this!!
    }
    return false;
}

bool D19cL1ReadoutInterface::CheckForWordsInReadout()
{
    uint32_t cIterations = 0;
    auto     cStartTime = std::chrono::high_resolution_clock::now(), cEndTime = cStartTime;
    auto     cDuration = std::chrono::duration_cast<std::chrono::microseconds>(cEndTime - cStartTime).count();
    auto     cNWords   = fTheRegManager->ReadReg("fc7_daq_stat.readout_block.general.words_cnt");
    do {
        std::this_thread::sleep_for(std::chrono::microseconds(fWait_us));
        cNWords = fTheRegManager->ReadReg("fc7_daq_stat.readout_block.general.words_cnt");
        // LOG(DEBUG) << BOLDYELLOW << "D19cL1ReadoutInterface::CheckForWordsInReadout words_cnt is " << +cNWords << RESET;
        cEndTime  = std::chrono::high_resolution_clock::now();
        cDuration = std::chrono::duration_cast<std::chrono::microseconds>(cEndTime - cStartTime).count();
        cIterations++;
    } while(cNWords == 0 && cDuration < fTimeout_us);
    if(cNWords == 0) { LOG(INFO) << BOLDRED << "No words in the readout .... " << RESET; }
    else
        LOG(INFO) << BOLDGREEN << "Found words in the readout ... " << RESET;
    return (cNWords > 0);
}
void D19cL1ReadoutInterface::FillData()
{
    fHandshake = fTheRegManager->ReadReg("fc7_daq_cnfg.readout_block.global.data_handshake_enable");
    fData.clear();
    uint32_t cNWords = fTheRegManager->ReadReg("fc7_daq_stat.readout_block.general.words_cnt");
    if(cNWords == 0) return;

    fData = fTheRegManager->ReadBlockRegOffset("fc7_daq_ddr3", cNWords, fDDR3Offset);
    // LOG(DEBUG) << BOLDGREEN << +cNWords << " words read back from DDR3 memory " << RESET;
    fDDR3Offset += cNWords;
    // LOG(DEBUG) << BOLDGREEN << "\t... " << +fDDR3Offset << " current offset in DDR3 " << RESET;
}
bool D19cL1ReadoutInterface::PollReadoutData(const BeBoard* pBoard, bool pWait)
{
    // LOG(DEBUG) << BOLDYELLOW << "D19cL1ReadoutInterface::PollReadoutData " << RESET;
    fData.clear();
    // here check if the trigger state machine is running
    if(fTriggerInterface->GetTriggerState() == 0) // 0, idle - 1 running
    {
        LOG(INFO) << BOLDRED << "Triggers not running.. no data to read " << RESET;
        return false;
    }

    bool cSuccess = !pWait;
    if(pWait)
    {
        if(fTriggerInterface->WaitForNTriggers(1)) // wait until at least 1 trigger has been sent
        {
            cSuccess = CheckForWordsInReadout();
        }
    }

    if(cSuccess)
    {
        FillData();
        CountFwEvents();
        // LOG(DEBUG) << BOLDYELLOW << "D19cL1ReadoutInterface::PollReadoutData " << fData.size() << " valid 32 bit words .. which are " << +fNReadoutEvents << " events." << RESET;
    }
    return cSuccess;
}
bool D19cL1ReadoutInterface::ReadEvents(const BeBoard* pBoard)
{
    // LOG(DEBUG) << BOLDYELLOW << "D19cL1ReadoutInterface::ReadEvents " << fNEvents << RESET;
    fReadoutAttempt = 0;
    bool cSuccess   = true;
    do {
        // clear internal data vector
        fData.clear();
        // configure readout
        fHandshake = 1;
        fTheRegManager->WriteReg("fc7_daq_cnfg.readout_block.global.data_handshake_enable", fHandshake);
        // auto cHandshake = fTheRegManager->ReadReg("fc7_daq_cnfg.readout_block.global.data_handshake_enable");
        // write number of triggers to accept
        // in the handshake mode offset is cleared after each handshake
        auto cMultiplicity = fTheRegManager->ReadReg("fc7_daq_cnfg.fast_command_block.misc.trigger_multiplicity");
        fNEvents           = fNEvents * (cMultiplicity + 1);
        fTriggerInterface->SetNTriggersToAccept(fNEvents);
        fTheRegManager->WriteReg("fc7_daq_cnfg.readout_block.packet_nbr", fNEvents - 1);
        // LOG(DEBUG) << BOLDYELLOW << "D19cL1ReadoutInterface::ReadEvents asking for " << fNEvents << " events handshake mode is currently " << cHandshake << RESET;

        // reset readout
        ResetReadout();
        cSuccess = WaitForNTriggers();
        cSuccess = cSuccess && WaitForReadout();
        cSuccess = cSuccess && CheckReadoutReq();

        if(cSuccess) // fill data vector + count FW events
        {
            FillData();
            CountFwEvents();
            // LOG(DEBUG) << BOLDYELLOW << "D19cL1ReadoutInterface::ReadEvent " << fData.size() << " valid 32 bit words .. which are " << +fNReadoutEvents << " events." << RESET;
        }

        fReadoutAttempt++;
    } while(fReadoutAttempt < fMaxAttempts && !cSuccess);

    if(fReadoutAttempt > 1) LOG(INFO) << BOLDYELLOW << "D19cL1ReadoutInterface::ReadEvents.. took " << (fReadoutAttempt) << " attempts to readout L1 data." << RESET;
    return (fData.size() > 0);
}

} // namespace Ph2_HwInterface
