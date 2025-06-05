/*!
  \file                  VTRx.h
  \brief                 VTRx description class, config of the VTRx
  \author                Fabio Ravera
  \version               1.0
  \date                  11/07/24
  Support:               email to fabio.ravera@cern.ch
*/

#include "HWDescription/VTRx.h"
#include "Utils/Utilities.h"

namespace Ph2_HwDescription
{

VTRx::VTRx(uint8_t pBeBoardId, uint8_t FMCId, uint8_t pOpticalGroupId, uint8_t pChipId, const std::string& fileName, const std::string& configFilePath, lpGBT* theLpGBT)
    : Chip(pBeBoardId, FMCId, pOpticalGroupId, 0, pChipId), fTheLpGBT(theLpGBT)
{
    fChipCode       = 5;
    fChipAddress    = 0x50;
    fMasterId       = 1;
    fMaxRegValue    = 255; // 8 bit registers in CIC
    fConfigFileName = fileName;
    setFrontEndType(FrontEndType::VTRx);
    VTRx::loadfRegMap(fConfigFileName);
}

void VTRx::initializeFreeRegisters()
{
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^STATUS$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^ID$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^UID[0-3]$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^SEU[0-3]$"), RegisterType::ReadOnly));
}

void VTRx::loadfRegMap(const std::string& fileName)
{
    std::ifstream     file(fileName.c_str(), std::ios::in);
    std::stringstream myString;

    if(file.good() == true)
    {
        std::string line, name, addressString, defValueString, valueString, bitSizeString;
        int         cLineCounter = 0;
        ChipRegItem fRegItem;

        initializeFreeRegisters();
        while(getline(file, line))
        {
            if(line.find_first_not_of(" \t") == std::string::npos || line.at(0) == '#' || line.at(0) == '*' || line.empty())
                fCommentMap[cLineCounter] = line;
            else
            {
                myString.str("");
                myString.clear();
                myString << line;
                myString >> name >> addressString >> defValueString >> valueString >> bitSizeString;

                fRegItem.fAddress  = strtoul(addressString.c_str(), 0, 16);
                fRegItem.fDefValue = convertAnyInt(defValueString);
                fRegItem.fValue    = convertAnyInt(valueString);
                fRegItem.fPage     = 0;
                fRegItem.fBitSize  = strtoul(bitSizeString.c_str(), 0, 10);
                fRegMap[name]      = fRegItem;
            }

            cLineCounter++;
        }

        file.close();
    }
    else
        throw Exception("[lpGBT::loadfRegMapd] The LpGBT file settings does not exist");
}

std::stringstream VTRx::getRegMapStream()
{
    const unsigned int Nspaces = 26; // @CONST@
    std::stringstream  theStream;

    std::set<ChipRegPair, RegItemComparer> fSetRegItem;
    for(const auto& it: fRegMap) fSetRegItem.insert({it.first, it.second});

    int cLineCounter = 0;
    for(const auto& v: fSetRegItem)
    {
        while(fCommentMap.find(cLineCounter) != std::end(fCommentMap))
        {
            auto cComment = fCommentMap.find(cLineCounter);

            theStream << cComment->second << std::endl;
            cLineCounter++;
        }

        theStream << v.first;
        for(auto j = 0u; j < Nspaces; j++) theStream << " ";
        theStream.seekp(-(v.first.size() < Nspaces ? v.first.size() : Nspaces - 2), std::ios_base::cur);
        theStream << "0x" << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << int(v.second.fAddress);
        for(auto j = 0u; j < 9 - (v.first.size() < Nspaces ? 0 : v.first.size() - Nspaces + 2); j++) theStream << " ";
        theStream << "0x" << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << int(v.second.fDefValue);
        for(auto j = 0u; j < 14; j++) theStream << " ";
        theStream << "0x" << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << int(v.second.fValue);
        for(auto j = 0u; j < 30; j++) theStream << " ";
        theStream << std::setfill('0') << std::setw(1) << std::dec << std::uppercase << int(v.second.fBitSize) << std::endl;

        cLineCounter++;
    }

    return theStream;
}

} // namespace Ph2_HwDescription