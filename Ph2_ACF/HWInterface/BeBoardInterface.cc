/*
  FileName :                    BeBoardInterface.cc
  Content :                     User Interface to the Boards
  Programmer :                  Lorenzo BIDEGAIN, Nicolas PIERRE
  Version :                     1.0
  Date of creation :            31/07/14
  Support :                     mail to : lorenzo.bidegain@gmail.com nico.pierre@icloud.com
*/

#include "HWInterface/BeBoardInterface.h"

using namespace Ph2_HwDescription;

namespace Ph2_HwInterface
{
BeBoardInterface::BeBoardInterface(const BeBoardFWMap& pBoardMap) : fBoardMap(pBoardMap), fBoardFW(nullptr), fPrevBoardIdentifier(65535) {}

BeBoardInterface::~BeBoardInterface() {}

void BeBoardInterface::setBoard(uint16_t pBoardIdentifier)
{
    if(fPrevBoardIdentifier != pBoardIdentifier)
    {
        fBoardFW             = fBoardMap.at(pBoardIdentifier);
        fPrevBoardIdentifier = pBoardIdentifier;
    }
}

void BeBoardInterface::SetFileHandler(const BeBoard* pBoard, FileHandler* pHandler)
{
    setBoard(pBoard->getId());
    fBoardFW->setFileHandler(pHandler);
}
void BeBoardInterface::enableFileHandler(BeBoard* pBoard)
{
    setBoard(pBoard->getId());
    fBoardFW->enableFileHandler();
}

void BeBoardInterface::disableFileHandler(BeBoard* pBoard)
{
    setBoard(pBoard->getId());
    fBoardFW->disableFileHandler();
}

void BeBoardInterface::WriteBoardReg(BeBoard* pBoard, const std::string& pRegNode, const uint32_t& pVal)
{
    setBoard(pBoard->getId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);
    fBoardFW->WriteReg(pRegNode, pVal);
}

void BeBoardInterface::WriteBlockBoardReg(BeBoard* pBoard, const std::string& pRegNode, const std::vector<uint32_t>& pValVec)
{
    setBoard(pBoard->getId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);
    fBoardFW->WriteBlockReg(pRegNode, pValVec);
}

void BeBoardInterface::WriteBoardMultReg(BeBoard* pBoard, const std::vector<std::pair<std::string, uint32_t>>& pRegVec)
{
    setBoard(pBoard->getId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);
    fBoardFW->WriteStackReg(pRegVec);
}

uint32_t BeBoardInterface::ReadBoardReg(BeBoard* pBoard, const std::string& pRegNode, bool updateRegs)
{
    setBoard(pBoard->getId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);
    uint32_t                              cRegValue = static_cast<uint32_t>(fBoardFW->ReadReg(pRegNode));
    return cRegValue;
}

void BeBoardInterface::ReadBoardMultReg(BeBoard* pBoard, std::vector<std::pair<std::string, uint32_t>>& pRegVec)
{
    setBoard(pBoard->getId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);
    for(auto& cReg: pRegVec)
    {
        try
        {
            cReg.second = static_cast<uint32_t>(fBoardFW->ReadReg(cReg.first));
        }
        catch(...)
        {
            std::string errorMessage = "Error while reading: " + cReg.first;
            std::cerr << errorMessage << std::endl;
            throw std::runtime_error(errorMessage);
        }
    }
}

std::vector<uint32_t> BeBoardInterface::ReadBlockBoardReg(BeBoard* pBoard, const std::string& pRegNode, uint32_t pSize)
{
    setBoard(pBoard->getId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);
    return fBoardFW->ReadBlockRegValue(pRegNode, pSize);
}

std::vector<uint32_t> BeBoardInterface::ReadBlockBoardReg(BeBoard* pBoard, const std::string& pRegNode, uint32_t pSize, uint32_t pOffset)
{
    setBoard(pBoard->getId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);
    return fBoardFW->ReadBlockRegOffset(pRegNode, pSize, pOffset);
}

uint32_t BeBoardInterface::getBoardInfo(const BeBoard* pBoard)
{
    setBoard(pBoard->getId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);
    return fBoardFW->getBoardInfo();
}

uint32_t BeBoardInterface::getBoardFirmwareVersion(const BeBoard* pBoard)
{
    setBoard(pBoard->getId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);
    return fBoardFW->getBoardFirmwareVersion();
}

BoardType BeBoardInterface::getBoardType(const BeBoard* pBoard)
{
    setBoard(pBoard->getId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);
    return fBoardFW->getBoardType();
}

void BeBoardInterface::ConfigureBoard(const BeBoard* pBoard)
{
    setBoard(pBoard->getId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);
    fBoardFW->ConfigureBoard(pBoard);
}

void BeBoardInterface::Start(const BeBoard* pBoard)
{
    setBoard(pBoard->getId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);
    fBoardFW->Start(pBoard);
}
void BeBoardInterface::Stop(const BeBoard* pBoard)
{
    setBoard(pBoard->getId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);
    fBoardFW->Stop();
}

void BeBoardInterface::Pause(const BeBoard* pBoard)
{
    setBoard(pBoard->getId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);
    fBoardFW->Pause();
}

void BeBoardInterface::Resume(const BeBoard* pBoard)
{
    setBoard(pBoard->getId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);
    fBoardFW->Resume();
}

uint32_t BeBoardInterface::ReadData(BeBoard* pBoard, bool pBreakTrigger, std::vector<uint32_t>& pData, bool pWait)
{
    setBoard(pBoard->getId());
    std::unique_lock<std::recursive_mutex> theGuard(fBoardFW->fMutex, std::defer_lock);

    uint32_t dataSize = 0;
    if(theGuard.try_lock() == true)
    {
        dataSize = fBoardFW->ReadData(pBoard, pBreakTrigger, pData, pWait);
        theGuard.unlock();
    }

    return dataSize;
}

void BeBoardInterface::ReadNEvents(BeBoard* pBoard, uint32_t pNEvents, std::vector<uint32_t>& pData, bool pWait)
{
    setBoard(pBoard->getId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);
    fBoardFW->ReadNEvents(pBoard, pNEvents, pData, pWait);
}

void BeBoardInterface::ChipReset(const BeBoard* pBoard)
{
    setBoard(pBoard->getId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);
    fBoardFW->ChipReset();
}

void BeBoardInterface::ChipTrigger(const BeBoard* pBoard)
{
    setBoard(pBoard->getId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);
    fBoardFW->ChipTrigger();
}

void BeBoardInterface::ChipTestPulse(const BeBoard* pBoard)
{
    setBoard(pBoard->getId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);
    fBoardFW->ChipTestPulse();
}

void BeBoardInterface::ChipReSync(const BeBoard* pBoard)
{
    setBoard(pBoard->getId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);
    fBoardFW->ChipReSync();
}

const uhal::Node& BeBoardInterface::getUhalNode(const BeBoard* pBoard, const std::string& pStrPath)
{
    setBoard(pBoard->getId());
    return fBoardFW->getUhalNode(pStrPath);
}

uhal::HwInterface* BeBoardInterface::getHardwareInterface(const BeBoard* pBoard)
{
    setBoard(pBoard->getId());
    return fBoardFW->getHardwareInterface();
}

void BeBoardInterface::SetForceStart(BeBoard* pBoard, bool bStart)
{
    setBoard(pBoard->getId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);
    fBoardFW->SetForceStart(bStart);
}

void BeBoardInterface::PowerOn(BeBoard* pBoard)
{
    setBoard(pBoard->getId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);
    fBoardFW->PowerOn();
}

void BeBoardInterface::PowerOff(BeBoard* pBoard)
{
    setBoard(pBoard->getId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);
    fBoardFW->PowerOff();
}

void BeBoardInterface::ReadVer(BeBoard* pBoard)
{
    setBoard(pBoard->getId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);
    fBoardFW->ReadVer();
}
} // namespace Ph2_HwInterface
