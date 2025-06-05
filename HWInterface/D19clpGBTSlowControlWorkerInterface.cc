#include "HWInterface/D19clpGBTSlowControlWorkerInterface.h"
#include "HWDescription/Chip.h"
#include "HWDescription/ChipRegItem.h"
#include "HWInterface/RegManager.h"
#include "Utils/ConsoleColor.h"

namespace Ph2_HwInterface
{
D19clpGBTSlowControlWorkerInterface::D19clpGBTSlowControlWorkerInterface(RegManager* theRegManager) : D19cCommandProcessorInterface(theRegManager)
{
    PrintState();
    LOG(INFO) << BOLDYELLOW << "D19clpGBTSlowControlWorkerInterface::D19clpGBTSlowControlWorkerInterface Constructor" << RESET;
}

D19clpGBTSlowControlWorkerInterface::~D19clpGBTSlowControlWorkerInterface() {}

void D19clpGBTSlowControlWorkerInterface::Reset()
{
    uint32_t                              sleepTimeInUs = 100000;
    std::lock_guard<std::recursive_mutex> theGuard(fTheRegManager->fMutex);
    LOG(DEBUG) << BOLDBLUE << "Resetting Command Processor" << RESET;
    // Soft reset the GBT-SC worker
    std::vector<uint32_t> cCommandVector;
    cCommandVector.clear();
    uint8_t cWorkerId = 0, cFunctionId = 2;
    // reset should be 0x00020010
    cCommandVector.push_back(cWorkerId << 24 | cFunctionId << 16 | 16 << 0);

    fTheRegManager->WriteReg("fc7_daq_ctrl.command_processor_block.i2c.control.reset_fifos", 1);
    usleep(sleepTimeInUs);
    fTheRegManager->WriteReg("fc7_daq_ctrl.command_processor_block.i2c.control.reset_fifos", 0);
    usleep(sleepTimeInUs);

    fTheRegManager->WriteReg("fc7_daq_ctrl.command_processor_block.cpb_ctrl_reg.command_fifo_reset", 1);
    usleep(sleepTimeInUs);
    fTheRegManager->WriteReg("fc7_daq_ctrl.command_processor_block.cpb_ctrl_reg.command_fifo_reset", 0);
    usleep(sleepTimeInUs);

    fTheRegManager->WriteReg("fc7_daq_ctrl.command_processor_block.cpb_ctrl_reg.reply_fifo_reset", 1);
    usleep(sleepTimeInUs);
    fTheRegManager->WriteReg("fc7_daq_ctrl.command_processor_block.cpb_ctrl_reg.reply_fifo_reset", 0);
    usleep(sleepTimeInUs);

    fTheRegManager->WriteBlockReg("fc7_daq_ctrl.command_processor_block.cpb_command_fifo", cCommandVector);
    usleep(sleepTimeInUs);
    fTheRegManager->ReadBlockReg("fc7_daq_ctrl.command_processor_block.cpb_reply_fifo", 10);
    usleep(sleepTimeInUs);

    fTheRegManager->WriteReg("fc7_daq_ctrl.command_processor_block.cpb_ctrl_reg.state_reset", 1);
    usleep(sleepTimeInUs);
    fTheRegManager->WriteReg("fc7_daq_ctrl.command_processor_block.cpb_ctrl_reg.state_reset", 0);
    usleep(sleepTimeInUs);

    fTheRegManager->WriteReg("fc7_daq_ctrl.command_processor_block.i2c.control.reset", 1);
    usleep(sleepTimeInUs);
    fTheRegManager->WriteReg("fc7_daq_ctrl.command_processor_block.i2c.control.reset", 0);
    usleep(sleepTimeInUs);

    fTheRegManager->WriteReg("fc7_daq_ctrl.command_processor_block.cpb_ctrl_reg.core_reset", 1);
    usleep(sleepTimeInUs);
    fTheRegManager->WriteReg("fc7_daq_ctrl.command_processor_block.cpb_ctrl_reg.core_reset", 0);
    usleep(sleepTimeInUs);
}

void D19clpGBTSlowControlWorkerInterface::SelectLink(uint8_t pLinkId) { fTheRegManager->WriteReg("fc7_daq_cnfg.optical_block.link_select", pLinkId); }
std::vector<uint32_t>
D19clpGBTSlowControlWorkerInterface::EncodeCommand(uint8_t pFunctionId, Ph2_HwDescription::Chip* pChip, const std::vector<Ph2_HwDescription::ChipRegItem>& pRegisterItems, bool pVerify)
{
    std::vector<uint32_t> cCommand;
    uint8_t               cWorkerId = LpGBTSlowControlWorker::BASE_ID + pChip->getOpticalGroupId();
    uint8_t               cChipId   = pChip->getFrontEndType() == FrontEndType::CIC2 ? 0 : (pChip->getId() % 8);
    uint8_t               cChipCode = pChip->getChipCode();
    uint8_t               cMasterId = pChip->getMasterId();
    uint16_t              cNWords   = pRegisterItems.size();

    // fill command header (if needed)
    switch(pFunctionId)
    {
    case LpGBTSlowControlWorker::READ_IC: cCommand.push_back(cWorkerId << 24 | pFunctionId << 16 | cNWords << 0); break;

    case LpGBTSlowControlWorker::WRITE_IC: cCommand.push_back(cWorkerId << 24 | pFunctionId << 16 | cNWords << 0); break;

    case LpGBTSlowControlWorker::READ_FE:
        cCommand.push_back(cWorkerId << 24 | pFunctionId << 16 | (cNWords + 1) << 0);
        cCommand.push_back(cChipCode << 29 | cChipId << 26 | cMasterId << 24 | pVerify << 23);
        break;

    case LpGBTSlowControlWorker::WRITE_FE:
        cCommand.push_back(cWorkerId << 24 | pFunctionId << 16 | (cNWords + 1) << 0);
        cCommand.push_back(cChipCode << 29 | cChipId << 26 | cMasterId << 24 | pVerify << 23);
        break;

    default:
        LOG(ERROR) << "D19clpGBTSlowControlWorkerInterface::EncodeCommand Header : LpGBT-SC Worker fuction doesn't exist" << RESET;
        throw std::runtime_error("D19clpGBTSlowControlWorkerInterface::EncodeCommand failure");
    }

    // fill command payload
    for(auto& cRegItem: pRegisterItems)
    {
        switch(pFunctionId)
        {
        case LpGBTSlowControlWorker::READ_IC: cCommand.push_back(cRegItem.fAddress << 16); break;

        case LpGBTSlowControlWorker::WRITE_IC: cCommand.push_back(cRegItem.fAddress << 16 | cRegItem.fValue << 8); break;

        case LpGBTSlowControlWorker::READ_FE: cCommand.push_back(cRegItem.fAddress << 16); break;

        case LpGBTSlowControlWorker::WRITE_FE: cCommand.push_back(cRegItem.fAddress << 16 | cRegItem.fValue << 8); break;

        default:
            LOG(ERROR) << "D19clpGBTSlowControlWorkerInterface::EncodeCommand Payload : LpGBT-SC Worker fuction doesn't exist" << RESET;
            throw std::runtime_error("D19clpGBTSlowControlWorkerInterface::EncodeCommand failure");
        }
    }
    return cCommand;
}

std::vector<uint32_t>
D19clpGBTSlowControlWorkerInterface::EncodeCommandI2C(uint8_t pFunctionId, Ph2_HwDescription::Chip* pChip, uint8_t pMasterId, uint8_t pMasterConfig, const std::vector<uint32_t>& pSlaveData)
{
    std::vector<uint32_t> cCommand;
    uint8_t               cWorkerId = LpGBTSlowControlWorker::BASE_ID + pChip->getOpticalGroupId();
    uint16_t              cNWords   = pSlaveData.size();

    // fill command header
    cCommand.push_back(cWorkerId << 24 | pFunctionId << 16 | (cNWords + 1) << 0);
    cCommand.push_back(pMasterId << 30 | pMasterConfig << 22);

    // fill command payload
    for(auto& cData: pSlaveData)
    {
        uint8_t  cSlaveAddress = (cData & (0xFF << 0)) >> 0;
        uint32_t cSlaveData    = (cData & (0xFFFFFF << 8)) >> 8;
        cCommand.push_back(cSlaveAddress << 24 | cSlaveData << 0);
    }
    return cCommand;
}

void D19clpGBTSlowControlWorkerInterface::PrintState()
{
    std::lock_guard<std::recursive_mutex> theGuard(fTheRegManager->fMutex);
    auto cCommandState = CommandProcessorArbitrators::COMMAND_ARBITRATOR_FSM_STATE_MAP.at(fTheRegManager->ReadReg("fc7_daq_stat.command_processor_block.command_arbitrator_fsm_state"));
    auto cReplyState   = CommandProcessorArbitrators::REPLY_ARBITRATOR_FSM_STATE_MAP.at(fTheRegManager->ReadReg("fc7_daq_stat.command_processor_block.reply_arbitrator_fsm_state"));
    auto cWorkerState  = LpGBTSlowControlWorker::WORKER_FSM_STATE_MAP.at(fTheRegManager->ReadReg("fc7_daq_stat.command_processor_block.worker.lpgbtsc_fsm_state.worker_state"));
    auto cICState      = LpGBTSlowControlWorker::IC_FSM_STATE_MAP.at(fTheRegManager->ReadReg("fc7_daq_stat.command_processor_block.worker.lpgbtsc_fsm_state.ic_state"));
    auto cI2CState     = LpGBTSlowControlWorker::I2C_FSM_STATE_MAP.at(fTheRegManager->ReadReg("fc7_daq_stat.command_processor_block.worker.lpgbtsc_fsm_state.i2c_state"));
    auto cFEState      = LpGBTSlowControlWorker::FE_FSM_STATE_MAP.at(fTheRegManager->ReadReg("fc7_daq_stat.command_processor_block.worker.lpgbtsc_fsm_state.fe_state"));
    LOG(INFO) << BLUE << "Command Arbitrator State = " << BOLDYELLOW << cCommandState << RESET;
    LOG(INFO) << BLUE << "Reply Arbitrator State = " << BOLDYELLOW << cReplyState << RESET;
    LOG(INFO) << BLUE << "Worker state = " << BOLDYELLOW << cWorkerState << RESET;
    LOG(INFO) << BLUE << "IC State = " << BOLDYELLOW << cICState << RESET;
    LOG(INFO) << BLUE << "I2C State = " << BOLDYELLOW << cI2CState << RESET;
    LOG(INFO) << BLUE << "FE State = " << BOLDYELLOW << cFEState << RESET;
}

bool D19clpGBTSlowControlWorkerInterface::WaitDone(uint8_t pFunctionId)
{
    // int cWaitCounter = 1500; // Irene checking connection
    int cWaitCounter = 150000;
    while(!IsDone(pFunctionId) && (cWaitCounter != 0))
    {
        cWaitCounter--;
        // usleep(1000); // Irene checking connection
        continue;
    }
    if(cWaitCounter == 0)
    {
        LOG(INFO) << BOLDBLUE << "Status before reset:" << RESET;
        PrintState();
        Reset();
        LOG(INFO) << BOLDBLUE << "Status after reset:" << RESET;
        PrintState();
        // throw std::runtime_error("D19c lpGBT Slow Control Worker is stuck");
        return false;
    }
    return true;
}

bool D19clpGBTSlowControlWorkerInterface::IsDone(uint8_t pFunctionId)
{
    std::lock_guard<std::recursive_mutex> theGuard(fTheRegManager->fMutex);
    uint8_t                               cState        = fTheRegManager->ReadReg("fc7_daq_stat.command_processor_block.worker.lpgbtsc_fsm_state.worker_state");
    bool                                  cWorkerDone   = (cState == 1);
    bool                                  cFunctionDone = false;
    if((pFunctionId == LpGBTSlowControlWorker::READ_IC) || (pFunctionId == LpGBTSlowControlWorker::WRITE_IC))
    {
        uint8_t cState = fTheRegManager->ReadReg("fc7_daq_stat.command_processor_block.worker.lpgbtsc_fsm_state.ic_state");
        cFunctionDone  = (cState == 1);
    }
    else if((pFunctionId == LpGBTSlowControlWorker::SINGLE_BYTE_READ_I2C) || (pFunctionId == LpGBTSlowControlWorker::MULTI_BYTE_WRITE_I2C))
    {
        uint8_t cState = fTheRegManager->ReadReg("fc7_daq_stat.command_processor_block.worker.lpgbtsc_fsm_state.i2c_state");
        cFunctionDone  = (cState == 1);
    }
    else if((pFunctionId == LpGBTSlowControlWorker::READ_FE) || (pFunctionId == LpGBTSlowControlWorker::WRITE_FE))
    {
        uint8_t cState = fTheRegManager->ReadReg("fc7_daq_stat.command_processor_block.worker.lpgbtsc_fsm_state.fe_state");
        cFunctionDone  = (cState == 1);
    }
    else
    {
        LOG(ERROR) << "D19clpGBTSlowControlWorkerInterface::IsDone : LpGBT-SC Worker fuction doesn't exist" << RESET;
        throw std::runtime_error("D19clpGBTSlowControlWorkerInterface::IsDone failure");
    }
    return cWorkerDone && cFunctionDone;
}

uint8_t D19clpGBTSlowControlWorkerInterface::GetTryCounter(uint8_t pFunctionId)
{
    std::lock_guard<std::recursive_mutex> theGuard(fTheRegManager->fMutex);
    uint8_t                               cCntr = 255;
    if((pFunctionId == LpGBTSlowControlWorker::READ_IC) || (pFunctionId == LpGBTSlowControlWorker::WRITE_IC))
    {
        cCntr = fTheRegManager->ReadReg("fc7_daq_stat.command_processor_block.worker.lpgbtsc_try_counters.ic_tool");
    }
    else if((pFunctionId == LpGBTSlowControlWorker::SINGLE_BYTE_READ_I2C) || (pFunctionId == LpGBTSlowControlWorker::MULTI_BYTE_WRITE_I2C))
    {
        cCntr = fTheRegManager->ReadReg("fc7_daq_stat.command_processor_block.worker.lpgbtsc_try_counters.i2c_tool");
    }
    else if((pFunctionId == LpGBTSlowControlWorker::READ_FE) || (pFunctionId == LpGBTSlowControlWorker::WRITE_FE))
    {
        cCntr = fTheRegManager->ReadReg("fc7_daq_stat.command_processor_block.worker.lpgbtsc_try_counters.fe_tool");
    }
    else
    {
        LOG(ERROR) << "D19clpGBTSlowControlWorkerInterface::GetTryCounter : LpGBT-SC Worker fuction doesn't exist" << RESET;
        throw std::runtime_error("D19clpGBTSlowControlWorkerInterface::GetTryCounter failure");
    }
    return cCntr;
}

} // namespace Ph2_HwInterface
