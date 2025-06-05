/*!

        \file                   SSA2.cc
        \brief                  SSA2 Description class, config of the SSAs
        \author                 Marc Osherson (copying from SSA.cc, SSA2 main difference is the implementation of the registers)
        \version                1.0
        \date                   28/12/2020
        Support :               mail to : oshersonmarc@gmail.com

 */

#include "SSA2.h"
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
{ // open namespace

std::vector<std::string> SSA2::fListOfGlobalRegisters{"ENFLAGS", "StripControl2", "THTRIMMING", "DigCalibPattern_L", "DigCalibPattern_H"};

SSA2::SSA2(const FrontEndDescription& pFeDesc, uint8_t pChipId, uint8_t pPartnerId, uint8_t pSSASide, const std::string& filename) : ReadoutChip(pFeDesc, pChipId)
{
    fChipCode         = 3;
    fChipAddress      = 0x20 + pChipId % 8;
    fMaxRegValue      = 255; // 8 bit registers in SSA2
    fChipOriginalMask = std::make_shared<ChannelGroup<1, NSSACHANNELS>>();
    fPartnerId        = pPartnerId;
    fConfigFileName   = filename;
    loadfRegMap(filename);
    // select control regs
    for(auto& cMapItem: fRegMap)
    {
        if(std::find(fListOfGlobalRegisters.begin(), fListOfGlobalRegisters.end(), cMapItem.first) == fListOfGlobalRegisters.end()) continue;
        LOG(INFO) << BOLDYELLOW << cMapItem.first << " is a CtrlReg" << RESET;
        cMapItem.second.fControlReg = 1;
    }
    setFrontEndType(FrontEndType::SSA2);
    fAverageNoise    = 4.0;
    fAveragePedestal = 8.0;
}

SSA2::SSA2(uint8_t pBeBoardId, uint8_t pFMCId, uint8_t pOpticalGroupId, uint8_t pHybridId, uint8_t pChipId, uint8_t pPartnerId, uint8_t pSSASide, const std::string& filename)
    : ReadoutChip(pBeBoardId, pFMCId, pOpticalGroupId, pHybridId, pChipId)
{
    fChipCode         = 3;
    fChipAddress      = 0x20 + pChipId % 8;
    fMaxRegValue      = 255; // 8 bit registers in CBC
    fChipOriginalMask = std::make_shared<ChannelGroup<1, NSSACHANNELS>>();
    fPartnerId        = pPartnerId;
    loadfRegMap(filename);
    std::vector<std::string> cCntrlRegs{"THTRIMMING", "StripControl2", "ENFLAGS", "DigCalibPattern_H", "DigCalibPattern_L"};
    for(auto& cMapItem: fRegMap)
    {
        if(std::find(cCntrlRegs.begin(), cCntrlRegs.end(), cMapItem.first) == cCntrlRegs.end()) continue;
        LOG(INFO) << BOLDYELLOW << cMapItem.first << " is a CtrlReg" << RESET;

        cMapItem.second.fControlReg = 1;
    }
    setFrontEndType(FrontEndType::SSA2);
    fAverageNoise    = 4.0;
    fAveragePedestal = 8.0;
}

void SSA2::initializeFreeRegisters()
{
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^SEUcnt$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^Ring_oscillator$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^ADC_out$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^bist_output$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^AC_ReadCounter.*"), RegisterType::Utility));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^status_reg$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^Fuse_Value_b[0-3]$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^status_reg$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex(".*_Cnt_[LH]$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^bist_output$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^ADC_out_[LH]$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^Ring_oscillator_out_loc[TB][LCR]_T[12]_[HL]$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^bist_memory_sram_output_[HL]_[0-9A-F]$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex(".*ync_SEUcnt_.*"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^ADC_control$"), RegisterType::Utility));

    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^mask_strip$"), RegisterType::Utility));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^mask_peri_[AD]$"), RegisterType::Utility));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^Fuse_Prog_b[0-3]$"), RegisterType::Utility));
    // Brodcast registers cannot be reset to avoid overriding local changes
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^ENFLAGS$"), RegisterType::Utility));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^StripControl2$"), RegisterType::Utility));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^THTRIMMING$"), RegisterType::Utility));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^DigCalibPattern_[LH]$"), RegisterType::Utility));
}

