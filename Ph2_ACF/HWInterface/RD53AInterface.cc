/*!
  \file                  RD53AInterface.h
  \brief                 User interface to the RD53A readout chip
  \author                Mauro DINARDO and Alkiviadis PAPADOPOULOS
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
  Support:               email to alkiviadis.papadopoulos@cern.ch
*/

#include "HWInterface/RD53AInterface.h"

using namespace Ph2_HwDescription;

namespace Ph2_HwInterface
{
bool RD53AInterface::ConfigureChip(Chip* pChip, bool pVerify, uint32_t pBlockSize)
{
    this->setBoard(pChip->getBeBoardId());

    auto        pRD53       = static_cast<RD53*>(pChip);
    ChipRegMap& pRD53RegMap = pChip->getRegMap();

    // #######################################
    // # Programming CLK_DATA_DELAY register #
    // #######################################
    const std::set<std::string> registerClkDataDelayList = {"CLK_DATA_DELAY", "CLK_DATA_DELAY_DATA", "CLK_DATA_DELAY_CLK", "CLK_DATA_DELAY_2INV"}; // @CONST@
    bool                        doWriteClkDataDelay      = false;

    for(auto i = 0u; i < registerClkDataDelayList.size(); i++)
    {
        auto cRegItem = pRD53RegMap.find(*std::next(registerClkDataDelayList.begin(), i));
        if((cRegItem != pRD53RegMap.end()) && (cRegItem->second.fPrmptCfg == true))
        {
            doWriteClkDataDelay = true;

            pChip->setReg("CLK_DATA_DELAY", SetSpecialRegister(std::string(cRegItem->first), cRegItem->second.fDefValue, pRD53RegMap).second);

            if(cRegItem->first == "CLK_DATA_DELAY") break;
        }
    }
    if(doWriteClkDataDelay == true) RD53AInterface::WriteClockDataDelay(pChip, pChip->getRegItem("CLK_DATA_DELAY").fValue);

    // ###############################
    // # Programmig global registers #
    // ###############################
    const std::set<std::string> registerPreEmphasisWhiteList = {"CML_CONFIG_SER_EN_TAP", "CML_CONFIG_SER_INV_TAP", "DAC_CML_BIAS_0", "DAC_CML_BIAS_1", "DAC_CML_BIAS_2"}; // @CONST@
    const std::set<std::string> registerBlackList            = {
        "HighGain_LIN", "RESISTORI2V", "NTCBETA", "RNTCAT25C", "ADC_OFFSET_VOLT", "ADC_MAXIMUM_VOLT", "TEMPSENS_IDEAL_FACTOR", "SAMPLE_N_TIMES", "VREF_ADC"}; // @CONST@
    const std::set<std::string> registerWhiteList = {"PA_IN_BIAS_LIN", "FC_BIAS_LIN", "KRUM_CURR_LIN", "LDAC_LIN", "COMP_LIN", "REF_KRUM_LIN", "Vthreshold_LIN"};        // @CONST@

    for(auto& cRegItem: pRD53RegMap)
        if(((cRegItem.second.fPrmptCfg == true) && (registerBlackList.find(cRegItem.first) == registerBlackList.end()) &&
            (registerClkDataDelayList.find(cRegItem.first) == registerClkDataDelayList.end()) && (registerPreEmphasisWhiteList.find(cRegItem.first) == registerPreEmphasisWhiteList.end())) ||
           (registerWhiteList.find(cRegItem.first) != registerWhiteList.end()))
        {
            if(cRegItem.first == "CDR_CONFIG")
            {
                RD53Interface::SendCommand(pRD53, RD53ACmd::ECR{});
                std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::DEEPSLEEP));
            }

            RD53Interface::WriteChipReg(pChip, cRegItem.first, cRegItem.second.fDefValue, pVerify);
        }
        else if((cRegItem.second.fPrmptCfg == true) && (registerBlackList.find(cRegItem.first) != registerBlackList.end()))
            pChip->setReg(cRegItem.first, cRegItem.second.fDefValue);

    // ###################################
    // # Programmig pixel cell registers #
    // ###################################
    RD53AInterface::WriteRD53Mask(pRD53, false, true);

    return true;
}

