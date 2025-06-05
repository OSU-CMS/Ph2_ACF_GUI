/*!
  \file                  D19clpGBTIn+erface.cc
  \brief                 Interface to access and control the low-power Gigabit Transceiver chip
  \author                Younes Otarid
  \version               1.0
  \date                  03/03/20
  Support:               email to younes.otarid@cern.ch
*/

#include "HWInterface/D19clpGBTInterface.h"
#include "HWDescription/OpticalGroup.h"
#include "HWDescription/lpGBT.h"
#include "Utils/LpGBTalignmentResult.h"
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <regex>
#include <thread>
#include <unordered_map>

using namespace Ph2_HwDescription;

namespace Ph2_HwInterface
{
bool D19clpGBTInterface::ConfigureChip(Ph2_HwDescription::Chip* pChip, bool pVerify, uint32_t pBlockSize)
{
    std::stringstream cOutput;
    setBoard(pChip->getBeBoardId());
    pChip->printChipType(cOutput);
    uint8_t cChipVersion = static_cast<lpGBT*>(pChip)->getVersion();
    LOG(INFO) << BOLDBLUE << cOutput.str() << "...Configuring chip with Id[" << +pChip->getId() << "] , Version[" << +cChipVersion << "]" << RESET;

    size_t numberOtIterations    = 0;
    size_t maxNumberOfIterations = 10;
    while(numberOtIterations < maxNumberOfIterations)
    {
        uint8_t chipMode = PrintChipMode(pChip);
        if(chipMode != 0) break;
        ++numberOtIterations;
        LOG(WARNING) << WARNING_FORMAT << "Error reading LpGBT chip mode on Board " << +pChip->getBeBoardId() << " Optical group " << +pChip->getOpticalGroupId()
                     << ", retry register read one more time" << RESET;
    }
    if(numberOtIterations >= maxNumberOfIterations)
    {
        LOG(ERROR) << ERROR_FORMAT << "Failed to read LpGBT chip mode on Board " << +pChip->getBeBoardId() << " Optical group " << +pChip->getOpticalGroupId() << RESET;
        throw std::runtime_error("Failed to read LpGBT chip mode");
    }

    // Waiting for at least PauseForDllConfig state before configuring chip. If state beyond, then I can still configure
    uint16_t cIter = 0, cMaxIter = 200;
    for(auto& ele: fPUSMStatusMap.at(cChipVersion)) revertedPUSMStatusMap[ele.second] = ele.first;
    uint8_t cPUSMState = 0;
    do {
        cPUSMState = GetPUSMStatus(pChip);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        cIter++;
    } while((cPUSMState < revertedPUSMStatusMap.at("PAUSE_FOR_DLL_CONFIG")) && (cIter < cMaxIter));
    if(cIter == cMaxIter) { throw std::runtime_error(std::string("lpGBT Power-Up State Machine Stuck at state " + fPUSMStatusMap.at(cChipVersion).at(cPUSMState))); }

    // Configuring chip
    ChipRegMap                                    clpGBTRegMap = pChip->getRegMap();
    std::vector<std::pair<std::string, uint16_t>> cRegVec;
    cRegVec.clear();

    for(const auto& cRegItem: clpGBTRegMap)
    {
        bool isFreeRegister = false;
        for(const auto& freeRegister: pChip->getFreeRegisters())
        {
            isFreeRegister = std::regex_match(cRegItem.first, freeRegister.first);
            if(isFreeRegister) break;
        }
        if(!isFreeRegister) { cRegVec.push_back(std::make_pair(cRegItem.first, cRegItem.second.fValue)); }
    } // get read/write registers

    WriteChipMultReg(pChip, cRegVec);

    // Setting PUSM Done bits
    SetPUSMDone(pChip, true, true);
    // Checking if lpGBT reaches Ready state
    bool cReady = false;
    cIter = 0, cMaxIter = 200;
    while(!cReady && cIter < cMaxIter)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        cReady = IsPUSMDone(pChip);
        cIter++;
    }
    if(cReady) { LOG(INFO) << BOLDGREEN << "lpGBT Configured [READY]" << RESET; }
    else { throw std::runtime_error(std::string("lpGBT Power-Up State Machine NOT DONE")); }
    // Reset I2C Masters
    ResetI2C(pChip, {0, 1, 2});
    // LoadCalibrationData(pChip, 0x00244200);
    if(cChipVersion == 1)
    {
        LOG(INFO) << BOLDBLUE << "Load calibration data and automatically tune vref (repeat if temperature changes)!" << RESET;

        LoadCalibrationData(static_cast<lpGBT*>(pChip), ReadChipFuseID(pChip, cChipVersion));

        // Fabio's comment: auto tune should not be done here because if it gets calibrated and you try to reload again the registers
        // it will be overwritten
        // auto theLpGBT = static_cast<lpGBT*>(pChip);
        // AutoTuneVref(theLpGBT);

        // LOG(INFO) << BOLDBLUE << "Reading ADC channels" << RESET;
        // LOG(INFO) << BOLDGREEN << "AdcGetVin(theLpGBT, \"ADC0\", \"VREF/2\", 0) " << RESET;
        // LOG(INFO) << BOLDGREEN << AdcGetVin(theLpGBT, "ADC0", "VREF/2", 0) << " V" << RESET;
        // LOG(INFO) << BOLDGREEN << "AdcGetVin(theLpGBT, \"ADC1\", \"VREF/2\", 0) " << RESET;
        // LOG(INFO) << BOLDGREEN << AdcGetVin(theLpGBT, "ADC1", "VREF/2", 0) << " V" << RESET;
        // LOG(INFO) << BOLDGREEN << "AdcGetVin(theLpGBT, \"ADC2\", \"VREF/2\", 0) " << RESET;
        // LOG(INFO) << BOLDGREEN << AdcGetVin(theLpGBT, "ADC2", "VREF/2", 0) << " V" << RESET;
        // LOG(INFO) << BOLDGREEN << "AdcGetVin(theLpGBT, \"ADC3\", \"VREF/2\", 0) " << RESET;
        // LOG(INFO) << BOLDGREEN << AdcGetVin(theLpGBT, "ADC3", "VREF/2", 0) << " V" << RESET;
        // LOG(INFO) << BOLDGREEN << "AdcGetVin(theLpGBT, \"ADC4\", \"VREF/2\", 0) " << RESET;
        // LOG(INFO) << BOLDGREEN << AdcGetVin(theLpGBT, "ADC4", "VREF/2", 0) << " V" << RESET;
        // LOG(INFO) << BOLDGREEN << "AdcGetVin(theLpGBT, \"ADC5\", \"VREF/2\", 0) " << RESET;
        // LOG(INFO) << BOLDGREEN << AdcGetVin(theLpGBT, "ADC5", "VREF/2", 0) << " V" << RESET;
        // LOG(INFO) << BOLDGREEN << "AdcGetVin(theLpGBT, \"ADC6\", \"VREF/2\", 0) " << RESET;
        // LOG(INFO) << BOLDGREEN << AdcGetVin(theLpGBT, "ADC6", "VREF/2", 0) << " V" << RESET;
        // LOG(INFO) << BOLDGREEN << "AdcGetVin(theLpGBT, \"ADC7\", \"VREF/2\", 0) " << RESET;
        // LOG(INFO) << BOLDGREEN << AdcGetVin(theLpGBT, "ADC7", "VREF/2", 0) << " V" << RESET;
        // Example on how to use the current source to measure resistance
        // Only for OT-2S
        // if(theLpGBT->getFrontEndType() == FrontEndType::OuterTracker2S) {
        // CdacSetCurrent(theLpGBT, "ADC4", _CdacCodeToCurrent(theLpGBT, "ADC4", 0xaa));
        // LOG(INFO) << BOLDGREEN << "MeasureResistance(theLpGBT,\"ADC4\", 1000, false) " << RESET;
        // LOG(INFO) << BOLDGREEN << MeasureResistance(theLpGBT, "ADC4", 1000, false) << " Ohms" << RESET;}
        // LOG(INFO) << BOLDGREEN << "MeasureTemperature(theLpGBT) " << RESET;
        // LOG(INFO) << BOLDGREEN << MeasureTemperature(theLpGBT) << " C" << RESET;

        // LOG(INFO) << BOLDGREEN << "MeasurePowerSupplyVoltage(theLpGBT, \"VDDTX\")" << RESET;
        // LOG(INFO) << BOLDGREEN << MeasurePowerSupplyVoltage(theLpGBT, "VDDTX") << " V" << RESET;
        // LOG(INFO) << BOLDGREEN << "MeasurePowerSupplyVoltage(theLpGBT, \"VDDRX\")" << RESET;
        // LOG(INFO) << BOLDGREEN << MeasurePowerSupplyVoltage(theLpGBT, "VDDRX") << " V" << RESET;
        // LOG(INFO) << BOLDGREEN << "MeasurePowerSupplyVoltage(theLpGBT, \"VDD\")" << RESET;
        // LOG(INFO) << BOLDGREEN << MeasurePowerSupplyVoltage(theLpGBT, "VDD") << " V" << RESET;
        // LOG(INFO) << BOLDGREEN << "MeasurePowerSupplyVoltage(theLpGBT, \"VDDA\")" << RESET;
        // LOG(INFO) << BOLDGREEN << MeasurePowerSupplyVoltage(theLpGBT, "VDDA") << " V" << RESET;
    }
    return cReady;
} //

