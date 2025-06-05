/*
  FileName :                    BeBoardFWInterface.cc
  Content :                     BeBoardFWInterface base class of all type of boards
  Programmer :                  Lorenzo BIDEGAIN, Nicolas PIERRE
  Version :                     1.0
  Date of creation :            31/07/14
  Support :                     mail to : lorenzo.bidegain@gmail.com, nico.pierre@icloud.com
*/

#include "HWInterface/BeBoardFWInterface.h"
#include "HWDescription/BeBoard.h"
#include "HWInterface/FpgaConfig.h"

using namespace Ph2_HwDescription;

namespace Ph2_HwInterface
{
BeBoardFWInterface::BeBoardFWInterface(const std::string& puHalConfigFileName, uint32_t pBoardId, BeBoard* theBoard)
    : RegManager(puHalConfigFileName, pBoardId, theBoard), fSaveToFile(false), fFileHandler(nullptr)
{
}

BeBoardFWInterface::BeBoardFWInterface(const std::string& pId, const std::string& pUri, const std::string& pAddressTable, BeBoard* theBoard)
    : RegManager(pId, pUri, pAddressTable, theBoard), fSaveToFile(false), fFileHandler(nullptr)
{
}

// BeBoardFWInterface::BeBoardFWInterface(RegManager&& theRegManager) : RegManager(std::move(theRegManager)), fSaveToFile(false), fFileHandler(nullptr) {}

std::string BeBoardFWInterface::readBoardType()
{
    std::string cBoardTypeString;

    uint32_t cBoardType = ReadReg("board_id");

    char cChar = ((cBoardType & cMask4) >> 24);
    cBoardTypeString.push_back(cChar);

    cChar = ((cBoardType & cMask3) >> 16);
    cBoardTypeString.push_back(cChar);

    cChar = ((cBoardType & cMask2) >> 8);
    cBoardTypeString.push_back(cChar);

    cChar = (cBoardType & cMask1);
    cBoardTypeString.push_back(cChar);

    return cBoardTypeString;
}

void BeBoardFWInterface::PowerOn() {}
void BeBoardFWInterface::PowerOff() {}
void BeBoardFWInterface::ReadVer() {}

} // namespace Ph2_HwInterface