void RD53AInterface::InitRD53Downlink(const BeBoard* pBoard)
{
    this->setBoard(pBoard->getId());

    static_cast<RD53FWInterface*>(fBoardFW)->WriteChipCommands(std::move(RD53Shared::firstChip->getLaneUpInitSequence()), -1);
    static_cast<RD53FWInterface*>(fBoardFW)->WriteChipCommands(std::move(std::vector<uint16_t>(RD53Constants::NSYNC_WORDS_L, RD53ACmd::RD53ACmdEncoder::SYNC)), -1);
}

void RD53AInterface::InitRD53Uplinks(Chip* pChip)
{
    this->setBoard(pChip->getBeBoardId());
    auto pRD53 = static_cast<RD53*>(pChip);

    // ##############################
    // # 1 Autora active lane       #
    // # OUTPUT_CONFIG = 0b00000100 #
    // # CML_CONFIG    = 0b00000001 #

    // # 2 Autora active lanes      #
    // # OUTPUT_CONFIG = 0b00001100 #
    // # CML_CONFIG    = 0b00000011 #

    // # 4 Autora active lanes      #
    // # OUTPUT_CONFIG = 0b00111100 #
    // # CML_CONFIG    = 0b00001111 #
    // ##############################

    RD53Interface::WriteChipReg(pChip, "OUTPUT_CONFIG", RD53Shared::setBits(pRD53->laneConfig.nOutputLanes) << 2, false); // Number of active lanes [5:2]
    // bits [8:7]: number of 40 MHz clocks +2 for data transfer out of pixel matrix
    // Default 0 means 2 clocks, may need higher value in case of large propagation
    // delays, for example at low VDDD voltage after irradiation
    // bits [5:2]: Aurora lanes. Default 0001 means single lane mode
    RD53Interface::WriteChipReg(pChip, "CML_CONFIG", 0b1111, false); // CML_EN_LANE[3:0]: the actual number of lanes is determined by OUTPUT_CONFIG
    RD53Interface::WriteChipReg(
        pChip, "GlobalPulseConf", pRD53->getFEtype()->GlobalPulseConfMap.find("RstAurora")->second | pRD53->getFEtype()->GlobalPulseConfMap.find("RstSerializer")->second, false);
    RD53Interface::SendCommand(pChip, RD53ACmd::GlobalPulse{pChip->getId(), 10});
    std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::DEEPSLEEP));

    // ###############################################
    // # Initialize driver strength and pre-emphasis #
    // ###############################################
    const std::set<std::string> registerPreEmphasisWhiteList = {"CML_CONFIG_SER_EN_TAP", "CML_CONFIG_SER_INV_TAP", "DAC_CML_BIAS_0", "DAC_CML_BIAS_1", "DAC_CML_BIAS_2"}; // @CONST@

    for(auto& cRegItem: pChip->getRegMap())
        if((cRegItem.second.fPrmptCfg == true) && (registerPreEmphasisWhiteList.find(cRegItem.first) != registerPreEmphasisWhiteList.end()))
            RD53Interface::WriteChipReg(pChip, cRegItem.first, cRegItem.second.fDefValue, false);

    // ##############################
    // # Set standard AURORA output #
    // ##############################
    RD53Interface::WriteChipReg(pChip, "SER_SEL_OUT", RD53Constants::PATTERN_AURORA, false);

    // ###############################################################
    // # Enable monitoring (needed for AutoRead register monitoring) #
    // ###############################################################
    RD53Interface::WriteChipReg(pChip, "GlobalPulseConf", pRD53->getFEtype()->GlobalPulseConfMap.find("StartMonitoring")->second, false);
    RD53Interface::SendCommand(pChip, RD53ACmd::GlobalPulse{pChip->getId(), 10});
    std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::DEEPSLEEP));

    // ##############
    // # Link speed #
    // ##############
    RD53Interface::WriteChipReg(pChip, "CDR_CONFIG_SEL_SER_CLK", static_cast<RD53FWInterface*>(fBoardFW)->ReadoutSpeed() == RD53FWconstants::ReadoutSpeed::x1280 ? 0 : 1, false);
    RD53Interface::SendCommand(pRD53, RD53ACmd::ECR{});
}