/*-----------------------*/
/* OT specific functions */
/*-----------------------*/

bool D19clpGBTInterface::enablePRBS(Ph2_HwDescription::OpticalGroup* theOpticalGroup, uint16_t phase)
{
    auto                                          theLpBGT              = theOpticalGroup->flpGBT;
    const std::map<uint8_t, std::vector<uint8_t>> theGroupAndChannelMap = theOpticalGroup->getLpGBTrxGroupsAndChannels();
    bool                                          is10G                 = GetChipRate(theLpBGT) == 10;

    std::vector<std::pair<std::string, uint16_t>> theRegisterVector;

    uint8_t driverStrenght = 3;
    uint8_t PS0delayValue  = phase & 0xff;
    uint8_t PS0configValue = ((phase >> 1) & 0x80) | (is10G ? 5 : 4) | (driverStrenght << 3) | 0x40;
    theRegisterVector.push_back({"PS0Config", PS0configValue});
    theRegisterVector.push_back({"PS0Delay", PS0delayValue});

    WriteChipMultReg(theLpBGT, theRegisterVector);
    theRegisterVector.clear();

    auto getPRBSenableCommand = [&theRegisterVector, &theGroupAndChannelMap](uint16_t registerNumber)
    {
        uint16_t registerValue = 0;
        for(auto channel: theGroupAndChannelMap.at(registerNumber * 2 + 0)) registerValue |= 1 << channel;
        if(registerNumber < 3)
            for(auto channel: theGroupAndChannelMap.at(registerNumber * 2 + 1)) registerValue |= 1 << (channel + 4);
        return registerValue;
    };

    for(uint16_t registerNumber = 0; registerNumber < 4; ++registerNumber)
    {
        std::string registerName = "EPRXPRBS" + std::to_string(registerNumber);
        theRegisterVector.push_back({registerName, getPRBSenableCommand(registerNumber)});
    }

    theRegisterVector.push_back({"EPRXTrain10", getPRBSenableCommand(0)});
    theRegisterVector.push_back({"EPRXTrain32", getPRBSenableCommand(1)});
    theRegisterVector.push_back({"EPRXTrain54", getPRBSenableCommand(2)});
    theRegisterVector.push_back({"EPRXTrainEc6", getPRBSenableCommand(3)});
    WriteChipMultReg(theLpBGT, theRegisterVector);

    theRegisterVector.clear();
    theRegisterVector.push_back({"EPRXTrain10", 0});
    theRegisterVector.push_back({"EPRXTrain32", 0});
    theRegisterVector.push_back({"EPRXTrain54", 0});
    theRegisterVector.push_back({"EPRXTrainEc6", 0});
    WriteChipMultReg(theLpBGT, theRegisterVector);

    bool   allAligned = true;
    size_t attempt    = 0;

    std::vector<std::string> alignmentResultRegister{"EPRX0Locked", "EPRX1Locked", "EPRX2Locked", "EPRX3Locked", "EPRX4Locked", "EPRX5Locked", "EPRX6Locked"};

    while(attempt < 100)
    {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        allAligned                 = true;
        auto alignmentResultValues = ReadChipMultReg(theLpBGT, alignmentResultRegister);

        for(const auto& theGroupAndChannels: theGroupAndChannelMap)
        {
            for(auto theChannel: theGroupAndChannels.second)
            {
                if(((alignmentResultValues.at(theGroupAndChannels.first).second >> (4 + theChannel)) & 0x1) != 1)
                {
                    allAligned = false;
                    break;
                }
            }
            if(!allAligned) break;
        }

        if(allAligned) break;

        ++attempt;
    }

    return allAligned;
}

