/*!
  \file                  ChipInterface.h
  \brief                 User Interface to the Chip, base class for, CBC, MPA, SSA, RD53
  \author                Fabio RAVERA
  \version               1.0
  \date                  25/02/19
  Support:               email to fabio.ravera@cern.ch
*/

#include "HWInterface/ChipInterface.h"
#include "HWDescription/BeBoard.h"
#include "HWDescription/Chip.h"
#include "HWDescription/Hybrid.h"
#include "Utils/ConsoleColor.h"
#include "Utils/easylogging++.h"

#define DEV_FLAG 0

using namespace Ph2_HwDescription;

namespace Ph2_HwInterface
{
ChipInterface::ChipInterface(const BeBoardFWMap& pBoardMap) : fBoardMap(pBoardMap), fBoardFW(nullptr), fPrevBoardIdentifier(65535) {}

void ChipInterface::setBoard(uint16_t pBoardIdentifier)
{
    if(fPrevBoardIdentifier != pBoardIdentifier)
    {
        fBoardFW             = fBoardMap.at(pBoardIdentifier);
        fPrevBoardIdentifier = pBoardIdentifier;
    }
}

std::vector<std::pair<std::string, uint16_t>> ChipInterface::ReadChipMultReg(Ph2_HwDescription::Chip* pChip, const std::vector<std::string>& theRegisterList)
{
    std::vector<std::pair<std::string, uint16_t>> theRegisterValues;
    for(const auto& registerName: theRegisterList) theRegisterValues.push_back(std::make_pair(registerName, ReadChipReg(pChip, registerName)));
    return theRegisterValues;
}

void ChipInterface::WriteHybridBroadcastChipReg(const Ph2_HwDescription::Hybrid* pHybrid, const std::string& pRegNode, uint16_t data)
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function is absent" << RESET;
}

void ChipInterface::WriteBoardBroadcastChipReg(const Ph2_HwDescription::BeBoard* pBoard, const std::string& pRegNode, uint16_t data)
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function is absent" << RESET;
}

bool ChipInterface::WriteChipMultReg(Ph2_HwDescription::Chip* pChip, const std::vector<std::pair<std::string, uint16_t>>& pVecReq, bool pVerify)
{
    LOG(ERROR) << BOLDRED << __PRETTY_FUNCTION__ << "\tError: implementation of virtual member function is absent" << RESET;
    return false;
}

uint32_t ChipInterface::ReadChipFuseID(Ph2_HwDescription::Chip* pChip, uint8_t version)
{
    LOG(WARNING) << BOLDYELLOW << __PRETTY_FUNCTION__ << "\tWarning: implementation of virtual member function is absent" << RESET;
    return 0xFFFFFFFF;
}

bool ChipInterface::lpGBTCheck(const Ph2_HwDescription::BeBoard* pBoard)
{
    fWithlpGBT = (pBoard->getFirstObject()->flpGBT != nullptr);
    return fWithlpGBT;
}

} // namespace Ph2_HwInterface
