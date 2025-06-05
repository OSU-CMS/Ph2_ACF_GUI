/*!

        Filename :                      Cbc.cc
        Content :                       Cbc Description class, config of the Cbcs
        Programmer :                    Lorenzo BIDEGAIN
        Version :                       1.0
        Date of Creation :              25/06/14
        Support :                       mail to : lorenzo.bidegain@gmail.com

 */

#include "Cbc.h"
#include "Definition.h"
#include "Utils/ChannelGroupHandler.h"
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <math.h>
#include <sstream>
#include <string.h>

namespace Ph2_HwDescription
{
// C'tors with object FE Description

Cbc::Cbc(const FrontEndDescription& pFeDesc, uint8_t pChipId, const std::string& filename) : ReadoutChip(pFeDesc, pChipId)
{
    fChipCode         = 1;
    fChipAddress      = 0x41 + pChipId % 8;
    fMaxRegValue      = 255; // 8 bit registers in CBC
    fChipOriginalMask = std::make_shared<ChannelGroup<1, NCHANNELS>>();
    fChipOriginalMask->enableAllChannels();
    fConfigFileName = filename;
    loadfRegMap(filename);
    setFrontEndType(FrontEndType::CBC3);
    fAverageNoise    = 6.7;
    fAveragePedestal = 590.0;
}

// C'tors which take BeBoardId, FMCId, HybridId, CbcId
Cbc::Cbc(uint8_t pBeBoardId, uint8_t pFMCId, uint8_t pOpticalGroupId, uint8_t pHybridId, uint8_t pChipId, const std::string& filename)
    : ReadoutChip(pBeBoardId, pFMCId, pOpticalGroupId, pHybridId, pChipId)
{
    fChipCode         = 1;
    fChipAddress      = 0x41 + pChipId % 8;
    fMaxRegValue      = 255; // 8 bit registers in CBC
    fChipOriginalMask = std::make_shared<ChannelGroup<1, NCHANNELS>>();
    fChipOriginalMask->enableAllChannels();
    loadfRegMap(filename);
    setFrontEndType(FrontEndType::CBC3);
    fAverageNoise    = 6.7;
    fAveragePedestal = 590.0;
}

void Cbc::initializeFreeRegisters()
{
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^ChipIDFuse[1-3]$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^BandgapFuse$"), RegisterType::ReadOnly));
}

// load fRegMap from file
void Cbc::loadfRegMap(const std::string& filename)
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
            // std::cout<< __PRETTY_FUNCTION__ << " " << line << std::endl;
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

                if(fRegItem.fPage == 0x00 && fRegItem.fAddress >= 0x20 && fRegItem.fAddress <= 0x3F)
                { // Register is a Mask
                    // if(!fhasMaskedChannels && fRegItem.fValue!=0xFF) fhasMaskedChannels=true;
                    // disable masked channels only
                    if(fRegItem.fValue != 0xFF)
                    {
                        for(uint8_t channel = 0; channel < 8; ++channel)
                        {
                            uint8_t chn = 1 << channel;
                            if((fRegItem.fValue && chn) == 0) { fChipOriginalMask->disableChannel(0, (fRegItem.fAddress - 0x20) * 8 + channel); }
                        }
                    }
                }

                fRegMap[fName] = fRegItem;
                // std::cout << __PRETTY_FUNCTION__ << +fRegItem.fValue << std::endl;
                cLineCounter++;
            }
        }

        file.close();
    }
    else
    {
        LOG(ERROR) << "The CBC Settings File " << filename << " does not exist!";
        exit(1);
    }
}

std::stringstream Cbc::getRegMapStream()
{
    std::stringstream                     theStream;
    std::set<CbcRegPair, RegItemComparer> fSetRegItem;

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

bool                          Cbc::isTopSensor(ReadoutChip* pChip, uint16_t pLocalColumn) { return (pLocalColumn % 2 != 0) ? true : false; }
std::pair<uint16_t, uint16_t> Cbc::getGlobalCoordinates(ReadoutChip* pChip, uint16_t pLocalColumn, uint16_t pLocalRow)
{
    if(pLocalColumn > pChip->getNumberOfCols())
    {
        throw std::runtime_error("The given column " + std::to_string(pLocalColumn) + " does not exist in an " + pChip->getFrontEndName(pChip->getFrontEndType()) +
                                 ". Acceptable values are between 0 and " + std::to_string(pChip->getNumberOfCols()));
    }
    if(pLocalRow > pChip->getNumberOfRows())
    {
        throw std::runtime_error("The given row " + std::to_string(pLocalRow) + " does not exist in an " + pChip->getFrontEndName(pChip->getFrontEndType()) + ". Acceptable values are between 0 and " +
                                 std::to_string(pChip->getNumberOfRows()));
    }

    uint16_t cGlobalY = 0;
    uint16_t cGlobalX = 0;
    uint16_t cCol     = pLocalColumn / 2;
    if(pChip->getHybridId() % 2 == 0) { cGlobalX = (pChip->getNumberOfCols() / 2 - cCol) + (NCHIPS_OT - pChip->getId() - 1) * pChip->getNumberOfCols() / 2; }
    else { cGlobalX = cCol + pChip->getId() * pChip->getNumberOfCols() / 2; }
    return std::make_pair(cGlobalX, cGlobalY);
}

uint8_t Cbc::convertMIPtoInjectedCharge(float numberOfMIPs)
{
    if(numberOfMIPs == 0) return 255;
    float chargeInElectron  = numberOfMIPs * TWOS_SENSOR_THICHNESS * NUMBER_OF_ELECTRON_PER_UM;
    float rawInjectionValue = CBC_ELECTRON_TO_CALDAC_INTERCEPT + CBC_ELECTRON_TO_CALDAC_SLOPE * chargeInElectron;
    if(rawInjectionValue < 0)
    {
        LOG(WARNING) << "Impossible to inject " << numberOfMIPs << " MIP in the CBC, Injecting maximum pulse";
        return 0;
    }
    return round(rawInjectionValue);
}

} // namespace Ph2_HwDescription
