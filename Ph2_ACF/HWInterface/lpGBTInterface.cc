/*!
  \file                  lpGBTInterface.h
  \brief                 ImInterface to access and control the low-power Gigabit Transceiver chip
  \author                Younes Otarid
  \version               1.0
  \date                  03/03/20
  Support:               email to younes.otarid@cern.ch
  Support:               email to mauro.dinardo@cern.ch
*/

#include "HWInterface/lpGBTInterface.h"
#include "HWDescription/lpGBT.h"
#include "HWInterface/ExceptionHandler.h"
#include "Utils/NTChandler.h"

using namespace Ph2_HwDescription;

namespace Ph2_HwInterface
{
void lpGBTInterface::StartPRBSpattern(Chip* pChip)
{
    for(const auto& RxProperty: static_cast<lpGBT*>(pChip)->getRxProperties())
    {
        lpGBTInterface::ConfigureRxPRBS(pChip, RxProperty.Group, RxProperty.Channel, true);
        lpGBTInterface::ConfigureRxSource(pChip, RxProperty.Group, lpGBTconstants::PATTERN_PRBS);
    }
}

void lpGBTInterface::StopPRBSpattern(Chip* pChip)
{
    for(const auto& RxProperty: static_cast<lpGBT*>(pChip)->getRxProperties())
    {
        lpGBTInterface::ConfigureRxPRBS(pChip, RxProperty.Group, RxProperty.Channel, false);
        lpGBTInterface::ConfigureRxSource(pChip, RxProperty.Group, lpGBTconstants::PATTERN_NORMAL);
    }
}

// ################################
// # Chip configuration functions #
// ################################

bool lpGBTInterface::WriteChipReg(Chip* pChip, const std::string& pDacName, uint16_t pDacValue, bool pVerify)
{
    this->setBoard(pChip->getBeBoardId());
    auto cBoardType = fBoardFW->getBoardType();
    auto cAddress   = pChip->getRegItem(pDacName).fAddress;

    bool cSuccess = false;
    if((cBoardType != BoardType::RD53) && (pChip->isOptical() == true))
    {
        auto cRegisterMap             = pChip->getRegMap();
        cRegisterMap[pDacName].fValue = pDacValue;
        cSuccess                      = fBoardFW->SingleRegisterWrite(pChip, cRegisterMap[pDacName], pVerify);
    }
    else if(pChip->isOptical() == true)
        cSuccess = fBoardFW->WriteOptoLinkRegister(pChip, cAddress, pDacValue, pVerify);

    if(cSuccess == false)
    {
        LOG(WARNING) << BOLDRED << "LpGBT register writing issue on Board ID " << BOLDYELLOW << +pChip->getBeBoardId() << BOLDRED << " OpticalGroup ID " << BOLDYELLOW << +pChip->getOpticalGroupId()
                     << RESET;
        LOG(WARNING) << BOLDBLUE << "\t--> OpticalGroup will be disabled" << RESET;
        ExceptionHandler::getInstance()->disableOpticalGroup(pChip->getBeBoardId(), pChip->getOpticalGroupId());
        return cSuccess;
    }

    pChip->setReg(pDacName, pDacValue);
    return cSuccess;
}

int32_t lpGBTInterface::ReadChipReg(Chip* pChip, const std::string& pDacName)
{
    this->setBoard(pChip->getBeBoardId());
    const auto cBoardType = fBoardFW->getBoardType();

    const auto cAddress = pChip->getRegItem(pDacName).fAddress;
    uint16_t   cValue   = 0x0;

    if((cBoardType != BoardType::RD53) && (pChip->isOptical() == true))
    {
        auto cRegisterMap = pChip->getRegMap();
        cValue            = fBoardFW->SingleRegisterRead(pChip, cRegisterMap[pDacName]);
    }
    else if(pChip->isOptical() == true)
        cValue = fBoardFW->ReadOptoLinkRegister(pChip, cAddress);

    pChip->setReg(pDacName, cValue);
    return cValue;
}

uint32_t lpGBTInterface::ReadVTRxChipFuseID(Chip* pChip)
{
    uint32_t cChipId        = 0;
    uint8_t  cReadBackValue = 0;
    lpGBTInterface::ResetI2C(pChip, {0, 1, 2});
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    // Configuring I2C Master pull-ups
    WriteChipReg(pChip, "I2CM1Config", 1 << 4 | 1 << 6);

    uint8_t cMasterId = 1, cSlaveAddress = 0x50, cNbyte = 1, cFrequency = 2;

    bool cRecent = lpGBTInterface::WriteI2C(pChip, cMasterId, cSlaveAddress, 0x15, cNbyte, cFrequency);
    if(cRecent == true) cReadBackValue = lpGBTInterface::ReadI2C(pChip, cMasterId, cSlaveAddress, cNbyte, cFrequency);
    if(cReadBackValue == 0x15)
    {
        LOG(INFO) << GREEN << "VTRx+ with LDD version 1.3" << RESET;

        for(int i = 0; i < 4; i++)
        {
            lpGBTInterface::WriteI2C(pChip, cMasterId, cSlaveAddress, i + 0x16, cNbyte, cFrequency);
            cReadBackValue = lpGBTInterface::ReadI2C(pChip, cMasterId, cSlaveAddress, cNbyte, cFrequency);
            cChipId        = cChipId | cReadBackValue << (i * 8);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    LOG(INFO) << GREEN << "FuseID from VTRx+ 0x" << BOLDYELLOW << std::hex << +cChipId << std::dec << RESET;

    return cChipId;
}

void lpGBTInterface::DumpChipRegisters(Chip* pChip)
{
    this->setBoard(pChip->getBeBoardId());

    for(auto& cRegItem: pChip->getRegMap())
    {
        auto value = ReadChipReg(pChip, cRegItem.first);
        std::cout << "\t--> Register " << std::left << std::setfill(' ') << std::setw(24) << cRegItem.first << " = " << std::setw(8) << std::dec << value << std::hex << "(0x" << value << ")"
                  << std::endl;
    }
}

uint32_t lpGBTInterface::ReadChipFuseID(Chip* pChip, uint8_t version)
{
    if(version == 1)
    {
        uint32_t cChipID   = 0;
        uint32_t cChipID_0 = lpGBTInterface::ReadChipFusedBlock(pChip, 0, 0);
        LOG(DEBUG) << GREEN << "1st FuseID from LpGBT 0x" << BOLDYELLOW << std::hex << +cChipID_0 << std::dec << RESET;
        uint32_t cChipID_1 = lpGBTInterface::ReadChipFusedBlock(pChip, 0, 8);
        cChipID_1          = ((cChipID_1 & 0xFFFFFFC0) >> 6) | ((cChipID_1 & 0x3f) << 26);
        LOG(DEBUG) << GREEN << "2nd FuseID from LpGBT 0x" << BOLDYELLOW << std::hex << +cChipID_1 << std::dec << RESET;
        uint32_t cChipID_2 = lpGBTInterface::ReadChipFusedBlock(pChip, 0, 12);
        cChipID_2          = ((cChipID_2 & 0xFFFFF000) >> 12) | ((cChipID_2 & 0xfff) << 20);
        LOG(DEBUG) << GREEN << "3rd FuseID from LpGBT 0x" << BOLDYELLOW << std::hex << +cChipID_2 << std::dec << RESET;
        uint32_t cChipID_3 = lpGBTInterface::ReadChipFusedBlock(pChip, 0, 16);
        cChipID_3          = ((cChipID_3 & 0xFFFC0000) >> 18) | ((cChipID_3 & 0x3ffff) << 14);
        LOG(DEBUG) << GREEN << "4th FuseID from LpGBT 0x" << BOLDYELLOW << std::hex << +cChipID_3 << std::dec << RESET;
        uint32_t cChipID_4 = lpGBTInterface::ReadChipFusedBlock(pChip, 0, 20);
        cChipID_4          = ((cChipID_4 & 0xFF000000) >> 24) | ((cChipID_4 & 0xffffff) << 8);
        LOG(DEBUG) << GREEN << "5th FuseID from LpGBT 0x" << BOLDYELLOW << std::hex << +cChipID_4 << std::dec << RESET;
        for(int i = 0; i < 32; i++)
        {
            uint8_t cTemp = 0;
            cTemp         = ((cChipID_0 >> i) & 1) | ((cChipID_1 >> i & 1) << 1) | ((cChipID_2 >> i & 1) << 2) | ((cChipID_3 >> i & 1) << 3) | ((cChipID_4 >> i & 1) << 4);
            if(__builtin_popcountll(cTemp) > 2) { cChipID = cChipID | (1 << i); }
            else
                cChipID = cChipID | (0 << i);
        }

        if(cChipID == 0)
        {
            LOG(DEBUG) << GREEN << "No redundant LpGBT ID, only use first register" << RESET;
            cChipID = cChipID_0;
        }
        LOG(INFO) << GREEN << "FuseID from LpGBT optical group " << BOLDYELLOW << +pChip->getOpticalGroupId() << RESET << GREEN << " on Board " << BOLDYELLOW << +pChip->getBeBoardId() << RESET
                  << GREEN << ": 0x" << BOLDYELLOW << std::hex << +cChipID << std::dec << RESET;
        return cChipID;
    }

    LOG(INFO) << GREEN << "No FuseID for version 0 LpGBT optical group " << BOLDYELLOW << +pChip->getOpticalGroupId() << RESET << GREEN << " on Board " << BOLDYELLOW << +pChip->getBeBoardId()
              << RESET;
    return 0;
}

uint32_t lpGBTInterface::ReadChipFusedBlock(Chip* pChip, uint8_t cFuseH, uint8_t cFuseL)
{
    WriteChipReg(pChip, "FUSEControl", 2);
    int      cReadBack = 0;
    uint32_t cResult   = 0;

    while(cReadBack != 4)
    {
        cReadBack = ReadChipReg(pChip, "FUSEStatus");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        LOG(DEBUG) << GREEN << "LpGBT FUSEStatus = " << BOLDYELLOW << +cReadBack << RESET;
    }
    WriteChipReg(pChip, "FUSEBlowAddH", cFuseH);
    WriteChipReg(pChip, "FUSEBlowAddL", cFuseL);

    LOG(DEBUG) << GREEN << "LpGBT FUSEBlowAddH = " << BOLDYELLOW << +cFuseH << RESET;
    LOG(DEBUG) << GREEN << "LpGBT FUSEBlowAddL = " << BOLDYELLOW << +cFuseL << RESET;

    cReadBack = ReadChipReg(pChip, "FUSEValuesA");
    cResult   = cResult | (cReadBack);
    LOG(DEBUG) << GREEN << "LpGBT FUSEValuesA = " << BOLDYELLOW << +cReadBack << RESET;

    cReadBack = ReadChipReg(pChip, "FUSEValuesB");
    cResult   = cResult | (cReadBack << 8);
    LOG(DEBUG) << GREEN << "LpGBT FUSEValuesB = " << BOLDYELLOW << +cReadBack << RESET;

    cReadBack = ReadChipReg(pChip, "FUSEValuesC");
    cResult   = cResult | (cReadBack << 16);
    LOG(DEBUG) << GREEN << "LpGBT FUSEValuesC = " << BOLDYELLOW << +cReadBack << RESET;

    cReadBack = ReadChipReg(pChip, "FUSEValuesD");
    cResult   = cResult | (cReadBack << 24);
    LOG(DEBUG) << GREEN << "LpGBT FUSEValuesD = " << BOLDYELLOW << +cReadBack << RESET;

    WriteChipReg(pChip, "FUSEControl", 0);

    return cResult;
}

bool lpGBTInterface::WriteChipMultReg(Chip* pChip, const std::vector<std::pair<std::string, uint16_t>>& pRegVec, bool pVerify)
{
    bool writeGood = true;
    for(const auto& cReg: pRegVec) writeGood &= WriteChipReg(pChip, cReg.first, cReg.second);
    return writeGood;
}

// #######################################
// # LpGBT block configuration functions #
// #######################################

void lpGBTInterface::SetPUSMDone(Chip* pChip, bool pPllConfigDone, bool pDllConfigDone) { WriteChipReg(pChip, "POWERUP2", pDllConfigDone << 2 | pPllConfigDone << 1); }

void lpGBTInterface::ConfigureRxAlignmentMode(Chip* pChip, const std::vector<uint8_t>& pGroups, uint8_t pTrackMode)
{
    for(const auto& cGroup: pGroups)
    {
        std::string cRXCntrReg = "EPRX" + std::to_string(cGroup) + "Control";
        auto        cRegValue  = ReadChipReg(pChip, cRXCntrReg);
        WriteChipReg(pChip, cRXCntrReg, (cRegValue & 0xFC) | pTrackMode);
    }
}

uint16_t lpGBTInterface::GetRxDataRate(Chip* pChip, uint8_t pGroup)
{
    uint16_t    cChipRate  = lpGBTInterface::GetChipRate(pChip);
    std::string cRXCntrReg = "EPRX" + std::to_string(pGroup) + "Control";
    auto        cRegValue  = ReadChipReg(pChip, cRXCntrReg);
    uint16_t    cValue     = (cRegValue & 0xC);
    return (cChipRate / 5.) * (int)cValue * (float)lpGBTconstants::ACCELERATOR_CLK / 1e6;
}

uint8_t lpGBTInterface::GetChipRate(Chip* pChip)
{
    uint8_t cValueConfigPins = ((ReadChipReg(pChip, "ConfigPins") & 0xF0) >> 4);

    if(cValueConfigPins <= 7)
        return 5;
    else if(cValueConfigPins <= 15)
        return 10;
    else
        throw std::runtime_error(std::string("lpGBT hard wired configuration doesn't exist"));
}

void lpGBTInterface::ConfigureRxGroup(Chip* pChip, uint8_t pGroup, uint8_t pChannel, uint8_t pDataRate, uint8_t pTrackMode)
{
    // #######################################################################
    // # Enable Rx Groups Channels and set Data Rate and Phase Tracking mode #
    // #######################################################################
    std::string cRXCntrReg     = "EPRX" + std::to_string(pGroup) + "Control";
    uint8_t     cValueEnableRx = (ReadChipReg(pChip, cRXCntrReg) >> 4);
    cValueEnableRx |= (1 << pChannel);
    WriteChipReg(pChip, cRXCntrReg, (cValueEnableRx << 4) | (pDataRate << 2) | (pTrackMode << 0));
}

void lpGBTInterface::ConfigureRxChannel(Chip* pChip, uint8_t pGroup, uint8_t pChannel, uint8_t pEqual, uint8_t pTerm, uint8_t pAcBias, uint8_t pInvert, uint8_t pPhase)
{
    // #######################################################################################################
    // # Configure Rx Channel Phase, Inversion, AcBias enabling, Termination enabling, Equalization enabling #
    // #######################################################################################################
    std::string cRXChnCntrReg = "EPRX" + std::to_string(pGroup) + std::to_string(pChannel) + "ChnCntr";
    WriteChipReg(pChip, cRXChnCntrReg, (pPhase << 4) | (pInvert << 3) | (pAcBias << 2) | (pTerm << 1) | (pEqual << 0));
}

void lpGBTInterface::ConfigureTxGroup(Chip* pChip, uint8_t pGroup, uint8_t pChannel, uint8_t pDataRate)
{
    // ##########################################################
    // # Configure Tx Group Data Rate value for specified group #
    // ##########################################################
    uint8_t cValueDataRate = ReadChipReg(pChip, "EPTXDataRate");
    WriteChipReg(pChip, "EPTXDataRate", (cValueDataRate & ~(0x03 << 2 * pGroup)) | (pDataRate << 2 * pGroup));

    // #############################################
    // # Enable given channels for specified group #
    // #############################################
    std::string cEnableTxReg;
    if(pGroup == 0 || pGroup == 1)
        cEnableTxReg = "EPTX10Enable";
    else if(pGroup == 2 || pGroup == 3)
        cEnableTxReg = "EPTX32Enable";

    uint8_t cValueEnableTx = ReadChipReg(pChip, cEnableTxReg);
    cValueEnableTx |= (1 << (pChannel + 4 * (pGroup % 2)));
    WriteChipReg(pChip, cEnableTxReg, cValueEnableTx);
}

void lpGBTInterface::ConfigureTxChannel(Chip* pChip, uint8_t pGroup, uint8_t pChannel, uint8_t pDriveStr, uint8_t pPreEmphMode, uint8_t pPreEmphStr, uint8_t pPreEmphWidth, uint8_t pInvert)
{
    // ############################################################################
    // # Configure Tx Channel PreEmphasisStrength, PreEmphasisMode, DriveStrength #
    // ############################################################################
    std::string cTXChnCntr = "EPTX" + std::to_string(pGroup) + std::to_string(pChannel) + "ChnCntr";
    WriteChipReg(pChip, cTXChnCntr, (pPreEmphStr << 5) | (pPreEmphMode << 3) | (pDriveStr << 0));

    // ####################################################
    // # Configure Tx Channel PreEmphasisWidth, Inversion #
    // ####################################################
    if(pChannel == 0 || pChannel == 1)
        cTXChnCntr = "EPTX" + std::to_string(pGroup) + "1_" + std::to_string(pGroup) + "0ChnCntr";
    else if(pChannel == 2 || pChannel == 3)
        cTXChnCntr = "EPTX" + std::to_string(pGroup) + "3_" + std::to_string(pGroup) + "2ChnCntr";

    uint8_t cValueChnCntr = ReadChipReg(pChip, cTXChnCntr);
    WriteChipReg(pChip, cTXChnCntr, (cValueChnCntr & ~(0x0F << 4 * (pChannel % 2))) | ((pInvert << 3 | pPreEmphWidth << 0) << 4 * (pChannel % 2)));
}

void lpGBTInterface::ConfigureClocks(Chip*                       pChip,
                                     const std::vector<uint8_t>& pClocks,
                                     uint8_t                     pFreq,
                                     uint8_t                     pDriveStr,
                                     uint8_t                     pInvert,
                                     uint8_t                     pPreEmphWidth,
                                     uint8_t                     pPreEmphMode,
                                     uint8_t                     pPreEmphStr)
{
    for(const auto& cClock: pClocks)
    {
        // #######################################################################################################################
        // # Configure Clocks Frequency, Drive Strength, Inversion, Pre-Emphasis Width, Pre-Emphasis Mode, Pre-Emphasis Strength #
        // #######################################################################################################################
        std::string cClkHReg = "EPCLK" + std::to_string(cClock) + "ChnCntrH";
        std::string cClkLReg = "EPCLK" + std::to_string(cClock) + "ChnCntrL";
        std::cout << __PRETTY_FUNCTION__ << " [" << __LINE__ << "] writing " << cClkHReg << " to 0x" << std::hex << (pInvert << 6 | pDriveStr << 3 | pFreq) << std::dec << std::endl;
        std::cout << __PRETTY_FUNCTION__ << " [" << __LINE__ << "] writing " << cClkLReg << " to 0x" << std::hex << (pPreEmphStr << 5 | pPreEmphMode << 3 | pPreEmphWidth) << std::dec << std::endl;
        WriteChipReg(pChip, cClkHReg, pInvert << 6 | pDriveStr << 3 | pFreq);
        WriteChipReg(pChip, cClkLReg, pPreEmphStr << 5 | pPreEmphMode << 3 | pPreEmphWidth);
    }
}

void lpGBTInterface::ConfigureHighSpeedPolarity(Chip* pChip, uint8_t pOutPolarity, uint8_t pInPolarity)
{
    uint8_t cPolarity = (pOutPolarity << 7 | pInPolarity << 6);
    WriteChipReg(pChip, "ChipConfig", cPolarity);
}

void lpGBTInterface::ConfigureDPPattern(Chip* pChip, uint32_t pPattern)
{
    WriteChipReg(pChip, "DPDataPattern0", (pPattern & 0xFF));
    WriteChipReg(pChip, "DPDataPattern1", ((pPattern & 0xFF00) >> 8));
    WriteChipReg(pChip, "DPDataPattern2", ((pPattern & 0xFF0000) >> 16));
    WriteChipReg(pChip, "DPDataPattern3", ((pPattern & 0xFF000000) >> 24));
}

void lpGBTInterface::ConfigureRxPRBS(Chip* pChip, uint8_t pGroup, uint8_t pChannel, bool pEnable)
{
    std::string cPRBSReg;
    if(pGroup == 1 || pGroup == 0)
        cPRBSReg = "EPRXPRBS0";
    else if(pGroup == 3 || pGroup == 2)
        cPRBSReg = "EPRXPRBS1";
    else if(pGroup == 5 || pGroup == 4)
        cPRBSReg = "EPRXPRBS2";
    else if(pGroup == 6)
        cPRBSReg = "EPRXPRBS3";

    uint8_t cEnabledCh       = 0;
    uint8_t cValueEnablePRBS = ReadChipReg(pChip, cPRBSReg);
    cEnabledCh |= pEnable << pChannel;
    WriteChipReg(pChip, cPRBSReg, (cValueEnablePRBS & ~(0xF << 4 * (pGroup % 2))) | (cEnabledCh << (4 * (pGroup % 2))));

} // namespace Ph2_HwInterface

void lpGBTInterface::ConfigureRxSource(Chip* pChip, uint8_t pGroup, uint8_t pSource)
{
    if(pSource == 0)
        LOG(INFO) << GREEN << "Configuring Rx group " << BOLDYELLOW << +pGroup << RESET << GREEN << " source to " << BOLDYELLOW << "NORMAL " << RESET;
    else if(pSource == 1)
        LOG(INFO) << GREEN << "Configuring Rx group " << BOLDYELLOW << +pGroup << RESET << GREEN << " source to " << BOLDYELLOW << "PRBS7 " << RESET;
    else if(pSource == 4 || pSource == 5)
        LOG(INFO) << GREEN << "Configuring Rx group " << BOLDYELLOW << +pGroup << RESET << GREEN << " source to " << BOLDYELLOW << "Constant Pattern" << RESET;

    std::string cRxSourceReg;
    if(pGroup == 0 || pGroup == 1)
        cRxSourceReg = "ULDataSource1";
    else if(pGroup == 2 || pGroup == 3)
        cRxSourceReg = "ULDataSource2";
    else if(pGroup == 4 || pGroup == 5)
        cRxSourceReg = "ULDataSource3";
    else if(pGroup == 6)
        cRxSourceReg = "ULDataSource4";

    uint8_t cValueRxSource = ReadChipReg(pChip, cRxSourceReg);
    WriteChipReg(pChip, cRxSourceReg, (cValueRxSource & ~(0x7 << 3 * (pGroup % 2))) | (pSource << 3 * (pGroup % 2)));
}

void lpGBTInterface::ConfigureTxSource(Chip* pChip, uint8_t pGroup, uint8_t pSource)
{
    if(pSource == 0)
        LOG(INFO) << GREEN << "Configuring Tx Group " << BOLDYELLOW << +pGroup << RESET << GREEN << " Source to NORMAL " << RESET;
    else if(pSource == 1)
        LOG(INFO) << GREEN << "Configuring Tx Group " << BOLDYELLOW << +pGroup << RESET << GREEN << " Source to PRBS7 " << RESET;
    else if(pSource == 2)
        LOG(INFO) << GREEN << "Configuring Tx Group " << BOLDYELLOW << +pGroup << RESET << GREEN << " Source to Binary counter " << RESET;
    else if(pSource == 3)
        LOG(INFO) << GREEN << "Configuring Tx Group " << BOLDYELLOW << +pGroup << RESET << GREEN << " Source to Constant Pattern" << RESET;

    uint8_t cULDataSrcValue = ReadChipReg(pChip, "ULDataSource5");
    cULDataSrcValue         = (cULDataSrcValue & ~(0x3 << (2 * pGroup))) | (pSource << (2 * pGroup));
    WriteChipReg(pChip, "ULDataSource5", cULDataSrcValue);
}

bool lpGBTInterface::ConfigureRxPhase(Chip* pChip, uint8_t pGroup, uint8_t pChannel, uint8_t pPhase)
{
    std::string cRegName      = "EPRX" + std::to_string(pGroup) + std::to_string(pChannel) + "ChnCntr";
    uint8_t     cValueChnCntr = ReadChipReg(pChip, cRegName);
    cValueChnCntr             = (cValueChnCntr & ~(0xF << 4)) | (pPhase << 4);
    return WriteChipReg(pChip, cRegName, cValueChnCntr);
}

void lpGBTInterface::ConfigureAllRxPhase(Chip* pChip, uint8_t pPhase, std::map<uint8_t, std::vector<uint8_t>> theGroupsAndChannels)
{
    for(const auto& groupAndChannels: theGroupsAndChannels)
    {
        for(const auto channel: groupAndChannels.second) { this->ConfigureRxPhase(pChip, groupAndChannels.first, channel, pPhase); }
    }
}

void lpGBTInterface::ConfigurePhShifter(Chip* pChip, const std::vector<uint8_t>& pClocks, uint8_t pFreq, uint8_t pDriveStr, uint8_t pEnFTune, uint16_t pDelay)
{
    for(const auto& cClock: pClocks)
    {
        std::string cDelayReg  = "PS" + std::to_string(cClock) + "Delay";
        std::string cConfigReg = "PS" + std::to_string(cClock) + "Config";
        WriteChipReg(pChip, cConfigReg, (((pDelay & 0x100) >> 8) << 7) | pEnFTune << 6 | pDriveStr << 3 | pFreq);
        WriteChipReg(pChip, cDelayReg, pDelay);
    }
}

void lpGBTInterface::SetPhaseTap(Chip* pChip, uint8_t pGroup, uint8_t pChannel, uint8_t pPhase)
{
    std::string cKey = "Group" + std::to_string(pGroup) + "Channel" + std::to_string(pChannel);
    auto        cIt  = fPhaseTapMap.find(cKey);
    if(cIt != fPhaseTapMap.end()) { cIt->second = pPhase; }
    else
        throw std::runtime_error(std::string("Unused Channel or Group!"));
}

uint8_t lpGBTInterface::GetPhaseTap(Chip* pChip, uint8_t pGroup, uint8_t pChannel)
{
    std::string cKey = "Group" + std::to_string(pGroup) + "Channel" + std::to_string(pChannel);
    auto        cIt  = fPhaseTapMap.find(cKey);

    if(cIt != fPhaseTapMap.end()) { return cIt->second; }
    else
        throw std::runtime_error(std::string("Unused Channel or Group!"));

    return 15;
}
std::map<std::string, uint8_t> lpGBTInterface::GetPhaseTapMap() { return fPhaseTapMap; }

// ####################################
// # LpGBT specific routine functions #
// ####################################

bool lpGBTInterface::ConfigureVref(Chip* pChip, uint8_t pEnable, uint8_t pCorrection)
{
    uint8_t cVal     = pEnable << 7 | (pCorrection & 0x3F);
    bool    cSuccess = WriteChipReg(pChip, "VREFCNTR", cVal);
    LOG(DEBUG) << BOLDBLUE << "VREFCNTR : 0x" << std::hex << +cVal << std::dec << RESET;
    std::this_thread::sleep_for(std::chrono::microseconds(lpGBTconstants::DEEPSLEEP));

    return cSuccess;
}

bool lpGBTInterface::EnableInternalVref(Chip* pChip, bool pEnable) { return true; }

bool lpGBTInterface::SetVrefTune(Chip* pChip, uint8_t pVrefTune)
{
    uint8_t     cNbits   = (static_cast<lpGBT*>(pChip)->getVersion() == 0) ? 5 : 8;
    std::string cRegName = (static_cast<lpGBT*>(pChip)->getVersion() == 0) ? "VREFCNTR" : "VREFTUNE";
    ChipRegMask cMask;
    cMask.fBitShift = 0;
    cMask.fNbits    = cNbits;
    pChip->setRegBits(cRegName, cMask, pVrefTune);
    WriteChipReg(pChip, cRegName, pVrefTune);
    auto cVrefTune = pChip->getRegItem(cRegName).fValue;

    return cVrefTune == pVrefTune;
}

uint8_t lpGBTInterface::GetVrefTune(Chip* pChip)
{
    uint8_t     cNbits    = (static_cast<lpGBT*>(pChip)->getVersion() == 0) ? 5 : 8;
    std::string cRegName  = (static_cast<lpGBT*>(pChip)->getVersion() == 0) ? "VREFCNTR" : "VREFTUNE";
    auto        cVrefTune = pChip->getRegItem(cRegName).fValue;
    uint8_t     mask      = (0xFF >> (8 - cNbits));

    return (mask & cVrefTune);
}

float lpGBTInterface::GetVref(Chip* pChip, const std::string& pADC, uint16_t pVinput) // pVinput in mV!
{
    auto cGain   = lpGBTInterface::GetADCGain(pChip, false);
    auto cOffset = lpGBTInterface::GetADCOffset(pChip, false);
    auto cADC    = lpGBTInterface::ReadADC(pChip, pADC);
    return ((int)pVinput / 1000. * cGain * 512) / (cADC - cOffset * (1 - cGain / 2.));
}

uint8_t lpGBTInterface::TuneVref(Chip* pChip)
{
    const std::string pADC         = static_cast<lpGBT*>(pChip)->getTuneVrefADC();
    uint16_t          pVinput      = static_cast<lpGBT*>(pChip)->getTuneVrefVoltage();
    uint8_t           cNbits       = (static_cast<lpGBT*>(pChip)->getVersion() == 0) ? 5 : 8;
    uint8_t           cCurrentStep = (0xFF >> (8 - cNbits));
    SetVrefTune(pChip, cCurrentStep);
    auto     cVrefTune     = GetVrefTune(pChip);
    float    cCurrentVref  = GetVref(pChip, pADC, pVinput);
    uint16_t cPreviousStep = cCurrentStep;

    LOG(INFO) << GREEN << "Tune Vref of lpGBT using input of " << BOLDYELLOW << pADC << RESET << GREEN << " and " << BOLDYELLOW << pVinput << RESET << GREEN << "mV" << RESET;

    for(int iBit = cNbits - 1; iBit >= 0; --iBit)
    {
        // ############
        // # Flip bit #
        // ############
        cCurrentStep = cPreviousStep + (1 << iBit);
        SetVrefTune(pChip, cCurrentStep);
        cVrefTune    = GetVrefTune(pChip);
        cCurrentVref = GetVref(pChip, pADC, pVinput);

        // ####################################
        // # Determine if it is better or not #
        // ####################################
        if(cCurrentVref < 1.) cPreviousStep = cCurrentStep;
        SetVrefTune(pChip, cCurrentStep);
        cVrefTune    = GetVrefTune(pChip);
        cCurrentVref = GetVref(pChip, pADC, pVinput);

        if(static_cast<lpGBT*>(pChip)->getVersion() == 0)
            LOG(INFO) << GREEN << " Flip Bit#" << BOLDYELLOW << +iBit << RESET << GREEN << " Tune =  " << BOLDYELLOW << std::bitset<5>(cVrefTune) << RESET << GREEN << " Vref = " << BOLDYELLOW
                      << cCurrentVref << RESET;
        else
            LOG(INFO) << GREEN << " Flip Bit#" << BOLDYELLOW << +iBit << RESET << GREEN << " Tune =  " << BOLDYELLOW << std::bitset<8>(cVrefTune) << RESET << GREEN << " Vref = " << BOLDYELLOW
                      << cCurrentVref << RESET;
    }

    LOG(INFO) << GREEN << "Vref tune set to " << BOLDYELLOW << +cVrefTune << RESET << GREEN << " - Vref = " << BOLDYELLOW << cCurrentVref << RESET;
    return cVrefTune;
}

void lpGBTInterface::PhaseTrainRx(Chip* pChip, const std::vector<uint8_t>& pGroups)
{
    for(const auto& cGroup: pGroups)
    {
        std::string cTrainRxReg;
        if(cGroup == 0 || cGroup == 1)
            cTrainRxReg = "EPRXTrain10";
        else if(cGroup == 2 || cGroup == 3)
            cTrainRxReg = "EPRXTrain32";
        else if(cGroup == 4 || cGroup == 5)
            cTrainRxReg = "EPRXTrain54";
        else if(cGroup == 6)
            cTrainRxReg = "EPRXTrainEc6";

        WriteChipReg(pChip, cTrainRxReg, 0x0F << 4 * (cGroup % 2));
        WriteChipReg(pChip, cTrainRxReg, 0x00 << 4 * (cGroup % 2));
    }
}

void lpGBTInterface::ResetRxDll(Chip* pChip, const std::vector<uint8_t>& pGroups)
{
    std::string cRegName = "RST1";
    uint8_t     cValue   = 0x00;
    for(auto cGroup: pGroups) { cValue = cValue | (1 << cGroup); }
    WriteChipReg(pChip, "RST1", cValue);
    std::this_thread::sleep_for(std::chrono::microseconds(lpGBTconstants::DEEPSLEEP));
    WriteChipReg(pChip, "RST1", 0x00);
}

// ################################
// # LpGBT Block Status functions #
// ################################

bool lpGBTInterface::IsPUSMDone(Chip* pChip) { return lpGBTInterface::GetPUSMStatus(pChip) == revertedPUSMStatusMap["READY"]; }

uint8_t lpGBTInterface::PrintChipMode(Chip* pChip)
{
    uint8_t cChipMode = (ReadChipReg(pChip, "ConfigPins") & 0xF0) >> 4;
    switch(cChipMode)
    {
    case 0:
        LOG(INFO) << GREEN << "LpGBT chip info: Tx Data Rate = " << BOLDYELLOW << "5 Gbit/s" << RESET << GREEN << "; TxEncoding = " << BOLDYELLOW << "FEC5" << RESET << GREEN
                  << "; LpGBT Mode = " << BOLDYELLOW << "Off" << RESET;
        break;
    case 1:
        LOG(INFO) << GREEN << "LpGBT chip info: Tx Data Rate = " << BOLDYELLOW << "5 Gbit/s" << RESET << GREEN << "; TxEncoding = " << BOLDYELLOW << "FEC5" << RESET << GREEN
                  << "; LpGBT Mode = " << BOLDYELLOW << "Simplex TX" << RESET;
        break;
    case 2:
        LOG(INFO) << GREEN << "LpGBT chip info; Tx Data Rate = " << BOLDYELLOW << "5 Gbit/s" << RESET << GREEN << "; TxEncoding = " << BOLDYELLOW << "FEC5" << RESET << GREEN
                  << "; LpGBT Mode = " << BOLDYELLOW << "Simplex RX" << RESET;
        break;
    case 3:
        LOG(INFO) << GREEN << "LpGBT chip info: Tx Data Rate = " << BOLDYELLOW << "5 Gbit/s" << RESET << GREEN << "; TxEncoding = " << BOLDYELLOW << "FEC5" << RESET << GREEN
                  << "; LpGBT Mode = " << BOLDYELLOW << "Transceiver" << RESET;
        break;
    case 4:
        LOG(INFO) << GREEN << "LpGBT chip info: Tx Data Rate = " << BOLDYELLOW << "5 Gbit/s" << RESET << GREEN << "; TxEncoding = " << BOLDYELLOW << "FEC12" << RESET << GREEN
                  << "; LpGBT Mode = " << BOLDYELLOW << "Off" << RESET;
        break;
    case 5:
        LOG(INFO) << GREEN << "LpGBT chip info: Tx Data Rate = " << BOLDYELLOW << "5 Gbit/s" << RESET << GREEN << "; TxEncoding = " << BOLDYELLOW << "FEC12" << RESET << GREEN
                  << "; LpGBT Mode = " << BOLDYELLOW << "Simplex TX" << RESET;
        break;
    case 6:
        LOG(INFO) << GREEN << "LpGBT chip info: Tx Data Rate = " << BOLDYELLOW << "5 Gbit/s" << RESET << GREEN << "; TxEncoding = " << BOLDYELLOW << "FEC12" << RESET << GREEN
                  << "; LpGBT Mode = " << BOLDYELLOW << "Simplex RX" << RESET;
        break;
    case 7:
        LOG(INFO) << GREEN << "LpGBT chip info: Tx Data Rate = " << BOLDYELLOW << "5 Gbit/s" << RESET << GREEN << "; TxEncoding = " << BOLDYELLOW << "FEC12" << RESET << GREEN
                  << "; LpGBT Mode = " << BOLDYELLOW << "Transceiver" << RESET;
        break;
    case 8:
        LOG(INFO) << GREEN << "LpGBT chip info: Tx Data Rate = " << BOLDYELLOW << "10 Gbit/s" << RESET << GREEN << "; TxEncoding = " << BOLDYELLOW << "FEC5" << RESET << GREEN
                  << "; LpGBT Mode = " << BOLDYELLOW << "Off" << RESET;
        break;
    case 9:
        LOG(INFO) << GREEN << "LpGBT chip info: Tx Data Rate = " << BOLDYELLOW << "10 Gbit/s" << RESET << GREEN << "; TxEncoding = " << BOLDYELLOW << "FEC5" << RESET << GREEN
                  << "; LpGBT Mode = " << BOLDYELLOW << "Simplex TX" << RESET;
        break;
    case 10:
        LOG(INFO) << GREEN << "LpGBT chip info: Tx Data Rate = " << BOLDYELLOW << "10 Gbit/s" << RESET << GREEN << "; TxEncoding = " << BOLDYELLOW << "FEC5" << RESET << GREEN
                  << "; LpGBT Mode = " << BOLDYELLOW << "Simplex RX" << RESET;
        break;
    case 11:
        LOG(INFO) << GREEN << "LpGBT chip info: Tx Data Rate = " << BOLDYELLOW << "10 Gbit/s" << RESET << GREEN << "; TxEncoding = " << BOLDYELLOW << "FEC5" << RESET << GREEN
                  << "; LpGBT Mode = " << BOLDYELLOW << "Transceiver" << RESET;
        break;
    case 12:
        LOG(INFO) << GREEN << "LpGBT chip info: Tx Data Rate = " << BOLDYELLOW << "10 Gbit/s" << RESET << GREEN << "; TxEncoding = " << BOLDYELLOW << "FEC12" << RESET << GREEN
                  << "; LpGBT Mode = " << BOLDYELLOW << "Off" << RESET;
        break;
    case 13:
        LOG(INFO) << GREEN << "LpGBT chip info: Tx Data Rate = " << BOLDYELLOW << "10 Gbit/s" << RESET << GREEN << "; TxEncoding = " << BOLDYELLOW << "FEC12" << RESET << GREEN
                  << "; LpGBT Mode = " << BOLDYELLOW << "Simplex TX" << RESET;
        break;
    case 14:
        LOG(INFO) << GREEN << "LpGBT chip info: Tx Data Rate = " << BOLDYELLOW << "10 Gbit/s" << RESET << GREEN << "; TxEncoding = " << BOLDYELLOW << "FEC12" << RESET << GREEN
                  << "; LpGBT Mode = " << BOLDYELLOW << "Simplex RX" << RESET;
        break;
    case 15:
        LOG(INFO) << GREEN << "LpGBT chip info: Tx Data Rate = " << BOLDYELLOW << "10 Gbit/s" << RESET << GREEN << "; TxEncoding = " << BOLDYELLOW << "FEC12" << RESET << GREEN
                  << "; LpGBT Mode = " << BOLDYELLOW << "Transceiver" << RESET;
        break;
    }

    return cChipMode;
}

uint8_t lpGBTInterface::GetPUSMStatus(Chip* pChip) { return ReadChipReg(pChip, "PUSMStatus"); }

uint8_t lpGBTInterface::GetRxPhase(Chip* pChip, uint8_t pGroup, uint8_t pChannel)
{
    std::string cRxPhaseReg;
    if(pChannel == 0 || pChannel == 1)
        cRxPhaseReg = "EPRX" + std::to_string(pGroup) + "CurrentPhase10";
    else if(pChannel == 3 || pChannel == 2)
        cRxPhaseReg = "EPRX" + std::to_string(pGroup) + "CurrentPhase32";

    uint8_t cRxPhaseRegValue = ReadChipReg(pChip, cRxPhaseReg);
    return ((cRxPhaseRegValue & (0x0F << 4 * (pChannel % 2))) >> 4 * (pChannel % 2));
}

bool lpGBTInterface::IsRxLocked(Chip* pChip, uint8_t pGroup)
{
    std::string cRXLockedReg = "EPRX" + std::to_string(pGroup) + "Locked";
    uint8_t     cChannelMask = 0;

    for(const auto& RxProperty: static_cast<lpGBT*>(pChip)->getRxProperties())
        if(RxProperty.Group == pGroup) cChannelMask |= (1 << RxProperty.Channel);

    return (((ReadChipReg(pChip, cRXLockedReg) & (cChannelMask << 4)) >> 4) == cChannelMask);
}

uint8_t lpGBTInterface::GetRxDllStatus(Chip* pChip, uint8_t pGroup)
{
    std::string cRXDllStatReg = "EPRX" + std::to_string(pGroup) + "DllStatus";
    return ReadChipReg(pChip, cRXDllStatReg);
}

uint8_t lpGBTInterface::GetSFPchannel(const OpticalGroup* pOpticalGroup)
{
    __attribute__((unused)) const uint8_t L8{8};   // @CONST@
    __attribute__((unused)) const uint8_t L12{12}; // @CONST@

    // @TMP@
    // const std::string FMCtype{"OPTO_QUAD"}; // @CONST@
    const std::string FMCtype{"OPTO_OCTA"}; // @CONST@

    uint8_t channel = pOpticalGroup->getId() - (pOpticalGroup->getFMCId() == L8 ? 0 : (FMCtype == "OPTO_QUAD" ? 4 : 8));
    return 3 - channel % 4 + 4 * (channel / 4);
}

// ########################
// # LpGBT GPIO functions #
// ########################

void lpGBTInterface::ConfigureGPIODirection(Chip* pChip, const std::vector<uint8_t>& pGPIOs, uint8_t pDir)
{
    uint8_t cDirH = ReadChipReg(pChip, "PIODirH");
    uint8_t cDirL = ReadChipReg(pChip, "PIODirL");

    for(auto cGPIO: pGPIOs)
    {
        if(cGPIO < 8)
            cDirL = (cDirL & ~(1 << cGPIO)) | (pDir << cGPIO);
        else
            cDirH = (cDirH & ~(1 << (cGPIO - 8))) | (pDir << (cGPIO - 8));
    }

    WriteChipReg(pChip, "PIODirH", cDirH);
    WriteChipReg(pChip, "PIODirL", cDirL);
}

void lpGBTInterface::ConfigureGPIOLevel(Chip* pChip, const std::vector<uint8_t>& pGPIOs, uint8_t pOut)
{
    uint8_t cOutH = ReadChipReg(pChip, "PIOOutH");
    uint8_t cOutL = ReadChipReg(pChip, "PIOOutL");

    for(auto cGPIO: pGPIOs)
    {
        if(cGPIO < 8)
            cOutL = (cOutL & ~(1 << cGPIO)) | (pOut << cGPIO);
        else
            cOutH = (cOutH & ~(1 << (cGPIO - 8))) | (pOut << (cGPIO - 8));
    }

    WriteChipReg(pChip, "PIOOutH", cOutH);
    WriteChipReg(pChip, "PIOOutL", cOutL);
}

void lpGBTInterface::ConfigureGPIODriverStrength(Chip* pChip, const std::vector<uint8_t>& pGPIOs, uint8_t pDriveStr)
{
    uint8_t cDriveStrH = ReadChipReg(pChip, "PIODriveStrengthH");
    uint8_t cDriveStrL = ReadChipReg(pChip, "PIODriveStrengthL");

    for(auto cGPIO: pGPIOs)
    {
        if(cGPIO < 8)
            cDriveStrL = (cDriveStrL & ~(1 << cGPIO)) | (pDriveStr << cGPIO);
        else
            cDriveStrH = (cDriveStrH & ~(1 << (cGPIO - 8))) | (pDriveStr << (cGPIO - 8));
    }

    WriteChipReg(pChip, "PIODriveStrengthH", cDriveStrH);
    WriteChipReg(pChip, "PIODriveStrengthL", cDriveStrL);
}

void lpGBTInterface::ConfigureGPIOPull(Chip* pChip, const std::vector<uint8_t>& pGPIOs, uint8_t pEnable, uint8_t pUpDown)
{
    uint8_t cPullEnH = ReadChipReg(pChip, "PIOPullEnaH"), cPullEnL = ReadChipReg(pChip, "PIOPullEnaL");
    uint8_t cUpDownH = ReadChipReg(pChip, "PIOUpDownH"), cUpDownL = ReadChipReg(pChip, "PIOUpDownL");

    for(auto cGPIO: pGPIOs)
    {
        if(cGPIO < 8)
        {
            cPullEnL = (cPullEnL & ~(1 << cGPIO)) | (pEnable << cGPIO);
            cUpDownL = (cUpDownL & ~(1 << cGPIO)) | (pUpDown << cGPIO);
        }
        else
        {
            cPullEnH = (cPullEnH & ~(1 << (cGPIO - 8))) | (pEnable << (cGPIO - 8));
            cUpDownH = (cUpDownH & ~(1 << (cGPIO - 8))) | (pUpDown << (cGPIO - 8));
        }
    }

    WriteChipReg(pChip, "PIOPullEnaH", cPullEnH);
    WriteChipReg(pChip, "PIOPullEnaL", cPullEnL);
    WriteChipReg(pChip, "PIOUpDownH", cUpDownH);
    WriteChipReg(pChip, "PIOUpDownL", cUpDownL);
}

bool lpGBTInterface::ReadGPIO(Chip* pChip, uint8_t pGPIO)
{
    LOG(INFO) << GREEN << "Reading GPIO value from " << BOLDYELLOW << std::to_string(pGPIO) << RESET;
    uint8_t cPIOInH = ReadChipReg(pChip, "PIOInH");
    uint8_t cPIOInL = ReadChipReg(pChip, "PIOInL");
    return ((cPIOInH << 8 | cPIOInL) >> pGPIO) & 1;
}

// ###########################
// # LpGBT ADC-DAC functions #
// ###########################

void lpGBTInterface::ConfigureADC(Chip* pChip, uint8_t pGainSelect, bool pADCEnable, bool pStartConversion) { WriteChipReg(pChip, "ADCConfig", pStartConversion << 7 | pADCEnable << 2 | pGainSelect); }

void lpGBTInterface::ConfigureCurrentDAC(Chip* pChip, const std::vector<std::string>& pCurrentDACChannels, uint8_t pCurrentDACOutput)
{
    // ########################################################
    // # Enables current DAC without changing the voltage DAC #
    // ########################################################
    uint8_t cDACConfigH = ReadChipReg(pChip, "DACConfigH");
    WriteChipReg(pChip, "DACConfigH", cDACConfigH | 0x40);

    // ############################################################################
    // # Sets output current for the current DAC. Current = CURDACSelect * 3.5 uA #
    // ############################################################################
    WriteChipReg(pChip, "CURDACValue", pCurrentDACOutput);

    // ###############################################################################################################################
    // # Setting Nth bit in this register attaches current DAC to ADCN pin. Current source can be attached to any number of channels #
    // ###############################################################################################################################
    uint8_t cCURDACCHN = 0;
    uint8_t cADCInput;

    for(auto cCurrentDACChannel: pCurrentDACChannels)
    {
        cADCInput = lpGBTInterface::fADCInputMap[cCurrentDACChannel];
        cCURDACCHN += 1 << cADCInput;
        WriteChipReg(pChip, "CURDACCHN", cCURDACCHN);
    }
}

void lpGBTInterface::ConfigureInternalMonitoring(Chip* pChip, uint8_t pEnable)
{
    WriteChipReg(pChip, "ADCMon", (pEnable == 1) ? 0x1F : 0x00);
    std::this_thread::sleep_for(std::chrono::microseconds(lpGBTconstants::DEEPSLEEP));
}

float lpGBTInterface::GetInternalTemperature(Chip* pChip)
{
    auto cVal = ReadChipReg(pChip, "ADCMon");

    // ######################################
    // # Enable reset on temperature sensor #
    // ######################################
    WriteChipReg(pChip, "ADCMon", (1 << 4 | cVal));
    std::this_thread::sleep_for(std::chrono::microseconds(lpGBTconstants::DEEPSLEEP));

    // #######################################
    // # Disable reset on temperature sensor #
    // #######################################
    WriteChipReg(pChip, "ADCMon", (0 << 4 | cVal));

    std::vector<float> cMeasurements(0);
    for(uint8_t cIndx = 0; cIndx < 10; cIndx++) cMeasurements.push_back(lpGBTInterface::ReadADC(pChip, "TEMP", "VREF/2", 0));
    return std::accumulate(cMeasurements.begin(), cMeasurements.end(), 0.) / cMeasurements.size();
}

float lpGBTInterface::ReadResistance(Chip* pChip, const std::string& pADC, const std::vector<uint8_t>& pCurrents, uint8_t pGain)
{
    std::vector<float> cTempVoltageReadings;
    std::vector<float> cTempCurrentValues;

    for(auto cCurrentDAC: pCurrents)
    {
        lpGBTInterface::ConfigureCurrentDAC(pChip, {pADC}, cCurrentDAC);
        float              cCurrent = (0.9e-3) * cCurrentDAC / 256;
        std::vector<float> cMeasurements(0);

        for(uint8_t cIndx = 0; cIndx < 10; cIndx++)
        {
            auto cMeasurement = lpGBTInterface::ReadADC(pChip, pADC, "VREF/2", pGain);
            if(cMeasurement != 1023)
            {
                // LOG(DEBUG) << BOLDBLUE << "Current DAC " << cCurrentDAC << " \t... " << cMeasurement << RESET;
                cMeasurements.push_back(cMeasurement);
            }
        }

        if(cMeasurements.size() > 0)
        {
            float cMean = std::accumulate(cMeasurements.begin(), cMeasurements.end(), 0.) / cMeasurements.size();
            cTempCurrentValues.push_back(cCurrent);
            cTempVoltageReadings.push_back(cMean);
            // LOG(DEBUG) << "Current of " << cCurrent << " mean voltage reading is " << cMean << " ADC units" << RESET;
        }
        // else
        //     LOG(DEBUG) << BOLDBLUE << "\t\t Current DAC " << +cCurrentDAC << " no valid ADC readings" << RESET;
    }

    lpGBTInterface::ConfigureCurrentDAC(pChip, {pADC}, 0x00);
    float cLSQResistance = (cTempVoltageReadings.size() != 0) ? getLeastSquareSlope<float>(cTempCurrentValues, cTempVoltageReadings) : -1;
    // LOG(DEBUG) << BOLDBLUE << "Resistance \t... " << cLSQResistance << RESET;

    return cLSQResistance;
}

uint16_t lpGBTInterface::GetADCOffset(Chip* pChip, bool pVerbose)
{
    uint16_t cMeasurement = lpGBTInterface::ReadADC(pChip, "VREF/2", "VREF/2");
    if(pVerbose) LOG(INFO) << BOLDBLUE << "Reading ADC Offset " << BOLDYELLOW << +cMeasurement << RESET;
    return cMeasurement;
}

float lpGBTInterface::GetADCVoltage(Chip* pChip, const std::string& pADCInputP, uint16_t cOffset, float cGain, bool pVerbose)
// #############################################################################################
// # Implements the ADC master formula for the a basic measurement  assuming a calibrated Vref #
// #############################################################################################
{
    uint16_t cMeasurement = lpGBTInterface::ReadADC(pChip, pADCInputP, "VREF/2");
    return (cMeasurement - cOffset * (1. - cGain / 2.)) / cGain / 512.;
}

float lpGBTInterface::GetADCVoltage(Chip* pChip, const std::string& pADCInputP, bool pVerbose)
{
    uint16_t cOffset = lpGBTInterface::GetADCOffset(pChip, pVerbose);
    float    cGain   = lpGBTInterface::GetADCGain(pChip, pVerbose);
    return GetADCVoltage(pChip, pADCInputP, cOffset, cGain, pVerbose);
}

float lpGBTInterface::GetRssiPower(Chip* pChip, const std::string& pADCInputP, float cResponsivity, bool pVerbose)
{
    uint16_t cOffset = lpGBTInterface::GetADCOffset(pChip, pVerbose);
    float    cGain   = lpGBTInterface::GetADCGain(pChip, pVerbose);
    return lpGBTInterface::GetRssiPower(pChip, pADCInputP, cResponsivity, cOffset, cGain, pVerbose);
}

// Calculation vaild for 2S SEH v3.2 prototypes in W
// Resistor values also valid for PS ROH v2
// R1=1k; Voltage divider 680k and 1000k
// Typical responsivity 0.45-0.55 A/W
// For SEHv5 its 47k and 100k
float lpGBTInterface::GetRssiPower(Chip* pChip, const std::string& pADCInputP, float cResponsivity, uint16_t cOffset, float cGain, bool pVerbose)
{
    float cAdcMeasurement = lpGBTInterface::GetADCVoltage(pChip, pADCInputP, cOffset, cGain, pVerbose);
    float cCurrent        = 1. / 1000. * (2.5 - cAdcMeasurement * 1680. / 680.);
    float cResult         = cCurrent / cResponsivity;
    if(pVerbose) LOG(INFO) << BOLDBLUE << "Measured RSSI Power " << BOLDYELLOW << +cResult << BOLDBLUE << " W" << RESET;
    return cResult;
}

float lpGBTInterface::GetADCGain(Chip* pChip, bool pVerbose)
{
    float cResult;
    WriteChipReg(pChip, "ADCMon", 0);
    // #######################################################
    // # Disable resistive divider, so "VDD" is actually GND #
    // #######################################################
    std::this_thread::sleep_for(std::chrono::microseconds(lpGBTconstants::DEEPSLEEP));
    uint16_t cMeasurement = lpGBTInterface::ReadADC(pChip, "VDD", "VREF/2");
    cResult               = ((cMeasurement * 1.) - (GetADCOffset(pChip, pVerbose) * 1.)) / 512. * 2. * -1.;

    if(pVerbose)
    {
        LOG(INFO) << BOLDBLUE << "Reading ADC value " << BOLDYELLOW << +cMeasurement << RESET;
        LOG(INFO) << BOLDBLUE << "Reading ADC Gain via GND-Vref/2 " << BOLDYELLOW << +cResult << RESET;
    }

    cMeasurement = lpGBTInterface::ReadADC(pChip, "VREF/2", "VDD");
    cResult      = ((cMeasurement * 1.) - (GetADCOffset(pChip, pVerbose) * 1.)) / 512. * 2.;

    if(pVerbose)
    {
        LOG(INFO) << BOLDBLUE << "Reading ADC value " << BOLDYELLOW << +cMeasurement << RESET;
        LOG(INFO) << BOLDBLUE << "Reading ADC Gain via Vref/2-GND " << BOLDYELLOW << +cResult << RESET;
    }

    return cResult;
}

uint16_t lpGBTInterface::ReadADC(Chip* pChip, const std::string& pADCInputP, const std::string& pADCInputN, uint8_t pGain, bool silentRunning)
{
    // ########################################################
    // # Read differential (converted) data on two ADC inputs #
    // ########################################################
    uint8_t cADCInputP = lpGBTInterface::fADCInputMap[pADCInputP];
    uint8_t cADCInputN = lpGBTInterface::fADCInputMap[pADCInputN];

    // if(silentRunning == false) LOG(DEBUG) << GREEN << "Reading ADC value from " << BOLDYELLOW << pADCInputP << RESET;

    // ####################
    // # Select ADC Input #
    // ####################
    WriteChipReg(pChip, "ADCSelect", cADCInputP << 4 | cADCInputN << 0);

    // ################################################
    // # Enable ADC Input without starting conversion #
    // ################################################
    lpGBTInterface::ConfigureADC(pChip, pGain, true, false);

    // ########################
    // # Enable Internal VREF #
    // ########################
    uint8_t cVrefcntrContent = ReadChipReg(pChip, "VREFCNTR");
    WriteChipReg(pChip, "VREFCNTR", 1 << 7 | (0x3f & cVrefcntrContent));
    std::this_thread::sleep_for(std::chrono::microseconds(lpGBTconstants::DEEPSLEEP));

    // ########################
    // # Start ADC conversion #
    // ########################
    lpGBTInterface::ConfigureADC(pChip, pGain, true, true);

    // ###########################
    // # Check conversion status #
    // ###########################
    uint8_t cIter    = 0;
    bool    cSuccess = false;
    do {
        // if(silentRunning == false) LOG(DEBUG) << GREEN << "Waiting for ADC conversion to end" << RESET;
        usleep(10000);
        cSuccess = lpGBTInterface::IsReadADCDone(pChip);
        cIter++;
    } while((cIter < lpGBTconstants::MAXATTEMPTS) && (cSuccess == false));

    if(!cSuccess)
    {
        LOG(ERROR) << BOLDRED << "[lpGBTInterface::ReadADC] Timed out error" << RESET;
        return 0xFFFF;
    }

    if(cIter == lpGBTconstants::MAXATTEMPTS)
    {
        if(silentRunning == false)
            LOG(WARNING) << BOLDRED << "LpGBT ADC conversion timed out on Board ID " << BOLDYELLOW << +pChip->getBeBoardId() << BOLDRED << " OpticalGroup ID " << BOLDYELLOW
                         << +pChip->getOpticalGroupId() << RESET;
        // ExceptionHandler::getInstance()->disableOpticalGroup(pChip->getBeBoardId(), pChip->getOpticalGroupId());
        throw std::runtime_error("LpGBT ADC conversion timed out");
    }

    // ##################
    // # Read ADC value #
    // ##################
    uint8_t cADCvalue1 = ReadChipReg(pChip, "ADCStatusH") & 0x3;
    uint8_t cADCvalue2 = ReadChipReg(pChip, "ADCStatusL");

    // ############################################
    // # Clear ADC conversion bit and disable ADC #
    // ############################################
    lpGBTInterface::ConfigureADC(pChip, pGain, false, false);

    // #########################
    // # Disable Internal VREF #
    // #########################
    WriteChipReg(pChip, "VREFCNTR", 0 << 7 | (0x3f & cVrefcntrContent));

    return (cADCvalue1 << 8 | cADCvalue2);
}

bool lpGBTInterface::IsReadADCDone(Chip* pChip) { return (((ReadChipReg(pChip, "ADCStatusH") & 0x40) >> 6) == 1); }

// #############################
// # LpGBT Bit Error Rate test #
// #############################

void lpGBTInterface::ConfigureBERT(Chip* pChip, uint8_t pCoarseSource, uint8_t pFineSource, uint8_t pMeasTime, bool pSkipDisable)
{
    WriteChipReg(pChip, "BERTSource", (pCoarseSource << 4) | pFineSource);
    WriteChipReg(pChip, "BERTConfig", (pMeasTime << 4) | (pSkipDisable << 1));
}

void lpGBTInterface::StartBERT(Chip* pChip, bool pStartBERT)
{
    uint8_t cRegisterValue = ReadChipReg(pChip, "BERTConfig");
    WriteChipReg(pChip, "BERTConfig", (cRegisterValue & ~(0x1 << 0)) | (pStartBERT << 0));
}

void lpGBTInterface::ConfigureBERTPattern(Chip* pChip, uint32_t pPattern)
{
    LOG(INFO) << GREEN << "Setting BERT pattern to " << BOLDYELLOW << std::bitset<32>(pPattern) << RESET;

    WriteChipReg(pChip, "BERTDataPattern0", (pPattern & (0xFF << 0)) >> 0);
    WriteChipReg(pChip, "BERTDataPattern1", (pPattern & (0xFF << 8)) >> 8);
    WriteChipReg(pChip, "BERTDataPattern2", (pPattern & (0xFF << 16)) >> 16);
    WriteChipReg(pChip, "BERTDataPattern3", (pPattern & (0xFF << 24)) >> 24);
}

uint8_t lpGBTInterface::GetBERTStatus(Chip* pChip) { return ReadChipReg(pChip, "BERTStatus"); }

bool lpGBTInterface::IsBERTDone(Chip* pChip) { return (lpGBTInterface::GetBERTStatus(pChip) & 0x1) == 1; }

bool lpGBTInterface::IsBERTEmptyData(Chip* pChip) { return ((lpGBTInterface::GetBERTStatus(pChip) & (0x1 << 2)) >> 2) == 1; }

uint64_t lpGBTInterface::GetBERTErrors(Chip* pChip)
{
    uint64_t cResult0 = ReadChipReg(pChip, "BERTResult0");
    uint64_t cResult1 = ReadChipReg(pChip, "BERTResult1");
    uint64_t cResult2 = ReadChipReg(pChip, "BERTResult2");
    uint64_t cResult3 = ReadChipReg(pChip, "BERTResult3");
    uint64_t cResult4 = ReadChipReg(pChip, "BERTResult4");

    return ((cResult4 << 32) | (cResult3 << 24) | (cResult2 << 16) | (cResult1 << 8) | cResult0);
}

double lpGBTInterface::GetBERTResult(Chip* pChip)
{
    lpGBTInterface::StartBERT(pChip, false); // Stop
    lpGBTInterface::StartBERT(pChip, true);  // Start
    std::this_thread::sleep_for(std::chrono::microseconds(lpGBTconstants::DEEPSLEEP));

    // ########################
    // # Wait for BERT to end #
    // ########################
    while(lpGBTInterface::IsBERTDone(pChip) == false)
    {
        LOG(INFO) << BOLDBLUE << "\t--> BERT still running ... " << RESET;
        std::this_thread::sleep_for(std::chrono::microseconds(lpGBTconstants::DEEPSLEEP));
    }

    // #############################
    // # Throw error if empty data #
    // #############################
    if(lpGBTInterface::IsBERTEmptyData(pChip) == true)
    {
        lpGBTInterface::StartBERT(pChip, false); // Stop
        LOG(WARNING) << BOLDRED << "LpGBT BERT: All zeros at input on Board ID " << BOLDYELLOW << +pChip->getBeBoardId() << BOLDRED << " OpticalGroup ID " << BOLDYELLOW << +pChip->getOpticalGroupId()
                     << RESET;
        LOG(WARNING) << BOLDBLUE << "\t--> OpticalGroup will be disabled" << RESET;
        ExceptionHandler::getInstance()->disableOpticalGroup(pChip->getBeBoardId(), pChip->getOpticalGroupId());
        return 0.;
    }

    // ##################################
    // # Compute number of bits checked #
    // ##################################
    uint64_t cErrors           = lpGBTInterface::GetBERTErrors(pChip);
    uint8_t  cMeasTime         = (ReadChipReg(pChip, "BERTConfig") & (0xF << 4)) >> 4;
    uint64_t cNClkCycles       = std::pow(2, 5 + cMeasTime * 2);
    uint8_t  cNBitsPerClkCycle = (lpGBTInterface::GetChipRate(pChip) == 5) ? 8 : 16; // 5G(320MHz) == 8 bits/clk, 10G(640MHz) == 16 bits/clk
    double   cBitsChecked      = cNClkCycles * cNBitsPerClkCycle;

    lpGBTInterface::StartBERT(pChip, false); // Stop
    LOG(INFO) << BOLDBLUE << "\t--> Bits checked  : " << BOLDYELLOW << +cBitsChecked << RESET;
    LOG(INFO) << BOLDBLUE << "\t--> Bits in error : " << BOLDYELLOW << +cErrors << RESET;

    // #############################
    // # Return fraction of errors #
    // #############################
    return cErrors / cBitsChecked;
}

double lpGBTInterface::RunBERtest(Chip* pChip, const std::vector<uint8_t>& pGroups, uint8_t pChannel, bool given_time, double frames_or_time, uint8_t frontendSpeed)
// ####################
// # frontendSpeed    #
// # 1.28 Gbit/s  = 0 #
// # 640 Mbit/s   = 1 #
// # 320 Mbit/s   = 2 #
// ####################
{
    const double bitPerFrame = 32. * std::pow(2, frontendSpeed); // Bits per frame
    const double fps         = 1.28e9 / bitPerFrame;             // Frames per second: 32-bit frame @ 1.28 Gbit/s, 64-bit frame @ 640 Mbit/s, 128-bit frame @ 320 Mbit/s
    const int    nPrints     = 10;                               // Only an indication, the real number of printouts will be driven by the length of the time steps @CONST@
    double       frames2run;
    double       time2run;

    if(given_time == true)
        time2run = frames_or_time;
    else
        time2run = frames_or_time / fps;
    size_t BERTMeasTime = (log2(time2run * lpGBTconstants::ACCELERATOR_CLK) - 5) / 2.;
    frames2run          = fBERTMeasTimeMap[BERTMeasTime];

    // ##########################################################################
    // # Configure number of printouts and calculate the frequency of printouts #
    // ##########################################################################
    double time_per_step = std::min(std::max(time2run / nPrints, 1.), 3600.); // The runtime of the PRBS test will have a precision of one step (at most 1h and at least 1s)

    // ###############
    // # Configuring #
    // ###############
    for(auto pGroup: pGroups)
    {
        lpGBTInterface::ConfigureRxSource(pChip, pGroup, lpGBTconstants::PATTERN_NORMAL);
        lpGBTInterface::ConfigureBERT(pChip, fGroup2BERTsourceCourse[pGroup], fChannelSpeed2BERTsourceFine[pChannel + 4 * (2 - frontendSpeed)], BERTMeasTime);
    }

    // #########
    // # Start #
    // #########
    lpGBTInterface::StartBERT(pChip, false); // Stop
    lpGBTInterface::StartBERT(pChip, true);  // Start
    std::this_thread::sleep_for(std::chrono::microseconds(lpGBTconstants::DEEPSLEEP));

    LOG(INFO) << BOLDGREEN << std::fixed << std::setprecision(0) << "===== BER run starting @ " << BOLDYELLOW << bitPerFrame << BOLDGREEN << "-bits/frame  =====" << RESET;
    int idx = 1;
    while(lpGBTInterface::IsBERTDone(pChip) == false)
    {
        std::this_thread::sleep_for(std::chrono::seconds(static_cast<unsigned int>(time_per_step)));
        LOG(INFO) << GREEN << "I've been running for " << BOLDYELLOW << time_per_step * idx << RESET << GREEN << "s" << RESET;
        idx++;
    }
    LOG(INFO) << BOLDGREEN << "========= Finished =========" << RESET;

    if(lpGBTInterface::IsBERTEmptyData(pChip) == true)
    {
        lpGBTInterface::StartBERT(pChip, false); // Stop
        LOG(WARNING) << BOLDRED << "All zeros at input on Board ID " << BOLDYELLOW << +pChip->getBeBoardId() << BOLDRED << " OpticalGroup ID " << BOLDYELLOW << +pChip->getOpticalGroupId() << RESET;
        LOG(WARNING) << BOLDBLUE << "\t--> OpticalGroup will be disabled" << RESET;
        ExceptionHandler::getInstance()->disableOpticalGroup(pChip->getBeBoardId(), pChip->getOpticalGroupId());
    }

    // ########
    // # Stop #
    // ########
    uint64_t nErrors = lpGBTInterface::GetBERTErrors(pChip);
    lpGBTInterface::StartBERT(pChip, false); // Stop

    // ###########################
    // # Read PRBS frame counter #
    // ###########################
    LOG(INFO) << BOLDGREEN << "===== BER test summary =====" << RESET;
    LOG(INFO) << GREEN << "Number of PRBS frames sent: " << BOLDYELLOW << frames2run << RESET;
    LOG(INFO) << GREEN << "Frames with error(s): " << BOLDYELLOW << nErrors / bitPerFrame << RESET << GREEN << ", i.e. bits with errors: " << BOLDYELLOW << nErrors << RESET;
    LOG(INFO) << GREEN << "Frame Error Rate: " << BOLDYELLOW << nErrors / frames2run << RESET << GREEN << " bits/clk (" << BOLDYELLOW << nErrors / bitPerFrame / frames2run * 100 << RESET << GREEN
              << "%)" << RESET;
    LOG(INFO) << GREEN << "BER test result: " << (nErrors == 0 ? BOLDYELLOW : BOLDRED) << (nErrors == 0 ? "PASSED" : "NOT PASSED") << RESET;
    LOG(INFO) << BOLDGREEN << "====== End of summary ======" << RESET;

    return nErrors / frames2run;
}

// ####################################
// # LpGBT eye opening monitor tester #
// ####################################

void lpGBTInterface::ConfigureEOM(Chip* pChip, uint8_t pEndOfCountSelect, bool pByPassPhaseInterpolator, bool pEnableEOM)
{
    WriteChipReg(pChip, "EOMConfigH", pEndOfCountSelect << 4 | pByPassPhaseInterpolator << 2 | pEnableEOM << 0);
    std::this_thread::sleep_for(std::chrono::microseconds(lpGBTconstants::DEEPSLEEP));
}

void lpGBTInterface::StartEOM(Chip* pChip, bool pStartEOM)
{
    uint8_t cRegisterValue = ReadChipReg(pChip, "EOMConfigH");
    WriteChipReg(pChip, "EOMConfigH", (cRegisterValue & ~(0x1 << 1)) | (pStartEOM << 1));
}

void lpGBTInterface::SelectEOMPhase(Chip* pChip, uint8_t pPhase) { WriteChipReg(pChip, "EOMConfigL", pPhase); }

void lpGBTInterface::SelectEOMVof(Chip* pChip, uint8_t pVof) { WriteChipReg(pChip, "EOMvofSel", pVof); }

uint8_t lpGBTInterface::GetEOMStatus(Chip* pChip)
{
    uint8_t cEOMStatus = ReadChipReg(pChip, "EOMStatus");
    // LOG(DEBUG) << GREEN << "Eye Opening Monitor status : " << BOLDYELLOW << lpGBTInterface::fEOMStatusMap[(cEOMStatus & (0x3 << 2)) >> 2] << RESET;
    return cEOMStatus;
}

uint16_t lpGBTInterface::GetEOMCounter(Chip* pChip) { return (ReadChipReg(pChip, "EOMCounterValueH") << 8 | ReadChipReg(pChip, "EOMCounterValueL") << 0); }

// ##############################################
// # LpGBT I2C Masters functions (Slow Control) #
// ##############################################

void lpGBTInterface::ResetI2C(Chip* pChip, const std::vector<uint8_t>& pMasters)
{
    LOG(INFO) << GREEN << "Reseting I2C Masters" << RESET;
    std::vector<uint8_t> cBitPosition = {2, 1, 0};
    uint8_t              cResetMask   = 0;

    for(const auto& cMaster: pMasters) cResetMask |= (1 << cBitPosition[cMaster]);

    WriteChipReg(pChip, "RST0", 0);
    WriteChipReg(pChip, "RST0", cResetMask);
    WriteChipReg(pChip, "RST0", 0);
}

void lpGBTInterface::ConfigureI2C(Chip* pChip, uint8_t pMaster, uint8_t pFreq, uint8_t pNBytes, uint8_t pSCLDriveMode, bool verify)
{
    // Write configuration data into the I2C Master Data register
    std::string cI2CDataReg = "I2CM" + std::to_string(pMaster) + "Data0";
    uint8_t     cValueData  = (pFreq << 0) | (pNBytes << 2) | (pSCLDriveMode << 7);
    WriteChipReg(pChip, cI2CDataReg, cValueData, verify);

    // Write Command (0x00) to the Command register to tranfer Configuration to the I2C Master Control register
    std::string cI2CCmdReg = "I2CM" + std::to_string(pMaster) + "Cmd";
    WriteChipReg(pChip, cI2CCmdReg, 0x00, verify);
}

uint8_t lpGBTInterface::GetI2CConfiguration(Chip* pChip, uint8_t pMaster)
{
    std::string cI2CCntrReg = "I2CM" + std::to_string(pMaster) + "Ctrl";
    return ReadChipReg(pChip, cI2CCntrReg);
}

bool lpGBTInterface::WriteI2C(Chip* pChip, uint8_t pMaster, uint8_t pSlaveAddress, uint32_t pData, uint8_t pNBytes, uint8_t pFreq, bool verify)
{
    // Write Data to Slave Address using I2C Master
    lpGBTInterface::ConfigureI2C(pChip, pMaster, pFreq, (pNBytes > 1) ? pNBytes : 0, 0, verify);

    // Prepare Address Register
    // Write Slave Address
    std::string cI2CAddressReg = "I2CM" + std::to_string(pMaster) + "Address";
    WriteChipReg(pChip, cI2CAddressReg, pSlaveAddress, verify);

    // Write Data to Data Register
    for(uint8_t cByte = 0; cByte < 4; cByte++)
    {
        std::string cI2CDataReg = "I2CM" + std::to_string(pMaster) + "Data" + std::to_string(cByte);
        if(cByte < pNBytes)
            WriteChipReg(pChip, cI2CDataReg, (pData & (0xFF << 8 * cByte)) >> 8 * cByte, verify);
        else
            WriteChipReg(pChip, cI2CDataReg, 0x00, verify);
    }

    // Prepare Command Register
    std::string cI2CCmdReg = "I2CM" + std::to_string(pMaster) + "Cmd";
    // If Multi-Byte, write command to save data locally before transfer to slave
    // FIXME for now this only provides a maximum of 32 bits (4 Bytes) write
    // Write Command to launch I2C transaction
    if(pNBytes == 1)
        WriteChipReg(pChip, cI2CCmdReg, 0x2, verify);
    else
    {
        WriteChipReg(pChip, cI2CCmdReg, 0x8, verify);
        WriteChipReg(pChip, cI2CCmdReg, 0xC, verify);
    }

    // Wait until the transaction is done
    if(verify)
    {
        uint8_t cIter = 0;
        do {
            // LOG(DEBUG) << GREEN << "Waiting for I2C Write transaction to finisih" << RESET;
            cIter++;
        } while(cIter < lpGBTconstants::MAXATTEMPTS && !IsI2CSuccess(pChip, pMaster));

        if(cIter == lpGBTconstants::MAXATTEMPTS)
        {
            LOG(INFO) << BOLDRED << "I2C Write transaction failed" << RESET;
#if defined(__TCUSB__)
            // In the test system a run time error is undesired
            return false;
#else
            LOG(WARNING) << BOLDBLUE << "\t--> OpticalGroup will be disabled" << RESET;
            ExceptionHandler::getInstance()->disableOpticalGroup(pChip->getBeBoardId(), pChip->getOpticalGroupId());
            return false;
#endif
        }
    }

    return true;
}

uint32_t lpGBTInterface::ReadI2C(Chip* pChip, uint8_t pMaster, uint8_t pSlaveAddress, uint8_t pNBytes, uint8_t pFreq)
{
    // Read Data from Slave Address using I2C Master
    lpGBTInterface::ConfigureI2C(pChip, pMaster, pFreq, pNBytes, 0);
    // Prepare Address Register
    std::string cI2CAddressReg = "I2CM" + std::to_string(pMaster) + "Address";
    // Write Slave Address
    WriteChipReg(pChip, cI2CAddressReg, pSlaveAddress);

    // Prepare Command Register
    std::string cI2CCmdReg = "I2CM" + std::to_string(pMaster) + "Cmd";
    // Write Read Command and then Read from Read Data Register
    // Procedure and registers depend on number on Bytes

    if(pNBytes == 1) { WriteChipReg(pChip, cI2CCmdReg, 0x3); }
    else
        WriteChipReg(pChip, cI2CCmdReg, 0xD);

    // Wait until the transaction is done
    uint8_t cIter = 0;
    do {
        // LOG(DEBUG) << GREEN << "Waiting for I2C Read transaction to finisih" << RESET;
        cIter++;
    } while(cIter < lpGBTconstants::MAXATTEMPTS && !lpGBTInterface::IsI2CSuccess(pChip, pMaster));

    if(cIter == lpGBTconstants::MAXATTEMPTS)
    {
        LOG(INFO) << BOLDRED << "I2C Read Transaction failed" << RESET;
#if defined(__TCUSB__)
        // In the test system a run time error is undesired
        return false;
#else
        LOG(WARNING) << BOLDBLUE << "\t--> OpticalGroup will be disabled" << RESET;
        ExceptionHandler::getInstance()->disableOpticalGroup(pChip->getBeBoardId(), pChip->getOpticalGroupId());
        return false;
#endif
    }

    // Return read back value
    if(pNBytes == 1)
    {
        std::string cI2CDataReg = "I2CM" + std::to_string(pMaster) + "ReadByte";
        return ReadChipReg(pChip, cI2CDataReg);
    }
    else
    {
        uint32_t cReadData = 0;
        for(uint8_t cByte = 0; cByte < pNBytes; cByte++)
        {
            std::string cI2CDataReg = "I2CM" + std::to_string(pMaster) + "Read" + std::to_string(15 - cByte);
            cReadData |= ((uint32_t)ReadChipReg(pChip, cI2CDataReg) << 8 * cByte);
        }
        return cReadData;
    }
}

uint8_t lpGBTInterface::GetI2CStatus(Chip* pChip, uint8_t pMaster)
{
    std::string cI2CStatReg = "I2CM" + std::to_string(pMaster) + "Status";
    uint8_t     cStatus     = ReadChipReg(pChip, cI2CStatReg);
    // LOG(DEBUG) << GREEN << "I2C Master " << +pMaster << " -- Status : " << lpGBTInterface::fI2CStatusMap[cStatus] << RESET;
    return cStatus;
}

std::string lpGBTInterface::GetI2CState(Chip* pChip, uint8_t pStatus) { return fI2CStatusMap[pStatus]; }

bool lpGBTInterface::IsI2CSuccess(Chip* pChip, uint8_t pMaster) { return (lpGBTInterface::GetI2CStatus(pChip, pMaster) == 4); }

void lpGBTInterface::LoadCalibrationData(lpGBT* pChip, uint32_t pChipId, std::string pFileName)
{
    // # Load fCalibration data from a local CSV file based for the specific chipid

    // # Arguments:
    // # pFileName: Path of CSV file containing fCalibration data
    // # pChipId: ChipID for which fCalibration data should be loaded

    // # Raises:
    // # LpgbtCalibrationWarning: If loading fCalibration data failed
    // # FileNotFoundError: If the file does not exis

    LOG(INFO) << GREEN << "Loading calibration data for LpGBT on Board " << BOLDYELLOW << +pChip->getBeBoardId() << RESET << GREEN << " OpticalGroup " << BOLDYELLOW << +pChip->getOpticalGroupId()
              << RESET << GREEN << " with Fuse ID 0x" << BOLDYELLOW << std::hex << +pChipId << std::dec << RESET;

    bool                                  cCalibrationLoaded = false;
    std::ifstream                         file(pFileName.c_str(), std::ios::in);
    std::vector<std::vector<std::string>> content;
    std::vector<std::string>              row;
    std::vector<std::string>              cHeaderRow;
    std::map<std::string, float>          theADCcalibrationMap;
    std::string                           line, word;
    uint32_t                              cRowCounter = 0;

    if(file.is_open())
    {
        while(getline(file, line))
        {
            cRowCounter++;
            if(cRowCounter < 5)
            {
                // Skip header (version check possible)
                if(cRowCounter == 4)
                {
                    // Read field names
                    cHeaderRow.clear();
                    std::stringstream str(line);
                    while(getline(str, word, ',')) cHeaderRow.push_back(word);
                }
                continue;
            }

            row.clear();
            std::stringstream str(line);

            while(getline(str, word, ',')) row.push_back(word);
            uint32_t cRowChipId = strtoul(row[0].c_str(), 0, 16);

            if(cRowChipId == pChipId)
            {
                for(uint32_t j = 1; j < row.size(); j++)
                {
                    LOG(DEBUG) << BOLDBLUE << cHeaderRow[j] << RESET;
                    LOG(DEBUG) << BOLDBLUE << row[j] << RESET;
                    theADCcalibrationMap[cHeaderRow[j]] = std::stof(row[j]);
                }

                pChip->setADCCalibrationData(theADCcalibrationMap);
                cCalibrationLoaded = true;

                break;
            }
        }

        if(cCalibrationLoaded == false)
        {
            LOG(WARNING) << BOLDRED << "\t--> Calibration data not available for LpGBT on Board ID " << BOLDYELLOW << +pChip->getBeBoardId() << BOLDRED << " OpticalGroup ID " << BOLDYELLOW
                         << +pChip->getOpticalGroupId() << BOLDRED << " with Fuse ID 0x" << BOLDYELLOW << std::hex << +pChipId << std::dec << RESET;
            LOG(WARNING) << BOLDBLUE << "\t--> Proceeding without LpGBT ADC calibrations" << RESET;
        }
        else
            LOG(WARNING) << GREEN << "\t--> Calibration data available for LpGBT on Board ID " << BOLDYELLOW << +pChip->getBeBoardId() << RESET << GREEN << " OpticalGroup ID " << BOLDYELLOW
                         << +pChip->getOpticalGroupId() << RESET << GREEN << " with Fuse ID 0x" << BOLDYELLOW << std::hex << +pChipId << std::dec << RESET;
    }
    else
    {
        LOG(WARNING) << BOLDRED << "\t--> " << BOLDYELLOW << pFileName << BOLDRED << " could not be opened. Please check file path" << RESET;
        LOG(WARNING) << BOLDBLUE << "\t--> Proceeding without LpGBT ADC calibrations" << RESET;
        throw std::runtime_error(std::string("FileNotFoundError"));
    }

    pChip->setIsCalibrationDataLoaded(cCalibrationLoaded);
    file.close();
}

float lpGBTInterface::EstimateTemperatureUncalibVref(lpGBT* pChip, bool pResetTempSensor)
{
    // # Estimate temperature using internal temperature sensor and uncalibrated VREF.
    // # WARNING: this routine WILL NOT WORK for irradiated chips (TID>0)
    // # Side effects:
    // #    ADC configuration
    // # Arguments:
    // #    pResetTempSensor: Reset temperature sensor before using it

    // # Return:
    // #    Temperature estimate in degree C

    uint8_t cVrefCode = (uint32_t)std::round(pChip->getADCCalibrationData()["VREF_OFFSET"]);
    // LOG(DEBUG) << GREEN << "Enable VREF at code: 0x" << BOLDYELLOW << std::hex << +cVrefCode << std::dec << RESET << GREEN << " LSB" << RESET;

    EnableInternalVref(pChip, true);
    SetVrefTune(pChip, cVrefCode);
    if(pResetTempSensor)
    {
        auto cVal = ReadChipReg(pChip, "ADCMon");
        // ######################################
        // # Enable reset on temperature sensor #
        // ######################################
        WriteChipReg(pChip, "ADCMon", (1 << 4 | cVal));
        std::this_thread::sleep_for(std::chrono::microseconds(lpGBTconstants::DEEPSLEEP));
        // #######################################
        // # Disable reset on temperature sensor #
        // #######################################
        WriteChipReg(pChip, "ADCMon", (0 << 4 | cVal));
    }

    std::vector<float> cMeasurements(0);
    for(uint8_t cIndx = 0; cIndx < 10; cIndx++)
    {
        uint16_t cAdcVal = lpGBTInterface::ReadADC(pChip, "TEMP", "VREF/2", 0);
        // LOG(DEBUG) << GREEN << "Temperature readout: 0x" << BOLDYELLOW << std::hex << +cAdcVal << std::dec << RESET << GREEN << " LSB" << RESET;

        // Estimate the junction temperature
        cMeasurements.push_back(cAdcVal * pChip->getADCCalibrationData()["TEMPERATURE_UNCALVREF_SLOPE"] + pChip->getADCCalibrationData()["TEMPERATURE_UNCALVREF_OFFSET"]);
    }
    float cTemperature = std::accumulate(cMeasurements.begin(), cMeasurements.end(), 0.) / cMeasurements.size();

    // LOG(DEBUG) << GREEN << "LpGBT temperature estimate: " << BOLDYELLOW << std::setprecision(3) << cTemperature << std::setprecision(-1) << RESET << GREEN << " C" << RESET;
    return cTemperature;
}

void lpGBTInterface::TuneVrefControlLib(lpGBT* pChip, bool pEnable)
{
    /*  Calculate the optimum VREFTUNE code based on the junction temperature
           and apply the setting to the chip. Prior calling this method, the user
           is expected to set the temperature estimate (set_temperature) first if it
           is know or to to use estimate_temperature_uncalib_vref in order to
           estimate it automatically. In the later case, it is advice to use
           auto_tune_vref method instead.

        Arguments:
            pEnable: Enable VREF generator
    */

    uint8_t cCodeOpt = (uint32_t)std::round(pChip->getADCCalibrationData()["VREF_SLOPE"] * pChip->getTemperature() + pChip->getADCCalibrationData()["VREF_OFFSET"]);
    // LOG(DEBUG) << GREEN << "REFTune = 0x" << BOLDYELLOW << std::hex << +cCodeOpt << std::dec << RESET;
    EnableInternalVref(pChip, pEnable);
    SetVrefTune(pChip, cCodeOpt);
}

void lpGBTInterface::AutoTuneVref(lpGBT* pChip, bool pResetTempSensor)
{
    /*  Auto tune VREF based on the internal temperature sensor.

        WARNING: this routine WILL NOT WORK for irradiated chips (TID>0)

        Side effects:
            ADC configuration.
            Junction temperature.

        Arguments:
            pResetTempSensor: Reset temperature sensor before using it.
    */
    float cTemperature = EstimateTemperatureUncalibVref(pChip, pResetTempSensor);

    // Update temperature estimate
    pChip->setTemperature(cTemperature);

    // Tune VREF
    TuneVrefControlLib(pChip);
}

void lpGBTInterface::VdacSetVout(lpGBT* pChip, float pVoltageV, bool pEnable)
{
    /*  """Set requested voltage at the output of the lpGBT voltage DAC.

        Prerequisites:
            VREF should be tuned to 1V

        Arguments:
            pVoltageV: voltage in volts (0-1)
            pEnable: voltage DAC state

        Raises:
            LpGBTOutOfRangeError: If the requested voltage cannot be achieved
    """ */
    if((pVoltageV > 1.) or (pVoltageV < 0.))
    {
        LOG(ERROR) << BOLDRED << "lpGBTInterface::VdacSetVout: Invalid voltage for VDAC" << RESET;
        throw std::runtime_error(std::string("Invalid voltage for VDAC"));
    }
    int32_t cDacCode = (int32_t)std::round((pChip->getADCCalibrationData()["VDAC_SLOPE"] + pChip->getTemperature() * pChip->getADCCalibrationData()["VDAC_SLOPE_TEMP"]) * pVoltageV +
                                           pChip->getADCCalibrationData()["VDAC_OFFSET"] + pChip->getTemperature() * pChip->getADCCalibrationData()["VDAC_OFFSET_TEMP"]);

    if(cDacCode < 0 or cDacCode > 4095)
    {
        LOG(ERROR) << BOLDRED << "VdacSetVout::_CdacCodeToCurrent: VDAC can not deliver requested voltage." << RESET;
        throw std::runtime_error(std::string("VDAC can not deliver requested voltage."));
    }
}

float lpGBTInterface::AdcGetVin(lpGBT* pChip, const std::string& pADCInputP, const std::string& pADCInputN, uint8_t pGain, uint8_t pSamples)
{
    /* Get input voltage.

        Prerequisites:
            VREF should be tuned to 1V.

        Returns:
            Calibrated voltage reading (mean of number of samples).

        Raises:
            LpgbtException: in case the conversion timeout is exceeded
    */

    std::vector<uint16_t> cMeasurements(0);
    for(uint8_t cIndx = 0; cIndx < pSamples; cIndx++) cMeasurements.push_back(lpGBTInterface::ReadADC(pChip, pADCInputP, pADCInputN, pGain));
    uint16_t cResult = (uint16_t)std::round(std::accumulate(cMeasurements.begin(), cMeasurements.end(), 0.) / cMeasurements.size());

    std::string cAdcStr = "ADC_" + fADCGainMap[pGain];

    float cCalRes = ((pChip->getADCCalibrationData()[cAdcStr + "_SLOPE"] + pChip->getTemperature() * pChip->getADCCalibrationData()[cAdcStr + "_SLOPE_TEMP"]) * cResult +
                     pChip->getADCCalibrationData()[cAdcStr + "_OFFSET"] + pChip->getTemperature() * pChip->getADCCalibrationData()[cAdcStr + "_OFFSET_TEMP"]);

    // LOG(DEBUG) << GREEN << "Measured calibrated Vin for " << BOLDYELLOW << pADCInputP << RESET << GREEN << " and " << BOLDYELLOW << pADCInputN << RESET << GREEN << " is " << BOLDYELLOW << cCalRes
    //            << RESET << GREEN << " V" << RESET;
    return cCalRes;
}

float lpGBTInterface::_CdacCodeToCurrent(lpGBT* pChip, const std::string& pChannel, uint8_t pCode)
{
    /* """Return estimate of the CDAC current for specific code.

        Arguments:
            pChannel: CDAC channel
            pCode: CDAC code

        Returns:
            Estimate of the output current in Amps
    """ */
    uint8_t cChannel = fADCInputMap[pChannel];
    if(cChannel > 8)
    {
        LOG(ERROR) << BOLDRED << "lpGBTInterface::_CdacCodeToCurrent: Invalid CDAC channel" << RESET;
        throw std::runtime_error(std::string("Invalid CDAC channel"));
    }
    return (pCode - pChip->getADCCalibrationData()["CDAC" + std::to_string(cChannel) + "_OFFSET"] -
            pChip->getTemperature() * pChip->getADCCalibrationData()["CDAC" + std::to_string(cChannel) + "_OFFSET_TEMP"]) /
           (pChip->getADCCalibrationData()["CDAC" + std::to_string(cChannel) + "_SLOPE"] + pChip->getTemperature() * pChip->getADCCalibrationData()["CDAC" + std::to_string(cChannel) + "_SLOPE_TEMP"]);
}

float lpGBTInterface::_CdacCodeToRout(lpGBT* pChip, const std::string& pChannel, uint8_t pCode)
{
    /* """Return estimate of the CDAC output resistance for specific code

        Arguments:
            pChannel: CDAC channel
            pCode: CDAC code

        Returns:
            Estimate of the output resistance in Ohms
    """ */

    uint8_t cChannel = fADCInputMap[pChannel];
    if(cChannel > 8)
    {
        LOG(ERROR) << BOLDRED << "lpGBTInterface::_CdacCodeToRout: Invalid CDAC channel" << RESET;
        throw std::runtime_error(std::string("Invalid CDAC channel"));
    }
    // The rout for code zero is very large and not properly modeled.
    // Return the rout estimate for code 1 instead.
    if(pCode == 0) { pCode = 1; }

    float cR0 = (pChip->getADCCalibrationData()["CDAC" + std::to_string(cChannel) + "_R0"] + pChip->getTemperature() * pChip->getADCCalibrationData()["CDAC" + std::to_string(cChannel) + "_R0_TEMP"]);
    return cR0 / pCode;
}

uint8_t lpGBTInterface::_CdacGetOptimumCodeForCurrent(lpGBT* pChip, const std::string& pChannel, float pCurrentA)
{
    /* """Return optimum CDAC code for the requested current (in amps)

        Arguments:
            pChannel: CDAC channel
            pCurrentA: Output current in Amps

        Returns:
            The optimum CDAC code

        Raises:
            LpGBTOutOfRangeError: If the requested current cannot be achieved
    """ */

    uint8_t cChannel = fADCInputMap[pChannel];
    if(cChannel > 8)
    {
        LOG(ERROR) << BOLDRED << "lpGBTInterface::_CdacGetOptimumCodeForCurrent: Invalid CDAC channel" << RESET;
        throw std::runtime_error(std::string("Invalid CDAC channel"));
    }
    // if((pCurrentA > 2e-3) or (pCurrentA <= 0))
    // {
    //     LOG(ERROR) << BOLDRED << "lpGBTInterface::_CdacGetOptimumCodeForCurrent: Invalid CDAC current = " << pCurrentA << RESET;
    //     throw std::runtime_error(std::string("Invalid CDAC current"));
    // }

    uint16_t cCode = (uint16_t)std::round(
        (pChip->getADCCalibrationData()["CDAC" + std::to_string(cChannel) + "_SLOPE"] + pChip->getTemperature() * pChip->getADCCalibrationData()["CDAC" + std::to_string(cChannel) + "_SLOPE_TEMP"]) *
            pCurrentA +
        pChip->getADCCalibrationData()["CDAC" + std::to_string(cChannel) + "_OFFSET"] + pChip->getTemperature() * pChip->getADCCalibrationData()["CDAC" + std::to_string(cChannel) + "_OFFSET_TEMP"]);

    /* code = round(
            (
                self.fCalibration[f"CDAC{chn}_SLOPE"]
                + self.temperature_c * self.fCalibration[f"CDAC{chn}_SLOPE_TEMP"]
            )
            * current_a
            + self.fCalibration[f"CDAC{chn}_OFFSET"]
            + self.temperature_c * self.fCalibration[f"CDAC{chn}_OFFSET_TEMP"]
        ) */

    if(cCode > 255)
    {
        // LOG(ERROR) << BOLDRED << "CDAC can not deliver requested current = " << cCode << RESET;
        // throw std::runtime_error(std::string("CDAC can not deliver requested current."));
        cCode = 255;
    }

    return cCode;
}

void lpGBTInterface::CdacSetCurrent(lpGBT* pChip, const std::string& pChannel, float pCurrentA)
{
    /* """Configure the lpGBT current DAC

        Side effects:
            Disable all other current sources

        Arguments:
            pChannel: ADC channel to connect to current DAC to
            pCurrentA: Output current in Amps
    """ */

    uint8_t cChannel = fADCInputMap[pChannel];
    if(cChannel > 8)
    {
        LOG(ERROR) << BOLDRED << "lpGBTInterface::CdacSetCurrent: Invalid CDAC channel" << RESET;
        throw std::runtime_error(std::string("Invalid CDAC channel"));
    }
    // if((pCurrentA > 1e-3) or (pCurrentA <= 0))
    // {
    //     LOG(ERROR) << BOLDRED << "lpGBTInterface::CdacSetCurrent: Invalid CDAC current" << RESET;
    //     throw std::runtime_error(std::string("Invalid CDAC current"));
    // }
    uint8_t cCode = _CdacGetOptimumCodeForCurrent(pChip, pChannel, pCurrentA);
    ConfigureCurrentDAC(pChip, std::vector<std::string>{pChannel}, {cCode});
}

float lpGBTInterface::MeasureResistance(lpGBT* pChip, const std::string& pChannel, bool pImprovePrecision)
{
    LOG(INFO) << GREEN << "pExpectedROhm not provided. Performing auto ranging" << RESET;

    uint8_t cCdacCode = 1;
    float   cVAdc     = 0;
    while(true)
    {
        ConfigureCurrentDAC(pChip, std::vector<std::string>{pChannel}, {cCdacCode});
        cVAdc = AdcGetVin(pChip, pChannel, "VREF/2", 0);
        if(cVAdc > 0.5 or cCdacCode >= 128) { break; }
        cCdacCode *= 2.;
    }
    float cExpectedROhm = cVAdc / _CdacCodeToCurrent(pChip, pChannel, cCdacCode);
    // LOG(DEBUG) << BOLDBLUE << "First estimate of resistance: " << cExpectedROhm / 1e3 << " kOhm" << RESET;
    return MeasureResistance(pChip, pChannel, cExpectedROhm, pImprovePrecision);
}

float lpGBTInterface::MeasureResistance(lpGBT* pChip, const std::string& pChannel, float pExpectedROhm, bool pImprovePrecision)
{
    /* """Measure resistance connected between the ground (VSS) and a given ADC channel.
       If the pExpectedROhm is provided (see overloaded function), it will be used to set the current source
       (CDAC) in order to obtain optimum voltage drop across the resistors (around 0.5 Vref).
       Alternatively, if pExpectedROhm is not provided, an auto ranging procedure will
       be executed in order to estimate the value of the resistor first.

       When measuring resistive temperature sensors (e.g. PT1000 or NTC) it is recommended
       to set pExpectedROhm in order to prevent auto ranging as it will minimize the error
       introduced by integral non-linearity errors of the current source and should result
       in less "noisy" results.

       In order to improve the measurement precision, one could set pImprovePrecision
       to true. It will result in the method performing several measurements for different
       CDAC codes in order to average out the errors introduced by the CDAC non-linearity.
       The number of measurements depends on the resistance value and it is set automatically
       for maximum precision.

    Prerequisites:
        VREF should be tuned to 1V

    Side effects:
        ADC settings
        CDAC settings

    Arguments:
        pChannel: ADC channel to be used for the measurement
        pExpectedROhm: Expected resistance [Ohms]. If not set, auto ranging will be done
                        automatically
        pImprovePrecision: Improve precision by preforming multiple measurements

    Returns:
        Resistance [Ohms]
    """ */

    float   cCurrentA = 0.5 / pExpectedROhm;
    uint8_t cCdacCode = _CdacGetOptimumCodeForCurrent(pChip, pChannel, cCurrentA);

    // LOG(DEBUG) << BOLDBLUE << "Optimum cdac code: " << +cCdacCode << RESET;

    std::vector<uint8_t> cCdacCodesVec;
    std::vector<float>   cRloadsVec;
    if(pImprovePrecision)
    {
        for(uint8_t i = (uint8_t)cCdacCode * 0.9; i < (uint8_t)cCdacCode * 1.1; i++) { cCdacCodesVec.push_back(i); }
    }
    else { cCdacCodesVec.push_back(cCdacCode); }

    for(auto cdac_code: cCdacCodesVec)
    {
        ConfigureCurrentDAC(pChip, std::vector<std::string>{pChannel}, {cdac_code});

        float iout = _CdacCodeToCurrent(pChip, pChannel, cdac_code);
        float rout = _CdacCodeToRout(pChip, pChannel, cdac_code);

        float vadc = AdcGetVin(pChip, pChannel, "VREF/2", 0, 1);

        float rmeas = vadc / iout;
        // LOG(DEBUG) << BOLDBLUE << "VADC: " << vadc << " V" << RESET;
        if(vadc < 0.25) { LOG(INFO) << BOLDBLUE << "Warning: Initial estimate of the resistance was too high" << RESET; }
        if(vadc > 0.75) { LOG(INFO) << BOLDBLUE << "Warning: Initial estimate of the resistance was too low" << RESET; }
        float rload = rmeas / (1 - rmeas / rout);
        // LOG(DEBUG) << BOLDBLUE << "CODE: " << +cdac_code << " IOUT: " << 1e3 * iout << " [mA] ROUT: " << rout * 1e-3 << " [kOhm] VADC: " << vadc << " [V] LOAD: " << rload << " [Ohm]" << RESET;

        cRloadsVec.push_back(rload);
    }

    return (std::accumulate(cRloadsVec.begin(), cRloadsVec.end(), 0.) / cRloadsVec.size());
}

float lpGBTInterface::MeasureTemperature(lpGBT* pChip, uint8_t pSamples, bool pResetTempSensor)
{
    /* """Measure junction temperature

        Prerequisites:
            VREF should be tuned to 1V

        Side effects:
            ADC settings

        Arguments:
            pSamples: Number of ADC samples to average during the measurement
            pResetTempSensor: Should the temperature sensor be reset prior to its use

        Returns:
            Temperature [C]

        Raises:
            LpGBTException: in case the conversion timeout is exceeded
    """ */

    if(pResetTempSensor)
    {
        auto cVal = ReadChipReg(pChip, "ADCMon");
        // ######################################
        // # Enable reset on temperature sensor #
        // ######################################
        WriteChipReg(pChip, "ADCMon", (1 << 4 | cVal));
        std::this_thread::sleep_for(std::chrono::microseconds(lpGBTconstants::DEEPSLEEP));

        // #######################################
        // # Disable reset on temperature sensor #
        // #######################################
        WriteChipReg(pChip, "ADCMon", (0 << 4 | cVal));
    }

    float cAdcVal = AdcGetVin(pChip, "TEMP", "VREF/2", 0, pSamples);
    float cTemp   = (cAdcVal * pChip->getADCCalibrationData()["TEMPERATURE_SLOPE"] + pChip->getADCCalibrationData()["TEMPERATURE_OFFSET"]);

    return cTemp;
}

float lpGBTInterface::MeasurePowerSupplyVoltage(lpGBT* pChip, const std::string& pPowerSupply, uint8_t pSamples, bool pDisableMonitorAfterMeasurement)
{
    /* """Measure power supply voltage

        Prerequisites:
            VREF should be tuned to 1V

        Side effects:
            ADC settings
            Settings of VDD monitors

        Arguments:
            pPowerSupply: Power supply rail to be measured (VDDTX
                          VDDRX, VDD, VDDA)
            pSamples: how many conversions to perform

        Returns:
            Calibrated reading of a power supply voltage in V

        Raises:
            LpGBTException: in case the conversion timeout is exceeded
    """ */

    if(!(pPowerSupply != "VDDTX" or pPowerSupply != "VDDRX" or pPowerSupply != "VDD" or pPowerSupply != "VDDA"))
    {
        LOG(ERROR) << BOLDRED << "[lpGBTInterface::MeasurePowerSupplyVoltage] Invalid pPowerSupply" << RESET;
        throw std::runtime_error(std::string("Invalid pPowerSupply"));
    }

    // ######################
    // # Enable VDD monitor #
    // ######################
    ConfigureInternalMonitoring(pChip, true);

    // ######################
    // # Perform conversion #
    // ######################
    float cVadc = AdcGetVin(pChip, pPowerSupply, "VREF/2", 0, pSamples);
    float cVsup = cVadc * (pChip->getADCCalibrationData()["VDDMON_SLOPE"] + pChip->getTemperature() * pChip->getADCCalibrationData()["VDDMON_SLOPE_TEMP"]);

    // ######################################
    // # Disable VDD monitor (if requested) #
    // ######################################
    if(pDisableMonitorAfterMeasurement) ConfigureInternalMonitoring(pChip, false);

    return cVsup;
}

float lpGBTInterface::ReadChipMonitor(const OpticalGroup* pOpticalGroup, const std::string& registerName, bool silentRunning)
{
    const int RSensTemp = 1000;  // @CONST@
    const int RVTRxTemp = 10000; // @CONST@
    float     value;

    auto cChip = pOpticalGroup->flpGBT;
    if(registerName.find("TEMP") != std::string::npos)
    {
        value = lpGBTInterface::MeasureTemperature(cChip);
        if(silentRunning == false) LOG(INFO) << BOLDBLUE << "\t--> LpGBT temperature measurement " BOLDYELLOW << std::setprecision(3) << value << BOLDBLUE << " C" << std::setprecision(-1) << RESET;
    }
    else if((registerName.find("VDDTX") != std::string::npos) || (registerName.find("VDDRX") != std::string::npos) || (registerName.find("VDD") != std::string::npos) ||
            (registerName.find("VDDA") != std::string::npos))
    {
        value = lpGBTInterface::MeasurePowerSupplyVoltage(cChip, registerName);
        if(silentRunning == false)
            LOG(INFO) << BOLDBLUE << "\t--> LpGBT voltage measurement from power supply " << BOLDYELLOW << registerName << BOLDBLUE << " is " << BOLDYELLOW << std::setprecision(3) << value << BOLDBLUE
                      << " V" << std::setprecision(-1) << RESET;
    }
    else if(registerName.find("ADC") != std::string::npos)
    {
        std::string sensorType("");

        for(const auto& ele: pOpticalGroup->getNTCMap())
            if(ele.second == registerName) sensorType = ele.first;
        if(sensorType == "")
        {
            value = lpGBTInterface::ReadADC(cChip, registerName, "VREF/2", 0, silentRunning);
            if((silentRunning == false) && (value != 0xFFFF))
                LOG(WARNING) << BOLDBLUE << "\t--> LpGBT register " << BOLDYELLOW << registerName << BOLDBLUE << " has no calibration file. Raw value is " << BOLDYELLOW << value << RESET;
            return value;
        }

        lpGBTInterface::CdacSetCurrent(cChip, registerName, lpGBTInterface::_CdacCodeToCurrent(cChip, registerName, 0xAA));
        float resistance = lpGBTInterface::MeasureResistance(cChip, registerName, sensorType.find("Sensor") != std::string::npos ? RSensTemp : RVTRxTemp, false);

        try
        {
            value = NTChandler::getInstance().getTemperature(sensorType, resistance);
            if(silentRunning == false)
                LOG(INFO) << BOLDBLUE << "\t--> LpGBT temperature measurement from register " << BOLDYELLOW << registerName << BOLDBLUE << " is " << BOLDYELLOW << std::setprecision(3) << value
                          << BOLDBLUE << " C" << std::setprecision(-1) << RESET;
        }
        catch(const std::runtime_error& error)
        {
            value = lpGBTInterface::ReadADC(cChip, registerName, "VREF/2", 0, silentRunning);
            if((silentRunning == false) && (value != 0xFFFF))
                LOG(WARNING) << BOLDBLUE << "\t--> LpGBT register " << BOLDYELLOW << registerName << BOLDBLUE << " has no calibration file. Raw value is " << BOLDYELLOW << value << RESET;
        }
    }
    else
        value = lpGBTInterface::ReadADC(cChip, registerName, "VREF/2", 0, silentRunning);

    return value;
}

float lpGBTInterface::GetLastNTCResistance(lpGBT* pChip, const std::string& theNTCtype)
{
    if(theNTCtype == "Sensor")
        return pChip->getNTCResistance();
    else if(theNTCtype == "VTRx+")
        return pChip->getVtrxNTCResistance();
    throw std::runtime_error("GetLastNTCResistance - No NTC type found");
}
void lpGBTInterface::SetLastNTCResistance(lpGBT* pChip, const std::string& theNTCtype, float resistance)

{
    if(theNTCtype == "Sensor")
        pChip->setNTCResistance(resistance);
    else if(theNTCtype == "VTRx+")
        pChip->setVtrxNTCResistance(resistance);
    else
        throw std::runtime_error("SetLastNTCResistance - No NTC type found");
}

void lpGBTInterface::hardReset(Ph2_HwDescription::Chip* pChip)
{
    LOG(INFO) << BOLDMAGENTA << "Sending hard reset to LpGBT" << RESET;
    WriteChipReg(pChip, "RST2", 0x40, false);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    WriteChipReg(pChip, "RST2", 0x00);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

} // namespace Ph2_HwInterface
