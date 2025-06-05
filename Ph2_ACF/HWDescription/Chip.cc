/*!
  Filename :                      Chip.cc
  Content :                       Chip Description class, config of the Chips
  Programmer :                    Lorenzo BIDEGAIN
  Version :                       1.0
  Date of Creation :              25/06/14
  Support :                       mail to : lorenzo.bidegain@gmail.com
*/

#include "Chip.h"
#include "Definition.h"
#include "Utils/ChannelGroupHandler.h"
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string.h>

namespace Ph2_HwDescription
{
// C'tors with object FE Description
Chip::Chip(const FrontEndDescription& pFeDesc, uint8_t pChipId, uint16_t pMaxRegValue) : FrontEndDescription(pFeDesc), fChipId(pChipId), fMaxRegValue(pMaxRegValue) {}

// C'tors which take Board ID, Frontend ID/Hybrid ID, FMC ID, Chip ID
Chip::Chip(uint8_t pBeBoardId, uint8_t pFMCId, uint8_t pOpticalGroupId, uint8_t pHybridId, uint8_t pChipId, uint16_t pMaxRegValue)
    : FrontEndDescription(pBeBoardId, pFMCId, pOpticalGroupId, pHybridId), fChipId(pChipId), fMaxRegValue(pMaxRegValue)
{
}

// D'Tor
Chip::~Chip()
{
    fRegMap.clear();
    fCommentMap.clear();
    fListOfFreeRegisters.clear();
}

const ChipRegItem& Chip::getRegItem(const std::string& pReg) const
{
    auto i = fRegMap.find(pReg);

    if(i != std::end(fRegMap)) return (i->second);

    LOG(ERROR) << BOLDRED << "Error, no register " << BOLDYELLOW << pReg << BOLDRED << " found in the RegisterMap of ChipID: " << BOLDYELLOW << +fChipId << RESET;
    throw Exception("Chip: no matching register found");
}

ChipRegItem& Chip::getRegItem(const std::string& pReg)
{
    auto i = fRegMap.find(pReg);

    if(i != std::end(fRegMap)) return (i->second);

    LOG(ERROR) << BOLDRED << "Error, no register " << BOLDYELLOW << pReg << BOLDRED << " found in the RegisterMap of ChipID: " << BOLDYELLOW << +fChipId << RESET;
    throw Exception("Chip: no matching register found");
}

uint16_t Chip::getReg(const std::string& pReg) const
{
    ChipRegMap::const_iterator i = fRegMap.find(pReg);

    if(i == fRegMap.end())
    {
        LOG(INFO) << "The Chip object: " << +fChipId << " doesn't have " << pReg;
        return 0;
    }
    else
        return i->second.fValue & fMaxRegValue;
}

void Chip::setReg(const std::string& pReg, uint16_t psetValue, uint8_t pStatusReg)
{
    ChipRegMap::iterator i = fRegMap.find(pReg);

    if(i == fRegMap.end()) LOG(ERROR) << BOLDRED << "The Chip object: " << BOLDYELLOW << +fChipId << BOLDRED << " doesn't have the register " << BOLDYELLOW << pReg << RESET;
    if(psetValue > fMaxRegValue)
        LOG(ERROR) << BOLDRED << "Chip register are at most " << BOLDYELLOW << fMaxRegValue << BOLDRED << " bits, impossible to write " << BOLDYELLOW << psetValue << BOLDRED << " on registed "
                   << BOLDYELLOW << pReg << RESET;
    else
    {
        auto oldRegister     = i->second;
        i->second.fValue     = psetValue & fMaxRegValue;
        i->second.fStatusReg = pStatusReg;

        if(fTrackModifiedRegistersEnabled)
        {
            if(fModifiedRegisters.find(i->first) == fModifiedRegisters.end()) // Check if it is already tracked
            {
                bool isFreeRegister = false;
                for(const auto& freeRegister: fListOfFreeRegisters)
                {
                    isFreeRegister = std::regex_match(pReg, freeRegister.first);
                    if(isFreeRegister) break;
                }
                if(!isFreeRegister && oldRegister != i->second) fModifiedRegisters[i->first] = oldRegister.fValue;
            }
        }
    }
}

bool ChipComparer::operator()(const Chip& chip1, const Chip& chip2) const
{
    if(chip1.getBeBoardId() != chip2.getBeBoardId())
        return chip1.getBeBoardId() < chip2.getBeBoardId();
    else if(chip1.getFMCId() != chip2.getFMCId())
        return chip1.getFMCId() < chip2.getFMCId();
    else if(chip1.getHybridId() != chip2.getHybridId())
        return chip1.getHybridId() < chip2.getHybridId();
    else
        return chip1.getId() < chip2.getId();
}

bool RegItemComparer::operator()(const ChipRegPair& pRegItem1, const ChipRegPair& pRegItem2) const
{
    if(pRegItem1.second.fPage != pRegItem2.second.fPage)
        return pRegItem1.second.fPage < pRegItem2.second.fPage;
    else
        return pRegItem1.second.fAddress < pRegItem2.second.fAddress;
}

// Write RegValues in a file
void Chip::saveRegMap(const std::string& fileName)
{
    std::ofstream file(fileName, std::ios::out | std::ios::trunc);

    if(file)
    {
        auto theStream = getRegMapStream();
        file << theStream.str();

        file.close();
    }
    else
        LOG(ERROR) << BOLDRED << "Error opening file " << BOLDYELLOW << fileName << RESET;
}

void Chip::takeSnapshot()
{
    clearSnapshot();
    reinitializeFreeRegisters();
    fTrackModifiedRegistersEnabled = true;
}

void Chip::clearSnapshot()
{
    fTrackModifiedRegistersEnabled = false;
    fModifiedRegisters.clear();
}

void Chip::reinitializeFreeRegisters()
{
    auto pointerToLastValidValue =
        std::remove_if(fListOfFreeRegisters.begin(), fListOfFreeRegisters.end(), [](std::pair<std::regex, RegisterType> theRegister) { return (theRegister.second == RegisterType::User); });
    fListOfFreeRegisters.erase(pointerToLastValidValue, fListOfFreeRegisters.end());
}

void Chip::addFreeRegister(const std::regex& theRegisterName) { fListOfFreeRegisters.push_back(std::make_pair(theRegisterName, RegisterType::User)); }

std::vector<std::pair<std::string, uint16_t>> Chip::getSnapshot() const
{
    std::vector<std::pair<std::string, uint16_t>> theModifiedRegisterVector(fModifiedRegisters.begin(), fModifiedRegisters.end());
    return theModifiedRegisterVector;
}

std::vector<std::string> Chip::getReadOnlyRegisterList() const
{
    std::vector<std::string> readOnlyRegisterList;
    for(const auto& registerItem: fRegMap)
    {
        for(const auto& freeRegister: fListOfFreeRegisters)
        {
            if(freeRegister.second != RegisterType::ReadOnly) continue;
            if(std::regex_match(registerItem.first, freeRegister.first))
            {
                readOnlyRegisterList.push_back(registerItem.first);
                break;
            }
        }
    }
    return readOnlyRegisterList;
}

} // namespace Ph2_HwDescription