std::vector<std::pair<uint16_t, uint16_t>> RD53AInterface::ReadRD53Reg(ReadoutChip* pChip, const std::string& regName)
{
    this->setBoard(pChip->getBeBoardId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);

    auto nameAndValue(SetSpecialRegister(regName, 0, pChip->getRegMap()));
    RD53Interface::SendCommand(pChip, RD53ACmd::RdReg{pChip->getId(), pChip->getRegItem(nameAndValue.first).fAddress});
    auto regReadback = static_cast<RD53FWInterface*>(fBoardFW)->ReadChipRegisters(pChip);

    for(auto i = 0u; i < regReadback.size(); i++)
    {
        regReadback[i].first  = regReadback[i].first & static_cast<uint16_t>(RD53Shared::setBits(RD53Constants::NBIT_ADDR)); // Removing bit related to PIX_PORTAL register identification
        regReadback[i].second = RD53AInterface::GetSpecialRegisterValue(regName, regReadback[i].second, pChip->getRegMap());
    }

    return regReadback;
}

std::pair<std::string, uint16_t> RD53AInterface::SetSpecialRegister(std::string regName, uint16_t value, ChipRegMap& pRD53RegMap)
{
    auto it = RD53A::specialRegMap.find(regName);
    if(it == RD53A::specialRegMap.end())
        return {regName, value};
    else
    {
        try
        {
            pRD53RegMap.at(regName);
        }
        catch(const std::out_of_range& error)
        {
            throw std::out_of_range("Register " + regName + " not found in RD53 register-map file. I can not proceed. Please verify that you are using the latest RD53 registre-map.");
        }
        ChipRegItem& specialReg = pRD53RegMap.at(regName);
        ChipRegItem& Reg        = pRD53RegMap.at(it->second.regName);
        return {it->second.regName, RD53Interface::SetFieldValue(Reg.fValue, value, it->second.start, specialReg.fBitSize)};
    }
}

uint16_t RD53AInterface::GetSpecialRegisterValue(std::string regName, uint16_t value, ChipRegMap& pRD53RegMap)
{
    auto it = RD53A::specialRegMap.find(regName);
    if(it == RD53A::specialRegMap.end())
        return value;
    else
    {
        ChipRegItem& specialReg = pRD53RegMap.at(regName);
        return RD53Interface::GetFieldValue(value, it->second.start, specialReg.fBitSize);
    }
}