bool D19clpGBTInterface::disablePRBS(Ph2_HwDescription::OpticalGroup* theOpticalGroup)
{
    auto                                          theLpBGT = theOpticalGroup->flpGBT;
    std::vector<std::pair<std::string, uint16_t>> prbsRegisters{{"EPRXPRBS3", 0x0}, {"EPRXPRBS2", 0x0}, {"EPRXPRBS1", 0x0}, {"EPRXPRBS0", 0x0}};

    return WriteChipMultReg(theLpBGT, prbsRegisters);
}

bool D19clpGBTInterface::WriteChipMultReg(Ph2_HwDescription::Chip* pChip, const std::vector<std::pair<std::string, uint16_t>>& pRegVec, bool pVerify)
{
    // first, identify the correct BeBoardFWInterface
    setBoard(pChip->getBeBoardId());
    auto                     cRegMap = pChip->getRegMap();
    std::vector<ChipRegItem> cRegItems;
    for(auto cReq: pRegVec)
    {
        auto cIterator = cRegMap.find(cReq.first);
        if(cIterator == cRegMap.end())
        {
            LOG(ERROR) << BOLDRED << "D19clpGBTInterface::WriteChipMultReg trying to write to a register that doesn't exist in the map : " << cReq.first << RESET;
            continue;
        }

        ChipRegItem cItem = cIterator->second;
        cItem.fValue      = cReq.second;
        cRegItems.push_back(cItem);
    }
    return fBoardFW->MultiRegisterWrite(pChip, cRegItems, pVerify);
}

