/*!

        \file                   MPA2.h
        \brief                  MPA2 Description class, config of the MPA2s
        \author                 Kevin Nash
        \version                1.0
        \date                   12/06/21
        Support :               mail to : knash201@gmail.com

 */
// pretty much a copy of MPA.cc, does not seem to be any relevant changes...

#include "MPA2.h"
#include "Definition.h"
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <math.h>
#include <sstream>
#include <string.h>

namespace Ph2_HwDescription
{
std::vector<std::string> MPA2::fListOfGlobalPixelRegisters{"ENFLAGS_ALL", "TrimDAC_ALL", "DigPattern_ALL"};
std::vector<std::string> MPA2::fListOfGlobalRowRegisters{"PixelControl_ALL", "MemoryControl_1_ALL", "MemoryControl_2_ALL"};

// C'tors which take BeBoardId, FMCId, HybridId, ChipId
MPA2::MPA2(uint8_t pBeBoardId, uint8_t pFMCId, uint8_t pOpticalGroupId, uint8_t pHybridId, uint8_t pChipId, uint8_t pPartnerId, const std::string& filename)
    : ReadoutChip(pBeBoardId, pFMCId, pOpticalGroupId, pHybridId, pChipId)
{
    fChipCode         = 2;
    fChipAddress      = 0x40 + pChipId % 8;
    fMaxRegValue      = 255;
    fChipOriginalMask = std::make_shared<ChannelGroup<NMPAROWS, NSSACHANNELS>>();
    fChipOriginalMask->enableAllChannels();
    fPartnerId = pPartnerId;
    loadfRegMap(filename);
    setFrontEndType(FrontEndType::MPA2);
    for(auto& cMapItem: fRegMap)
    {
        if(cMapItem.first.find("_ALL") == std::string::npos) continue;
        cMapItem.second.fControlReg = 1;
    }
    fAverageNoise    = 2.5;
    fAveragePedestal = 75.0;
}

MPA2::MPA2(const FrontEndDescription& pFeDesc, uint8_t pChipId, uint8_t pPartnerId, const std::string& filename) : ReadoutChip(pFeDesc, pChipId)
{
    fChipCode         = 2;
    fChipAddress      = 0x40 + pChipId % 8;
    fMaxRegValue      = 255; // 8 bit registers in MPA
    fChipOriginalMask = std::make_shared<ChannelGroup<NMPAROWS, NSSACHANNELS>>();
    fChipOriginalMask->enableAllChannels();
    fPartnerId      = pPartnerId;
    fConfigFileName = filename;
    loadfRegMap(filename);
    setFrontEndType(FrontEndType::MPA2);
    for(auto& cMapItem: fRegMap)
    {
        if(cMapItem.first.find("_ALL") == std::string::npos) continue;
        cMapItem.second.fControlReg = 1;
    }
    fAverageNoise    = 2.5;
    fAveragePedestal = 75.0;
}

void MPA2::initializeFreeRegisters()
{
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^EfuseValue[0-3]$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex(".*ync_SEUcnt.*"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^ErrorL1$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^Ofcnt$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^DLLlocked$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^ReadCounter.*"), RegisterType::Utility));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^ADC_output_[ML]SB$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^L1_.*_.*"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^OF_.*_count$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex(".*BIST_.*"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^RO_.*"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^EfuseProg[0-3]$"), RegisterType::Utility));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^Mask$"), RegisterType::Utility));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^ADC.*"), RegisterType::Utility));
    // Brodcast registers cannot be reset to avoid overriding local changes
    fListOfFreeRegisters.push_back(std::make_pair(std::regex(".*_ALL"), RegisterType::Utility));
}

