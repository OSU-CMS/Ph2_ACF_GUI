#include "HWInterface/CommandProcessorInterface.h"
#include "HWDescription/BeBoard.h"
#include "HWInterface/RegManager.h"
#include "Utils/ConsoleColor.h"

namespace Ph2_HwInterface
{
CommandProcessorInterface::CommandProcessorInterface(RegManager* theRegManager) : fTheRegManager(theRegManager) {}

void CommandProcessorInterface::Reset() { LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function CommandProcessorInterface::Reset is absent" << RESET; }

void CommandProcessorInterface::WriteCommand(const std::vector<uint32_t>& pCommand)
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function CommandProcessorInterface::WriteCommand is absent" << RESET;
}

std::vector<uint32_t> CommandProcessorInterface::ReadReply(int pNWords)
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function CommandProcessorInterface::ReadReply is absent" << RESET;
    return {};
}

} // namespace Ph2_HwInterface