uint16_t RD53AInterface::GetPixelConfig(const pixelMask& mask, uint16_t row, uint16_t col, bool highGain)
// ##############################################################################################################
// # Encodes the configuration for a pixel pair                                                                 #
// # In the LIN FE TDAC is unsigned and increasing it reduces the local threshold                               #
// # In the DIFF FE TDAC is signed and increasing it increases the local threshold                              #
// # To prevent having to deal with that in the rest of the code, we map the TDAC range of the DIFF FE like so: #
// # -15 -> 30, -14 -> 29, ... 0 -> 15, ... 15 -> 0                                                             #
// # So for the rest of the code the TDAC range of the DIFF FE is [0, 30] and                                   #
// # the only difference with the LIN FE is the number of possible values                                       #
// ##############################################################################################################
{
    if(col <= RD53A::SYNC.colStop)
        return bits::pack<8, 8>(bits::pack<1, 1, 1>(mask.HitBus[row + RD53A::NROWS * (col + 1)], mask.InjEn[row + RD53A::NROWS * (col + 1)], mask.Enable[row + RD53A::NROWS * (col + 1)]),
                                bits::pack<1, 1, 1>(mask.HitBus[row + RD53A::NROWS * (col + 0)], mask.InjEn[row + RD53A::NROWS * (col + 0)], mask.Enable[row + RD53A::NROWS * (col + 0)]));
    else if(col <= RD53A::LIN.colStop)
        return bits::pack<8, 8>(bits::pack<1, 4, 1, 1, 1>(highGain,
                                                          mask.TDAC[row + RD53A::NROWS * (col + 1)],
                                                          mask.HitBus[row + RD53A::NROWS * (col + 1)],
                                                          mask.InjEn[row + RD53A::NROWS * (col + 1)],
                                                          mask.Enable[row + RD53A::NROWS * (col + 1)]),
                                bits::pack<1, 4, 1, 1, 1>(highGain,
                                                          mask.TDAC[row + RD53A::NROWS * (col + 0)],
                                                          mask.HitBus[row + RD53A::NROWS * (col + 0)],
                                                          mask.InjEn[row + RD53A::NROWS * (col + 0)],
                                                          mask.Enable[row + RD53A::NROWS * (col + 0)]));
    else
        return bits::pack<8, 8>(bits::pack<1, 4, 1, 1, 1>(mask.TDAC[row + RD53A::NROWS * (col + 1)] > 15,
                                                          abs(15 - mask.TDAC[row + RD53A::NROWS * (col + 1)]),
                                                          mask.HitBus[row + RD53A::NROWS * (col + 1)],
                                                          mask.InjEn[row + RD53A::NROWS * (col + 1)],
                                                          mask.Enable[row + RD53A::NROWS * (col + 1)]),
                                bits::pack<1, 4, 1, 1, 1>(mask.TDAC[row + RD53A::NROWS * (col + 0)] > 15,
                                                          abs(15 - mask.TDAC[row + RD53A::NROWS * (col + 0)]),
                                                          mask.HitBus[row + RD53A::NROWS * (col + 0)],
                                                          mask.InjEn[row + RD53A::NROWS * (col + 0)],
                                                          mask.Enable[row + RD53A::NROWS * (col + 0)]));
}

