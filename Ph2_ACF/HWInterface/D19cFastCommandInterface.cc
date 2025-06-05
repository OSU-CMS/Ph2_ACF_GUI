#include "HWInterface/D19cFastCommandInterface.h"
#include "HWInterface/RegManager.h"

namespace Ph2_HwInterface
{
D19cFastCommandInterface::D19cFastCommandInterface(RegManager* theRegManager) : FastCommandInterface(theRegManager) {}

D19cFastCommandInterface::~D19cFastCommandInterface() {}

void D19cFastCommandInterface::SendGlobalReSync(uint8_t pDuration)
{
    FastCommand cFastCommand;
    cFastCommand.resync_en = 1;
    cFastCommand.duration  = pDuration;
    fFastCmd               = cFastCommand;
    ComposeFastCommand(fFastCmd);
    fTheRegManager->WriteReg("fc7_daq_ctrl.fast_command_block.control", fFastCommand);
}
void D19cFastCommandInterface::SendGlobalCalPulse(uint8_t pDuration)
{
    FastCommand cFastCommand;
    cFastCommand.cal_pulse_en = 1;
    cFastCommand.duration     = pDuration;
    fFastCmd                  = cFastCommand;
    ComposeFastCommand(fFastCmd);
    fTheRegManager->WriteReg("fc7_daq_ctrl.fast_command_block.control", fFastCommand);
}
void D19cFastCommandInterface::SendGlobalL1A(uint8_t pDuration)
{
    FastCommand cFastCommand;
    cFastCommand.l1a_en   = 1;
    cFastCommand.duration = pDuration;
    fFastCmd              = cFastCommand;
    ComposeFastCommand(fFastCmd);
    fTheRegManager->WriteReg("fc7_daq_ctrl.fast_command_block.control", fFastCommand);
}

void D19cFastCommandInterface::SendGlobalCounterReset(uint8_t pDuration)
{
    FastCommand cFastCommand;
    cFastCommand.bc0_en   = 1;
    cFastCommand.duration = pDuration;
    fFastCmd              = cFastCommand;
    ComposeFastCommand(fFastCmd);
    fTheRegManager->WriteReg("fc7_daq_ctrl.fast_command_block.control", fFastCommand);
}
void D19cFastCommandInterface::SendGlobalCounterResetResync(uint8_t pDuration)
{
    FastCommand cFastCommand;
    cFastCommand.bc0_en    = 1;
    cFastCommand.resync_en = 1;
    cFastCommand.duration  = pDuration;
    fFastCmd               = cFastCommand;
    ComposeFastCommand(fFastCmd);
    fTheRegManager->WriteReg("fc7_daq_ctrl.fast_command_block.control", fFastCommand);
}
void D19cFastCommandInterface::SendGlobalCounterResetL1A(uint8_t pDuration)
{
    FastCommand cFastCommand;
    cFastCommand.bc0_en   = 1;
    cFastCommand.l1a_en   = 1;
    cFastCommand.duration = pDuration;
    fFastCmd              = cFastCommand;
    ComposeFastCommand(fFastCmd);
    fTheRegManager->WriteReg("fc7_daq_ctrl.fast_command_block.control", fFastCommand);
}
void D19cFastCommandInterface::SendGlobalCounterResetCalPulse(uint8_t pDuration)
{
    FastCommand cFastCommand;
    cFastCommand.bc0_en       = 1;
    cFastCommand.cal_pulse_en = 1;
    cFastCommand.duration     = pDuration;
    fFastCmd                  = cFastCommand;
    ComposeFastCommand(fFastCmd);
    fTheRegManager->WriteReg("fc7_daq_ctrl.fast_command_block.control", fFastCommand);
}
void D19cFastCommandInterface::SendGlobalCustomFastCommands(std::vector<FastCommand>& pFastCmd)
{
    std::vector<std::pair<std::string, uint32_t>> cVecReq;
    for(auto cFastCmd: pFastCmd)
    {
        fFastCmd = cFastCmd;
        ComposeFastCommand(fFastCmd);
        cVecReq.push_back({"fc7_daq_ctrl.fast_command_block.control", fFastCommand});
    }
    fTheRegManager->WriteStackReg(cVecReq);
}

void D19cFastCommandInterface::ComposeFastCommand(const FastCommand& pFastCommand)
{
    uint32_t encode_resync    = pFastCommand.resync_en << 16;
    uint32_t encode_cal_pulse = pFastCommand.cal_pulse_en << 17;
    uint32_t encode_l1a       = pFastCommand.l1a_en << 18;
    uint32_t encode_bc0       = pFastCommand.bc0_en << 19;
    uint32_t encode_duration  = pFastCommand.duration << 28;
    fFastCommand              = encode_resync + encode_l1a + encode_cal_pulse + encode_bc0 + encode_duration;
}

} // namespace Ph2_HwInterface