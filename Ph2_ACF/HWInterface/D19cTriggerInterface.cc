#include "HWInterface/D19cTriggerInterface.h"
#include "HWInterface/RegManager.h"
#include "Utils/ConsoleColor.h"
#include "Utils/easylogging++.h"
#include <cmath>

namespace Ph2_HwInterface
{
D19cTriggerInterface::D19cTriggerInterface(RegManager* theRegManager) : TriggerInterface(theRegManager)
{
    LOG(INFO) << BOLDYELLOW << "D19cTriggerInterface::D19cTriggerInterface Constructor" << RESET;
}

D19cTriggerInterface::~D19cTriggerInterface() {}

void D19cTriggerInterface::PrintStatus()
{
    // temporary used for board status printing
    LOG(INFO) << YELLOW << "============================" << RESET;
    LOG(INFO) << BOLDBLUE << "Current Status" << RESET;

    int    source_id      = fTheRegManager->ReadReg("fc7_daq_stat.fast_command_block.general.source");
    double user_frequency = fTheRegManager->ReadReg("fc7_daq_cnfg.fast_command_block.user_trigger_frequency");

    if(source_id == 1)
        LOG(INFO) << "Trigger Source: " << BOLDGREEN << "L1-Trigger" << RESET;
    else if(source_id == 2)
        LOG(INFO) << "Trigger Source: " << BOLDGREEN << "Stubs" << RESET;
    else if(source_id == 3)
        LOG(INFO) << "Trigger Source: " << BOLDGREEN << "User Frequency (" << user_frequency << " kHz)" << RESET;
    else if(source_id == 4)
        LOG(INFO) << "Trigger Source: " << BOLDGREEN << "TLU" << RESET;
    else if(source_id == 5)
        LOG(INFO) << "Trigger Source: " << BOLDGREEN << "Ext Trigger (DIO5)" << RESET;
    else if(source_id == 6)
        LOG(INFO) << "Trigger Source: " << BOLDGREEN << "Test Pulse Trigger" << RESET;
    else
        LOG(WARNING) << " Trigger Source: " << BOLDRED << "Unknown" << RESET;

    int state_id = GetTriggerState();

    if(state_id == 0)
        LOG(INFO) << "Trigger State: " << BOLDGREEN << "Idle" << RESET;
    else if(state_id == 1)
        LOG(INFO) << "Trigger State: " << BOLDGREEN << "Running" << RESET;
    else if(state_id == 2)
        LOG(INFO) << "Trigger State: " << BOLDGREEN << "Paused. Waiting for readout" << RESET;
    else
        LOG(WARNING) << " Trigger State: " << BOLDRED << "Unknown" << RESET;
}
void D19cTriggerInterface::TriggerConfiguration()
{
    fTriggerConfiguration.fTriggerSource = fTheRegManager->ReadReg("fc7_daq_stat.fast_command_block.general.source");
    auto cSource                         = fTheRegManager->ReadReg("fc7_daq_cnfg.fast_command_block.trigger_source");
    if(fTriggerConfiguration.fTriggerSource != cSource)
    {
        LOG(WARNING) << WARNING_FORMAT << "Mismatch in trigger source configuration... going to reload and check again " << RESET;
        std::vector<std::pair<std::string, uint32_t>> cRegVec;
        LOG(WARNING) << WARNING_FORMAT << "Re-configuring trigger source to be " << +cSource << RESET;
        cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.trigger_source", cSource});
        cRegVec.push_back({"fc7_daq_ctrl.fast_command_block.control.load_config", 0x1});
        fTheRegManager->WriteStackReg(cRegVec);
        TriggerConfiguration();
    }
    else
    {
        fTriggerConfiguration.fTriggerRate       = fTheRegManager->ReadReg("fc7_daq_cnfg.fast_command_block.user_trigger_frequency");
        fTriggerConfiguration.fNtriggersToAccept = fTheRegManager->ReadReg("fc7_daq_cnfg.fast_command_block.triggers_to_accept");
        // LOG(DEBUG) << BOLDGREEN << "Trigger source is : " << +cSource << " matches configured source " << +fTriggerConfiguration.fTriggerSource << " number of triggers to accept is "
        //            << +fTriggerConfiguration.fNtriggersToAccept << RESET;
    }
}

uint32_t D19cTriggerInterface::GetTriggerState()
{
    int cState = fTheRegManager->ReadReg("fc7_daq_stat.fast_command_block.general.fsm_state");
    // if(cState == 0)
    //     LOG(DEBUG) << "Trigger State: " << BOLDGREEN << "Idle" << RESET;
    // else if(cState == 1)
    //     LOG(DEBUG) << "Trigger State: " << BOLDGREEN << "Running" << RESET;
    // else if(cState == 2)
    //     LOG(DEBUG) << "Trigger State: " << BOLDGREEN << "Paused. Waiting for readout" << RESET;
    // else
    //     LOG(WARNING) << " Trigger State: " << BOLDRED << "Unknown" << RESET;
    return cState;
}
bool D19cTriggerInterface::Stop()
{
    // here close the shutter for the stub counter block
    fTheRegManager->WriteReg("fc7_daq_ctrl.stub_counter_block.general.shutter_close", 0x1);
    fTheRegManager->WriteReg("fc7_daq_ctrl.stub_counter_block.general.shutter_close", 0x0);
    std::this_thread::sleep_for(std::chrono::microseconds(fWait_us));

    auto cTriggerState = GetTriggerState();
    do {
        // LOG(DEBUG) << BOLDBLUE << "D19cFWInterface::Stop Trigger state is " << cTriggerState << RESET;
        fTheRegManager->WriteReg("fc7_daq_ctrl.fast_command_block.control.stop_trigger", 0x1);
        std::this_thread::sleep_for(std::chrono::microseconds(fWait_us));
        cTriggerState = GetTriggerState();
    } while(cTriggerState == 1);

    return (cTriggerState == 0);
}
// reconfigure trigger
void D19cTriggerInterface::ResetTriggerFSM()
{
    // LOG(DEBUG) << BOLDYELLOW << "D19cTriggerInterface::ResetTriggerFSM" << RESET;
    this->Stop();

    // reset trigger
    fTheRegManager->WriteReg("fc7_daq_ctrl.fast_command_block.control.reset", 0x1);
    std::this_thread::sleep_for(std::chrono::microseconds(fWait_us * 1));
    // load new trigger configuration
    fTheRegManager->WriteReg("fc7_daq_ctrl.fast_command_block.control.load_config", 0x1);
    std::this_thread::sleep_for(std::chrono::microseconds(fWait_us * 1));

    // check trigger source and rate
    this->TriggerConfiguration();
}
void D19cTriggerInterface::Pause()
{
    LOG(INFO) << BOLDBLUE << "................................ Pausing run ... " << RESET;
    fTheRegManager->WriteReg("fc7_daq_ctrl.fast_command_block.control.stop_trigger", 0x1);
    std::this_thread::sleep_for(std::chrono::microseconds(fWait_us));
}

void D19cTriggerInterface::Resume()
{
    LOG(INFO) << BOLDBLUE << "................................ Resuming run ... " << RESET;
    fTheRegManager->WriteReg("fc7_daq_ctrl.fast_command_block.control.start_trigger", 0x1);
    std::this_thread::sleep_for(std::chrono::microseconds(fWait_us));
}

bool D19cTriggerInterface::Start(bool checkIfStarted)
{
    // LOG(DEBUG) << BOLDYELLOW << "................................Starting triggers  ... " << RESET;
    // auto cTriggerState = GetTriggerState();
    // LOG(DEBUG) << BOLDYELLOW << "D19cTriggerInterface::Start - trigger state is " << cTriggerState << RESET;
    // this stops triggers  + resets
    this->ResetTriggerFSM();

    // here open the shutter for the stub counter block (for some reason self clear doesn't work, that why we have to
    // clear the register manually)
    fTheRegManager->WriteReg("fc7_daq_ctrl.stub_counter_block.general.shutter_open", 0x1);
    fTheRegManager->WriteReg("fc7_daq_ctrl.stub_counter_block.general.shutter_open", 0x0);
    std::this_thread::sleep_for(std::chrono::microseconds(fWait_us));

    size_t iterationNumber            = 0;
    size_t maximumnNumberOfIterations = 10;
    while(iterationNumber < maximumnNumberOfIterations)
    {
        fTheRegManager->WriteReg("fc7_daq_ctrl.fast_command_block.control.start_trigger", 0x1);
        std::this_thread::sleep_for(std::chrono::microseconds(fWait_us));
        if(!checkIfStarted) break;
        if(GetTriggerState() == 1) break;
        ++iterationNumber;
    }

    if(iterationNumber >= maximumnNumberOfIterations)
    {
        LOG(ERROR) << ERROR_FORMAT << "Failed to start triggers" << RESET;
        return false;
    }

    std::this_thread::sleep_for(std::chrono::microseconds(fWait_us));
    return true;
}
// configure number of triggers to accept
bool D19cTriggerInterface::SetNTriggersToAccept(uint32_t pNTriggersToAccept)
{
    std::vector<std::pair<std::string, uint32_t>> cTriggerConfig;
    cTriggerConfig.push_back({"fc7_daq_cnfg.fast_command_block.triggers_to_accept", pNTriggersToAccept});
    ReconfigureTriggerFSM(cTriggerConfig);
    return (fTheRegManager->ReadReg("fc7_daq_cnfg.fast_command_block.triggers_to_accept") == pNTriggersToAccept);
}

// reconfigure trigger
void D19cTriggerInterface::ReconfigureTriggerFSM(std::vector<std::pair<std::string, uint32_t>> pTriggerConfig)
{
    // reset trigger
    fTheRegManager->WriteReg("fc7_daq_ctrl.fast_command_block.control.reset", 0x1);
    std::this_thread::sleep_for(std::chrono::microseconds(fWait_us));
    // configure
    fTheRegManager->WriteStackReg(pTriggerConfig);
    std::this_thread::sleep_for(std::chrono::microseconds(fWait_us));
    // load new trigger configuration
    fTheRegManager->WriteReg("fc7_daq_ctrl.fast_command_block.control.load_config", 0x1);
    std::this_thread::sleep_for(std::chrono::microseconds(fWait_us));
}
bool D19cTriggerInterface::SendNTriggers(uint32_t pNTriggers)
{
    // count triggers sent to the CIC
    uint32_t cTimeSingleTrigger_us = std::ceil(1.5 / fTheRegManager->ReadReg("fc7_daq_cnfg.fast_command_block.user_trigger_frequency"));
    uint32_t cTimeoutValue         = cTimeSingleTrigger_us * pNTriggers * 10;
    bool     cAllTriggersSent      = false;
    size_t   cAttempt              = 0;
    size_t   cMaxAttempts          = 10;
    this->ResetTriggerFSM();
    do {
        this->Start();
        auto cStartTime = std::chrono::high_resolution_clock::now(), cEndTime = cStartTime;
        auto cDuration      = std::chrono::duration_cast<std::chrono::microseconds>(cEndTime - cStartTime).count();
        auto cNTriggersSent = fTheRegManager->ReadReg("fc7_daq_stat.fast_command_block.trigger_in_counter");
        do {
            std::this_thread::sleep_for(std::chrono::microseconds(fWait_us));
            cNTriggersSent   = fTheRegManager->ReadReg("fc7_daq_stat.fast_command_block.trigger_in_counter");
            cAllTriggersSent = (cNTriggersSent >= pNTriggers);
            cEndTime         = std::chrono::high_resolution_clock::now();
            cDuration        = std::chrono::duration_cast<std::chrono::microseconds>(cEndTime - cStartTime).count();
        } while(!cAllTriggersSent && cDuration < cTimeoutValue);
        this->ResetTriggerFSM();
        cAttempt++;
        this->Stop();
    } while(!cAllTriggersSent && cAttempt < cMaxAttempts);
    return cAllTriggersSent;
}
bool D19cTriggerInterface::WaitForNTriggers(uint32_t pNTriggers)
{
    bool cFailed = false;
    TriggerConfiguration();
    uint32_t cNtriggers            = fTheRegManager->ReadReg("fc7_daq_stat.fast_command_block.trigger_in_counter");
    uint32_t cTimeSingleTrigger_us = std::ceil(1.5 / (fTriggerConfiguration.fTriggerRate));
    uint32_t cTimeoutValue         = cTimeSingleTrigger_us * pNTriggers * 10;
    // wait until all triggers received
    uint32_t cNtriggersPrev = cNtriggers;
    size_t   cFoundSame     = 0;
    // size_t   cCounter       = 0;
    do {
        std::this_thread::sleep_for(std::chrono::microseconds(fWait_us * 10));
        cNtriggers = fTheRegManager->ReadReg("fc7_daq_stat.fast_command_block.trigger_in_counter");
        cFoundSame += (cNtriggers == cNtriggersPrev) ? 1 : 0;
        cNtriggersPrev = cNtriggers;
        // if(cCounter % 100 == 0) LOG(DEBUG) << BOLDRED << "D19cL1ReadoutInterface::WaitForReadout Number of triggers received is " << +cNtriggers << RESET;
        // cCounter++;
    } while(cNtriggers < pNTriggers && cFoundSame < cTimeoutValue);
    cFailed = !(cNtriggers >= pNTriggers);
    if(cFailed)
    {
        auto cState = GetTriggerState();
        LOG(INFO) << BOLDRED << "Trigger FSM failed to receive all triggers .. expected " << +pNTriggers << " and received " << +cNtriggers << " FSM state is " << +cState << " .. re-trying" << RESET;
    }
    return !cFailed;
}
bool D19cTriggerInterface::RunTriggerFSM()
{
    this->Start();
    auto cRunningTime      = 0;
    bool cCheckRunningTime = true;
    // check if trigger state machine is running
    if(cCheckRunningTime)
    {
        do {
            std::this_thread::sleep_for(std::chrono::microseconds(fWait_us));
            cRunningTime = fTheRegManager->ReadReg("fc7_daq_stat.fast_command_block.running_time");
            if(cRunningTime == 0)
            {
                LOG(INFO) << BOLDRED << " Trigger FSM not running (running time == 0) despite start.. will stop and try again" << RESET;
                this->Start();
                std::this_thread::sleep_for(std::chrono::microseconds(fWait_us));
            }
            // else
            //     LOG(DEBUG) << BOLDGREEN << " Trigger FSM (running time == " << cRunningTime << " ) " << RESET;
        } while(cRunningTime == 0);
    }

    // wait until all triggers have been received or
    auto        cNTriggersToAccept = fTheRegManager->ReadReg("fc7_daq_cnfg.fast_command_block.triggers_to_accept");
    std::string cRegName           = (cNTriggersToAccept == 0) ? "fc7_daq_stat.fast_command_block.general.fsm_state" : "fc7_daq_stat.fast_command_block.trigger_in_counter";
    uint32_t    cRegValue          = (cNTriggersToAccept == 0) ? 0 : cNTriggersToAccept;
    float       cMaxTime           = (cNTriggersToAccept == 0) ? 30. : 0.;
    fTheRegManager->pollRegister(cRegName, cRegValue, cMaxTime);
    bool cFailed    = GetTriggerState();
    auto cNtriggers = fTheRegManager->ReadReg("fc7_daq_stat.fast_command_block.trigger_in_counter");
    if(cFailed) LOG(WARNING) << BOLDRED << "D19cTriggerInterface::RunTriggerFSM " << cNtriggers << " triggers received. FAILED set to " << cFailed << RESET;
    this->Stop();
    // return true;
    return !cFailed;
}

} // namespace Ph2_HwInterface