void RD53AInterface::WriteRD53Mask(RD53* pRD53, int writeMode, bool doDefault, size_t theRow, size_t theCol)
// ##################################
// # wireMode = 0 --> all pixels    #
// # wireMode = 1 --> sparse pixels #
// # wireMode = 2 --> single pixel  #
// ##################################
{
    this->setBoard(pRD53->getBeBoardId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);

    std::vector<uint16_t> commandList;
    const uint16_t        REGION_COL_ADDR    = pRD53->getRegItem("REGION_COL").fAddress;
    const uint16_t        REGION_ROW_ADDR    = pRD53->getRegItem("REGION_ROW").fAddress;
    const uint16_t        PIX_MODE_ADDR      = pRD53->getRegItem("PIX_MODE").fAddress;
    const uint16_t        PIX_PORTAL_ADDR    = pRD53->getRegItem("PIX_PORTAL").fAddress;
    const uint8_t         highGain           = pRD53->getRegItem("HighGain_LIN").fValue;
    const uint8_t         chipID             = pRD53->getId();
    auto&                 mask               = doDefault == true ? pRD53->getPixelsMaskDefault() : pRD53->getPixelsMask();
    const auto            n16bitWordsWrt     = RD53ACmd::getN16bitWords<RD53ACmd::WrReg>();
    const auto            n16bitWordsWrtLong = RD53ACmd::getN16bitWords<RD53ACmd::WrRegLong>();

    // ##########################
    // # Disable default config #
    // ##########################
    RD53ACmd::serialize(RD53ACmd::WrReg{chipID, pRD53->getRegItem("PIX_DEFAULT_CONFIG").fAddress, 0x0}, commandList);

    // ############
    // # PIX_MODE #
    // ############
    // bit[5]: enable broadcast
    // bit[4]: enable auto-col
    // bit[3]: enable auto-row
    // bit[2]: broadcast to SYNC FE
    // bit[1]: broadcast to LIN FE
    // bit[0]: broadcast to DIFF FE

    if(writeMode == 0)
    {
        // #####################
        // # Set autoincrement #
        // #####################
        RD53ACmd::serialize(RD53ACmd::WrReg{chipID, PIX_MODE_ADDR, 0x8}, commandList);

        RD53ACmd::WrRegLong wrRegLongCmd{chipID, PIX_PORTAL_ADDR, {}};
        const size_t        nValuesLongCmd = wrRegLongCmd.values.size();
        const size_t        nLongCommands  = RD53A::NROWS / nValuesLongCmd;

        for(auto col = 0u; col < RD53A::NCOLS; col += 2)
        {
            // #######################
            // # Starting pixel cell #
            // #######################
            RD53ACmd::serialize(RD53ACmd::WrReg{chipID, REGION_COL_ADDR, col / 2}, commandList);
            RD53ACmd::serialize(RD53ACmd::WrReg{chipID, REGION_ROW_ADDR, 0x0}, commandList);

            for(auto longCmdId = 0u; longCmdId < nLongCommands; longCmdId++)
            {
                for(size_t i = 0; i < nValuesLongCmd; i++) wrRegLongCmd.values[i] = RD53AInterface::GetPixelConfig(mask, nValuesLongCmd * longCmdId + i, col, highGain);
                RD53ACmd::serialize(wrRegLongCmd, commandList);
            }

            // ###################################
            // # Write commands to frontend chip #
            // ###################################
            auto n16bitWords = commandList.size() + nLongCommands * n16bitWordsWrtLong + 2 * n16bitWordsWrt;
            if((n16bitWords / 2 + n16bitWords % 2) > (1 << RD53FWconstants::NBIT_SLOWCMD_FIFO))
            {
                static_cast<RD53FWInterface*>(fBoardFW)->WriteChipCommands(commandList, pRD53->getHybridId());
                commandList.clear();
            }
        }
    }
    else if(writeMode == 1)
    {
        // ############################
        // # Clear whole pixel matrix #
        // ############################
        RD53ACmd::serialize(RD53ACmd::WrReg{chipID, PIX_MODE_ADDR, 0x27}, commandList);
        RD53ACmd::serialize(RD53ACmd::WrReg{chipID, PIX_PORTAL_ADDR, 0x0}, commandList);
        RD53ACmd::serialize(RD53ACmd::WrReg{chipID, PIX_MODE_ADDR, 0x0}, commandList);

        for(auto col = 0u; col < RD53A::NCOLS; col += 2)
        {
            if((std::find(mask.Enable.begin() + (0 + RD53A::NROWS * col), mask.Enable.begin() + (RD53A::NROWS + RD53A::NROWS * col), true) ==
                (mask.Enable.begin() + (RD53A::NROWS + RD53A::NROWS * col))) &&
               (std::find(mask.Enable.begin() + (0 + RD53A::NROWS * (col + 1)), mask.Enable.begin() + (RD53A::NROWS + RD53A::NROWS * (col + 1)), true) ==
                (mask.Enable.begin() + (RD53A::NROWS + RD53A::NROWS * (col + 1)))) &&
               (std::find(mask.InjEn.begin() + (0 + RD53A::NROWS * col), mask.InjEn.begin() + (RD53A::NROWS + RD53A::NROWS * col), true) ==
                (mask.InjEn.begin() + (RD53A::NROWS + RD53A::NROWS * col))) &&
               (std::find(mask.InjEn.begin() + (0 + RD53A::NROWS * (col + 1)), mask.InjEn.begin() + (RD53A::NROWS + RD53A::NROWS * (col + 1)), true) ==
                (mask.InjEn.begin() + (RD53A::NROWS + RD53A::NROWS * (col + 1)))))
                continue;

            RD53ACmd::serialize(RD53ACmd::WrReg{chipID, REGION_COL_ADDR, col / 2}, commandList);

            for(auto row = 0u; row < RD53A::NROWS; row++)
                if((mask.Enable[row + RD53A::NROWS * col] == true) || (mask.Enable[row + RD53A::NROWS * (col + 1)] == true) || (mask.InjEn[row + RD53A::NROWS * col] == true) ||
                   (mask.InjEn[row + RD53A::NROWS * (col + 1)] == true))
                {
                    auto data = RD53AInterface::GetPixelConfig(mask, row, col, highGain);

                    RD53ACmd::serialize(RD53ACmd::WrReg{chipID, REGION_ROW_ADDR, row}, commandList);
                    RD53ACmd::serialize(RD53ACmd::WrReg{chipID, PIX_PORTAL_ADDR, data}, commandList);
                }

            // ###################################
            // # Write commands to frontend chip #
            // ###################################
            auto n16bitWords = commandList.size() + (RD53A::NROWS * 2 + 1) * n16bitWordsWrt;
            if((n16bitWords / 2 + n16bitWords % 2) > (1 << RD53FWconstants::NBIT_SLOWCMD_FIFO))
            {
                static_cast<RD53FWInterface*>(fBoardFW)->WriteChipCommands(commandList, pRD53->getHybridId());
                commandList.clear();
            }
        }
    }
    else
    {
        auto data = RD53AInterface::GetPixelConfig(mask, theRow, theCol, highGain);

        RD53ACmd::serialize(RD53ACmd::WrReg{chipID, PIX_MODE_ADDR, 0x0}, commandList);
        RD53ACmd::serialize(RD53ACmd::WrReg{chipID, REGION_COL_ADDR, theCol / 2}, commandList);
        RD53ACmd::serialize(RD53ACmd::WrReg{chipID, REGION_ROW_ADDR, theRow}, commandList);
        RD53ACmd::serialize(RD53ACmd::WrReg{chipID, PIX_PORTAL_ADDR, data}, commandList);
    }

    // ###################################
    // # Write commands to frontend chip #
    // ###################################
    if(commandList.size() != 0) static_cast<RD53FWInterface*>(fBoardFW)->WriteChipCommands(commandList, pRD53->getHybridId());
}