void SSA2::setReg(const std::string& pReg, uint16_t psetValue, uint8_t pStatusReg)
{
    if(std::find(fListOfGlobalRegisters.begin(), fListOfGlobalRegisters.end(), pReg) != fListOfGlobalRegisters.end())
    {
        for(uint8_t strip = 0; strip < getNumberOfCols(); ++strip) Chip::setReg(getStripRegisterName(pReg, strip), psetValue, pStatusReg);
    }

    Chip::setReg(pReg, psetValue, pStatusReg);
    return;
}

void SSA2::loadfRegMap(const std::string& filename)
{ // start loadfRegMap
    std::ifstream file(filename.c_str(), std::ios::in);

    if(file)
    {
        initializeFreeRegisters();
        std::string line, fName, fPage_str, fAddress_str, fDefValue_str, fValue_str;
        int         cLineCounter = 0;
        ChipRegItem fRegItem;

        while(getline(file, line))
        {
            if(line.find_first_not_of(" \t") == std::string::npos)
            {
                fCommentMap[cLineCounter] = line;
                cLineCounter++;
            }

            else if(line.at(0) == '#' || line.at(0) == '*' || line.empty())
            {
                // if it is a comment, save the line mapped to the line number so I can later insert it in the same place
                fCommentMap[cLineCounter] = line;
                cLineCounter++;
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
                cLineCounter++;
            }
        }

        file.close();
    }
    else
    {
        LOG(ERROR) << "The SSA2 Settings File " << filename << " does not exist!";
        exit(1);
    }
} // end loadfRegMap

std::stringstream SSA2::getRegMapStream()
{
    std::stringstream                     theStream;
    std::set<SSARegPair, RegItemComparer> fSetRegItem;

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

bool                          SSA2::isTopSensor(ReadoutChip* pChip, uint16_t pLocalColumn) { return true; }
std::pair<uint16_t, uint16_t> SSA2::getGlobalCoordinates(ReadoutChip* pChip, uint16_t pLocalColumn, uint16_t pLocalRow)
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
    cGlobalY          = (pChip->getHybridId() % 2 == 0) ? 0 : 1;

    if(pChip->getHybridId() % 2 == 0) { cGlobalX = (pChip->getNumberOfCols() - pLocalColumn) + (NCHIPS_OT - pChip->getId() - 1) * pChip->getNumberOfCols(); }
    else { cGlobalX = pLocalColumn + pChip->getId() * pChip->getNumberOfCols(); }

    return std::make_pair(cGlobalX, cGlobalY);
}

std::string SSA2::getStripRegisterName(const std::string& theRegisterName, uint16_t strip)
{
    std::string stripRegisterName = theRegisterName + "_S" + std::to_string(strip + 1);
    return stripRegisterName;
}

void SSA2::setADCCalibrationValue(const std::string& theCalibrationName, float theCalibrationValue) { fADCcalibrationMap.at(theCalibrationName) = theCalibrationValue; }

float SSA2::getADCCalibrationValue(const std::string& theCalibrationName) const { return fADCcalibrationMap.at(theCalibrationName); }

uint8_t SSA2::convertMIPtoInjectedCharge(float numberOfMIPs)
{
    float chargeInElectron  = numberOfMIPs * PSS_SENSOR_THICHNESS * NUMBER_OF_ELECTRON_PER_UM;
    float rawInjectionValue = chargeInElectron / SSA_CALDAC_ELECTRON_UNIT;
    if(rawInjectionValue > 255)
    {
        LOG(WARNING) << "Impossible to inject " << numberOfMIPs << " MIP in the MPA, Injecting maximum pulse";
        return 255;
    }
    return round(rawInjectionValue);
}

} // namespace Ph2_HwDescription
