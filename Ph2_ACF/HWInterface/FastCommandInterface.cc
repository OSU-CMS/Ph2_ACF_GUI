#include "HWInterface/FastCommandInterface.h"
#include "HWInterface/RegManager.h"
#include "Utils/ConsoleColor.h"
#include "Utils/easylogging++.h"

namespace Ph2_HwInterface
{
FastCommandInterface::FastCommandInterface(RegManager* theRegManager) : fTheRegManager(theRegManager) {}

FastCommandInterface::~FastCommandInterface() {}

void FastCommandInterface::SendGlobalReSync(uint8_t pDuration) // 1 clk cycle
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function FastCommandInterface::SendGlobalReSync is absent" << RESET;
}
void FastCommandInterface::SendGlobalCalPulse(uint8_t pDuration) // 1 clk cycle
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function FastCommandInterface::SendGlobalCalPulse is absent" << RESET;
}
void FastCommandInterface::SendGlobalL1A(uint8_t pDuration) // 1 clk cycle
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function FastCommandInterface::SendGlobalL1A is absent" << RESET;
}
void FastCommandInterface::SendGlobalCounterReset(uint8_t pDuration) // 1 clk cycle
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function FastCommandInterface::SendGlobalCounterReset is absent" << RESET;
}
void FastCommandInterface::SendGlobalCounterResetResync(uint8_t pDuration) // 1 clk cycle  // BC0 + ReScync
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function FastCommandInterface::SendGlobalCounterResetResync is absent" << RESET;
}
void FastCommandInterface::SendGlobalCounterResetL1A(uint8_t pDuration) // 1 clk cycle  // BC0 + ReScync
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function FastCommandInterface::SendGlobalCounterResetL1A is absent" << RESET;
}
void FastCommandInterface::SendGlobalCounterResetCalPulse(uint8_t pDuration) // 1 clk cycle  // BC0 + ReScync
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function FastCommandInterface::SendGlobalCounterResetCalPulse is absent" << RESET;
}
void FastCommandInterface::SendGlobalCustomFastCommands(std::vector<FastCommand>& pFastCmd)
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function FastCommandInterface::SendGlobalCustomFastCommands is absent" << RESET;
}
void FastCommandInterface::ComposeFastCommand(const FastCommand& pFastCommand)
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function FastCommandInterface::ComposeFastCommand is absent" << RESET;
}

} // namespace Ph2_HwInterface