void RD53AInterface::PackWriteCommand(Chip* pChip, const std::string& regName, uint16_t data, std::vector<uint16_t>& chipCommandList, bool updateReg)
{
    RD53ACmd::serialize(RD53ACmd::WrReg{(uint8_t)pChip->getId(), pChip->getRegItem(regName).fAddress, data}, chipCommandList);

    if(updateReg == true) pChip->setReg(regName, data);
}

void RD53AInterface::PackWriteBroadcastCommand(const BeBoard* pBoard, const std::string& regName, uint16_t data, std::vector<uint16_t>& chipCommandList, bool updateReg)
{
    RD53ACmd::serialize(RD53ACmd::WrReg{RD53Shared::firstChip->getFEtype()->broadcastChipId, RD53Shared::firstChip->getRegItem(regName).fAddress, data}, chipCommandList);

    if(updateReg == true)
        for(auto cOpticalGroup: *pBoard)
            for(auto cHybrid: *cOpticalGroup)
                for(auto cChip: *cHybrid) cChip->setReg(regName, data);
}

void RD53AInterface::WriteClockDataDelay(Chip* pChip, uint16_t value)
{
    this->setBoard(pChip->getBeBoardId());

    RD53Interface::WriteChipReg(pChip, "CLK_DATA_DELAY", value, false);
    static_cast<RD53FWInterface*>(fBoardFW)->WriteChipCommands(std::vector<uint16_t>(RD53Constants::NSYNC_WORDS_L, RD53ACmd::RD53ACmdEncoder::SYNC), -1);
    RD53Interface::WriteChipReg(pChip, "CLK_DATA_DELAY", value, true);
}

void RD53AInterface::SendBoardClear(const BeBoard* pBoard)
{
    this->setBoard(pBoard->getId());

    for(auto cOpticalGroup: *pBoard)
        for(auto cHybrid: *cOpticalGroup)
            for(auto cChip: *cHybrid)
            {
                RD53Interface::SendCommand(cChip, RD53ACmd::BCR{});
                RD53Interface::SendCommand(cChip, RD53ACmd::ECR{});
            }
}

// ###########################
// # Dedicated to monitoring #
// ###########################

