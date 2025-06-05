#include "HWInterface/LinkInterface.h"
#include "Utils/ConsoleColor.h"
#include "Utils/easylogging++.h"

using namespace Ph2_HwDescription;
namespace Ph2_HwInterface
{
LinkInterface::LinkInterface(RegManager* theRegManager) : fTheRegManager(theRegManager) {}

LinkInterface::~LinkInterface() {}

void LinkInterface::ResetLinks() { LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function LinkInterface::ResetLinks is absent" << RESET; }

void LinkInterface::ResetLink(uint8_t pLinkId) { LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function LinkInterface::ResetLink is absent" << RESET; }

bool LinkInterface::GetLinkStatus(uint8_t pLinkId)
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function LinkInterface::GetLinkStatus is absent" << RESET;
    return false;
}

void LinkInterface::GeneralLinkReset(const BeBoard* pBoard)
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function LinkInterface::GeneralLinkReset is absent" << RESET;
}

} // namespace Ph2_HwInterface