#include "HWInterface/VTRxInterface.h"
#include "HWDescription/VTRx.h"
#include "HWDescription/lpGBT.h"
#include "HWInterface/BeBoardFWInterface.h"
#include "HWInterface/D19cFWInterface.h"
#include "HWInterface/D19cOpticalInterface.h"
#include "HWInterface/lpGBTInterface.h"

#include <sstream>

using namespace Ph2_HwDescription;

namespace Ph2_HwInterface
{
VTRxInterface::VTRxInterface(const BeBoardFWMap& pBoardMap, lpGBTInterface* theLpGBTInterface) : ChipInterface(pBoardMap), fTheLpGBTinterface(theLpGBTInterface) {}

VTRxInterface::~VTRxInterface() {}

bool VTRxInterface::ConfigureChip(Chip* theVTRx, bool pVerify, uint32_t pBlockSize)
{
    std::stringstream cOutput;
    setBoard(theVTRx->getBeBoardId());
    theVTRx->printChipType(cOutput);
    LOG(INFO) << BOLDBLUE << cOutput.str() << "...Configuring chip with Id[" << +theVTRx->getId() << "]" << RESET;
    ChipRegMap cCicRegMap = theVTRx->getRegMap();
    // get register map
    std::vector<std::pair<std::string, uint16_t>> cRegItems;
    auto                                          theListOfFreeRegisters = theVTRx->getFreeRegisters();
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
    bool cSuccess = WriteChipMultReg(theVTRx, cRegItems, pVerify);
    if(cSuccess) LOG(INFO) << BOLDGREEN << "Succesful write to " << cRegItems.size() << " registers on CIC" << RESET;
    return cSuccess;
}

bool VTRxInterface::WriteChipReg(Chip* pChip, const std::string& pRegNode, uint16_t pValue, bool pVerify)
{
    ChipRegMap  cRegMap = pChip->getRegMap();
    ChipRegItem theRegister;
    try
    {
        theRegister = cRegMap.at(pRegNode);
    }
    catch(const std::exception& e)
    {
        LOG(ERROR) << BOLDRED << "VTRxInterface::WriteChipReg trying to write to a register that doesn't exist in the map : " << pRegNode << RESET;
        return false;
    }

    auto     theLpGBT      = static_cast<VTRx*>(pChip)->fTheLpGBT;
    uint8_t  masterId      = pChip->getMasterId();
    uint8_t  slaveAddress  = pChip->getChipAddress();
    uint8_t  numberOfBytes = 2, frequency = 2;
    uint32_t slaveData = pValue << 8 | theRegister.fAddress;

    bool success = fTheLpGBTinterface->WriteI2C(theLpGBT, masterId, slaveAddress, slaveData, numberOfBytes, frequency, pVerify);

    if(pVerify)
    {
        auto theReadValue = ReadChipReg(pChip, pRegNode);
        if(theReadValue != pValue)
        {
            LOG(ERROR) << BOLDRED << "VTRxInterface::WriteChipReg : Wrong value read back for " << pRegNode << ": written 0x" << std::hex << +pValue << " but read back 0x" << +theReadValue << std::dec
                       << RESET;
            return false;
        }
    }
    else if(success)
        theRegister.fValue = pValue;

    return success;
}

bool VTRxInterface::WriteChipMultReg(Chip* pChip, const std::vector<std::pair<std::string, uint16_t>>& pVecReq, bool pVerify)
{
    bool success = true;
    for(const auto& theRegister: pVecReq) success &= WriteChipReg(pChip, theRegister.first, theRegister.second, pVerify);
    return success;
}

int32_t VTRxInterface::ReadChipReg(Chip* pChip, const std::string& pRegNode)
{
    auto    theLpGBT      = static_cast<VTRx*>(pChip)->fTheLpGBT;
    uint8_t masterId      = pChip->getMasterId();
    uint8_t slaveAddress  = pChip->getChipAddress();
    uint8_t numberOfBytes = 1, frequency = 2;

    ChipRegItem theRegister;
    try
    {
        theRegister = pChip->getRegMap().at(pRegNode);
    }
    catch(const std::exception& e)
    {
        LOG(ERROR) << BOLDRED << "VTRxInterface::ReadChipReg trying to write to a register that doesn't exist in the map : " << pRegNode << RESET;
        return 0;
    }

    fTheLpGBTinterface->WriteI2C(theLpGBT, masterId, slaveAddress, theRegister.fAddress, numberOfBytes, frequency);
    auto theReadValue = fTheLpGBTinterface->ReadI2C(theLpGBT, masterId, slaveAddress, numberOfBytes, frequency);

    theRegister.fValue = theReadValue;

    return theReadValue;
}

std::vector<std::pair<std::string, uint16_t>> VTRxInterface::ReadChipMultReg(Ph2_HwDescription::Chip* pChip, const std::vector<std::string>& theRegisterList)
{
    std::vector<std::pair<std::string, uint16_t>> theRegisterValues;
    for(auto theRegister: theRegisterList) { theRegisterValues.push_back({theRegister, ReadChipReg(pChip, theRegister)}); }

    return theRegisterValues;
}

uint32_t VTRxInterface::ReadChipFuseID(Ph2_HwDescription::Chip* pChip, uint8_t version)
{
    std::vector<std::string> theIdRegisterList;
    uint8_t                  numberOfRegisters = 4;
    for(uint8_t registerNumber = 0; registerNumber < numberOfRegisters; ++registerNumber) { theIdRegisterList.push_back("UID" + std::to_string(+registerNumber)); }

    auto theReadBackIdRegisters = ReadChipMultReg(pChip, theIdRegisterList);

    uint32_t uniqueId = 0;
    for(uint8_t registerNumber = 0; registerNumber < numberOfRegisters; ++registerNumber) { uniqueId |= (theReadBackIdRegisters.at(registerNumber).second << (registerNumber * 8)); }

    return uniqueId;
}

} // namespace Ph2_HwInterface
