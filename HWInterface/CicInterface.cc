/*

        FileName :                     CicInterface.cc
        Content :                      User Interface to the Cics
        Version :                      1.0
        Date of creation :             10/07/14

 */

#include "HWInterface/CicInterface.h"
#include "HWDescription/Cic.h"
#include "HWInterface/BeBoardFWInterface.h"
#include "HWInterface/D19cFWInterface.h"
#include "HWInterface/D19clpGBTInterface.h"
#include "HWInterface/ExceptionHandler.h"
#include "Utils/GenericDataArray.h"
#include "boost/format.hpp"
#include <numeric>

#define DEV_FLAG 0
// #define COUNT_FLAG 0

using namespace Ph2_HwDescription;

namespace Ph2_HwInterface
{
CicInterface::CicInterface(const BeBoardFWMap& pBoardMap) : ChipInterface(pBoardMap)
{
    fRegisterWrites = 0;
    fWriteErrorMap.clear();
    fReadBackErrorMap.clear();
    fMap.clear();
}

CicInterface::~CicInterface() {}

bool CicInterface::runVerification(Ph2_HwDescription::Chip* pChip, uint8_t pValue, std::string pRegName)
{
    auto     cRegItem = pChip->getRegItem(pRegName);
    uint32_t cValue   = pValue;
    // only check against map if this is
    // a register than *can* be read back
    // from
    bool cSuccess = ((cRegItem.fStatusReg == 0x01) ? true : (cRegItem.fValue == cValue));
    if(cSuccess)
    {
        LOG(DEBUG) << BOLDGREEN << "\t...[DEBUG] Have written 0x" << std::hex << +cRegItem.fValue << std::dec << " CIC register with address 0x" << std::hex << +cRegItem.fAddress << std::dec
                   << " value read back is 0x" << std::hex << +cValue << std::dec << RESET;
        pChip->setReg(pRegName, pValue);
    }
    else if(!cSuccess)
        LOG(DEBUG) << BOLDRED << "\t...[DEBUG] Have written 0x" << std::hex << +cRegItem.fValue << std::dec << " CIC register with address 0x" << std::hex << +cRegItem.fAddress << std::dec
                   << " value read back is 0x" << std::hex << +cValue << std::dec << RESET;
    return cSuccess;
}

std::pair<int, float> CicInterface::getWRattempts()
{
    float cReWR = 0;
    float cN    = 0;
    for(auto cIter: fReWrMap)
    {
        if(cIter.second != 0)
        {
            cN++;
            cReWR += cIter.second;
        }
    }
    float cMean = (cN == 0) ? 0 : cReWR / cN;
    return std::make_pair(cN, cMean);
}
std::pair<float, float> CicInterface::getMinMaxWRattempts()
{
    float cReWRmin = 0;
    float cReWRmax = 0;
    for(auto cIter: fReWrMap)
    {
        if(cIter.second != 0 && cReWRmin == 0)
            cReWRmin = cIter.second;
        else
        {
            if(cIter.second < cReWRmin) cReWRmin = cIter.second;
        }
        if(cIter.second > cReWRmax) cReWRmax = cIter.second;
    }
    return std::make_pair(cReWRmin, cReWRmax);
}
void CicInterface::printErrorSummary()
{
    LOG(INFO) << BOLDRED << "CIC total write error count : " << +fWriteErrors << " out of a total of " << +fRegisterWrites << " writes" << RESET;

    LOG(INFO) << BOLDRED << "CIC total read-back error count : " << +fReadBackErrors << " out of a total of " << +fRegisterWrites << " writes" << RESET;
}
void CicInterface::CheckConfig(Chip* pChip)
{
    LOG(DEBUG) << BOLDMAGENTA << "Running verification loop for CicInterface::WriteRegs" << RESET;
    fReadBackErrors = 0;
    for(const auto& cMapItem: fMap)
    {
        auto     cRegItem = pChip->getRegItem(cMapItem.second);
        uint32_t cValue   = fBoardFW->SingleRegisterRead(pChip, cRegItem);
        bool     cSuccess = this->runVerification(pChip, cValue, cMapItem.second);
        if(!cSuccess)
        {
            LOG(INFO) << BOLDRED << "Readback error for CIC register 0x" << std::hex << +cRegItem.fAddress << std::dec << " have written " << +cRegItem.fValue << " and have read back " << cValue
                      << RESET;

            auto cIter = fReadBackErrorMap.find(cRegItem.fAddress);
            if(cIter == fReadBackErrorMap.end())
                fReadBackErrorMap[cRegItem.fAddress] = 1;
            else
                fReadBackErrorMap.at(cRegItem.fAddress) = fReadBackErrorMap.at(cRegItem.fAddress) + 1;
            fReadBackErrors++;
        }
    }
}

uint32_t CicInterface::ReadChipFuseID(Chip* pCic, uint8_t version)
{
    /* pre-production (CERN tested wafers)
    CIC ID = AABBBCCCD (decimal!)
    AA : LOT number
    BBB : WAFER number
    CCC : RETICLE number
    D : DIE number
    */
    /* production (vendor tested wafers)
    Byte order to be confirmed!
    fuseID32 = (((process & 0x0F) << 22) | ((status & 0x3) << 20) | ((lot & 0xF) << 16) | ((wafer & 0x1F) << 11) | ((reticle & 0x7F)<<4) | (die & 0xF);
    Where:
    process = 15
    status = pass/fail
    lot = the wafer lot number
    wafer = the wafer number
    reticle = the reticle number (0 to 127)
    die = the die number in the reticle (0 to 3)
    */
    this->WriteChipReg(pCic, "EFUSEMODE", 0x0);
    std::this_thread::sleep_for(std::chrono::microseconds(10));
    this->WriteChipReg(pCic, "EFUSEMODE", 0xF);
    std::this_thread::sleep_for(std::chrono::microseconds(10));
    this->WriteChipReg(pCic, "EFUSEMODE", 0x0);

    uint32_t val =
        (this->ReadChipReg(pCic, "EfuseValue0") << 24) | (this->ReadChipReg(pCic, "EfuseValue1") << 16) | (this->ReadChipReg(pCic, "EfuseValue2") << 8) | (this->ReadChipReg(pCic, "EfuseValue3") << 0);
    // pCic->pChipFuseID.SetId(val);
    if(((val >> 22) & 0x0F) == 0x0F)
    {
        LOG(INFO) << BOLDYELLOW << "FuseID from production CIC2 0x" << std::hex << +val << std::dec << " Status " << +((val >> 20) & 0x03) << " LOT " << +((val >> 16) & 0x0F) << " Wafer "
                  << +((val >> 11) & 0x1F) << " RETICLE " << +((val >> 4) & 0x7F) << " DIE " << +((val) & 0x0F) << RESET;
    }
    else
    {
        LOG(INFO) << BOLDYELLOW << "FuseID from pre-production CIC2 " << +val << " LOT " << +(int)val / (int)1e7 << " Wafer " << +(int)(val % (int)1e7) / (int)1e4 << " RETICLE "
                  << +(int)(val % (int)1e4) / 10 << " DIE " << +(int)(val % 10) << RESET;
    }
    return val;
}
bool CicInterface::ConfigureChip(Chip* pCic, bool pVerify, uint32_t pBlockSize)
{
    std::stringstream cOutput;
    setBoard(pCic->getBeBoardId());
    pCic->printChipType(cOutput);
    LOG(INFO) << BOLDBLUE << cOutput.str() << "...Configuring chip with Id[" << +pCic->getId() << "]" << RESET;
    ChipRegMap cCicRegMap = pCic->getRegMap();
    // get register map
    std::vector<std::pair<std::string, uint16_t>> cRegItems;
    auto                                          theListOfFreeRegisters = pCic->getFreeRegisters();
    for(auto cItem: cCicRegMap)
    {
        bool isFreeRegister = false;
        for(const auto& freeRegister: theListOfFreeRegisters)
        {
            isFreeRegister = std::regex_match(cItem.first, freeRegister.first);
            if(isFreeRegister) break;
        }
        if(isFreeRegister) continue; // skipping readonly registers

        cRegItems.push_back(std::make_pair(cItem.first, cItem.second.fValue));
    }
    bool cSuccess = WriteChipMultReg(pCic, cRegItems, pVerify);
    if(cSuccess) LOG(INFO) << BOLDGREEN << "Succesful write to " << cRegItems.size() << " registers on CIC" << RESET;
    return cSuccess;
}

bool CicInterface::WriteChipReg(Chip* pChip, const std::string& pRegNode, uint16_t pValue, bool pVerify)
{
    setBoard(pChip->getBeBoardId());
    // LOG(DEBUG) << BOLDMAGENTA << "CicInterface::WriteChipReg trying to write to register 0x" << pRegNode << RESET;
    ChipRegMap cRegMap          = pChip->getRegMap();
    cRegMap.at(pRegNode).fValue = pValue;
    return fBoardFW->SingleRegisterWrite(pChip, cRegMap.at(pRegNode), pVerify);
}

bool CicInterface::WriteChipMultReg(Chip* pChip, const std::vector<std::pair<std::string, uint16_t>>& pVecReq, bool pVerify)
{
    // first, identify the correct BeBoardFWInterface
    setBoard(pChip->getBeBoardId());
    auto                     cRegMap = pChip->getRegMap();
    std::vector<ChipRegItem> cRegItems;
    for(auto cReq: pVecReq)
    {
        auto cIterator = cRegMap.find(cReq.first);
        if(cIterator == cRegMap.end())
        {
            LOG(ERROR) << BOLDRED << "CicInterface::WriteChipMultReg trying to write to a register that doesn't exist in the map : " << cReq.first << RESET;
            continue;
        }

        ChipRegItem cItem = cIterator->second;
        cItem.fValue      = cReq.second;
        cRegItems.push_back(cItem);
    }
    return fBoardFW->MultiRegisterWrite(pChip, cRegItems, pVerify);
}

int32_t CicInterface::ReadChipReg(Chip* pChip, const std::string& pRegNode)
{
    setBoard(pChip->getBeBoardId());
    // LOG(DEBUG) << BOLDMAGENTA << "CicInterface::ReadChipReg(string) Register " << pRegNode << RESET;

    ChipRegMap cRegMap = pChip->getRegMap();
    if(cRegMap.find(pRegNode) == cRegMap.end()) { LOG(INFO) << BOLDRED << "Could not find CIC register " << pRegNode << RESET; }

    ChipRegItem cRegItem = pChip->getRegItem(pRegNode);
    return fBoardFW->SingleRegisterRead(pChip, cRegItem);
}

std::vector<std::pair<std::string, uint16_t>> CicInterface::ReadChipMultReg(Ph2_HwDescription::Chip* pChip, const std::vector<std::string>& theRegisterList)
{
    setBoard(pChip->getBeBoardId());
    auto                     cRegMap = pChip->getRegMap();
    std::vector<ChipRegItem> cRegItems;
    for(auto cReq: theRegisterList)
    {
        auto cIterator = cRegMap.find(cReq);
        if(cIterator == cRegMap.end())
        {
            LOG(ERROR) << BOLDRED << "CicInterface::WriteChipMultReg trying to write to a register that doesn't exist in the map : " << cReq << RESET;
            abort();
        }

        ChipRegItem cItem = cIterator->second;
        cRegItems.push_back(cItem);
    }

    fBoardFW->MultiRegisterRead(pChip, cRegItems);

    std::vector<std::pair<std::string, uint16_t>> theRegisterValues;
    for(size_t i = 0; i < theRegisterList.size(); ++i) theRegisterValues.push_back(std::make_pair(theRegisterList.at(i), cRegItems.at(i).fValue));
    return theRegisterValues;
}

bool CicInterface::GetResyncRequest(Chip* pChip)
{
    uint16_t cRegAddress = 0xA6;

    setBoard(pChip->getBeBoardId());
    LOG(DEBUG) << BOLDBLUE << "Checking if CIC requires a ReSync." << RESET;
    ChipRegItem cRegItem;
    cRegItem.fPage      = 0x00;
    cRegItem.fAddress   = cRegAddress;
    cRegItem.fStatusReg = 0x01;
    auto cRegValue      = fBoardFW->SingleRegisterRead(pChip, cRegItem);

    LOG(DEBUG) << BOLDBLUE << "Read back value of " << std::bitset<5>(cRegValue) << " from RO status register" << RESET;
    auto cResyncNeeded = ((cRegValue & 0x8) >> 3);
    return (cResyncNeeded == 1);
}
bool CicInterface::CheckReSync(Chip* pChip)
{
    bool     cResyncNeeded = GetResyncRequest(pChip);
    uint16_t cRegAddress   = 0xA6;
    setBoard(pChip->getBeBoardId());
    ChipRegItem cRegItem;
    cRegItem.fPage      = 0x00;
    cRegItem.fAddress   = cRegAddress;
    cRegItem.fStatusReg = 0x01;
    return cResyncNeeded;
}
bool CicInterface::CheckFastCommandLock(Chip* pChip)
{
    uint16_t cRegAddress = 0xA6;
    setBoard(pChip->getBeBoardId());
    LOG(INFO) << BOLDBLUE << "Checking CIC" << +pChip->getHybridId() << " - has fast command decoder locked?" << RESET;
    ChipRegItem cRegItem;
    cRegItem.fPage      = 0x00;
    cRegItem.fAddress   = cRegAddress;
    cRegItem.fStatusReg = 0x01;
    auto cRegValue      = fBoardFW->SingleRegisterRead(pChip, cRegItem);
    LOG(INFO) << BOLDYELLOW << "Read back value of " << std::bitset<5>(cRegValue) << " from RO status register" << RESET;
    return ((cRegValue & 0x10) >> 4);
}
// configure alignment patterns on CIC
bool CicInterface::ConfigureAlignmentPatterns(Chip* pChip, std::vector<uint8_t> pAlignmentPatterns)
{
    bool cSuccess = true;
    setBoard(pChip->getBeBoardId());
    LOG(DEBUG) << BOLDBLUE << "Configuring word alignment patterns on CIC" << RESET;
    for(uint8_t cIndex = 0; cIndex < (uint8_t)pAlignmentPatterns.size(); cIndex += 1)
    {
        std::stringstream cBuffer;
        cBuffer << "CALIB_PATTERN" << +cIndex;
        std::string cRegName(cBuffer.str()); //, sizeof(cBuffer));
        cSuccess = cSuccess && this->WriteChipReg(pChip, cRegName, pAlignmentPatterns.at(cIndex));
        if(cSuccess) { LOG(INFO) << BOLDBLUE << "Calibration pattern [for word alignment] on stub line " << +cIndex << " set to " << std::bitset<8>(pAlignmentPatterns.at(cIndex)) << RESET; }
    }
    return cSuccess;
}
// manually set Bx0 alignment
bool CicInterface::ManualBx0Alignment(Chip* pChip, uint8_t pBx0delay)
{
    std::string cRegName  = "BX0_ALIGN_CONFIG";
    uint16_t    cRegValue = this->ReadChipReg(pChip, cRegName);
    uint16_t    cValue    = ((cRegValue & 0x7F) | (0x01 << 7));
    setBoard(pChip->getBeBoardId());
    LOG(INFO) << BOLDBLUE << "Manually settomg BX0 delay value in CIC on FE" << +pChip->getHybridId() << " to " << +pBx0delay << " clock cycles." << RESET;
    bool cSuccess = this->WriteChipReg(pChip, cRegName, cValue);
    // std::cout << __PRETTY_FUNCTION__ << " [" << __LINE__ << "] writing " << cRegName << " to 0x" << std::hex << +cValue << std::dec << std::endl;
    if(!cSuccess) return cSuccess;
    cSuccess = cSuccess && this->WriteChipReg(pChip, "EXT_BX0_DELAY", pBx0delay);
    // std::cout << __PRETTY_FUNCTION__ << " [" << __LINE__ << "] writing EXT_BX0_DELAY to 0x" << std::hex << +pBx0delay << std::dec << std::endl;
    return cSuccess;
}
// run automated Bx0 alignment - FIX ME
bool CicInterface::ConfigureBx0Alignment(Chip* pChip, std::vector<uint8_t> pAlignmentPatterns, uint8_t pFEId, uint8_t pLineId)
{
    // std::vector<uint8_t> cFeMapping = static_cast<Cic*>(pChip)->getMapping();
    std::vector<uint8_t> cFeMapping{3, 2, 1, 0, 4, 5, 6, 7}; // FE --> FE CIC
    setBoard(pChip->getBeBoardId());
    LOG(DEBUG) << BOLDBLUE << "Running automated word alignment in CIC on FE" << +pChip->getHybridId() << RESET;
    LOG(DEBUG) << BOLDBLUE << "Configuring word alignment patterns on CIC" << RESET;
    bool cSuccess = ConfigureAlignmentPatterns(pChip, pAlignmentPatterns);

    std::string cRegName  = "BX0_ALIGN_CONFIG";
    uint16_t    cRegValue = this->ReadChipReg(pChip, cRegName);
    uint16_t    cValue    = ((cRegValue & 0x7F) | (0x00 << 7));
    cSuccess              = cSuccess && this->WriteChipReg(pChip, cRegName, cValue);
    if(!cSuccess) return cSuccess;

    cRegName  = "BX0_ALIGN_CONFIG";
    cRegValue = this->ReadChipReg(pChip, cRegName);
    cValue    = ((cRegValue & 0xC7) | (cFeMapping.at(pFEId) << 3));
    cSuccess  = cSuccess && this->WriteChipReg(pChip, cRegName, cValue);
    if(!cSuccess) return cSuccess;

    cRegName  = "BX0_ALIGN_CONFIG";
    cRegValue = this->ReadChipReg(pChip, cRegName);
    cValue    = ((cRegValue & 0xF8) | (pLineId << 0));
    cSuccess  = cSuccess && this->WriteChipReg(pChip, cRegName, cValue);
    fBoardFW->ChipReSync();
    return cSuccess;
}
bool CicInterface::AutoBx0Alignment(Chip* pChip, uint8_t pStatus)
{
    // make sure auto WA request is 0
    std::string cRegName   = "MISC_CTRL";
    uint16_t    cRegValue  = this->ReadChipReg(pChip, cRegName);
    uint16_t    cToggleOff = ((cRegValue & 0x1D) | (0x0 << 0));
    bool        cSuccess   = this->WriteChipReg(pChip, cRegName, cToggleOff);
    if(!cSuccess) return cSuccess;

    cRegName        = "BX0_ALIGN_CONFIG";
    cRegValue       = this->ReadChipReg(pChip, cRegName);
    uint16_t cValue = ((cRegValue & 0xF8) | (pStatus << 6));
    cSuccess        = this->WriteChipReg(pChip, cRegName, cValue);
    if(!cSuccess) return cSuccess;

    return cSuccess;
}
std::pair<bool, uint8_t> CicInterface::CheckBx0Alignment(Chip* pChip)
{
    uint8_t cDelay = 0;
    setBoard(pChip->getBeBoardId());

    bool cSuccess = this->AutoBx0Alignment(pChip, 0);
    if(!cSuccess) return std::make_pair(cSuccess, cDelay);
    // remember to send a resync
    fBoardFW->ChipReSync();

    // check status
    ChipRegItem cRegItem;
    cRegItem.fPage          = 0x00;
    cRegItem.fAddress       = 0xA6;
    cRegItem.fStatusReg     = 0x01;
    auto     cRegValue      = fBoardFW->SingleRegisterRead(pChip, cRegItem);
    uint16_t cReadBackValue = ((cRegValue & 0x02) >> 1);
    cSuccess                = (cReadBackValue == 1);

    // read back delay
    cRegItem.fPage      = 0x00;
    cRegItem.fAddress   = 0xA7;
    cRegItem.fStatusReg = 0x01;
    cRegValue           = fBoardFW->SingleRegisterRead(pChip, cRegItem);
    return std::make_pair(cSuccess, cRegValue);
}
// run automated word alignment
// assumes FEs have been configured to output alignment pattern
bool CicInterface::AutomatedWordAlignment(Chip* pChip, std::vector<uint8_t> pAlignmentPatterns)
{
    setBoard(pChip->getBeBoardId());
    LOG(DEBUG) << BOLDBLUE << "Running automated word alignment in CIC on FE" << +pChip->getHybridId() << RESET;
    LOG(DEBUG) << BOLDBLUE << "Configuring word alignment patterns on CIC" << RESET;
    bool cSuccess = ConfigureAlignmentPatterns(pChip, pAlignmentPatterns);
    if(!cSuccess)
    {
        LOG(INFO) << BOLDRED << "Cannot configure patterns on CIC on Board id " << +pChip->getBeBoardId() << " OpticalGroup id" << +pChip->getOpticalGroupId() << " Hybrid id " << +pChip->getHybridId()
                  << " --- Hybrid will be disabled" << RESET;
        ExceptionHandler::getInstance()->disableHybrid(pChip->getBeBoardId(), pChip->getOpticalGroupId(), pChip->getHybridId());
        return false;
    }

    std::string cRegName             = "MISC_CTRL";
    uint16_t    cRegValue            = this->ReadChipReg(pChip, cRegName);
    uint16_t    useInternalWordDelay = (cRegValue & 0x1D) | (0x0 << 1); // required for automatic alignment

    cSuccess = this->WriteChipReg(pChip, cRegName, useInternalWordDelay);
    if(!cSuccess)
    {
        LOG(INFO) << BOLDRED << "Cannot disable external word alignment value on CIC on Board id " << +pChip->getBeBoardId() << " OpticalGroup id" << +pChip->getOpticalGroupId() << " Hybrid id "
                  << +pChip->getHybridId() << " --- Hybrid will be disabled" << RESET;
        ExceptionHandler::getInstance()->disableHybrid(pChip->getBeBoardId(), pChip->getOpticalGroupId(), pChip->getHybridId());
        return false;
    }

    cRegValue                       = this->ReadChipReg(pChip, cRegName);
    uint16_t startAutoWordAlignment = (cRegValue & 0x1E) | 0x1;
    cSuccess                        = this->WriteChipReg(pChip, cRegName, startAutoWordAlignment);
    if(!cSuccess)
    {
        LOG(INFO) << BOLDRED << "Cannot send external word alignment request to CIC on Board id " << +pChip->getBeBoardId() << " OpticalGroup id" << +pChip->getOpticalGroupId() << " Hybrid id "
                  << +pChip->getHybridId() << " --- Hybrid will be disabled" << RESET;
        ExceptionHandler::getInstance()->disableHybrid(pChip->getBeBoardId(), pChip->getOpticalGroupId(), pChip->getHybridId());
        return false;
    }
    LOG(DEBUG) << BOLDBLUE << "Running automated word alignment .... " << RESET;
    // check if word alingment is done
    uint8_t maxNumberOfIterations  = 100;
    uint8_t currentIterationNumber = 0;
    bool    alignmentCompleted     = false;

    while(currentIterationNumber < maxNumberOfIterations)
    {
        std::this_thread::sleep_for(std::chrono::microseconds(10));
        auto cRegValue = ReadChipReg(pChip, "timingStatusBits");
        if((cRegValue & 0x01) == 1)
        {
            alignmentCompleted = true;
            break;
        }
        ++currentIterationNumber;
    }
    if(!alignmentCompleted) return false;

    LOG(DEBUG) << BOLDBLUE << "Requesting CIC to stop automated word alignment..." << RESET;
    uint16_t stopAutoWordAlignment = (cRegValue & 0x1E) | 0x0;
    cSuccess                       = this->WriteChipReg(pChip, cRegName, stopAutoWordAlignment);
    if(!cSuccess)
    {
        LOG(INFO) << BOLDRED << "Cannot disable automated Word alignment request on CIC on Board id " << +pChip->getBeBoardId() << " OpticalGroup id" << +pChip->getOpticalGroupId() << " Hybrid id "
                  << +pChip->getHybridId() << " --- Hybrid will be disabled" << RESET;
        ExceptionHandler::getInstance()->disableHybrid(pChip->getBeBoardId(), pChip->getOpticalGroupId(), pChip->getHybridId());
        return false;
    }

    return cSuccess;
}

bool CicInterface::PrepareForAutomatedBX0Alignment(Chip* theCic, std::vector<uint8_t> pAlignmentPatterns, uint8_t pLine, uint8_t pFEChip)
{
    setBoard(theCic->getBeBoardId());
    LOG(INFO) << BOLDBLUE << "Running automated BX0 alignment in CIC on FE" << +theCic->getHybridId() << RESET;
    LOG(INFO) << BOLDBLUE << "Configuring BX0 alignment patterns on CIC" << RESET;
    bool cSuccess;
    cSuccess = ConfigureAlignmentPatterns(theCic, pAlignmentPatterns);
    // cSuccess = ConfigureAlignmentPatterns(theCic, {0x80,0x00,0x00,0x00,0x00});
    if(!cSuccess)
    {
        LOG(INFO) << BOLDRED << "Cannot configure patterns on CIC on Board id " << +theCic->getBeBoardId() << " OpticalGroup id" << +theCic->getOpticalGroupId() << " Hybrid id "
                  << +theCic->getHybridId() << " --- Hybrid will be disabled" << RESET;
        ExceptionHandler::getInstance()->disableHybrid(theCic->getBeBoardId(), theCic->getOpticalGroupId(), theCic->getHybridId());
        return false;
    }

    std::string cRegName  = "BX0_ALIGN_CONFIG";
    uint16_t    cRegValue = this->ReadChipReg(theCic, cRegName);
    LOG(INFO) << BOLDBLUE << "BX0_ALIGN_CONFIG intially was set to 0x" << std::hex << cRegValue << std::dec << " bin " << std::bitset<8>(cRegValue) << RESET;

    // set to read another chip or line then 0 0
    uint16_t useChip = (cRegValue & 0xC7) | (pFEChip << 3);
    // std::cout << " useChip " << useChip << std::endl;
    this->WriteChipReg(theCic, cRegName, useChip);
    cRegValue = this->ReadChipReg(theCic, cRegName);
    LOG(INFO) << BOLDBLUE << "BX0_ALIGN_CONFIG set to 0x" << std::hex << cRegValue << std::dec << " bin " << std::bitset<8>(cRegValue) << " to use another chip or line" << RESET;

    // set to read another chip or line then 0 0
    uint16_t useLine = (cRegValue & 0xF8) | (pLine << 0);
    // std::cout << " useLine " << useLine << std::endl;
    this->WriteChipReg(theCic, cRegName, useLine);
    cRegValue = this->ReadChipReg(theCic, cRegName);
    LOG(INFO) << BOLDBLUE << "BX0_ALIGN_CONFIG set to 0x" << std::hex << cRegValue << std::dec << " bin " << std::bitset<8>(cRegValue) << " to use another chip or line" << RESET;

    uint16_t useInternalBX0Delay = (cRegValue & 0x7F) | (0x0 << 7); // required for automatic alignment
    LOG(INFO) << BOLDBLUE << "BX0_ALIGN_CONFIG set to 0x" << std::hex << useInternalBX0Delay << std::dec << " bin " << std::bitset<8>(useInternalBX0Delay) << " to use internal delay" << RESET;
    cSuccess = this->WriteChipReg(theCic, cRegName, useInternalBX0Delay);
    if(!cSuccess)
    {
        LOG(INFO) << BOLDRED << "Cannot disable external BX0 alignment value on CIC on Board id " << +theCic->getBeBoardId() << " OpticalGroup id" << +theCic->getOpticalGroupId() << " Hybrid id "
                  << +theCic->getHybridId() << " --- Hybrid will be disabled" << RESET;
        ExceptionHandler::getInstance()->disableHybrid(theCic->getBeBoardId(), theCic->getOpticalGroupId(), theCic->getHybridId());
        return false;
    }

    // auto BX0doneRegValue2 = ReadChipReg(theCic, "timingStatusBits");
    // std::cout << " timingStatusBits after using internal delays "<<  std::bitset<5>(BX0doneRegValue2) << std::endl;

    cRegValue                      = this->ReadChipReg(theCic, cRegName);
    uint16_t startAutoBX0Alignment = (cRegValue & 0xBF) | (0x1 << 6);
    LOG(INFO) << BOLDBLUE << "BX0_ALIGN_CONFIG set to 0x" << std::hex << startAutoBX0Alignment << std::dec << " bin " << std::bitset<8>(startAutoBX0Alignment) << " to start BX0 alignment " << RESET;
    cSuccess = this->WriteChipReg(theCic, cRegName, startAutoBX0Alignment);
    if(!cSuccess)
    {
        LOG(INFO) << BOLDRED << "Cannot send external BX0 alignment request to CIC on Board id " << +theCic->getBeBoardId() << " OpticalGroup id" << +theCic->getOpticalGroupId() << " Hybrid id "
                  << +theCic->getHybridId() << " --- Hybrid will be disabled" << RESET;
        ExceptionHandler::getInstance()->disableHybrid(theCic->getBeBoardId(), theCic->getOpticalGroupId(), theCic->getHybridId());
        return false;
    }

    // BX0doneRegValue2 = ReadChipReg(theCic, "timingStatusBits");
    // std::cout << " timingStatusBits after start BX0 alignment "<<  std::bitset<5>(BX0doneRegValue2) << std::endl;

    // auto bx0delay = this->retrieveExternalBX0AlignmentValue(theCic);
    // std::cout << " BXO delay before resync " << bx0delay << std::endl;
    return cSuccess;
}

bool CicInterface::CheckAutomatedBX0Alignment(Chip* pChip)
{
    setBoard(pChip->getBeBoardId());
    LOG(INFO) << BOLDMAGENTA << "Check automated BX0 alignment in CIC on FE" << +pChip->getHybridId() << RESET;
    bool cSuccess;
    bool alignmentCompleted = false;
    auto cRegValue          = ReadChipReg(pChip, "timingStatusBits");
    // std::cout << " timingStatusBits "<<  std::bitset<5>(cRegValue) << std::endl;
    // std::cout << (cRegValue & 0x02) << std::endl;
    if((cRegValue & 0x02) == 2)
    {
        auto bx0delay = this->retrieveExternalBX0AlignmentValue(pChip);
        LOG(INFO) << BOLDGREEN << " AUTO_BXO_DONE " << (cRegValue & 0x02) << " delay found " << bx0delay << RESET;
        std::string cRegName        = "BX0_ALIGN_CONFIG";
        uint16_t    cRegValueConfig = this->ReadChipReg(pChip, cRegName);
        LOG(INFO) << BOLDBLUE << "BX0_ALIGN_CONFIG after BXO done set to 0x" << std::hex << cRegValueConfig << std::dec << " bin " << std::bitset<8>(cRegValueConfig) << RESET;

        alignmentCompleted = true;
    }

    std::string cRegName = "BX0_ALIGN_CONFIG";
    cRegValue            = ReadChipReg(pChip, cRegName);

    LOG(INFO) << BOLDBLUE << "Requesting CIC to stop automated BX0 alignment..." << RESET;
    uint16_t stopAutoBX0Alignment = (cRegValue & 0xBF) | (0x0 << 6);
    LOG(INFO) << BOLDBLUE << " BX0_ALIGN_CONFIG set to 0x" << std::hex << stopAutoBX0Alignment << std::dec << " bin " << std::bitset<8>(stopAutoBX0Alignment) << " to stop BX0 alignment " << RESET;
    cSuccess = this->WriteChipReg(pChip, cRegName, stopAutoBX0Alignment);
    if(!cSuccess)
    {
        LOG(INFO) << BOLDRED << "Cannot disable automated Word alignment request on CIC on Board id " << +pChip->getBeBoardId() << " OpticalGroup id" << +pChip->getOpticalGroupId() << " Hybrid id "
                  << +pChip->getHybridId() << " --- Hybrid will be disabled" << RESET;
        ExceptionHandler::getInstance()->disableHybrid(pChip->getBeBoardId(), pChip->getOpticalGroupId(), pChip->getHybridId());
        return false;
    }

    return cSuccess && alignmentCompleted;
}

// run automated BX0 alignment
// assumes FEs have been configured to output alignment pattern
bool CicInterface::AutomatedBX0Alignment(Chip* pChip, std::vector<uint8_t> pAlignmentPatterns)
{
    setBoard(pChip->getBeBoardId());
    LOG(INFO) << BOLDBLUE << "Running automated BX0 alignment in CIC on FE" << +pChip->getHybridId() << RESET;
    LOG(INFO) << BOLDBLUE << "Configuring BX0 alignment patterns on CIC" << RESET;
    bool cSuccess;
    cSuccess = ConfigureAlignmentPatterns(pChip, pAlignmentPatterns);
    // cSuccess = ConfigureAlignmentPatterns(pChip, {0x80,0x00,0x00,0x00,0x00});
    if(!cSuccess)
    {
        LOG(INFO) << BOLDRED << "Cannot configure patterns on CIC on Board id " << +pChip->getBeBoardId() << " OpticalGroup id" << +pChip->getOpticalGroupId() << " Hybrid id " << +pChip->getHybridId()
                  << " --- Hybrid will be disabled" << RESET;
        ExceptionHandler::getInstance()->disableHybrid(pChip->getBeBoardId(), pChip->getOpticalGroupId(), pChip->getHybridId());
        return false;
    }

    std::string cRegName  = "BX0_ALIGN_CONFIG";
    uint16_t    cRegValue = this->ReadChipReg(pChip, cRegName);
    LOG(INFO) << BOLDBLUE << "BX0_ALIGN_CONFIG intially was set to 0x" << std::hex << cRegValue << std::dec << " bin " << std::bitset<8>(cRegValue) << RESET;

    // set to read another chip or line then 0 0
    uint16_t useChip = (cRegValue & 0xC7) | (0x2 << 3);
    // std::cout << " useChip " << useChip << std::endl;
    this->WriteChipReg(pChip, cRegName, useChip);
    cRegValue = this->ReadChipReg(pChip, cRegName);
    LOG(INFO) << BOLDBLUE << "BX0_ALIGN_CONFIG set to 0x" << std::hex << cRegValue << std::dec << " bin " << std::bitset<8>(cRegValue) << " to use another chip or line" << RESET;

    // set to read another chip or line then 0 0
    uint16_t useLine = (cRegValue & 0xF8) | (0x2 << 0);
    // std::cout << " useLine " << useLine << std::endl;
    this->WriteChipReg(pChip, cRegName, useLine);
    cRegValue = this->ReadChipReg(pChip, cRegName);
    LOG(INFO) << BOLDBLUE << "BX0_ALIGN_CONFIG set to 0x" << std::hex << cRegValue << std::dec << " bin " << std::bitset<8>(cRegValue) << " to use another chip or line" << RESET;

    uint16_t useInternalBX0Delay = (cRegValue & 0x7F) | (0x0 << 7); // required for automatic alignment
    LOG(INFO) << BOLDBLUE << "BX0_ALIGN_CONFIG set to 0x" << std::hex << useInternalBX0Delay << std::dec << " bin " << std::bitset<8>(useInternalBX0Delay) << " to use internal delay" << RESET;
    cSuccess = this->WriteChipReg(pChip, cRegName, useInternalBX0Delay);
    if(!cSuccess)
    {
        LOG(INFO) << BOLDRED << "Cannot disable external BX0 alignment value on CIC on Board id " << +pChip->getBeBoardId() << " OpticalGroup id" << +pChip->getOpticalGroupId() << " Hybrid id "
                  << +pChip->getHybridId() << " --- Hybrid will be disabled" << RESET;
        ExceptionHandler::getInstance()->disableHybrid(pChip->getBeBoardId(), pChip->getOpticalGroupId(), pChip->getHybridId());
        return false;
    }

    // auto BX0doneRegValue2 = ReadChipReg(pChip, "timingStatusBits");
    // std::cout << " timingStatusBits after using internal delays "<<  std::bitset<5>(BX0doneRegValue2) << std::endl;

    cRegValue                      = this->ReadChipReg(pChip, cRegName);
    uint16_t startAutoBX0Alignment = (cRegValue & 0xBF) | (0x1 << 6);
    LOG(INFO) << BOLDBLUE << "BX0_ALIGN_CONFIG set to 0x" << std::hex << startAutoBX0Alignment << std::dec << " bin " << std::bitset<8>(startAutoBX0Alignment) << " to start BX0 alignment " << RESET;
    cSuccess = this->WriteChipReg(pChip, cRegName, startAutoBX0Alignment);
    if(!cSuccess)
    {
        LOG(INFO) << BOLDRED << "Cannot send external BX0 alignment request to CIC on Board id " << +pChip->getBeBoardId() << " OpticalGroup id" << +pChip->getOpticalGroupId() << " Hybrid id "
                  << +pChip->getHybridId() << " --- Hybrid will be disabled" << RESET;
        ExceptionHandler::getInstance()->disableHybrid(pChip->getBeBoardId(), pChip->getOpticalGroupId(), pChip->getHybridId());
        return false;
    }

    LOG(INFO) << BOLDBLUE << "Running automated BX0 alignment .... " << RESET;
    uint8_t trials = 100;
    // check if BX0 alingment is done
    uint8_t maxNumberOfIterations  = 100;
    uint8_t currentIterationNumber = 0;
    uint8_t currentTrial           = 0;
    bool    alignmentCompleted     = false;
    while(currentTrial < trials)
    {
        // std::cout << " currentTrial " << +currentTrial << std::endl;
        alignmentCompleted = false;

        // auto BX0doneRegValue = ReadChipReg(pChip, "timingStatusBits");
        // std::cout << " timingStatusBits after starting BX0 alignment "<<  std::bitset<5>(BX0doneRegValue) << std::endl;
        // if((BX0doneRegValue & 0x02) == 2) alignmentCompleted = true;

        // if(!alignmentCompleted)
        // {

        while(currentIterationNumber < maxNumberOfIterations)
        {
            LOG(INFO) << RED << " Iteration " << +currentIterationNumber << RESET;
            // remember to send a resync
            fBoardFW->ChipReSync();

            // std::this_thread::sleep_for(std::chrono::microseconds(10));
            auto cRegValue = ReadChipReg(pChip, "timingStatusBits");
            // std::cout << " timingStatusBits "<<  std::bitset<5>(cRegValue) << std::endl;
            // std::cout << (cRegValue & 0x02) << std::endl;
            if((cRegValue & 0x02) == 2)
            {
                auto bx0delay = this->retrieveExternalBX0AlignmentValue(pChip);
                LOG(INFO) << BOLDGREEN << " AUTO_BXO_DONE " << (cRegValue & 0x02) << " delay found " << bx0delay << RESET;

                alignmentCompleted = true;
                break;
            }
            ++currentIterationNumber;
        }
        // }
        ++currentTrial;
    }
    if(!alignmentCompleted) return false;

    LOG(INFO) << BOLDBLUE << "Requesting CIC to stop automated BX0 alignment..." << RESET;
    uint16_t stopAutoBX0Alignment = (cRegValue & 0xBF) | (0x0 << 6);
    LOG(INFO) << BOLDBLUE << " BX0_ALIGN_CONFIG set to 0x" << std::hex << stopAutoBX0Alignment << std::dec << " bin " << std::bitset<8>(stopAutoBX0Alignment) << " to stop BX0 alignment " << RESET;

    cSuccess = this->WriteChipReg(pChip, cRegName, stopAutoBX0Alignment);
    if(!cSuccess)
    {
        LOG(INFO) << BOLDRED << "Cannot disable automated Word alignment request on CIC on Board id " << +pChip->getBeBoardId() << " OpticalGroup id" << +pChip->getOpticalGroupId() << " Hybrid id "
                  << +pChip->getHybridId() << " --- Hybrid will be disabled" << RESET;
        ExceptionHandler::getInstance()->disableHybrid(pChip->getBeBoardId(), pChip->getOpticalGroupId(), pChip->getHybridId());
        return false;
    }

    return cSuccess;
}

bool CicInterface::ResetDLL(Chip* pChip, uint16_t pWait_ms)
{
    setBoard(pChip->getBeBoardId());
    LOG(INFO) << BOLDBLUE << "Resetting DLL in CIC" << +pChip->getHybridId() << RESET;
    // apply a channel reset
    LOG(DEBUG) << BOLDBLUE << "\t.... Enabling RESET on DLL" << RESET;

    // enable resets
    std::vector<std::pair<std::string, uint16_t>> listOfRegisters;
    for(uint8_t cIndex = 0; cIndex < 2; cIndex += 1)
    {
        std::stringstream cBuffer;
        cBuffer << "scDllResetReq" << +cIndex;
        std::string cRegName(cBuffer.str());
        listOfRegisters.push_back(std::make_pair(cRegName, 0xFF));
    }
    if(!this->WriteChipMultReg(pChip, listOfRegisters))
    {
        LOG(INFO) << BOLDRED << "Error setting CIC DLL reset on Board id " << +pChip->getBeBoardId() << " OpticalGroup id" << +pChip->getOpticalGroupId() << " Hybrid id " << +pChip->getHybridId()
                  << " --- Hybrid will be disabled" << RESET;
        ExceptionHandler::getInstance()->disableHybrid(pChip->getBeBoardId(), pChip->getOpticalGroupId(), pChip->getHybridId());
        return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(pWait_ms));

    // disable resets
    for(auto& registerAndValue: listOfRegisters) registerAndValue.second = 0x00;
    if(!this->WriteChipMultReg(pChip, listOfRegisters))
    {
        LOG(INFO) << BOLDRED << "Error setting CIC DLL reset on Board id " << +pChip->getBeBoardId() << " OpticalGroup id" << +pChip->getOpticalGroupId() << " Hybrid id " << +pChip->getHybridId()
                  << " --- Hybrid will be disabled" << RESET;
        ExceptionHandler::getInstance()->disableHybrid(pChip->getBeBoardId(), pChip->getOpticalGroupId(), pChip->getHybridId());
        return false;
    }
    return true;
}
// check DLL lock in CIC
bool CicInterface::CheckDLL(Chip* pChip)
{
    uint16_t cRegAddress = 0x9A;
    setBoard(pChip->getBeBoardId());
    LOG(INFO) << BOLDBLUE << "Checking DLL lock in CIC" << +pChip->getHybridId() << RESET;
    std::vector<ChipRegItem> cRegItems;
    for(int cIndx = 0; cIndx < 2; cIndx += 1)
    {
        ChipRegItem cRegItem;
        cRegItem.fPage      = 0x00;
        cRegItem.fAddress   = cRegAddress + cIndx;
        cRegItem.fStatusReg = 0x01;
        cRegItems.push_back(cRegItem);
    }
    auto     cValues = fBoardFW->MultiRegisterRead(pChip, cRegItems);
    uint16_t cLock   = (cValues.at(1) << 8) | cValues.at(0);
    bool     cLocked = (cLock == 0xFFF);
    return cLocked;
}
bool CicInterface::SetAutomaticPhaseAlignment(Chip* pChip, bool pAuto)
{
    setBoard(pChip->getBeBoardId());
    if(pAuto)
        LOG(INFO) << BOLDBLUE << "Configuring CIC" << +pChip->getHybridId() << " to use automatic phase aligner..." << RESET;
    else
        LOG(INFO) << BOLDBLUE << "Configuring CIC" << +pChip->getHybridId() << " to use static phase aligner..." << RESET;
    // set phase aligner in static mode
    std::string cRegName  = "PHY_PORT_CONFIG";
    uint16_t    cRegValue = this->ReadChipReg(pChip, cRegName);
    uint16_t    cValue    = ((cRegValue & 0x07) | (pAuto << 3));
    bool        cSuccess  = this->WriteChipReg(pChip, cRegName, cValue);
    if(!cSuccess)
    {
        LOG(INFO) << BOLDRED << "Error setting automatic phase alignment in CI on Board id " << +pChip->getBeBoardId() << " OpticalGroup id" << +pChip->getOpticalGroupId() << " Hybrid id "
                  << +pChip->getHybridId() << " --- Hybrid will be disabled" << RESET;
        ExceptionHandler::getInstance()->disableHybrid(pChip->getBeBoardId(), pChip->getOpticalGroupId(), pChip->getHybridId());
        return false;
    }
    else
        LOG(INFO) << BOLDBLUE << "Phase aligner mode set with register " << cRegName << " to 0x" << std::hex << cValue << std::dec << RESET;
    if(pAuto) { this->ResetPhaseAligner(pChip); }
    return cSuccess;
}
bool CicInterface::PhaseAlignerPorts(Chip* pChip, uint8_t pState)
{
    bool cSuccess = false;
    setBoard(pChip->getBeBoardId());
    if(pState == 1)
        LOG(INFO) << BOLDGREEN << "Enabling " << BOLDBLUE << " all CIC phase aligner input..." << RESET;
    else
        LOG(INFO) << BOLDRED << "Disabling " << BOLDBLUE << " all CIC phase aligner input..." << RESET;
    for(uint8_t cIndex = 0; cIndex < 6; cIndex += 1)
    {
        // char cBuffer[14];
        // sprintf(cBuffer, "scEnableLine%.1d", cIndex);
        std::stringstream cBuffer;
        cBuffer << "scEnableLine" << +cIndex;
        std::string cRegName(cBuffer.str()); //, sizeof(cBuffer));
        // std::string cRegName = std::string(cBuffer, sizeof(cBuffer));
        cSuccess = this->WriteChipReg(pChip, cRegName, (pState == 1) ? 0xFF : 0x00);
        if(!cSuccess)
        {
            LOG(INFO) << BOLDRED << "Error selecting phase aligner ports on CI on Board id " << +pChip->getBeBoardId() << " OpticalGroup id" << +pChip->getOpticalGroupId() << " Hybrid id "
                      << +pChip->getHybridId() << " --- Hybrid will be disabled" << RESET;
            ExceptionHandler::getInstance()->disableHybrid(pChip->getBeBoardId(), pChip->getOpticalGroupId(), pChip->getHybridId());
            return false;
        }
    }
    return cSuccess;
}
bool CicInterface::ResetPhaseAligner(Chip* pChip, uint16_t pWait_ms)
{
    // LOG(DEBUG) << BOLDBLUE << "Resetting CIC phase aligner..." << RESET;
    // apply a channel reset
    // LOG(DEBUG) << BOLDBLUE << "\t.... Enabling RESET on all phase aligner inputs" << RESET;
    std::vector<std::pair<std::string, uint16_t>> resetEnableRegisterVector;
    resetEnableRegisterVector.push_back({"scResetChannels0", 0xFF});
    resetEnableRegisterVector.push_back({"scResetChannels1", 0xFF});
    bool cSuccess = WriteChipMultReg(pChip, resetEnableRegisterVector);

    std::this_thread::sleep_for(std::chrono::milliseconds(pWait_ms));
    // this->CheckPhaseAlignerLock(pChip, 0x00);
    // release channel reset
    // LOG(DEBUG) << BOLDBLUE << "\t... Disabling RESET on all phase aligner inputs" << RESET;
    std::vector<std::pair<std::string, uint16_t>> resetDiasbleRegisterVector;
    resetDiasbleRegisterVector.push_back({"scResetChannels0", 0x00});
    resetDiasbleRegisterVector.push_back({"scResetChannels1", 0x00});
    cSuccess = cSuccess && WriteChipMultReg(pChip, resetDiasbleRegisterVector);

    if(!cSuccess)
    {
        LOG(INFO) << BOLDRED << "Error setting CIC phase aligner reset on Board id " << +pChip->getBeBoardId() << " OpticalGroup id" << +pChip->getOpticalGroupId() << " Hybrid id "
                  << +pChip->getHybridId() << " --- Hybrid will be disabled" << RESET;
        ExceptionHandler::getInstance()->disableHybrid(pChip->getBeBoardId(), pChip->getOpticalGroupId(), pChip->getHybridId());
        return false;
    }

    return cSuccess;
}
bool CicInterface::SetStaticPhaseAlignment(Chip* pChip) { return SetAutomaticPhaseAlignment(pChip, false); }

bool CicInterface::ConfigureExternalWordAlignment(Chip* pChip, const GenericDataArray<uint8_t, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS - 1>& theWordAlignmentValues)
{
    size_t  cCounter = 0;
    uint8_t cValue   = 0x00;
    int     cIndx    = 0;
    bool    cSuccess = true;
    for(size_t cFeId = 0; cFeId < 8; cFeId++)
    {
        for(size_t cLine = 0; cLine < 5; cLine++)
        {
            uint8_t cAlVal = (theWordAlignmentValues.at(cFeId).at(cLine) & 0xF);
            cValue         = cValue | (cAlVal << (cCounter % 2) * 4);
            if((1 + cCounter) % 2 == 0)
            {
                std::string cRegName = "EXT_WA_DELAY" + (boost::format("%|02|") % cIndx).str();
                cSuccess             = cSuccess && this->WriteChipReg(pChip, cRegName, cValue);
                cValue               = 0x00;
                cIndx++;
            }
            cCounter++;
        }
    }
    return cSuccess;
}
bool CicInterface::ConfigureExternalBX0Delay(Chip* pChip, const uint16_t theBX0AlignmentValues)
{
    bool        cSuccess = true;
    std::string cRegName = "EXT_BX0_DELAY";
    cSuccess             = cSuccess && this->WriteChipReg(pChip, cRegName, theBX0AlignmentValues);

    cRegName           = "BX0_ALIGN_CONFIG";
    uint16_t cRegValue = this->ReadChipReg(pChip, cRegName);
    uint16_t cValue    = ((cRegValue & 0x7F) | (0x1 << 7));
    LOG(INFO) << BOLDBLUE << " BX0_ALIGN_CONFIG set to 0x" << std::hex << cValue << std::dec << " bin " << std::bitset<8>(cValue) << RESET;
    cSuccess = cSuccess && this->WriteChipReg(pChip, cRegName, cValue);
    return cSuccess;
}

bool CicInterface::SetStaticWordAlignment(Chip* pChip)
{
    std::string cRegName  = "MISC_CTRL";
    uint16_t    cRegValue = this->ReadChipReg(pChip, cRegName);
    uint8_t     cValue    = ((cRegValue & 0x1D) | (0x1 << 1));

    bool cSuccess = this->WriteChipReg(pChip, cRegName, cValue);
    return cSuccess;
}

GenericDataArray<uint8_t, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS - 1> CicInterface::retrieveExternalWordAlignmentValues(Chip* pChip)
{
    GenericDataArray<uint8_t, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS - 1> theWordAlignmentValues;

    uint8_t cLineCounter = 0;
    uint8_t cFECounter   = 0;
    for(uint8_t cIndex = 0; cIndex < 20; cIndex += 1)
    {
        std::string cRegName  = "WA_DELAY" + (boost::format("%|02|") % int(cIndex)).str();
        auto        cRegValue = ReadChipReg(pChip, cRegName);

        LOG(DEBUG) << BOLDBLUE << "Word alignment value found to be " << std::bitset<8>(cRegValue) << RESET;
        for(uint8_t cNibble = 0; cNibble < 2; cNibble += 1)
        {
            uint8_t cWordAlignment                                 = (cRegValue & (0xF << cNibble * 4)) >> 4 * cNibble;
            theWordAlignmentValues.at(cFECounter).at(cLineCounter) = cWordAlignment;
            LOG(DEBUG) << BOLDBLUE << "Word alignment for FE" << +cFECounter << " Line" << +cLineCounter << " value found to be " << +cWordAlignment << RESET;
            cLineCounter += 1;
            if(cLineCounter > 4)
            {
                cLineCounter = 0;
                cFECounter += 1;
            }
        }
    }

    return theWordAlignmentValues;
}

uint16_t CicInterface::retrieveExternalBX0AlignmentValue(Chip* pChip)
{
    std::string cRegName             = "BX0_DELAY";
    uint16_t    theBX0AlignmentValue = ReadChipReg(pChip, cRegName);

    LOG(INFO) << BOLDBLUE << "BX0 alignment value found to be " << std::bitset<8>(theBX0AlignmentValue) << RESET;
    return theBX0AlignmentValue;
}

std::pair<uint8_t, uint8_t> CicInterface::GetPhyPortConfig(Chip* pChip, uint8_t pFeId, uint8_t pLineId)
{
    std::pair<uint8_t, uint8_t> cCnfg;
    std::vector<uint8_t>        cFeMapping  = static_cast<Cic*>(pChip)->getMapping();
    size_t                      cNSLVSLines = 6; // 6 stub lines from a front-end chip to the CIC - 5 stubs + 1 L1
    std::vector<uint8_t>        cPhaseTaps(cNSLVSLines, 15);

    // read back phase alignment on stub lines - 6 stub lines
    size_t cPortCounter       = 0;
    size_t cInputCounter      = 0;
    size_t cFeCounter         = 0;
    size_t cInputLineCounter  = 0;
    size_t cCounter           = 0;
    size_t cNStubLines        = 5;
    size_t cL1Line            = 5;
    bool   cLastStubLineFound = false;
    bool   cFound             = false;
    for(int cIndex = 0; cIndex < 6; cIndex++)
    {
        if(cFound) break;
        // 1 bit per line
        for(size_t cBitIndex = 0; cBitIndex < 8; cBitIndex++)
        {
            if(cFound) break;

            cInputCounter      = (cBitIndex & 0x3);
            cInputLineCounter  = (cIndex < 5) ? (cCounter % cNStubLines) : cL1Line;
            cLastStubLineFound = cLastStubLineFound || (cFeCounter == 7 && cInputLineCounter == 4);
            cFeCounter         = (cLastStubLineFound) ? cBitIndex : cFeCounter;

            // get CIC FEId
            uint8_t cChipId_forCic = cFeMapping.at(pFeId);
            if(cFeCounter == cChipId_forCic && cInputLineCounter == pLineId)
            {
                LOG(DEBUG) << BOLDYELLOW << "FE#" << +pFeId << " Line#" << +pLineId << " corresponds to PhyPort#" << +cPortCounter << " input#" << +cInputCounter << " which is CIC_FE#"
                           << +cChipId_forCic << RESET;
                cCnfg.first  = cPortCounter;
                cCnfg.second = cInputCounter;
                cFound       = true;
            }
            cPortCounter += (cBitIndex == 3) || (cBitIndex == 7);
            cFeCounter = (!cLastStubLineFound) ? (cFeCounter + (((1 + cCounter) % cNStubLines == 0) ? 1 : 0)) : cBitIndex;
            cCounter++;
        }
    } // each register stores information from 4 phyport inputs
    return cCnfg;
}
bool CicInterface::SetFePhaseTap(Chip* pChip, uint8_t pFeId, uint8_t pLineId, int pPhaseTap)
{
    auto cCnfg = GetPhyPortConfig(pChip, pFeId, pLineId);
    LOG(DEBUG) << BOLDYELLOW << "FE#" << +pFeId << " corresponds to .. PhyPort#" << +cCnfg.first << " input#" << +cCnfg.second << RESET;
    return SetPhaseTap(pChip, cCnfg.first, cCnfg.second, pPhaseTap);
}

bool CicInterface::SetPhaseTap(Chip* pChip, uint8_t pPhyPort, uint8_t pPhyPortChannel, int pPhaseTap)
{
    bool cVerifloop = true;
    setBoard(pChip->getBeBoardId());
    auto cRegisterMap = pChip->getRegMap();

    ChipRegItem cRegItem;
    uint16_t    cBaseAddress = 0x25;
    uint16_t    cBaseReg     = cBaseAddress + pPhyPortChannel * 6;

    // 4 bits per phyPorts --> 12 phy ports --> 48 bits --> 6 registers
    uint8_t cRegOffset = (pPhyPort * 4 / 8);
    uint8_t cBitShift  = (pPhyPort % 2) * 4;

    cRegItem.fAddress = cBaseReg + cRegOffset;
    auto cIterator    = find_if(cRegisterMap.begin(), cRegisterMap.end(), [&cRegItem](const ChipRegPair& obj) { return obj.second.fAddress == cRegItem.fAddress; });
    cRegItem          = cRegisterMap.at(cIterator->first);
    uint8_t cRegMask  = (0xF << cBitShift); //
    cRegMask          = ~(cRegMask);
    auto cRegValue    = fBoardFW->SingleRegisterRead(pChip, cRegItem);
    cRegItem.fValue   = (cRegValue & cRegMask) | (pPhaseTap << cBitShift);
    LOG(DEBUG) << BOLDGREEN << "Setting optimal tap for PhyPort" << +pPhyPort << " PhyPortChannel " << +pPhyPortChannel << " Register mask is 0x" << std::hex << +cRegMask << " Register 0x"
               << +(cBaseReg + cRegOffset) << " BitOffset " << +cBitShift << " to 0x" << +cRegItem.fValue << " to set a phase tap of 0x" << +pPhaseTap << " original register value was 0x"
               << +cRegValue << std::dec << RESET;
    return fBoardFW->SingleRegisterWrite(pChip, cRegItem, cVerifloop);
}

GenericDataArray<uint8_t, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS> CicInterface::getAllOptimalTaps(Chip* pChip)
{
    std::vector<std::string> phaseRegisterVector;
    for(uint8_t phyPortPair = 0; phyPortPair < 6; ++phyPortPair)
    {
        for(uint8_t channel = 0; channel < 4; ++channel)
        {
            std::stringstream phaseRegisterName;
            phaseRegisterName << "scPhaseSelectB" << +channel << "o" << +(phyPortPair);
            phaseRegisterVector.push_back(phaseRegisterName.str());
        }
    }
    auto phaseRegisterValueVector = ReadChipMultReg(pChip, phaseRegisterVector);

    // convert int a map for easier access
    std::unordered_map<std::string, uint8_t> phaseRegisterMap;
    for(const auto& registerNameAndValue: phaseRegisterValueVector) phaseRegisterMap[registerNameAndValue.first] = registerNameAndValue.second;

    auto getPhaseValue = [&phaseRegisterMap](uint8_t phyPort, uint8_t channel)
    {
        std::stringstream phaseRegisterName;
        phaseRegisterName << "scPhaseSelectB" << +channel << "o" << +phyPort / 2;
        return (phaseRegisterMap.at(phaseRegisterName.str()) >> (phyPort % 2 * 4)) & 0xF;
    };

    GenericDataArray<uint8_t, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS> theOptimalPhase2DArray;
    std::vector<uint8_t>                                                          cicFrontEndMapping = static_cast<Cic*>(pChip)->getMapping();
    for(uint8_t frontEnd = 0; frontEnd < NUMBER_OF_CIC_PORTS; ++frontEnd) // using the same Id of the chip
    {
        // L1 lines are on phyport 10 and 11 and go on first line of the ouput array
        auto l1PhyPortAndChannel                  = fromChipL1ToPhyPortAndChannel(pChip, cicFrontEndMapping, frontEnd);
        theOptimalPhase2DArray.at(frontEnd).at(0) = getPhaseValue(l1PhyPortAndChannel.first, l1PhyPortAndChannel.second);

        // Stub lines are on pyPort 0 to 9
        for(uint8_t line = 0; line < NUMBER_OF_LINES_PER_CIC_PORTS - 1; ++line)
        {
            auto stubPhyPortAndChannel                       = fromChipStubToPhyPortAndChannel(pChip, cicFrontEndMapping, frontEnd, line);
            theOptimalPhase2DArray.at(frontEnd).at(line + 1) = getPhaseValue(stubPhyPortAndChannel.first, stubPhyPortAndChannel.second);
        }
    }

    return theOptimalPhase2DArray;
}

GenericDataArray<bool, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS> CicInterface::getLineLocked(Chip* pChip)
{
    GenericDataArray<bool, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS> theIsLocked2DArray;

    std::vector<std::string> isLockedRegisterVector;
    for(size_t phyPortPair = 0; phyPortPair < 6; ++phyPortPair)
    {
        std::stringstream isLockedRegisterName;
        isLockedRegisterName << "scChannelLocked" << +phyPortPair;
        isLockedRegisterVector.push_back(isLockedRegisterName.str());
    }
    auto isLockedRegisterValueVector = ReadChipMultReg(pChip, isLockedRegisterVector);

    // convert int a map for easier access
    std::unordered_map<std::string, uint8_t> isLockedRegisterMap;
    for(const auto& registerNameAndValue: isLockedRegisterValueVector) isLockedRegisterMap[registerNameAndValue.first] = registerNameAndValue.second;

    auto isLocked = [&isLockedRegisterMap](uint8_t phyPort, uint8_t channel)
    {
        std::stringstream isLockedRegisterName;
        isLockedRegisterName << "scChannelLocked" << +phyPort / 2;
        bool isLocked = (isLockedRegisterMap.at(isLockedRegisterName.str()) >> (phyPort % 2 * 4 + channel)) & 0x1;
        return isLocked;
    };

    std::vector<uint8_t> cicFrontEndMapping = static_cast<Cic*>(pChip)->getMapping();

    for(uint8_t frontEnd = 0; frontEnd < NUMBER_OF_CIC_PORTS; ++frontEnd) // using the same Id of the chip
    {
        // L1 lines are on phyport 10 and 11 and go on first line of the ouput array
        auto l1PhyPortAndChannel = fromChipL1ToPhyPortAndChannel(pChip, cicFrontEndMapping, frontEnd);
        if(isLocked(l1PhyPortAndChannel.first, l1PhyPortAndChannel.second))
            theIsLocked2DArray.at(frontEnd).at(0) = true;
        else
            theIsLocked2DArray.at(frontEnd).at(0) = false;

        // Stub lines are on pyPort 0 to 9
        for(uint8_t line = 0; line < NUMBER_OF_LINES_PER_CIC_PORTS - 1; ++line)
        {
            auto stubPhyPortAndChannel = fromChipStubToPhyPortAndChannel(pChip, cicFrontEndMapping, frontEnd, line);
            if(isLocked(stubPhyPortAndChannel.first, stubPhyPortAndChannel.second))
                theIsLocked2DArray.at(frontEnd).at(line + 1) = true;
            else
                theIsLocked2DArray.at(frontEnd).at(line + 1) = false;
        }
    }

    return theIsLocked2DArray;
}

bool CicInterface::writeAllTaps(Ph2_HwDescription::Chip* pChip, GenericDataArray<uint8_t, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS> cicInputTaps)
{
    std::unordered_map<std::string, uint8_t> phaseRegisterMap;
    auto                                     setPhaseValue = [&phaseRegisterMap](uint8_t phyPort, uint8_t channel, uint8_t phase)
    {
        std::stringstream phaseRegisterName;
        phaseRegisterName << "scPhaseSelectB" << +channel << "i" << +phyPort / 2;
        auto& theCurrentRegisterValue = phaseRegisterMap[phaseRegisterName.str()];
        theCurrentRegisterValue       = (theCurrentRegisterValue & (0xF << ((phyPort + 1) % 2 * 4))) | ((phase & 0xF) << (phyPort % 2 * 4));
    };

    std::vector<uint8_t> cicFrontEndMapping = static_cast<Cic*>(pChip)->getMapping();
    for(uint8_t frontEnd = 0; frontEnd < NUMBER_OF_CIC_PORTS; ++frontEnd)
    {
        auto l1PhyPortAndChannel = fromChipL1ToPhyPortAndChannel(pChip, cicFrontEndMapping, frontEnd);
        setPhaseValue(l1PhyPortAndChannel.first, l1PhyPortAndChannel.second, cicInputTaps.at(frontEnd).at(0));
        for(uint8_t line = 0; line < NUMBER_OF_LINES_PER_CIC_PORTS - 1; ++line)
        {
            auto stubPhyPortAndChannel = fromChipStubToPhyPortAndChannel(pChip, cicFrontEndMapping, frontEnd, line);
            setPhaseValue(stubPhyPortAndChannel.first, stubPhyPortAndChannel.second, cicInputTaps.at(frontEnd).at(line + 1));
        }
    }

    std::vector<std::pair<std::string, uint16_t>> phaseRegisterVector;
    for(auto theRegister: phaseRegisterMap) { phaseRegisterVector.push_back(theRegister); }

    return WriteChipMultReg(pChip, phaseRegisterVector);
}

std::pair<uint8_t, uint8_t> CicInterface::fromChipL1ToPhyPortAndChannel(Chip* pChip, std::vector<uint8_t> chipToCICMapping, uint8_t frontEndId)
{
    uint8_t chipIdForCic    = chipToCICMapping.at(frontEndId);
    uint8_t phyPortForL1    = 10 + chipIdForCic / 4;
    uint8_t phyChannelForL1 = chipIdForCic % 4;
    return std::make_pair(phyPortForL1, phyChannelForL1);
}

std::pair<uint8_t, uint8_t> CicInterface::fromChipStubToPhyPortAndChannel(Chip* pChip, std::vector<uint8_t> chipToCICMapping, uint8_t frontEndId, uint8_t stubLine)
{
    uint8_t chipIdForCic     = chipToCICMapping.at(frontEndId);
    uint8_t phyPortFoStub    = (chipIdForCic * 5 + stubLine) / 4;
    uint8_t phyChannelFoStub = (chipIdForCic * 5 + stubLine) % 4;
    return std::make_pair(phyPortFoStub, phyChannelFoStub);
}

std::pair<uint8_t, uint8_t> CicInterface::fromPhyPortAndChanneltoChipIdAndLine(Ph2_HwDescription::Chip* pChip, uint8_t phyPort, uint8_t channel)
{
    if(phyPort < 10)
    {
        uint16_t cumulativeNumberOfStubPort = phyPort * 4 + channel;
        return std::make_pair(fromCICFEidToChipId(pChip, cumulativeNumberOfStubPort / 5), cumulativeNumberOfStubPort % 5 + 1);
    }
    else { return std::make_pair(fromCICFEidToChipId(pChip, channel + 4 * (phyPort % 10)), 0); }
}

uint8_t CicInterface::fromChipIdToCICFEid(Ph2_HwDescription::Chip* pChip, uint8_t chipId)
{
    auto cicFEmapping = static_cast<Cic*>(pChip)->getMapping();
    return cicFEmapping.at(chipId % 8);
}

uint8_t CicInterface::fromCICFEidToChipId(Ph2_HwDescription::Chip* pChip, uint8_t cicFEid)
{
    auto cicFEmapping = static_cast<Cic*>(pChip)->getMapping();
    return std::find_if(cicFEmapping.begin(), cicFEmapping.end(), [cicFEid](uint8_t value) { return value == cicFEid; }) - cicFEmapping.begin();
}

bool CicInterface::CheckPhaseAlignerLock(Chip* pChip, uint8_t pCheckValue)
{
    // first .. get enabled FEs
    setBoard(pChip->getBeBoardId());

    std::vector<std::string> theLockedRegisterVector{"scChannelLocked0", "scChannelLocked1", "scChannelLocked2", "scChannelLocked3", "scChannelLocked4", "scChannelLocked5"};
    auto                     isLockedRegisterVector = ReadChipMultReg(pChip, theLockedRegisterVector);

    uint16_t    cRegBaseAddress = 0xA0;
    ChipRegItem cRegItem;
    bool        cLocked = true;

    size_t cFeCounter         = 0;
    size_t cInputLineCounter  = 0;
    size_t cCounter           = 0;
    size_t cNStubLines        = 5;
    size_t cL1Line            = 5;
    bool   cLastStubLineFound = false;

    std::vector<uint8_t>        cFeMapping = static_cast<Cic*>(pChip)->getMapping();
    std::vector<std::bitset<6>> theFeStates(8, 0);

    // read back phase alignment on stub lines
    for(int cIndex = 0; cIndex < 6; cIndex++)
    {
        cRegItem.fPage      = 0x00;
        cRegItem.fAddress   = cRegBaseAddress + cIndex;
        cRegItem.fStatusReg = 0x01;
        auto cRegValue      = fBoardFW->SingleRegisterRead(pChip, cRegItem);

        for(size_t cBitIndex = 0; cBitIndex < 8; cBitIndex++)
        {
            cInputLineCounter                             = (cIndex < 5) ? (cCounter % cNStubLines) : cL1Line;
            cLastStubLineFound                            = cLastStubLineFound || (cFeCounter == 7 && cInputLineCounter == 4);
            cFeCounter                                    = (cLastStubLineFound) ? cBitIndex : cFeCounter;
            theFeStates.at(cFeCounter)[cInputLineCounter] = std::bitset<8>(cRegValue)[cBitIndex];

            cFeCounter = (!cLastStubLineFound) ? (cFeCounter + (((1 + cCounter) % cNStubLines == 0) ? 1 : 0)) : cBitIndex;
            cCounter++;
        }
    } // each register stores information from 4 phyport inputs
    // 1 bit for each of the 4 channels of the 12 PHYPorts

    for(cFeCounter = 0; cFeCounter < 8; cFeCounter++)
    {
        auto cCheckValue = (pCheckValue & (0x1 << cFeCounter)) >> cFeCounter;
        for(cInputLineCounter = 0; cInputLineCounter < (1 + cNStubLines); cInputLineCounter++) { cLocked = cLocked & (theFeStates.at(cFeCounter)[cInputLineCounter] == cCheckValue); }
    }
    return cLocked;
}

bool CicInterface::SoftReset(Chip* pChip, uint32_t cWait_ms)
{
    setBoard(pChip->getBeBoardId());

    std::string cRegName   = "MISC_CTRL";
    uint16_t    cRegValue  = this->ReadChipReg(pChip, cRegName);
    uint16_t    cToggleOn  = (cRegValue & 0x0F) | (0x1 << 4);
    uint16_t    cToggleOff = (cRegValue & 0x0F) | (0x0 << 4);

    LOG(DEBUG) << BOLDBLUE << "Setting register " << cRegName << " to " << std::bitset<5>(cToggleOn) << " to toggle ON soft reset." << RESET;
    if(!this->WriteChipReg(pChip, cRegName, cToggleOn)) return false;
    std::this_thread::sleep_for(std::chrono::milliseconds(cWait_ms));

    LOG(DEBUG) << BOLDBLUE << "Setting register " << cRegName << " to " << std::bitset<5>(cToggleOff) << " to toggle OFF soft reset." << RESET;
    if(!this->WriteChipReg(pChip, cRegName, cToggleOff)) return false;
    std::this_thread::sleep_for(std::chrono::milliseconds(cWait_ms));
    return true;
}
bool CicInterface::SelectOutput(Chip* pChip, bool pFixedPattern)
{
    setBoard(pChip->getBeBoardId());
    if(pFixedPattern)
        LOG(DEBUG) << BOLDBLUE << "Want to configure CIC to output fixed pattern on all lines... " << RESET;
    else
        LOG(DEBUG) << BOLDBLUE << "Want to configure CIC to output data from readout chips... " << RESET;

    // enable output pattern from CIC
    std::string cRegName  = "MISC_CTRL";
    uint16_t    cRegValue = this->ReadChipReg(pChip, cRegName);
    uint16_t    cValue    = ((cRegValue & 0x1B) | (static_cast<uint8_t>(pFixedPattern) << 2));

    if(!this->WriteChipReg(pChip, cRegName, cValue)) return false;

    cRegValue = this->ReadChipReg(pChip, cRegName);
    LOG(DEBUG) << BOLDBLUE << "CIC output pattern configured by setting " << cRegName << " to " << std::bitset<8>(cRegValue) << RESET;
    return true;
}
bool CicInterface::SetSparsification(Chip* pChip, uint8_t pEnable)
{
    std::string cRegName  = "FE_CONFIG";
    uint16_t    cRegValue = this->ReadChipReg(pChip, cRegName);
    uint16_t    cValue    = (cRegValue & 0x2F) | (pEnable << 4);
    return this->WriteChipReg(pChip, cRegName, cValue);
}
bool CicInterface::EnableFEs(Chip* pChip, std::vector<uint8_t> pFeIds, bool pEnable)
{
    setBoard(pChip->getBeBoardId());

    //  read type of CIC to figure out which mapping to use
    std::vector<uint8_t> cFeMapping = static_cast<Cic*>(pChip)->getMapping();
    // read enable register
    std::string cRegName = "FE_ENABLE";
    uint16_t    cValue   = this->ReadChipReg(pChip, cRegName);
    // LOG (INFO) << BOLDMAGENTA << "FE_ENABLE register set to 0x" << std::hex  << +cValue << std::dec << RESET;
    for(auto pFeId: pFeIds)
    {
        uint8_t cChipId_forCic = cFeMapping.at(pFeId); // std::distance(fFeMapping.begin(), std::find(fFeMapping.begin(), fFeMapping.end(), pFeId));
        uint8_t cMask          = ~(0x1 << cChipId_forCic) & 0xFF;
        // LOG(INFO) << BOLDMAGENTA << "For ROC [Hybrid Id " << +pFeId << "] CIC FE#" << +cChipId_forCic << " mask is " << std::bitset<8>(cMask) << RESET;
        cValue = (cValue & cMask) | (static_cast<uint8_t>(pEnable) << cChipId_forCic);
    }
    return this->WriteChipReg(pChip, cRegName, cValue);
}

bool CicInterface::configureEnabledFEs(Chip* pChip, std::vector<uint8_t> pFeIds)
{
    setBoard(pChip->getBeBoardId());
    //  read type of CIC to figure out which mapping to use
    std::vector<uint8_t> cFeMapping = static_cast<Cic*>(pChip)->getMapping();
    // read enable register
    std::string cRegName = "FE_ENABLE";
    uint8_t     cValue   = 0;
    for(auto pFeId: pFeIds)
    {
        uint8_t cChipId_forCic = cFeMapping.at(pFeId);
        cValue                 = cValue | (1u << cChipId_forCic);
    }
    LOG(DEBUG) << BOLDMAGENTA << "New value of FE_ENABLE for CIC is " << std::hex << +cValue << std::dec << RESET;
    return this->WriteChipReg(pChip, cRegName, cValue);
}

bool CicInterface::ConfigureStubOutput(Chip* pChip, uint8_t pLineSel)
{
    setBoard(pChip->getBeBoardId());
    std::string cRegName = "FE_CONFIG";
    auto        cFeType  = this->ReadChipReg(pChip, cRegName);
    bool        c2S      = ((cFeType & 0x01) == 0);
    uint8_t     cValue   = c2S ? 0 : 1;
    cRegName             = "FE_CONFIG";
    auto    cRegValue    = this->ReadChipReg(pChip, cRegName);
    uint8_t cMask        = c2S ? 0xFE : 0xF7;
    uint8_t cBitShift    = c2S ? 0 : 3;
    // if line select is not 0 . then don't auto configure
    cValue                = (pLineSel == 5 || pLineSel == 6) ? (uint8_t)(pLineSel == 6) : cValue;
    uint8_t cValueToWrite = (cRegValue & cMask) | (cValue << cBitShift);
    uint8_t cNlines       = 5 + cValue;
    LOG(INFO) << BOLDMAGENTA << "Configuring CIC" << +pChip->getHybridId() << " to produce stubs on " << +cNlines << "/6 output lines... writing 0x" << std::hex << +cValueToWrite << std::dec
              << " to CIC register " << cRegName << RESET;
    // std::cout << __PRETTY_FUNCTION__ << " [" << __LINE__ << "] writing " << cRegName << " to 0x" << std::hex << +cValueToWrite << std::dec << std::endl;
    return this->WriteChipReg(pChip, cRegName, cValueToWrite);
}
bool CicInterface::SelectMode(Chip* pChip, uint8_t pMode)
{
    setBoard(pChip->getBeBoardId());

    LOG(INFO) << BOLDBLUE << "Want to configure CIC mode : " << +pMode << RESET;
    std::string cRegName  = "FE_CONFIG";
    uint16_t    cRegValue = this->ReadChipReg(pChip, cRegName);
    uint16_t    cValue    = ((cRegValue & 0x3E) | pMode);
    if(pMode == 0) // for CBC mode .. always320 MHz and without last line
        cValue = (cValue & 0x35) | (pMode << 1) | (pMode << 3);

    if(!this->WriteChipReg(pChip, cRegName, cValue)) return false;

    cRegValue = this->ReadChipReg(pChip, cRegName);
    LOG(INFO) << BOLDBLUE << "Register " << cRegName << " set to " << std::bitset<6>(cRegValue) << " to select CIC Mode " << +pMode << RESET;
    return true;
}

bool CicInterface::CheckSoftReset(Chip* pChip)
{
    setBoard(pChip->getBeBoardId());
    LOG(INFO) << BOLDBLUE << "Checking if CIC requires a Soft reset." << RESET;
    ChipRegItem cRegItem;
    cRegItem.fPage        = 0x00;
    cRegItem.fStatusReg   = 0x01;
    cRegItem.fAddress     = 0xA6;
    auto cRegValue        = fBoardFW->SingleRegisterRead(pChip, cRegItem);
    bool cSoftResetNeeded = (((cRegValue & 0x04) >> 2) == 1);
    // if readback worked and the CIC says it needs a Resync then send
    // a resync
    if(cSoftResetNeeded)
    {
        LOG(INFO) << BOLDBLUE << "....... Reset  needed - sending one now ...... " << RESET;
        SoftReset(pChip);
    }
    // check if CIC still needs one
    cRegValue        = fBoardFW->SingleRegisterRead(pChip, cRegItem);
    cSoftResetNeeded = (((cRegValue & 0x04) >> 2) == 1);
    if(cSoftResetNeeded)
        return false;
    else
        return true;
}
bool CicInterface::SelectMux(Chip* pChip, uint8_t pPhyPort)
{
    // first .. enable bypass of logic
    if(!this->ControlMux(pChip, 1)) return false;

    // then select phy port
    LOG(DEBUG) << BOLDBLUE << "Selecting phyPort" << +pPhyPort << " on CIC on " << +pChip->getHybridId() << RESET;
    setBoard(pChip->getBeBoardId());
    std::string cRegName  = "MUX_CTRL";
    uint16_t    cRegValue = this->ReadChipReg(pChip, cRegName);
    uint16_t    cValue    = (cRegValue & 0x10) | pPhyPort;
    LOG(DEBUG) << BOLDBLUE << "Selecting phyPort [0-11]: " << +pPhyPort << " by setting register to 0x" << std::hex << +cValue << std::dec << RESET;
    return this->WriteChipReg(pChip, cRegName, cValue);
}
bool CicInterface::ControlMux(Chip* pChip, uint8_t pEnable)
{
    setBoard(pChip->getBeBoardId());
    std::string cRegName  = "MUX_CTRL";
    uint16_t    cRegValue = this->ReadChipReg(pChip, cRegName);
    uint16_t    cValue    = (cRegValue & 0xF) | (pEnable << 4);
    if(pEnable == 1)
        LOG(DEBUG) << BOLDBLUE << " Enabling CIC MUX .. so bypassing CIC logic " << RESET;
    else
        LOG(DEBUG) << BOLDBLUE << " Disabling CIC MUX .. so activating CIC logic  " << RESET;

    return this->WriteChipReg(pChip, cRegName, cValue);
}
bool CicInterface::ConfigureTermination(Chip* pChip, uint8_t pClkTerm, uint8_t pRxTerm)
{
    std::string cRegName  = "SLVS_PADS_CONFIG";
    uint16_t    cRegValue = this->ReadChipReg(pChip, cRegName);
    auto        cValue    = (pRxTerm << 4) | (pClkTerm << 3) | (cRegValue & 0x7);
    LOG(INFO) << BOLDBLUE << "Configuring termination on CIC CLk + Rx pads . register set to 0x" << std::hex << +cValue << std::dec << RESET;
    LOG(INFO) << BOLDBLUE << "\t\t.. Clk Term set to " << +pClkTerm << RESET;
    LOG(INFO) << BOLDBLUE << "\t\t.. Rx Term set to " << +pRxTerm << RESET;
    // std::cout << __PRETTY_FUNCTION__ << " [" << __LINE__ << "] SLVS_PADS_CONFIG  to 0x" << std::hex << cValue << std::dec << std::endl;
    return this->WriteChipReg(pChip, "SLVS_PADS_CONFIG", cValue);
}
// configure drive strength
bool CicInterface::ConfigureDriveStrength(Chip* pChip, uint8_t pDriveStrength)
{
    std::string cRegName  = "SLVS_PADS_CONFIG";
    uint16_t    cRegValue = this->ReadChipReg(pChip, cRegName);
    auto        cIterator = fTxDriveStrength.find(pDriveStrength);
    bool        cSuccess  = true;
    if(cIterator != fTxDriveStrength.end())
    {
        auto cValue = (cRegValue & 0xF8) | cIterator->second; //(cRxTermination << 4) | (cClkTermination << 3) | cIterator->second;
        cSuccess    = this->WriteChipReg(pChip, cRegName, cValue);
        LOG(DEBUG) << BOLDBLUE << "Configuring drive strength on CIC output pads: 0x" << std::hex << +cValue << std::dec << RESET;
        if(!cSuccess)
        {
            LOG(INFO) << BOLDRED << "Could not configure drive strength on CIC output pads on Board id " << +pChip->getBeBoardId() << " OpticalGroup id" << +pChip->getOpticalGroupId() << " Hybrid id "
                      << +pChip->getHybridId() << " --- Hybrid will be disabled" << RESET;
            ExceptionHandler::getInstance()->disableHybrid(pChip->getBeBoardId(), pChip->getOpticalGroupId(), pChip->getHybridId());
            return false;
        }
        cRegValue = this->ReadChipReg(pChip, cRegName);
        LOG(DEBUG) << BOLDGREEN << "SUCCESSFULLY " << BOLDBLUE << " configured drive strength on CIC output pads: 0x" << std::hex << +cRegValue << std::dec << "[ drive strength set to "
                   << +pDriveStrength << " ]" << RESET;
    }
    return cSuccess;
}

uint8_t CicInterface::ReadFCMDEdge(Chip* pChip)
{
    std::string cRegName  = "MISC_CTRL";
    auto        cRegValue = this->ReadChipReg(pChip, cRegName);
    uint8_t     cNegEdge  = ((cRegValue & 0x8) >> 3);
    if(cNegEdge == 1)
        LOG(INFO) << BOLDBLUE << "Fast command block in CIC locks on falling edge." << RESET;
    else
        LOG(INFO) << BOLDBLUE << "Fast command block in CIC locks on rising edge." << RESET;
    return cNegEdge;
}

// configure fast command edge
bool CicInterface::ConfigureFCMDEdge(Chip* pChip, uint8_t pUseNegEdge)
{
    // select fast command edge
    bool cNegEdge = (pUseNegEdge == 1); // was false for PS - need to check
    if(cNegEdge)
        LOG(INFO) << BOLDBLUE << "Configuring fast command block in CIC to lock on falling edge." << RESET;
    else
        LOG(INFO) << BOLDBLUE << "Configuring fast command block in CIC to lock on rising edge." << RESET;
    std::string cRegName  = "MISC_CTRL";
    auto        cRegValue = this->ReadChipReg(pChip, cRegName);
    uint16_t    cValue    = (cRegValue & 0x17) | (cNegEdge << 3);
    bool        cSuccess  = this->WriteChipReg(pChip, cRegName, cValue);
    if(!cSuccess)
    {
        LOG(INFO) << BOLDRED << "Error selecting FC edge in CIC on Board id " << +pChip->getBeBoardId() << " OpticalGroup id" << +pChip->getOpticalGroupId() << " Hybrid id " << +pChip->getHybridId()
                  << " --- Hybrid will be disabled" << RESET;
        ExceptionHandler::getInstance()->disableHybrid(pChip->getBeBoardId(), pChip->getOpticalGroupId(), pChip->getHybridId());
        return false;
    }
    return cSuccess;
}
// start-up sequence for CIC [everything that does not require interaction
// with the BE or the other readout ASICs on the chip
bool CicInterface::StartUp(Chip* pChip)
{
    std::string cOut = ".... Starting CIC start-up ........ on hybrid " + std::to_string(pChip->getHybridId()) + " for CIC2.";
    LOG(INFO) << BOLDBLUE << cOut << RESET;

    auto boardId        = pChip->getBeBoardId();
    auto opticalGroupId = pChip->getOpticalGroupId();
    auto hybridId       = pChip->getHybridId();

    auto exceptionHandleFunction = [boardId, opticalGroupId, hybridId, this](const std::string&& failMode)
    {
        LOG(INFO) << BOLDRED << "FAILED to " << failMode << " for Board id " << +boardId << " OpticalGroup id " << +opticalGroupId << " Hybrid id " << +hybridId << " --- Disabled" << RESET;
        ExceptionHandler::getInstance()->disableHybrid(boardId, opticalGroupId, hybridId);
    };

    if(!this->CheckSoftReset(pChip))
    {
        exceptionHandleFunction("clear CIC SOFT reset request");
        return false;
    }

    // reset DLL for each of the 12 phy ports
    if(!this->ResetDLL(pChip))
    {
        exceptionHandleFunction("Could not reset CIC DLL");
        return false;
    }

    // checking DLL lock
    if(!this->CheckDLL(pChip))
    {
        exceptionHandleFunction("Could not lock CIC DLL");
        return false;
    }
    LOG(INFO) << BOLDBLUE << "DLL in CIC " << BOLDGREEN << " LOCKED." << RESET;

    // check fast command lock
    if(!this->CheckFastCommandLock(pChip))
    {
        exceptionHandleFunction("Could not lock CIC DLL");
        return false;
    }

    LOG(INFO) << BOLDGREEN << "SUCCESSFULLY " << BOLDBLUE << " locked fast command decoder in CIC." << RESET;
    return true;
}

} // namespace Ph2_HwInterface
