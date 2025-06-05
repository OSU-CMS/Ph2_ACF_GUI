#include "HWInterface/L1ReadoutInterface.h"
#include "HWDescription/BeBoard.h"
#include "Utils/ConsoleColor.h"
#include "Utils/easylogging++.h"

using namespace Ph2_HwDescription;
namespace Ph2_HwInterface
{
L1ReadoutInterface::L1ReadoutInterface(RegManager* theRegManager) : fTheRegManager(theRegManager) { fData.clear(); }

L1ReadoutInterface::~L1ReadoutInterface() {}

void L1ReadoutInterface::FillData() { LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function L1ReadoutInterface::ReadData is absent" << RESET; }

bool L1ReadoutInterface::WaitForReadout()
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function L1ReadoutInterface::WaitForData is absent" << RESET;
    return false;
}

bool L1ReadoutInterface::WaitForNTriggers()
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function L1ReadoutInterface::WaitForNTriggers is absent" << RESET;
    return false;
}

bool L1ReadoutInterface::ReadEvents(const Ph2_HwDescription::BeBoard* pBoard)
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function L1ReadoutInterface::ReadEvents is absent" << RESET;
    return false;
}

bool L1ReadoutInterface::PollReadoutData(const Ph2_HwDescription::BeBoard* pBoard, bool pWait)
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function L1ReadoutInterface::ReadEvents is absent" << RESET;
    return false;
}

bool L1ReadoutInterface::ResetReadout()
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function L1ReadoutInterface::ResetReadout is absent" << RESET;
    return false;
}

bool L1ReadoutInterface::CheckBuffers()
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function L1ReadoutInterface::CheckBuffers is absent" << RESET;
    return false;
}

} // namespace Ph2_HwInterface