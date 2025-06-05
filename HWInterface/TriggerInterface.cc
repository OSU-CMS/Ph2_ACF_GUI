#include "HWInterface/TriggerInterface.h"
#include "HWInterface/RegManager.h"
#include "Utils/ConsoleColor.h"
#include "Utils/easylogging++.h"

namespace Ph2_HwInterface
{
TriggerInterface::TriggerInterface(RegManager* theRegManager) : fTheRegManager(theRegManager) {}

TriggerInterface::~TriggerInterface() {}

bool TriggerInterface::SetNTriggersToAccept(uint32_t pNTriggersToAccept)
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function TriggerInterface::SetNTriggersToAccept is absent" << RESET;
    return false;
}

void TriggerInterface::ResetTriggerFSM() { LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function TriggerInterface::ResetTriggerFSM is absent" << RESET; }

void TriggerInterface::ReconfigureTriggerFSM(std::vector<std::pair<std::string, uint32_t>> pTriggerConfig)
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function TriggerInterface::ReconfigureTriggerFSM is absent" << RESET;
}

bool TriggerInterface::Start(bool checkIfStarted)
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function TriggerInterface::Start is absent" << RESET;
    return false;
}

bool TriggerInterface::Stop()
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function TriggerInterface::Stop is absent" << RESET;
    return false;
}

void TriggerInterface::Pause() { LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function TriggerInterface::Start is absent" << RESET; }

void TriggerInterface::Resume() { LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function TriggerInterface::Stop is absent" << RESET; }

uint32_t TriggerInterface::GetTriggerState()
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function TriggerInterface::GetTriggerState is absent" << RESET;
    return 0;
}

bool TriggerInterface::RunTriggerFSM()
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function TriggerInterface::RunTriggerFSM is absent" << RESET;
    return false;
}

bool TriggerInterface::WaitForNTriggers(uint32_t pNTriggers)
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function TriggerInterface::WaitForNTriggers is absent" << RESET;
    return false;
}

bool TriggerInterface::SendNTriggers(uint32_t pNTriggers)
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function TriggerInterface::SendNTriggers is absent" << RESET;
    return false;
}

void TriggerInterface::PrintStatus()
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function TriggerInterface::PrintStatus is absent" << RESET;
    return;
}

} // namespace Ph2_HwInterface