std::vector<std::pair<std::string, uint16_t>> D19clpGBTInterface::ReadChipMultReg(Ph2_HwDescription::Chip* pChip, const std::vector<std::string>& theRegisterList)
{
    setBoard(pChip->getBeBoardId());
    auto                     cRegMap = pChip->getRegMap();
    std::vector<ChipRegItem> cRegItems;
    for(auto cReq: theRegisterList)
    {
        auto cIterator = cRegMap.find(cReq);
        if(cIterator == cRegMap.end())
        {
            LOG(ERROR) << BOLDRED << "D19clpGBTInterface::WriteChipMultReg trying to write to a register that doesn't exist in the map : " << cReq << RESET;
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

void D19clpGBTInterface::SetConfigMode(bool pOptical, bool pToggleTC)
{
    if(pOptical)
    {
        LOG(INFO) << BOLDGREEN << "Using Serial Interface configuration mode" << RESET;
#if defined(__TC_USB__)
        LOG(INFO) << BOLDBLUE << "Toggling Test Card" << RESET;
        if(pToggleTC && fTC_PSROH != nullptr) fTC_PSROH->toggle_SCI2C();
#endif
        fOptical = true;
    }
    else
    {
        LOG(INFO) << BOLDGREEN << "Using I2C Slave Interface configuration mode" << RESET;
        fOptical = false;
    }
}

void D19clpGBTInterface::hold2SModuleResets(Ph2_HwDescription::Chip* pChip)
{
    // Reset I2C Masters
    ResetI2C(pChip, {0, 1, 2});
    // hold resets
    for(uint8_t cSide = 0; cSide < 2; cSide++)
    {
        this->cbcReset(pChip, true, cSide);
        this->cicReset(pChip, true, cSide);
    }

    // Fabio: I do not think this part should be here, but I keep it for consistency with the previous code
#if defined(__TCUSB__)
    std::vector<uint8_t> cEportGroups = {4, 4, 5, 5, 6, 0};
    std::vector<uint8_t> cEportChnls  = {0, 2, 0, 2, 0, 0};
    InitialPhaseAlignRx(pChip, cEportGroups, cEportChnls);
    cEportGroups = {0, 1, 1, 2, 2, 3};
    cEportChnls  = {2, 0, 2, 0, 2, 2};
    InitialPhaseAlignRx(pChip, cEportGroups, cEportChnls);
    ConfigureCurrentDAC(pChip, std::vector<std::string>{"ADC4"}, 0x1c); // current chosen according to measurement range
#endif
}

void D19clpGBTInterface::holdPSModuleResets(Ph2_HwDescription::Chip* pChip)
{
    // Reset I2C Masters
    ResetI2C(pChip, {0, 1, 2});
    // hold resets
    for(uint8_t cSide = 0; cSide < 2; cSide++)
    {
        this->ssaReset(pChip, true, cSide);
        this->mpaReset(pChip, true, cSide);
        this->cicReset(pChip, true, cSide);
    }

    // Fabio: I do not think this part should be here, but I keep it for consistency with the previous code
#if defined(__TCUSB__)
    std::vector<uint8_t> cEportGroups = {4, 4, 5, 5, 6, 6, 0};
    std::vector<uint8_t> cEportChnls  = {0, 2, 0, 2, 0, 2, 0};
    InitialPhaseAlignRx(pChip, cEportGroups, cEportChnls);
    cEportGroups = {0, 1, 1, 2, 2, 3, 3};
    cEportChnls  = {2, 0, 2, 0, 2, 0, 2};
    InitialPhaseAlignRx(pChip, cEportGroups, cEportChnls);
#endif
}

void D19clpGBTInterface::configureClockSettings(Ph2_HwDescription::Chip* pChip, uint8_t pClk, lpGBTClockConfig pClkCnfg)
{
    fClkConfig.fClkFreq         = pClkCnfg.fClkFreq;
    fClkConfig.fClkInvert       = pClkCnfg.fClkInvert;
    fClkConfig.fClkDriveStr     = pClkCnfg.fClkDriveStr;
    fClkConfig.fClkPreEmphWidth = pClkCnfg.fClkPreEmphWidth;
    fClkConfig.fClkPreEmphMode  = pClkCnfg.fClkPreEmphMode;
    fClkConfig.fClkPreEmphStr   = pClkCnfg.fClkPreEmphStr;

    std::string cClkHReg = "EPCLK" + std::to_string(pClk) + "ChnCntrH";
    std::string cClkLReg = "EPCLK" + std::to_string(pClk) + "ChnCntrL";
    LOG(INFO) << BOLDBLUE << __PRETTY_FUNCTION__ << " [" << __LINE__ << "] writing " << cClkHReg << " to 0x" << std::hex
              << (fClkConfig.fClkInvert << 6 | fClkConfig.fClkDriveStr << 3 | fClkConfig.fClkFreq) << std::dec << RESET;
    LOG(INFO) << BOLDBLUE << __PRETTY_FUNCTION__ << " [" << __LINE__ << "] writing " << cClkLReg << " to 0x" << std::hex
              << (fClkConfig.fClkPreEmphStr << 5 | fClkConfig.fClkPreEmphMode << 3 | fClkConfig.fClkPreEmphWidth) << std::dec << RESET;
    WriteChipReg(pChip, cClkHReg, fClkConfig.fClkInvert << 6 | fClkConfig.fClkDriveStr << 3 | fClkConfig.fClkFreq);
    WriteChipReg(pChip, cClkLReg, fClkConfig.fClkPreEmphStr << 5 | fClkConfig.fClkPreEmphMode << 3 | fClkConfig.fClkPreEmphWidth);
}

// Preliminary
void D19clpGBTInterface::Configure2SSEH(Ph2_HwDescription::Chip* pChip)
{
    uint8_t cChipRate = GetChipRate(pChip);
    LOG(INFO) << BOLDGREEN << "Applying 2S-SEH lpGBT configuration for " << +cChipRate << "G module." << RESET;
    // Forcing driver attenuation to be 1
    uint8_t cEQAttenuation = 3;
    WriteChipReg(pChip, "EQConfig", cEQAttenuation << 3);
    // Clocks - by default all are off
    std::vector<uint8_t> cClocks  = {fClock_RHS_Hybrid, fClock_LHS_Hybrid}; // Reduced number of clocks and only 320 MHz
    uint8_t              cClkFreq = 0, cClkDriveStr = 7, cClkInvert = 1;
    uint8_t              cClkPreEmphWidth = 0, cClkPreEmphMode = 0, cClkPreEmphStr = 0;
    ConfigureClocks(pChip, cClocks, cClkFreq, cClkDriveStr, cClkInvert, cClkPreEmphWidth, cClkPreEmphMode, cClkPreEmphStr);

    // Tx Groups and Channels

    uint8_t cTxDataRate = 3, cTxDriveStr = 7, cTxPreEmphMode = 0, cTxPreEmphStr = 0, cTxPreEmphWidth = 0;
    for(const auto& TxProperty: static_cast<lpGBT*>(pChip)->getTxProperties()) { lpGBTInterface::ConfigureTxGroup(pChip, TxProperty.Group, TxProperty.Channel, cTxDataRate); }

    for(const auto& TxProperty: static_cast<lpGBT*>(pChip)->getTxProperties())
    {
        ConfigureTxChannel(pChip, TxProperty.Group, TxProperty.Channel, cTxDriveStr, cTxPreEmphMode, cTxPreEmphStr, cTxPreEmphWidth, TxProperty.Polarity);
    }

    // Rx configuration and Phase Align
    // Configure Rx Groups
    // WriteChipReg(pChip, "EPRXDllConfig", , false);

    uint8_t cRxDataRate = 2, cRxTrackMode = 0; // manual mode by default
    for(const auto& RxProperty: static_cast<lpGBT*>(pChip)->getRxProperties()) { lpGBTInterface::ConfigureRxGroup(pChip, RxProperty.Group, RxProperty.Channel, cRxDataRate, cRxTrackMode); }

    uint8_t cRxEqual = 0, cRxTerm = 1, cRxAcBias = 0, cRxPhase = 5;
    for(const auto& RxProperty: static_cast<lpGBT*>(pChip)->getRxProperties())
    {
        ConfigureRxChannel(pChip, RxProperty.Group, RxProperty.Channel, cRxEqual, cRxTerm, cRxAcBias, RxProperty.Polarity, cRxPhase);
    }
    // Configuring I2C Master pull-ups for VTRx+
    WriteChipReg(pChip, "I2CM1Config", 1 << 4 | 1 << 6);
    // Reset I2C Masters
    ResetI2C(pChip, {0, 1, 2});
    // Setting GPIO levels Uncomment this for Skeleton test
    // Setting GPIO levels for Skeleton test
    ConfigureGPIODirection(pChip, {fReset_LHS_CIC, fReset_LHS_CBC, fReset_RHS_CIC, fReset_RHS_CBC}, 1);
    ConfigureGPIOLevel(pChip, {fReset_LHS_CIC, fReset_LHS_CBC, fReset_RHS_CIC, fReset_RHS_CBC, fReset_VTRx}, 1);
    // hold resets
    for(uint8_t cSide = 0; cSide < 2; cSide++)
    {
        this->cbcReset(pChip, true, cSide);
        this->cicReset(pChip, true, cSide);
    }
#if defined(__TCUSB__)
    std::vector<uint8_t> cEportGroups = {4, 4, 5, 5, 6, 0};
    std::vector<uint8_t> cEportChnls  = {0, 2, 0, 2, 0, 0};
    InitialPhaseAlignRx(pChip, cEportGroups, cEportChnls);
    cEportGroups = {0, 1, 1, 2, 2, 3};
    cEportChnls  = {2, 0, 2, 0, 2, 2};
    InitialPhaseAlignRx(pChip, cEportGroups, cEportChnls);
    ConfigureCurrentDAC(pChip, std::vector<std::string>{"ADC4"}, 0x1c); // current chosen according to measurement range
#endif
}
void D19clpGBTInterface::Add2SSEHeLinkProperties(Ph2_HwDescription::Chip* pChip)
{
    std::vector<uint8_t> cRxGroups = {0, 1, 2, 3, 4, 5, 6}, cRxChannels = {0, 2};
    uint8_t              cRxInvert = 0;
    for(const auto& cGroup: cRxGroups)
    {
        for(const auto cChannel: cRxChannels)
        {
            if(cGroup == 6 && cChannel == 0)
                cRxInvert = 0;
            else if(cGroup == 5 && cChannel == 0)
                cRxInvert = 0;
            else
                cRxInvert = 1;

            if(!((cGroup == 6 && cChannel == 2) || (cGroup == 3 && cChannel == 0))) { static_cast<lpGBT*>(pChip)->addRxProperty(cGroup, cChannel, cRxInvert); }
        }
    }
    std::vector<uint8_t> cTxGroups = {0, 2};
    uint8_t              cTxInvert = 0;

    for(const auto& cGroup: cTxGroups)
    {
        if(cGroup == 0) cTxInvert = 1;
        if(cGroup == 2) cTxInvert = 0;

        static_cast<lpGBT*>(pChip)->addTxProperty(cGroup, 0, cTxInvert);
    }
}

void D19clpGBTInterface::InitialPhaseAlignRx(Chip* pChip, const std::vector<uint8_t>& pGroups, const std::vector<uint8_t>& pChannels)
{
    std::vector<uint8_t>                    cOptimalTaps = {};
    std::map<uint8_t, std::vector<uint8_t>> groupsAndChannels;

    for(size_t i = 0; i < pGroups.size(); ++i) { groupsAndChannels[pGroups.at(i)].push_back(pChannels.at(i)); }

    PhaseAlignRx(pChip, groupsAndChannels, 5);
    // find mode
    for(size_t cIndx = 0; cIndx < pGroups.size(); cIndx++) { cOptimalTaps.push_back(GetPhaseTap(pChip, pGroups.at(cIndx), pChannels.at(cIndx))); }
    std::vector<uint8_t> cTapsHist(15, 0);
    for(auto cItem: cOptimalTaps) cTapsHist.at(cItem)++;
    // return cTapsHist;
    auto cTapMode = std::max_element(cTapsHist.begin(), cTapsHist.end()) - cTapsHist.begin();
    LOG(INFO) << BOLDGREEN << "Applying Phase " << cTapMode << RESET;
    if(cTapMode != 15)
        for(size_t cIndx = 0; cIndx < pGroups.size(); cIndx++) { ConfigureRxPhase(pChip, pGroups.at(cIndx), pChannels.at(cIndx), cTapMode); }
}

LpGBTalignmentResult D19clpGBTInterface::PhaseAlignRx(Ph2_HwDescription::Chip* pChip, const std::map<uint8_t, std::vector<uint8_t>>& groupsAndChannels, size_t pMaxAttempts)
{
    LOG(INFO) << BOLDBLUE << "Aligning lpGBT#" << +pChip->getId() << RESET;
    const uint8_t cChipRate = lpGBTInterface::GetChipRate(pChip);

    // Configure Rx Phase Shifter
    uint16_t cDelay = 0;
    uint8_t  cFreq = (cChipRate == 5) ? 4 : 5, cEnFTune = 0, cDriveStr = 3; // 4 --> 320 MHz || 5 --> 640 MHz
    lpGBTInterface::ConfigurePhShifter(pChip, {0, 2}, cFreq, cDriveStr, cEnFTune, cDelay);

    LpGBTalignmentResult theAlignmentResults; // {Group : {channel : {successRate, bestPhaseHistogram}}}

    for(const auto& theGroupAndChannels: groupsAndChannels)
    {
        uint8_t cGroup = theGroupAndChannels.first;
        for(const auto cChannel: theGroupAndChannels.second)
        {
            float                       alignmentSuccessRate = 0.;
            GenericDataArray<float, 16> bestPhaseHistogram;
            std::fill(bestPhaseHistogram.begin(), bestPhaseHistogram.end(), 0);
            for(auto& value: bestPhaseHistogram) value = 0;

            cFreq         = 2;
            uint8_t cMode = 1; // Initial training mode
            lpGBTInterface::ConfigureRxGroup(pChip, cGroup, cChannel, cFreq, cMode);
            std::string cTrainRxReg;
            if(cGroup == 0 || cGroup == 1)
                cTrainRxReg = "EPRXTrain10";
            else if(cGroup == 2 || cGroup == 3)
                cTrainRxReg = "EPRXTrain32";
            else if(cGroup == 4 || cGroup == 5)
                cTrainRxReg = "EPRXTrain54";
            else if(cGroup == 6)
                cTrainRxReg = "EPRXTrainEc6";

            LOG(DEBUG) << BOLDYELLOW << "Group#" << +cGroup << " Channel#" << +cChannel << "...checking phase aligner" << RESET;
            for(size_t cAttempt = 0; cAttempt < pMaxAttempts; cAttempt++)
            {
                uint8_t cChipVersion = static_cast<lpGBT*>(pChip)->getVersion();
                if(cChipVersion == 0) { ResetRxDll(pChip, {cGroup}); }
                // Enable training
                uint8_t cTrainingShift = cChannel + 4 * (cGroup % 2);

                WriteChipReg(pChip, cTrainRxReg, (0x1 << cTrainingShift));
                std::this_thread::sleep_for(std::chrono::microseconds(10));
                WriteChipReg(pChip, cTrainRxReg, (0x0 << cTrainingShift));
                // Check for lock
                std::string cRXLockedReg = "EPRX" + std::to_string(cGroup) + "Locked";
                uint8_t     cLockShift   = cChannel + 4;
                auto        cLock        = 0;
                uint8_t     cCurrPhase   = 255;
                bool        cContinue    = true;
                uint8_t     cMaxIters    = 10;
                uint8_t     cIter        = 0;
                do {
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                    cLock     = (ReadChipReg(pChip, cRXLockedReg) & (1 << cLockShift)) >> cLockShift;
                    cContinue = cLock == 0;
                    cIter++;
                } while(cContinue && cIter < cMaxIters);
                if(cLock) alignmentSuccessRate += 1;
                WriteChipReg(pChip, cTrainRxReg, (0x0 << cTrainingShift));
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                cCurrPhase = lpGBTInterface::GetRxPhase(pChip, cGroup, cChannel);
                // LOG(DEBUG) << BOLDGREEN << "\t\t..Attempt# " << +cAttempt << "\t... RxPhase found  is... " << +cCurrPhase << RESET;
                bestPhaseHistogram.at(cCurrPhase)++;
            }

            // find phase with highest entries
            uint8_t bestPhase              = 15;
            int     highestCount           = 0;
            int     numberOfPossiblePhases = 0;
            for(size_t phaseValue = 0; phaseValue < bestPhaseHistogram.size(); ++phaseValue)
            {
                if(bestPhaseHistogram.at(phaseValue) > highestCount)
                {
                    highestCount = bestPhaseHistogram.at(phaseValue);
                    bestPhase    = phaseValue;
                }
                if(bestPhaseHistogram.at(phaseValue) > 0) ++numberOfPossiblePhases;
            }

            LOG(INFO) << BOLDGREEN << "Group#" << +cGroup << " Channel#" << +cChannel << "...\t\t..Most frequently found phase is " << +bestPhase << " out of " << numberOfPossiblePhases
                      << " possibilities" << RESET;
            SetPhaseTap(pChip, cGroup, cChannel, bestPhase);
            ConfigureRxPhase(pChip, cGroup, cChannel, bestPhase);

            // Normalize over total attempts
            alignmentSuccessRate /= pMaxAttempts;
            for(auto& phaseOccurrence: bestPhaseHistogram) phaseOccurrence /= pMaxAttempts;

            theAlignmentResults.setGroupAndChannelResult(cGroup, cChannel, alignmentSuccessRate, bestPhase, bestPhaseHistogram);
        }
    }

    uint8_t cMode = 0; // 2, continuous phase tracking : 0, fixed phase
    for(const auto& theGroupAndChannels: groupsAndChannels)
        for(const auto& channel: theGroupAndChannels.second) lpGBTInterface::ConfigureRxGroup(pChip, theGroupAndChannels.first, channel, 2, cMode);

    return theAlignmentResults;
}

bool D19clpGBTInterface::didAlignmentSucceded(LpGBTalignmentResult& theOpticalGroupAlignmentResult, float minAlignmentSuccessRate, const Ph2_HwDescription::OpticalGroup* theOpticalGroup)
{
    bool isAligned                     = true;
    auto theGroupsAndChannelsPerHybrid = theOpticalGroup->getLpGBTrxGroupsAndChannelsPerHybrid();

    for(const auto& theGoupAlignmenResult: theOpticalGroupAlignmentResult.fResultContainer)
    {
        for(const auto& theChannelAlignmentResult: theGoupAlignmenResult.second)
        {
            bool skipGroupAndChannel = false;
            // Check if the group and channel belong to a disabled hybrid
            for(auto cHybrid: *theOpticalGroup)
            {
                auto hybridAndChannel = theGroupsAndChannelsPerHybrid.at(std::make_pair(theGoupAlignmenResult.first, theChannelAlignmentResult.first));
                if(hybridAndChannel.first != (cHybrid->getId() % 2) && theOpticalGroup->size() != 2) { skipGroupAndChannel = true; }
            }

            float   alignmentSuccessRate = std::get<0>(theChannelAlignmentResult.second);
            uint8_t bestPhaseFound       = std::get<1>(theChannelAlignmentResult.second);
            if((alignmentSuccessRate < minAlignmentSuccessRate || bestPhaseFound == 15) && !skipGroupAndChannel)
            {
                isAligned = false;
                std::stringstream errorMessage;
                errorMessage << "OTalignLpGBTinputs::AlignLpGBTInputs - Error in aligning LpGBT Group " << +theGoupAlignmenResult.first << " Channel " << +theChannelAlignmentResult.first;
                if(alignmentSuccessRate < minAlignmentSuccessRate)
                    errorMessage << " - alignmen success rate = " << alignmentSuccessRate << " less then minimum requited (" << minAlignmentSuccessRate << ")";
                if(bestPhaseFound == 15) errorMessage << " best phase = 15 (error flag)";
                LOG(ERROR) << BOLDRED << errorMessage.str() << RESET;
            }
        }
    }
    return isAligned;
}

void D19clpGBTInterface::ConfigurePSROH(Ph2_HwDescription::Chip* pChip)
{
    uint8_t cChipRate = GetChipRate(pChip);
    LOG(INFO) << BOLDGREEN << "Applying PS-ROH-" << +cChipRate << "G lpGBT configuration" << RESET;
    // Forcing driver attenuation to be 1
    uint8_t cEQAttenuation = 3;
    WriteChipReg(pChip, "EQConfig", cEQAttenuation << 3);

    // Configuring I2C Master pull-ups for VTRx+
    WriteChipReg(pChip, "I2CM1Config", 1 << 4 | 1 << 6);
    // Clocks
    std::vector<uint8_t> cClocks = {fClock_LHS_Hybrid, fClock_LHS_CIC, fClock_RHS_Hybrid, fClock_RHS_CIC};
    // clock frequency set to 0 to disable it at first and only later configure what is needed
    uint8_t cClkFreq = 0, cClkDriveStr = 0, cClkInvert = 0;
    uint8_t cClkPreEmphWidth = 0, cClkPreEmphMode = 0, cClkPreEmphStr = 0;
    ConfigureClocks(pChip, cClocks, cClkFreq, cClkDriveStr, cClkInvert, cClkPreEmphWidth, cClkPreEmphMode, cClkPreEmphStr);

    // Tx Groups and Channels
    uint8_t cTxDataRate = 3, cTxDriveStr = 7, cTxPreEmphMode = 1, cTxPreEmphStr = 4, cTxPreEmphWidth = 0;
    for(const auto& TxProperty: static_cast<lpGBT*>(pChip)->getTxProperties()) { lpGBTInterface::ConfigureTxGroup(pChip, TxProperty.Group, TxProperty.Channel, cTxDataRate); }

    for(const auto& TxProperty: static_cast<lpGBT*>(pChip)->getTxProperties())
    {
        ConfigureTxChannel(pChip, TxProperty.Group, TxProperty.Channel, cTxDriveStr, cTxPreEmphMode, cTxPreEmphStr, cTxPreEmphWidth, TxProperty.Polarity);
    }

    // Rx configuration and Phase Align
    // Configure Rx Groups
    uint8_t cRxDataRate = 2, cRxTrackMode = 0;
    for(const auto& RxProperty: static_cast<lpGBT*>(pChip)->getRxProperties()) { lpGBTInterface::ConfigureRxGroup(pChip, RxProperty.Group, RxProperty.Channel, cRxDataRate, cRxTrackMode); }

    uint8_t cRxEqual = 0, cRxTerm = 1, cRxAcBias = 0, cRxPhase = 9;
    for(const auto& RxProperty: static_cast<lpGBT*>(pChip)->getRxProperties())
    {
        ConfigureRxChannel(pChip, RxProperty.Group, RxProperty.Channel, cRxEqual, cRxTerm, cRxAcBias, RxProperty.Polarity, cRxPhase);
    }

    // Reset I2C Masters
    ResetI2C(pChip, {0, 1, 2});
    // Setting GPIO levels for PS ROH
    ConfigureGPIODirection(pChip, {fReset_LHS_CIC, fReset_LHS_MPA, fReset_LHS_SSA, fReset_RHS_CIC, fReset_RHS_MPA, fReset_RHS_SSA}, 1);
    ConfigureGPIOLevel(pChip, {fReset_LHS_CIC, fReset_LHS_MPA, fReset_LHS_SSA, fReset_RHS_CIC, fReset_RHS_MPA, fReset_RHS_SSA, fReset_VTRx}, 1);
    // hold resets
    for(uint8_t cSide = 0; cSide < 2; cSide++)
    {
        this->ssaReset(pChip, true, cSide);
        this->mpaReset(pChip, true, cSide);
        this->cicReset(pChip, true, cSide);
    }
#if defined(__TCUSB__)
    std::vector<uint8_t> cEportGroups = {4, 4, 5, 5, 6, 6, 0};
    std::vector<uint8_t> cEportChnls  = {0, 2, 0, 2, 0, 2, 0};
    InitialPhaseAlignRx(pChip, cEportGroups, cEportChnls);
    cEportGroups = {0, 1, 1, 2, 2, 3, 3};
    cEportChnls  = {2, 0, 2, 0, 2, 0, 2};
    InitialPhaseAlignRx(pChip, cEportGroups, cEportChnls);

#endif
    LOG(INFO) << BOLDGREEN << "PS-ROH-" << +cChipRate << "G lpGBT configuration APPLIED" << RESET;
}
void D19clpGBTInterface::AddPSROHeLinkProperties(Ph2_HwDescription::Chip* pChip)
{
    std::vector<uint8_t> cTxGroups = {0, 1, 2, 3};
    uint8_t              cTxInvert = 0;
    for(const auto& cGroup: cTxGroups)
    {
        cTxInvert = (cGroup % 2 == 0) ? 1 : 0;
        static_cast<lpGBT*>(pChip)->addTxProperty(cGroup, 0, cTxInvert);
    }
    std::vector<uint8_t> cGrpsLeft{0, 1, 1, 2, 2, 3, 3};
    std::vector<uint8_t> cChnlsLeft{2, 0, 2, 0, 2, 0, 2};
    std::vector<uint8_t> cInvrtLeft{1, 1, 0, 1, 1, 1, 1};
    for(size_t cIndx = 0; cIndx < cInvrtLeft.size(); cIndx++)
    {
        uint8_t cGroup    = cGrpsLeft.at(cIndx);
        uint8_t cChannel  = cChnlsLeft.at(cIndx);
        uint8_t cRxInvert = cInvrtLeft.at(cIndx);
        static_cast<lpGBT*>(pChip)->addRxProperty(cGroup, cChannel, cRxInvert);
    }
    std::vector<uint8_t> cGrpsRight{4, 4, 5, 5, 6, 6, 0};
    std::vector<uint8_t> cChnlsRight{2, 0, 2, 0, 2, 0, 0};
    std::vector<uint8_t> cInvrtRight{0, 0, 0, 0, 0, 0, 1};
    for(size_t cIndx = 0; cIndx < cInvrtLeft.size(); cIndx++)
    {
        uint8_t cGroup    = cGrpsRight.at(cIndx);
        uint8_t cChannel  = cChnlsRight.at(cIndx);
        uint8_t cRxInvert = cInvrtRight.at(cIndx);
        static_cast<lpGBT*>(pChip)->addRxProperty(cGroup, cChannel, cRxInvert);
    }
}

void D19clpGBTInterface::updateCICinputClockToMatchPSrate(Ph2_HwDescription::Chip* pChip)
{
    auto        theChipRate               = this->GetChipRate(pChip);
    std::string cicClockRightRegisterName = "EPCLK" + std::to_string(fClock_RHS_CIC) + "ChnCntrH";
    std::string cicClockLeftRegisterName  = "EPCLK" + std::to_string(fClock_LHS_CIC) + "ChnCntrH";

    uint8_t expectedCicClockSetting;
    if(theChipRate == 5)
        expectedCicClockSetting = 0x4;
    else if(theChipRate == 10)
        expectedCicClockSetting = 0x5;
    else
    {
        std::string errorMessage =
            std::string(__PRETTY_FUNCTION__) + " LpGBT TX rate not identified on BeBoard " + std::to_string(pChip->getBeBoardId()) + " OpticalGroup " + std::to_string(pChip->getOpticalGroupId());
        throw std::runtime_error(errorMessage);
    }

    auto updateClockFunction = [this, theChipRate, pChip, expectedCicClockSetting](std::string registerName)
    {
        auto theCurrentRegisterValue = this->ReadChipReg(pChip, registerName);
        if((theCurrentRegisterValue & 0x7) != expectedCicClockSetting)
        {
            uint16_t theNewRegisterValue = (theCurrentRegisterValue & 0xF8) | (expectedCicClockSetting & 0x7);
            LOG(DEBUG) << BOLDYELLOW << "Attention! Updating " << registerName << " from 0x" << std::hex << +theCurrentRegisterValue << " to 0x" << +theNewRegisterValue << std::dec
                       << " to provide the CIC with the correct clock based on the LpGBT data rate (" << +theChipRate << "Gb) for on BeBoard " << +pChip->getBeBoardId() << " OpticalGroup "
                       << +pChip->getOpticalGroupId() << RESET;
            this->WriteChipReg(pChip, registerName, theNewRegisterValue);
        }
    };

    updateClockFunction(cicClockRightRegisterName);
    updateClockFunction(cicClockLeftRegisterName);
}

void D19clpGBTInterface::setCICClockPolarityAndStrength(Ph2_HwDescription::Chip* pChip, uint8_t pPolarity, uint8_t pStrength, Ph2_HwDescription::OpticalGroup* theOpticalGroup)
{
    std::string cicClockRightRegisterName = "EPCLK" + std::to_string(fClock_RHS_CIC) + "ChnCntrH";
    std::string cicClockLeftRegisterName  = "EPCLK" + std::to_string(fClock_LHS_CIC) + "ChnCntrH";
    if(theOpticalGroup->getFrontEndType() == FrontEndType::OuterTracker2S)
    {
        cicClockRightRegisterName = "EPCLK" + std::to_string(fClock_RHS_Hybrid) + "ChnCntrH";
        cicClockLeftRegisterName  = "EPCLK" + std::to_string(fClock_LHS_Hybrid) + "ChnCntrH";
    }

    auto updateClockFunction = [this, pChip](std::string registerName, uint8_t pPolarity, uint8_t pStrength)
    {
        auto theCurrentRegisterValue = this->ReadChipReg(pChip, registerName);
        if((theCurrentRegisterValue & 0xC0) != pPolarity || (theCurrentRegisterValue & 0x38) != pStrength)
        {
            uint16_t theNewRegisterValue = pPolarity << 6 | pStrength << 3 | (theCurrentRegisterValue & 0x7);
            LOG(DEBUG) << BOLDYELLOW << "Attention! Updating " << registerName << " from 0x" << std::hex << +theCurrentRegisterValue << " to 0x" << +theNewRegisterValue << std::dec
                       << " to update the CIC clock polarity and strenght on BeBoard " << +pChip->getBeBoardId() << " OpticalGroup " << +pChip->getOpticalGroupId() << RESET;
            this->WriteChipReg(pChip, registerName, theNewRegisterValue);
        }
    };

    updateClockFunction(cicClockRightRegisterName, pPolarity, pStrength);
    updateClockFunction(cicClockLeftRegisterName, pPolarity, pStrength);
}

void D19clpGBTInterface::vtrxReset(Ph2_HwDescription::Chip* pChip, bool pEnable)
{
    std::string registerName    = "PIOOutH";
    uint16_t    currentPIOvalue = pChip->getReg(registerName); // if I am resetting the VTRx there may be a communication issue and I cannot read from the LpGBT directly
    WriteChipReg(pChip, "PIOOutH", (currentPIOvalue & 0x7F) | ((pEnable ? 1 : 0) << 7), false);
}

} // namespace Ph2_HwInterface
