#include "HWInterface/D19cLinkInterface.h"
#include "HWDescription/BeBoard.h"
#include "HWDescription/OpticalGroup.h"
#include "HWInterface/ExceptionHandler.h"
#include "HWInterface/RegManager.h"
#include "Utils/ConsoleColor.h"
#include "Utils/easylogging++.h"
#include <thread>

using namespace Ph2_HwDescription;

namespace Ph2_HwInterface
{
// ##########################################
// # Constructors #
// #########################################

D19cLinkInterface::D19cLinkInterface(RegManager* theRegManager) : LinkInterface(theRegManager)
{
    fConfiguration.fResetWait_ms = 2000;
    fConfiguration.fReTry        = 1;
    fConfiguration.fMaxAttempts  = 5;
}

D19cLinkInterface::~D19cLinkInterface() {}

void D19cLinkInterface::ResetLinks()
{
    // reset lpGBT core - for now there is one reset signal for all links
    fTheRegManager->WriteReg("fc7_daq_ctrl.optical_block.general", 0x1);
    std::this_thread::sleep_for(std::chrono::milliseconds(fConfiguration.fResetWait_ms));
    fTheRegManager->WriteReg("fc7_daq_ctrl.optical_block.general", 0x0);
    std::this_thread::sleep_for(std::chrono::milliseconds(fWait_ms));
}
bool D19cLinkInterface::GetLinkStatus(uint8_t pLinkId)
{
    bool     cLocked    = true;
    uint8_t  cCommandId = 1; // command id for  link status request is 1
    uint32_t cCommand   = ((pLinkId & 0x3f) << 26) | (cCommandId << 22);
    fTheRegManager->WriteReg("fc7_daq_ctrl.optical_block.general", cCommand);
    std::this_thread::sleep_for(std::chrono::milliseconds(fWait_ms));
    // read back status register
    LOG(INFO) << BOLDBLUE << "lpGBT Link Status..." << RESET;
    uint32_t cLinkStatus = fTheRegManager->ReadReg("fc7_daq_stat.optical_block");
    LOG(INFO) << BOLDBLUE << "lpGBT Link" << +pLinkId << " status " << std::bitset<32>(cLinkStatus) << RESET;
    std::vector<std::string> cStates = {"lpGBT TX Ready", "MGT Ready", "lpGBT RX Ready"};
    uint8_t                  cIndex  = 1;
    for(auto cState: cStates)
    {
        uint8_t cStatus = (cLinkStatus >> (3 - cIndex)) & 0x1;
        cLocked &= (cStatus == 1);
        if(cStatus == 1)
            LOG(INFO) << BOLDBLUE << "D19cLinkInterface:GetLinkStatus\t... " << cState << BOLDGREEN << "\t : LOCKED" << RESET;
        else
            LOG(INFO) << BOLDBLUE << "D19cLinkInterface:GetLinkStatus\t... " << cState << BOLDRED << "\t : FAILED" << RESET;
        cIndex++;
    }
    return cLocked;
}
void D19cLinkInterface::GeneralLinkReset(const BeBoard* pBoard)
{
    bool   cAllLocked   = false;
    size_t cMaxAttempts = fConfiguration.fReTry ? fConfiguration.fMaxAttempts : 1;
    size_t cAttempts    = 0;
    bool   cLinkStatus  = false;
    do {
        cAllLocked = true;
        LOG(INFO) << BOLDMAGENTA << "D19cLinkInterface::GeneralLinkReset Resetting lpGBT-FPGA core on BeBoard#" << +pBoard->getId() << " [Attempt#" << cAttempts++ << "]" << RESET;
        ResetLinks();
        for(auto cOpticalReadout: *pBoard)
        {
            // cLinkStatus = cOpticalReadout->fIsLocked;
            // if(!cLinkStatus)
            // {
            cLinkStatus = GetLinkStatus(cOpticalReadout->getId());
#ifdef __TCUSB__
            if(cLinkStatus)
            {
                // cOpticalReadout->fIsLocked = true;
                break;
            }
#endif
            // }
            cAllLocked = cAllLocked && cLinkStatus;
        }
#ifdef __TCUSB__
        cAllLocked = false;
        if(cLinkStatus) break;
#endif
    } while(cAttempts < cMaxAttempts && !cAllLocked);
    if(!cAllLocked)
    {
        LOG(ERROR) << BOLDRED << "Failed to lock all links after a general reset, disabling problematic OpticalGroups and continuing" << RESET;
        for(auto cOpticalReadout: *pBoard)
        {
            if(!GetLinkStatus(cOpticalReadout->getId()))
            {
                LOG(ERROR) << BOLDRED << "Disabling Board " << RESET;
                ExceptionHandler::getInstance()->disableOpticalGroup(pBoard->getId(), cOpticalReadout->getId());
            }
        }
    }
}
} // namespace Ph2_HwInterface