int RD53AInterface::getADCobservable(const std::string& observableName, bool& isCurrentNotVoltage, bool silentRunning)
{
    uint32_t voltageObservable(0), currentObservable(0);

    const std::unordered_map<std::string, uint32_t> currentMultiplexer = {
        {"Iref", 0x00},          {"IBIASP1_SYNC", 0x01}, {"IBIASP2_SYNC", 0x02},   {"IBIAS_DISC_SYNC", 0x03}, {"IBIAS_SF_SYNC", 0x04},  {"ICTRL_SYNCT_SYNC", 0x05}, {"IBIAS_KRUM_SYNC", 0x06},
        {"COMP_LIN", 0x07},      {"FC_BIAS_LIN", 0x08},  {"KRUM_CURR_LIN", 0x09},  {"LDAC_LIN", 0x0A},        {"PA_IN_BIAS_LIN", 0x0B}, {"COMP_DIFF", 0x0C},        {"PRECOMP_DIFF", 0x0D},
        {"FOL_DIFF", 0x0E},      {"PRMP_DIFF", 0x0F},    {"LCC_DIFF", 0x10},       {"VFF_DIFF", 0x11},        {"VTH1_DIFF", 0x12},      {"VTH2_DIFF", 0x13},        {"CDR_CP_IBIAS", 0x14},
        {"VCO_BUFF_BIAS", 0x15}, {"VCO_IBIAS", 0x16},    {"DAC_CML_BIAS_0", 0x17}, {"DAC_CML_BIAS_1", 0x18},  {"DAC_CML_BIAS_2", 0x19}};

    const std::unordered_map<std::string, uint32_t> voltageMultiplexer = {
        {"ADCbandgap", 0x00},      {"CAL_MD", 0x01},          {"CAL_HI", 0x02},         {"TEMPSENS_1", 0x03},      {"RADSENS_1", 0x04},       {"TEMPSENS_2", 0x05},      {"RADSENS_2", 0x06},
        {"TEMPSENS_4", 0x07},      {"RADSENS_4", 0x08},       {"VREF_VDAC", 0x09},      {"VOUT_BG", 0x0A},         {"IMUXoutput", 0x0B},      {"CAL_MD", 0x0C},          {"CAL_HI", 0x0D},
        {"RADSENS_3", 0x0E},       {"TEMPSENS_3", 0x0F},      {"REF_KRUM_LIN", 0x10},   {"Vthreshold_LIN", 0x11},  {"VTH_SYNC", 0x12},        {"VBL_SYNC", 0x13},        {"VREF_KRUM_SYNC", 0x14},
        {"VTH_HI_DIFF", 0x15},     {"VTH_LO_DIFF", 0x16},     {"VIN_ana_ShuLDO", 0x17}, {"VOUT_ana_ShuLDO", 0x18}, {"VREF_ana_ShuLDO", 0x19}, {"VOFF_ana_ShuLDO", 0x1A}, {"VIN_dig_ShuLDO", 0x1D},
        {"VOUT_dig_ShuLDO", 0x1E}, {"VREF_dig_ShuLDO", 0x1F}, {"VOFF_dig_ShuLDO", 0x20}};

    auto search = currentMultiplexer.find(observableName);
    if(search == currentMultiplexer.end())
    {
        if((search = voltageMultiplexer.find(observableName)) == voltageMultiplexer.end())
        {
            if(silentRunning == false) LOG(DEBUG) << BOLDRED << "Wrong observable name: " << BOLDYELLOW << observableName << RESET;
            return -1;
        }
        else
            voltageObservable = search->second;
        isCurrentNotVoltage = false;
    }
    else
    {
        currentObservable   = search->second;
        voltageObservable   = voltageMultiplexer.find("IMUXoutput")->second;
        isCurrentNotVoltage = true;
    }

    return bits::pack<1, 6, 7>(true, currentObservable, voltageObservable);
}