void MPA2::setReg(const std::string& pReg, uint16_t psetValue, uint8_t pStatusReg)
{
    if(std::find(fListOfGlobalPixelRegisters.begin(), fListOfGlobalPixelRegisters.end(), pReg) != fListOfGlobalPixelRegisters.end())
    {
        std::string registerName = pReg.substr(0, pReg.length() - 4);
        for(uint8_t col = 0; col < getNumberOfCols(); ++col)
        {
            for(uint8_t row = 0; row < getNumberOfRows(); ++row) { Chip::setReg(getPixelRegisterName(registerName, row, col), psetValue, pStatusReg); }
        }
    }

    if(std::find(fListOfGlobalRowRegisters.begin(), fListOfGlobalRowRegisters.end(), pReg) != fListOfGlobalRowRegisters.end())
    {
        std::string registerName = pReg.substr(0, pReg.length() - 4);
        for(uint8_t row = 0; row < getNumberOfRows(); ++row) { Chip::setReg(getRowRegisterName(registerName, row), psetValue, pStatusReg); }
    }

    Chip::setReg(pReg, psetValue, pStatusReg);
    return;
}

void MPA2::loadfRegMap(const std::string& filename)
{ // start loadfRegMap
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

                fRegMap[fName] = fRegItem;
                // std::cout << __PRETTY_FUNCTION__ <<fName<<"," <<fRegItem.fValue << std::endl;
                cLineCounter++;
            }
        }

        file.close();
    }
    else
    {
        LOG(ERROR) << "The MPA2 Settings File " << filename << " does not exist!";
        exit(1);
    }

} // end loadfRegMap

std::stringstream MPA2::getRegMapStream()
{
    std::stringstream                     theStream;
    std::set<MPARegPair, RegItemComparer> fSetRegItem;

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

// Irene
bool MPA2RegItemComparer::operator()(const MPARegPair& pRegItem1, const MPARegPair& pRegItem2) const
{
    if(pRegItem1.second.fPage != pRegItem2.second.fPage)
        return pRegItem1.second.fPage < pRegItem2.second.fPage;
    else
        return pRegItem1.second.fAddress < pRegItem2.second.fAddress;
}

bool                          MPA2::isTopSensor(ReadoutChip* pChip, uint16_t pLocalColumn) { return false; }
std::pair<uint16_t, uint16_t> MPA2::getGlobalCoordinates(ReadoutChip* pChip, uint16_t pLocalColumn, uint16_t pLocalRow)
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
    cGlobalY          = (pChip->getHybridId() % 2 == 0) ? pLocalRow + 1 : 2 * pChip->getNumberOfRows() - (pLocalRow + 1);

    if(pChip->getHybridId() % 2 == 0) { cGlobalX = (pChip->getNumberOfCols() - pLocalColumn) + (NCHIPS_OT * 2 - pChip->getId() - 1) * pChip->getNumberOfCols(); }
    else { cGlobalX = pLocalColumn + (pChip->getId() - NCHIPS_OT) * pChip->getNumberOfCols(); }

    return std::make_pair(cGlobalX, cGlobalY);
}

std::string MPA2::getPixelRegisterName(const std::string& theRegisterName, uint16_t row, uint16_t col)
{
    std::string pixelRegisterName = theRegisterName + "_C" + std::to_string(col) + "_R" + std::to_string(row);
    return pixelRegisterName;
}

std::string MPA2::getRowRegisterName(const std::string& theRegisterName, uint16_t row)
{
    std::string rowRegisterName = theRegisterName + "_R" + std::to_string(row);
    return rowRegisterName;
}

void MPA2::setADCCalibrationValue(const std::string& theCalibrationName, float theCalibrationValue) { fADCcalibrationMap.at(theCalibrationName) = theCalibrationValue; }

float MPA2::getADCCalibrationValue(const std::string& theCalibrationName) const { return fADCcalibrationMap.at(theCalibrationName); }

uint8_t MPA2::convertMIPtoInjectedCharge(float numberOfMIPs)
{
    float chargeInElectron  = numberOfMIPs * PSP_SENSOR_THICHNESS * NUMBER_OF_ELECTRON_PER_UM;
    float rawInjectionValue = chargeInElectron / MPA_CALDAC_ELECTRON_UNIT;
    if(rawInjectionValue > 255)
    {
        LOG(WARNING) << "Impossible to inject " << numberOfMIPs << " MIP in the MPA, Injecting maximum pulse";
        return 255;
    }
    return round(rawInjectionValue);
}

} // namespace Ph2_HwDescription
