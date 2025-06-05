/*!

        Filename :                      Cic.cc
        Content :                       Cic Description class, config of the Cbcs
        Programmer :                    Davide Di Croce, Fabio Ravera, Sarah Seif El Nasr-Storey
        Version :                       1.0
        Date of Creation :              25/06/19
        Support :                       mail to : sarah.storey@cern.ch

 */

#include "Cic.h"
#include "Definition.h"
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string.h>

namespace Ph2_HwDescription
{
// C'tors with object FE Description

Cic::Cic(const FrontEndDescription& pFeDesc, uint8_t pChipId, const std::string& filename) : Chip(pFeDesc, pChipId)
{
    fChipCode       = 4;
    fChipAddress    = 0x60;
    fMaxRegValue    = 255; // 8 bit registers in CIC
    fConfigFileName = filename;
    loadfRegMap(filename);
    setFrontEndType(FrontEndType::CIC2);
    initializeLpGBTphasesForCICbypassMap();
}

// C'tors which take BeBoardId, FMCId, HybridId, CbcId
Cic::Cic(uint8_t pBeBoardId, uint8_t pFMCId, uint8_t pOpticalGroupId, uint8_t pHybridId, uint8_t pChipId, const std::string& filename) : Chip(pBeBoardId, pFMCId, pOpticalGroupId, pHybridId, pChipId)
{
    fChipCode       = 4;
    fChipAddress    = 0x60;
    fMaxRegValue    = 255; // 8 bit registers in CIC
    fConfigFileName = filename;
    loadfRegMap(filename);
    setFrontEndType(FrontEndType::CIC2);
    initializeLpGBTphasesForCICbypassMap();
}

void Cic::initializeLpGBTphasesForCICbypassMap()
{
    for(uint8_t phyPort = 0; phyPort < 12; ++phyPort)
    {
        for(uint8_t stubLine = 0; stubLine < 4; ++stubLine) { fLpGBTphasesForCICbypassMap[phyPort][stubLine] = 8; }
    }
}

void Cic::setLpGBTphaseForCICbypass(uint8_t phyPort, uint8_t stubLine, uint8_t lpgbtPhase)
{
    if(phyPort >= 12 || stubLine >= 4)
    {
        std::cerr << __PRETTY_FUNCTION__ << " [" << __LINE__ << "] phyport or stubline outside the limits" << std::endl;
        abort();
    }
    fLpGBTphasesForCICbypassMap[phyPort][stubLine] = lpgbtPhase;
}

uint8_t Cic::getLpGBTphaseForCICbypass(uint8_t phyPort, uint8_t stubLine) const { return fLpGBTphasesForCICbypassMap.at(phyPort).at(stubLine); }

void Cic::initializeFreeRegisters()
{
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^EfuseValue[0-3]$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^ASYNC_CNTL_BLOCK[0-3]$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^SYNC_CNTL_BLOCK[0-3]$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^scPhaseSelectB[0-3]o[0-5]$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^scDllInstantLock[01]$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^scDllLocked[01]$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^scChannelLocked[0-5]$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^timingStatusBits$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^BX0_DELAY$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^WA_DELAY\\d{2}$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^MASK_BLOCK[0-3]$"), RegisterType::Utility));
}

// load fRegMap from file
void Cic::loadfRegMap(const std::string& filename)
{
    std::ifstream file(filename.c_str(), std::ios::in);

    if(file)
    {
        initializeFreeRegisters();
        std::string line, fName, fPage_str, fAddress_str, fDefValue_str, fValue_str;
        int         cLineCounter = 0;
        ChipRegItem fRegItem;

        // fhasMaskedChannels = false;
        while(getline(file, line))
        {
            if(line.find_first_not_of(" \t") == std::string::npos)
            {
                fCommentMap[cLineCounter] = line;
                cLineCounter++;
                // continue;
            }

            else if(line.at(0) == '#' || line.at(0) == '*' || line.empty())
            {
                // if it is a comment, save the line mapped to the line number so I can later insert it in the same
                // place
                fCommentMap[cLineCounter] = line;
                cLineCounter++;
                // continue;
            }
            else
            {
                std::istringstream input(line);
                input >> fName >> fPage_str >> fAddress_str >> fDefValue_str >> fValue_str;

                fRegItem.fPage     = strtoul(fPage_str.c_str(), 0, 16);
                fRegItem.fAddress  = strtoul(fAddress_str.c_str(), 0, 16);
                fRegItem.fDefValue = strtoul(fDefValue_str.c_str(), 0, 16);
                fRegItem.fValue    = strtoul(fValue_str.c_str(), 0, 16);
                fRegMap[fName]     = fRegItem;
                // LOG (INFO) << BOLDYELLOW << "CIC:" << +fRegItem.fPage << "\t" << +fRegItem.fAddress << "\t" << +fRegItem.fValue << RESET;
                cLineCounter++;
            }
        }

        file.close();
    }
    else
    {
        LOG(ERROR) << "The CIC Settings File " << filename << " does not exist!";
        exit(1);
    }

    // for(auto& cRegItem: fRegMap) { LOG(DEBUG) << BOLDBLUE << "CIC register : " << cRegItem.first << " --- " << +cRegItem.second.fValue << RESET; }
}

std::stringstream Cic::getRegMapStream()
{
    std::stringstream                     theStream;
    std::set<CicRegPair, RegItemComparer> fSetRegItem;
    for(auto& it: fRegMap) fSetRegItem.insert({it.first, it.second});

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

        for(int j = 0; j < 48; j++) theStream << " ";

        theStream.seekp(-v.first.size(), std::ios_base::cur);

        theStream << "0x" << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << int(v.second.fPage) << "\t0x" << std::setfill('0') << std::setw(2) << std::hex << std::uppercase
                  << int(v.second.fAddress) << "\t0x" << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << int(v.second.fDefValue) << "\t0x" << std::setfill('0') << std::setw(2)
                  << std::hex << std::uppercase << int(v.second.fValue) << std::endl;

        cLineCounter++;
    }

    return theStream;
}

void Cic::setDriveStrength(uint8_t pDriveStrength)
{
    auto& theRegister            = fRegMap["SLVS_PADS_CONFIG"];
    auto  convertedRegisterValue = 0;
    try
    {
        convertedRegisterValue = fTxDriveStrength.at(pDriveStrength);
    }
    catch(const std::exception& e)
    {
        std::string errorMessage = "Error: impossible to set CIC driver strenght to " + std::to_string(pDriveStrength);
        throw std::runtime_error(errorMessage);
    }

    theRegister.fValue = (theRegister.fValue & 0xF8) | (convertedRegisterValue & 0x7);
}

void Cic::setEdgeSelect(uint8_t pEdgeSel)
{
    bool  cNegEdge     = (pEdgeSel == 1);
    auto& theRegister  = fRegMap["MISC_CTRL"];
    theRegister.fValue = (theRegister.fValue & 0xF7) | ((cNegEdge ? 1 : 0) << 3);
}

std::vector<uint8_t> Cic::getMapping()
{
    bool c2S = ((getReg("FE_CONFIG") & 0x01) == 0);
    if(c2S)
        return fChipToCicMapping2S;
    else // a bit too many else, but easier to read
    {
        if(getHybridId() % 2 == 0)
            return fChipToCicMappingPSR;
        else
            return fChipToCicMappingPSL;
    }
}

std::map<uint8_t, uint8_t> Cic::fTxDriveStrength = {{0, 0}, {1, 2}, {2, 6}, {3, 1}, {4, 3}, {5, 7}};

} // namespace Ph2_HwDescription