uint32_t RD53AInterface::measureADC(ReadoutChip* pChip, uint32_t data)
{
    this->setBoard(pChip->getBeBoardId());
    auto pRD53 = static_cast<RD53*>(pChip);

    const uint16_t GlbPulseAddr = pChip->getRegItem("GlobalPulseConf").fAddress;
    const uint8_t  chipID       = pChip->getId();
    const uint16_t trimADC      = bits::pack<1, 5, 6>(true, pChip->getRegItem("MONITOR_CONFIG_BG").fValue, pChip->getRegItem("MONITOR_CONFIG_ADC").fValue);
    // [10:6] band-gap trim [5:0] ADC trim. According to wafer probing they should give an average VrefADC of 0.9 V
    const uint16_t GlbPulseVal = RD53Interface::ReadChipReg(pChip, "GlobalPulseConf");

    std::vector<uint16_t> commandList;

    RD53ACmd::serialize(RD53ACmd::WrReg{chipID, pChip->getRegItem("MonitorConfig").fAddress, trimADC}, commandList);
    RD53ACmd::serialize(RD53ACmd::WrReg{chipID, GlbPulseAddr, pRD53->getFEtype()->GlobalPulseConfMap.find("RstADC")->second}, commandList);
    RD53ACmd::serialize(RD53ACmd::GlobalPulse{pChip->getId(), 0x04}, commandList);
    RD53ACmd::serialize(RD53ACmd::WrReg{chipID, GlbPulseAddr, pRD53->getFEtype()->GlobalPulseConfMap.find("RstServiceData")->second}, commandList);
    RD53ACmd::serialize(RD53ACmd::GlobalPulse{pChip->getId(), 0x04}, commandList);
    RD53ACmd::serialize(RD53ACmd::WrReg{chipID, pChip->getRegItem("MONITOR_SELECT").fAddress, data}, commandList); // 14 bits: bit 13 enable, bits 7:12 I-Mon, bits 0:6 V-Mon
    RD53ACmd::serialize(RD53ACmd::WrReg{chipID, GlbPulseAddr, pRD53->getFEtype()->GlobalPulseConfMap.find("ADCstatConversion")->second}, commandList);
    RD53ACmd::serialize(RD53ACmd::GlobalPulse{pChip->getId(), 0x04}, commandList);
    RD53ACmd::serialize(RD53ACmd::WrReg{chipID, GlbPulseAddr, GlbPulseVal}, commandList); // Restore value in Global Pulse Route

    static_cast<RD53FWInterface*>(fBoardFW)->WriteChipCommands(commandList, pChip->getHybridId());
    return RD53Interface::ReadChipReg(pChip, "MonitoringDataADC");
}

float RD53AInterface::measureTemperature(ReadoutChip* pChip, uint32_t data, const std::string& type, int beta)
{
    // ################################################################################################
    // # Temperature measurement is done by measuring twice, once with high bias, once with low bias  #
    // # Temperature is calculated based on the difference of the two, with the formula on the bottom #
    // # idealityFactor = 1225 [1/1000]                                                               #
    // ################################################################################################

    this->setBoard(pChip->getBeBoardId());

    // #####################
    // # Natural constants #
    // #####################
    const float   T0C            = 273.15;         // [Kelvin]
    const float   kb             = 1.38064852e-23; // [J/K]
    const float   e              = 1.6021766208e-19;
    const float   R              = 15;   // By circuit design
    const uint8_t sensorDEM      = 0x0E; // Sensor Dynamic Element Matching bits needed to trim the thermistors
    const float   idealityFactor = pChip->getRegItem("TEMPSENS_IDEAL_FACTOR").fValue / 1e3;

    uint16_t sensorConfigData; // Enable[5], DEM[4:1], SEL_BIAS[0]

    // Get high bias voltage
    sensorConfigData = bits::pack<1, 4, 1, 1, 4, 1>(true, sensorDEM, 0, true, sensorDEM, 0);
    RD53Interface::WriteChipReg(pChip, "SENSOR_CONFIG_0", sensorConfigData);
    RD53Interface::WriteChipReg(pChip, "SENSOR_CONFIG_1", sensorConfigData);
    auto valueLow = RD53Interface::convertADC2VorI(pChip, RD53AInterface::measureADC(pChip, data));

    // Get low bias voltage
    sensorConfigData = bits::pack<1, 4, 1, 1, 4, 1>(true, sensorDEM, 1, true, sensorDEM, 1);
    RD53Interface::WriteChipReg(pChip, "SENSOR_CONFIG_0", sensorConfigData);
    RD53Interface::WriteChipReg(pChip, "SENSOR_CONFIG_1", sensorConfigData);
    auto valueHigh = RD53Interface::convertADC2VorI(pChip, RD53AInterface::measureADC(pChip, data));

    return e / (idealityFactor * kb * log(R)) * (valueHigh - valueLow) - T0C;
}

} // namespace Ph2_HwInterface
