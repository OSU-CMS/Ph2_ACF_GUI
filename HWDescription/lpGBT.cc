/*!
  \file                  lpGBT.h
  \brief                 lpGBT implementation class, config of the lpGBT
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#include "lpGBT.h"

namespace Ph2_HwDescription
{
lpGBT::lpGBT(uint8_t pBeId, uint8_t FMCId, uint8_t pOpticalGroupId, uint8_t pChipId, const std::string& fileName, const std::string& pConfigFilePath) : Chip(pBeId, FMCId, pOpticalGroupId, 0, pChipId)
{
    fConfigFileName = fileName;
    fConfigFilePath = pConfigFilePath;
    setFrontEndType(FrontEndType::LpGBT);
    lpGBT::loadfRegMap(fConfigFileName);
}

void lpGBT::initializeFreeRegisters()
{
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^ConfigPins$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^I2CSlaveAddress$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^EPRX[0-6]Locked$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^EPRX[0-6]CurrentPhase[13][02]$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^EPRXEcCurrentPhase$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^EPRX[0-6]DLLStatus$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^I2CM[0-2]Ctrl$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^I2CM[0-2]Mask$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^I2CM[0-2]Status$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^I2CM[0-2]TranCnt$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^I2CM[0-2]Read.*"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^PSStatus$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^PIOIn[HL]$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^FUSEStatus$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^FUSEValues[A-D]$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^ProcessMonitorStatus$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^PMFreq[A-C]$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^SEUCount[HL]$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^CLKGStatus[0-9]$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^DLDPFecCorrectionCount[0-3]$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^ADCStatus[HL]$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^EOMStatus$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^EOMCounterValue[HL]$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^EOMCounter40M[HL]$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^BERTStatus$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^BERTResult[0-4]$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^ROM$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^PORBOR$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^PUSM.*"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^CRCValue[0-3]$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^FailedCRC$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^TOValue$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^SCStatus$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^FAState$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^FAHeader.*"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^FALossOfLockCount$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^ConfigErrorCounter[HL]$"), RegisterType::ReadOnly));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^FUSEControl$"), RegisterType::Utility));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^FUSEBlowData[A-D]$"), RegisterType::Utility));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^FUSEBlowAdd[HL]$"), RegisterType::Utility));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^FuseMagic$"), RegisterType::Utility));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^I2CM[0-1]Address$"), RegisterType::Utility));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^I2CM[0-2]Cmd$"), RegisterType::Utility));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^I2CM[0-2]Data[0-3]$"), RegisterType::Utility));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^POWERUP2$"), RegisterType::Utility));
    fListOfFreeRegisters.push_back(std::make_pair(std::regex("^EPRX[0-6][0-3]ChnCntr_phase$"), RegisterType::Utility));
}

void lpGBT::loadfRegMap(const std::string& fileName)
{
    std::ifstream     file(fileName.c_str(), std::ios::in);
    std::stringstream myString;

    if(file.good() == true)
    {
        std::string line, fName, fAddress_str, fDefValue_str, fValue_str, fBitSize_str;
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
                myString >> fName >> fAddress_str >> fDefValue_str >> fValue_str >> fBitSize_str;

                fRegItem.fAddress = strtoul(fAddress_str.c_str(), 0, 16);

                int baseType;
                if(fDefValue_str.compare(0, 2, "0x") == 0)
                    baseType = 16;
                else if(fDefValue_str.compare(0, 2, "0d") == 0)
                    baseType = 10;
                else if(fDefValue_str.compare(0, 2, "0b") == 0)
                    baseType = 2;
                else
                {
                    LOG(ERROR) << BOLDRED << "Unknown base " << BOLDYELLOW << fDefValue_str << RESET;
                    throw Exception("[lpGBT::loadfRegMap] Error, unknown base");
                }
                fDefValue_str.erase(0, 2);
                fRegItem.fDefValue = strtoul(fDefValue_str.c_str(), 0, baseType);

                if(fValue_str.compare(0, 2, "0x") == 0)
                    baseType = 16;
                else if(fValue_str.compare(0, 2, "0d") == 0)
                    baseType = 10;
                else if(fValue_str.compare(0, 2, "0b") == 0)
                    baseType = 2;
                else
                {
                    LOG(ERROR) << BOLDRED << "Unknown base " << BOLDYELLOW << fValue_str << RESET;
                    throw Exception("[lpGBT::loadfRegMap] Error, unknown base");
                }

                fValue_str.erase(0, 2);
                fRegItem.fValue = strtoul(fValue_str.c_str(), 0, baseType);

                fRegItem.fPage    = 0;
                fRegItem.fBitSize = strtoul(fBitSize_str.c_str(), 0, 10);
                fRegMap[fName]    = fRegItem;
            }

            cLineCounter++;
        }

        file.close();
    }
    else
        throw Exception("[lpGBT::loadfRegMapd] The LpGBT file settings does not exist");
}

std::stringstream lpGBT::getRegMapStream()
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
        theStream << "0x" << std::setfill('0') << std::setw(3) << std::hex << std::uppercase << int(v.second.fAddress);
        for(auto j = 0u; j < 9 - (v.first.size() < Nspaces ? 0 : v.first.size() - Nspaces + 2); j++) theStream << " ";
        theStream << "0x" << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << int(v.second.fDefValue);
        for(auto j = 0u; j < 14; j++) theStream << " ";
        theStream << "0x" << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << int(v.second.fValue);
        for(auto j = 0u; j < 26; j++) theStream << " ";
        theStream << std::setfill('0') << std::setw(1) << std::dec << std::uppercase << int(v.second.fBitSize) << std::endl;

        cLineCounter++;
    }

    return theStream;
}

void lpGBT::setInvertClock(uint8_t hybridId, bool pInvertClock)
{
    std::string registerName = (hybridId % 2 == 0) ? "EPCLK1ChnCntrH" : "EPCLK11ChnCntrH";
    auto&       theRegister  = fRegMap[registerName];
    theRegister.fValue       = (theRegister.fValue & 0xBF) | ((pInvertClock ? 1 : 0) << 6);
}

void lpGBT::setADCCalibrationData(const std::map<std::string, float>& theInputMap)
{
    for(const auto& theInput: theInputMap) fADCcalibrationData[theInput.first] = theInput.second;
}

void lpGBT::setNTCResistance(float cNTCResistance)
{
    if(cNTCResistance < fMinNTCResistance || cNTCResistance > fMaxNTCResistance)
    {
        LOG(WARNING) << WARNING_FORMAT << " New measurement of Sensor NTC resistance (" << cNTCResistance << " Ohm) is outside acceptable range (" << fMinNTCResistance << " Ohm <-> "
                     << fMaxNTCResistance << " Ohm). Probably a wrong reading of the ADC, not updating current value" << RESET;
        return;
    }
    fNTCResistance = cNTCResistance;
}

void lpGBT::setVtrxNTCResistance(float cVtrxNTCResistance)
{
    if(cVtrxNTCResistance < fMinVtrxNTCResistance || cVtrxNTCResistance > fMaxVtrxNTCResistance)
    {
        LOG(WARNING) << WARNING_FORMAT << " New measurement of VTRx NTC resistance (" << cVtrxNTCResistance << " Ohm) is outside acceptable range (" << fMinVtrxNTCResistance << " Ohm <-> "
                     << fMaxVtrxNTCResistance << " Ohm). Probably a wrong reading of the ADC, not updating current value" << RESET;
        return;
    }
    fVtrxNTCResistance = cVtrxNTCResistance;
}

} // namespace Ph2_HwDescription
