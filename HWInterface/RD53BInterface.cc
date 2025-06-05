/*!
  \file                  RD53BInterface.h
  \brief                 User interface to the RD53B readout chip
  \author                Mauro DINARDO and Alkiviadis PAPADOPOULOS
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
  Support:               email to alkiviadis.papadopoulos@cern.ch
*/

#include "HWInterface/RD53BInterface.h"

using namespace Ph2_HwDescription;

namespace Ph2_HwInterface
{
bool RD53BInterface::ConfigureChip(Chip* pChip, bool pVerify, uint32_t pBlockSize)
{
    this->setBoard(pChip->getBeBoardId());
    auto  pRD53       = static_cast<RD53*>(pChip);
    auto& pRD53RegMap = pChip->getRegMap();

    // ########################################################################
    // # Switching to pixel-register configuration, instead of the hard-wired #
    // ########################################################################
    RD53Interface::WriteChipReg(pChip, "PIX_DEFAULT_CONFIG", 0x9CE2, false);
    RD53Interface::WriteChipReg(pChip, "PIX_DEFAULT_CONFIG_B", 0x631D, false);

    // ######################
    // # Reset Core Columns #
    // ######################
    RD53BInterface::ResetCoreColumns(pRD53);

    // ##############
    // # Field data #
    // ##############
    if(RD53Interface::WriteChipReg(pChip, "DataConcentratorConf", 0, pVerify) == false)
    {
        const auto& theMap = pRD53->getFEtype()->GlobalPulseConfMap;
        RD53BInterface::SendGlobalPulse(
            pChip,
            (theMap.find("RstAuroraV1") != theMap.end() ? theMap.find("RstAuroraV1")->second : 0) | (theMap.find("RstSerializerV1") != theMap.end() ? theMap.find("RstSerializerV1")->second : 0) |
                (theMap.find("RstAuroraV2") != theMap.end() ? theMap.find("RstAuroraV2")->second : 0) | (theMap.find("RstSerializerV2") != theMap.end() ? theMap.find("RstSerializerV2")->second : 0),
            10);
        std::this_thread::sleep_for(std::chrono::milliseconds(RD53Shared::AURORASLEEP));
    }
    // # bit 12:   EnCRC         --> Map in FormatOptions: enableCRC
    // # bit 11:   EnBCId        --> Map in FormatOptions: enableBCID
    // # bit 10:   EnLv1Id       --> Map in FormatOptions: enableTriggerId
    // # bit 9:    EnEoS         --> Map in FormatOptions: enableEOSmarker
    // # bits 1-8: NumOfEventsInStream[7:0]
    RD53Interface::WriteChipReg(pChip, "CoreColEncoderConf", 0, pVerify);
    // # bit 9:    BinaryReadOut --> Map in FormatOptions: enableToT
    // # bit 8:    RawData       --> Map in FormatOptions: enableRawMap
    // # bits 4-7: MaxHits[3:0]
    // # bits 1-3: MaxToT[2:0]

    // ###################
    // # Ring oscillator #
    // ###################
    RD53Interface::WriteChipReg(pChip, "RingOscConfig", 0b111111111111111, pVerify);
    RD53Interface::WriteChipReg(pChip, "RingOscConfig", 0b101111011111111, pVerify);
    // # bit 15:   RingOscBClear
    // # bit 14:   RingOscBEnBL
    // # bit 13:   RingOscBEnBR
    // # bit 12:   RingOscBEnCAPA
    // # bit 11:   RingOscBEnFF
    // # bit 10:   RingOscBEnLVT
    // # bit 9:    RingOscAClear
    // # bits 1:8: RingOscAEnable[7:0]

    // ###################################################
    // # Impose SelfTriggerMultiplier = trigger_duration #
    // ###################################################
    RD53Interface::WriteChipReg(pChip, "SelfTriggerMultiplier", static_cast<RD53FWInterface*>(fBoardFW)->getLocalCfgFastCmd()->trigger_duration + 1, pVerify);

    // #######################################
    // # Programming CLK_DATA_DELAY register #
    // #######################################
    const std::set<std::string> registerClkDataDelayList = {"CLK_DATA_DELAY", "CLK_DATA_DELAY_DATA", "CLK_DATA_DELAY_CLK"}; // @CONST@
    bool                        doWriteClkDataDelay      = false;

    for(auto i = 0u; i < registerClkDataDelayList.size(); i++)
    {
        auto cRegItem = pRD53RegMap.find(*std::next(registerClkDataDelayList.begin(), i));
        if((cRegItem->second.fPrmptCfg == true) && (cRegItem != pRD53RegMap.end()))
        {
            doWriteClkDataDelay = true;

            pChip->getRegItem("CLK_DATA_DELAY").fValue = SetSpecialRegister(std::string(cRegItem->first), cRegItem->second.fDefValue, pRD53RegMap).second;

            if(cRegItem->first == "CLK_DATA_DELAY") break;
        }
    }
    if(doWriteClkDataDelay == true) RD53BInterface::WriteClockDataDelay(pChip, pChip->getRegItem("CLK_DATA_DELAY").fValue);

    // ###############################
    // # Programmig global registers #
    // ###############################
    const std::set<std::string> registerPreEmphasisWhiteList = {"CML_CONFIG_SER_EN_TAP", "CML_CONFIG_SER_INV_TAP", "DAC_CML_BIAS_0", "DAC_CML_BIAS_1", "DAC_CML_BIAS_2"}; // @CONST@
    const std::set<std::string> registerBlackList            = {"RESISTORI2V",
                                                                "NTCBETA",
                                                                "RNTCAT25C",
                                                                "ADC_OFFSET_VOLT",
                                                                "ADC_MAXIMUM_VOLT",
                                                                "TEMPSENS_IDEAL_FACTOR",
                                                                "TEMPSENS_IDEAL_FACTOR_ANA",
                                                                "TEMPSENS_IDEAL_FACTOR_DIG",
                                                                "RADSENS_IDEAL_FACTOR",
                                                                "RADSENS_IDEAL_FACTOR_ANA",
                                                                "RADSENS_IDEAL_FACTOR_DIG",
                                                                "TEMPSENS_OFFSET_TOP",
                                                                "TEMPSENS_OFFSET_BOTTOM",
                                                                "SAMPLE_N_TIMES",
                                                                "WAIT_MUX_CONFIG",
                                                                "VREF_ADC"}; // @CONST@
    const std::set<std::string> registerWhiteList            = {"DAC_PREAMP_L_LIN",
                                                                "DAC_PREAMP_R_LIN",
                                                                "DAC_PREAMP_TL_LIN",
                                                                "DAC_PREAMP_TR_LIN",
                                                                "DAC_PREAMP_T_LIN",
                                                                "DAC_PREAMP_M_LIN",
                                                                "DAC_FC_LIN",
                                                                "DAC_KRUM_CURR_LIN",
                                                                "DAC_REF_KRUM_LIN",
                                                                "DAC_COMP_LIN",
                                                                "DAC_COMP_TA_LIN",
                                                                "DAC_GDAC_L_LIN",
                                                                "DAC_GDAC_R_LIN",
                                                                "DAC_GDAC_M_LIN",
                                                                "DAC_LDAC_LIN"}; // @CONST@

    for(auto& cRegItem: pRD53RegMap)
        if(((cRegItem.second.fPrmptCfg == true) && (registerBlackList.find(cRegItem.first) == registerBlackList.end()) &&
            (registerClkDataDelayList.find(cRegItem.first) == registerClkDataDelayList.end()) && (registerPreEmphasisWhiteList.find(cRegItem.first) == registerPreEmphasisWhiteList.end())) ||
           (registerWhiteList.find(cRegItem.first) != registerWhiteList.end()))
            RD53Interface::WriteChipReg(pChip, cRegItem.first, cRegItem.second.fDefValue, pVerify);
        else if((cRegItem.second.fPrmptCfg == true) && (registerBlackList.find(cRegItem.first) != registerBlackList.end()))
            pChip->getRegItem(cRegItem.first).fValue = cRegItem.second.fDefValue;

    // ###################################
    // # Programmig pixel cell registers #
    // ###################################
    RD53BInterface::WriteRD53Mask(pRD53, false, true);

    // #################################################
    // # Important values to be checked before running #
    // #################################################
    LOG(INFO) << BOLDBLUE << "Parameters that the user should check from the database:" << RESET;
    LOG(INFO) << BOLDBLUE << "\t--> VOLTAGE_TRIM_DIG = " << BOLDYELLOW << RD53Interface::ReadChipReg(pChip, "VOLTAGE_TRIM_DIG") << RESET;
    LOG(INFO) << BOLDBLUE << "\t--> VOLTAGE_TRIM_ANA = " << BOLDYELLOW << RD53Interface::ReadChipReg(pChip, "VOLTAGE_TRIM_ANA") << RESET;
    LOG(INFO) << BOLDBLUE << "\t--> Wire bonded chip ID = " << BOLDYELLOW << RD53Interface::ReadChipReg(pChip, "ChipIdWireBonds") << RESET;
    LOG(INFO) << BOLDBLUE << "\t--> Wire bonded Iref = " << BOLDYELLOW << RD53Interface::ReadChipReg(pChip, "IrefWireBonds") << RESET;

    return true;
}

void RD53BInterface::InitRD53Downlink(const BeBoard* pBoard)
{
    this->setBoard(pBoard->getId());

    // #######################################################################
    // # Switching to chip-register configuration, instead of the hard-wired #
    // #######################################################################
    RD53Interface::WriteBoardBroadcastChipReg(pBoard, "GCR_DEFAULT_CONFIG", 0xAC75);
    RD53Interface::WriteBoardBroadcastChipReg(pBoard, "GCR_DEFAULT_CONFIG_B", 0x538A);
}

void RD53BInterface::InitRD53Uplinks(Chip* pChip)
{
    this->setBoard(pChip->getBeBoardId());
    auto        pRD53  = static_cast<RD53*>(pChip);
    const auto& theMap = pRD53->getFEtype()->GlobalPulseConfMap;

    RD53Interface::WriteChipReg(pChip, "SER_SEL_OUT", RD53Constants::PATTERN_AURORA, false);
    // # bits 7-8: SerSelOut3[1:0]
    // # bits 5-6: SerSelOut2[1:0]
    // # bits 3-4: SerSelOut1[1:0]
    // # bits 1-2: SerSelOut0[1:0]
    RD53Interface::WriteChipReg(pChip, "CML_CONFIG", pRD53->laneConfig.packOutputLanes(), false);
    // # bits 7-8: SER_INV_TAP[1:0]
    // # bits 5-6: SER_EN_TAP[1:0]
    // # bits 1-4: SER_EN_LANE[3:0] --> External output lanes

    // ###############################################
    // # Initialize driver strength and pre-emphasis #
    // ###############################################
    const std::set<std::string> registerPreEmphasisWhiteList = {"CML_CONFIG_SER_EN_TAP", "CML_CONFIG_SER_INV_TAP", "DAC_CML_BIAS_0", "DAC_CML_BIAS_1", "DAC_CML_BIAS_2"}; // @CONST@

    for(auto& cRegItem: pChip->getRegMap())
        if((cRegItem.second.fPrmptCfg == true) && (registerPreEmphasisWhiteList.find(cRegItem.first) != registerPreEmphasisWhiteList.end()))
            RD53Interface::WriteChipReg(pChip, cRegItem.first, cRegItem.second.fDefValue, false);

    // ##############
    // # Link speed #
    // ##############
    RD53Interface::WriteChipReg(
        pChip, "CDR_CONFIG_SEL_SER_CLK", (pRD53->laneConfig.isPrimary == false ? RD53FWconstants::ReadoutSpeed::x320 : static_cast<RD53FWInterface*>(fBoardFW)->ReadoutSpeed()), false);

    // ########################
    // # Aurora configuration #
    // ########################
    RD53Interface::WriteChipReg(pChip, "AuroraConfig", bits::pack<4, 6, 2>(RD53Shared::setBits(pRD53->laneConfig.nOutputLanes), 0b011010, 0b11), false);
    // # bit 14:    SendAltOutput
    // # bit 13:    EnablePRBS
    // # bits 9-12: ActiveLanes[3:0] --> Internal output lanes
    // # bits 3-8:  CCWait[5:0]
    // # bits 1-2:  CCSend[1:0]

    // #####################################
    // # Aurora Channel Bond configuration #
    // #####################################
    RD53Interface::WriteChipReg(pChip, "AURORA_CB_CONFIG0", 0x0961, false);
    // # bits 5-16: CBWait[11:0]
    // # bits 1-4:  CBSend[3:0]
    RD53Interface::WriteChipReg(pChip, "AURORA_CB_CONFIG1", 0x00, false);
    // # bits 1-8: CBWait[19:12]

    // #######################
    // # Reset communication #
    // #######################
    RD53BInterface::SendGlobalPulse(
        pChip,
        (theMap.find("RstAuroraV1") != theMap.end() ? theMap.find("RstAuroraV1")->second : 0) | (theMap.find("RstSerializerV1") != theMap.end() ? theMap.find("RstSerializerV1")->second : 0) |
            (theMap.find("RstAuroraV2") != theMap.end() ? theMap.find("RstAuroraV2")->second : 0) | (theMap.find("RstSerializerV2") != theMap.end() ? theMap.find("RstSerializerV2")->second : 0),
        10);
    std::this_thread::sleep_for(std::chrono::milliseconds(RD53Shared::AURORASLEEP));

    // ################
    // # Data merging #
    // ################
    RD53Interface::WriteChipReg(
        pChip, "DataMerging", bits::pack<4, 1, 1, 1, 5, 1>(0, 1, 0, 0, pRD53->laneConfig.serializeArray<bool, NCHIPLANES + 1, 1, uint16_t>(pRD53->laneConfig.internalLanesEnabled), 1), false);
    // # bits 10-13: DataMergingInputPolarityInvert[3:0]
    // # bit 9:      EnOutputDataChipId   --> Map in FormatOptions: enableChipId
    // # bit 8:      EnGatingDataMergeClk1280
    // # bit 7:      SelDataMergeClk
    // # bits 3-6:   EnDataMergeLane[3:0] --> Internal input lanes
    // # bit 2:      MergeChBonding       --> Channel bonding
    // # bit 1:      DataMergingGpoSel

    uint16_t val = bits::pack<8, 8>(pRD53->laneConfig.serializeArray<uint8_t, NCHIPLANES, 2, uint16_t>(pRD53->laneConfig.inputLaneMapping),
                                    pRD53->laneConfig.serializeArray<uint8_t, NCHIPLANES, 2, uint16_t>(pRD53->laneConfig.outputLaneMapping));
    RD53Interface::WriteChipReg(pChip, "IOLaneMappingMux", val, false); // Mux selection for input and output internal lane mapping to external lanes
    // # Internal inputs mapped to external inputs with 2 bits
    // # bits 15-16: DataMergingInMux_3[1:0]
    // # bits 13-14: DataMergingInMux_2[1:0]
    // # bits 11-12: DataMergingInMux_1[1:0]
    // # bits 9-10:  DataMergingInMux_0[1:0]
    // # Internal outputs mapped to external outputs with 2 bits
    // # bits 7-8:   DataMergingOutMux_3[1:0]
    // # bits 5-6:   DataMergingOutMux_2[1:0]
    // # bits 3-4:   DataMergingOutMux_1[1:0]
    // # bits 1-2:   DataMergingOutMux_0[1:0]

    // #######################
    // # Enable Service Data #
    // #######################
    RD53Interface::WriteChipReg(pChip, "ServiceDataConf", 0x100 | 50, false); // How many Data frames to skip before sending a Monitor Frame
    // # bit 9:    EnServiceData
    // # bits 1-8: ServiceFrameSkip [7:0]

    // ######################
    // # Reset Data merging #
    // ######################
    if(pRD53->laneConfig.isPrimary == true)
    {
        RD53BInterface::SendGlobalPulse(pChip,
                                        (theMap.find("RstDataMerging") != theMap.end() ? theMap.find("RstDataMerging")->second : 0) |
                                            (theMap.find("RstDataPath") != theMap.end() ? theMap.find("RstDataPath")->second : 0),
                                        10);
        std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::DEEPSLEEP));
    }
}

void RD53BInterface::TAP0slaveOptimization(const BeBoard* pBoard, const Hybrid* pHybrid) // @TMP@ : temporary for RD53Bv1
{
    this->setBoard(pHybrid->getBeBoardId());

    // ########################
    // # Disable Service Data #
    // ########################
    RD53Interface::WriteBoardBroadcastChipReg(pBoard, "EnServiceData", 0);

    for(const auto cChip: *pHybrid)
        if(cChip->getFrontEndType() == FrontEndType::RD53Bv1)
        {
            // #################################
            // # TAP0 optimization for modules #
            // #################################

            auto fwInterface = static_cast<RD53FWInterface*>(fBoardFW);
            fwInterface->WriteReg("user.ctrl_regs.Aurora_block.error_cntr_chip_addr", static_cast<Ph2_HwDescription::RD53*>(cChip)->laneConfig.masterLane);
            if(static_cast<Ph2_HwDescription::RD53*>(cChip)->laneConfig.isPrimary == false)
            {
                RD53Interface::WriteChipReg(cChip, "EnServiceData", 1, false);

                LOG(INFO) << GREEN << "Optimizing " << BOLDYELLOW << "TAP0" << RESET << GREEN << " setting for chip ID " << BOLDYELLOW << cChip->getId() << RESET;

                const auto            maxTAP0value = RD53Shared::setBits(cChip->getNumberOfBits("DAC_CML_BIAS_0"));
                const float           timeLimit    = 1;   // @CONST@
                const int             nSteps       = 20;  // @CONST@
                const int             nFrames2Read = 1e7; // @CONST@
                const int             step         = floor(maxTAP0value / nSteps);
                std::vector<uint32_t> vecFrameCounter;
                std::vector<uint16_t> vecTAP0Values(nSteps);
                uint16_t              value = 0;
                std::generate(vecTAP0Values.begin(), vecTAP0Values.end(), [&value, &step]() { return value += step; });
                for(auto& TAP0: vecTAP0Values)
                {
                    RD53Interface::WriteChipReg(cChip, "DAC_CML_BIAS_0", TAP0, false);

                    fwInterface->WriteReg("user.ctrl_regs.Aurora_block.rst_frame_cntr", 1);
                    fwInterface->WriteReg("user.ctrl_regs.Aurora_block.rst_frame_cntr", 0);
                    fwInterface->WriteReg("user.ctrl_regs.Aurora_block.start_frame_cntr", 1);
                    fwInterface->WriteReg("user.ctrl_regs.Aurora_block.start_frame_cntr", 0);

                    float elapsedSeconds = 0.;
                    while((fwInterface->ReadReg("user.stat_regs.aurora_frame_cntr") < nFrames2Read) && (elapsedSeconds < timeLimit))
                    {
                        std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::DEEPSLEEP));
                        elapsedSeconds += RD53Shared::DEEPSLEEP * 1e-6;
                    }

                    vecFrameCounter.push_back(round(fwInterface->ReadReg("user.stat_regs.aurora_service_blk_cntr") * RD53Shared::PRECISION) / RD53Shared::PRECISION);
                }

                // ########################
                // # Find best TAP0 value #
                // ########################
                auto it       = std::max_element(vecFrameCounter.begin(), vecFrameCounter.end());
                auto maxRange = std::equal_range(it, vecFrameCounter.end(), *it);
                it += (maxRange.second - maxRange.first) / 2;
                auto bestTAP0 = vecTAP0Values[it - vecFrameCounter.begin()];

                RD53Interface::WriteChipReg(cChip, "EnServiceData", 0, false);
                RD53Interface::WriteChipReg(cChip, "DAC_CML_BIAS_0", bestTAP0, false);
                if(bestTAP0 != 0)
                    LOG(INFO) << BOLDBLUE << "\t--> Best " << BOLDYELLOW << "TAP0" << BOLDBLUE << " setting is: " << BOLDYELLOW << +bestTAP0 << RESET;
                else
                    LOG(INFO) << BOLDRED << "\t--> Best " << BOLDYELLOW << "TAP0" << BOLDBLUE << " not found" << RESET;
            }
        }

    // #######################
    // # Enable Service Data #
    // #######################
    RD53Interface::WriteBoardBroadcastChipReg(pBoard, "EnServiceData", 1);
}

std::vector<std::pair<uint16_t, uint16_t>> RD53BInterface::ReadRD53Reg(ReadoutChip* pChip, const std::string& regName)
{
    this->setBoard(pChip->getBeBoardId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);

    auto nameAndValue(SetSpecialRegister(regName, 0, pChip->getRegMap()));
    RD53Interface::SendCommand(pChip, RD53BCmd::RdReg{pChip->getId(), pChip->getRegItem(nameAndValue.first).fAddress});
    auto regReadback = static_cast<RD53FWInterface*>(fBoardFW)->ReadChipRegisters(pChip);

    for(auto i = 0u; i < regReadback.size(); i++)
    {
        regReadback[i].first  = regReadback[i].first & static_cast<uint16_t>(RD53Shared::setBits(RD53Constants::NBIT_ADDR)); // Removing bit related to PIX_PORTAL register identification
        regReadback[i].second = RD53BInterface::GetSpecialRegisterValue(regName, regReadback[i].second, pChip->getRegMap());
    }

    return regReadback;
}

std::pair<std::string, uint16_t> RD53BInterface::SetSpecialRegister(std::string regName, uint16_t value, ChipRegMap& pRD53RegMap)
{
    auto it = RD53B::specialRegMap.find(regName);
    if(it == RD53B::specialRegMap.end())
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

uint16_t RD53BInterface::GetSpecialRegisterValue(std::string regName, uint16_t value, ChipRegMap& pRD53RegMap)
{
    auto it = RD53B::specialRegMap.find(regName);
    if(it == RD53B::specialRegMap.end())
        return value;
    else
    {
        ChipRegItem& specialReg = pRD53RegMap.at(regName);
        return RD53Interface::GetFieldValue(value, it->second.start, specialReg.fBitSize);
    }
}

uint16_t RD53BInterface::GetPixelConfig(const pixelMask& mask, uint16_t row, uint16_t col)
{
    return bits::pack<8, 8>(
        bits::pack<5, 1, 1, 1>(
            mask.TDAC[row + RD53B::NROWS * (col + 1)], mask.HitBus[row + RD53B::NROWS * (col + 1)], mask.InjEn[row + RD53B::NROWS * (col + 1)], mask.Enable[row + RD53B::NROWS * (col + 1)]),
        bits::pack<5, 1, 1, 1>(
            mask.TDAC[row + RD53B::NROWS * (col + 0)], mask.HitBus[row + RD53B::NROWS * (col + 0)], mask.InjEn[row + RD53B::NROWS * (col + 0)], mask.Enable[row + RD53B::NROWS * (col + 0)]));
}

uint16_t RD53BInterface::GetPixelConfigMask(const pixelMask& mask, uint16_t row, uint16_t col)
{
    return bits::pack<5, 5>(bits::pack<2, 1, 1, 1>(0, mask.HitBus[row + RD53B::NROWS * (col + 1)], mask.InjEn[row + RD53B::NROWS * (col + 1)], mask.Enable[row + RD53B::NROWS * (col + 1)]),
                            bits::pack<2, 1, 1, 1>(0, mask.HitBus[row + RD53B::NROWS * (col + 0)], mask.InjEn[row + RD53B::NROWS * (col + 0)], mask.Enable[row + RD53B::NROWS * (col + 0)]));
}

uint16_t RD53BInterface::GetPixelConfigTDAC(const pixelMask& mask, uint16_t row, uint16_t col)
{
    return bits::pack<5, 5>(mask.TDAC[row + RD53B::NROWS * (col + 1)], mask.TDAC[row + RD53B::NROWS * (col + 0)]);
}

void RD53BInterface::ResetCoreColumns(RD53* pRD53)
// #############################################################################
// # This function causes a fluctuation of the current consumption of the chip #
// #############################################################################
{
    for(auto suffix: {"_0", "_1", "_2"})
    {
        for(int i = 0; i < 2; i++)
        {
            const uint16_t value = 0x5555 << i;

            RD53Interface::WriteChipReg(pRD53, std::string("EN_CORE_COL_RESET") + suffix, value, false);
            RD53Interface::WriteChipReg(pRD53, std::string("EN_CORE_COL") + suffix, value, false);
            RD53Interface::SendCommand(pRD53, RD53BCmd::Clear{pRD53->getId()});
        }
        RD53Interface::WriteChipReg(pRD53, std::string("EN_CORE_COL_RESET") + suffix, 0, false);
        RD53Interface::WriteChipReg(pRD53, std::string("EN_CORE_COL") + suffix, 0, false);
    }

    RD53Interface::WriteChipReg(pRD53, "EN_CORE_COL_RESET_3", 0x3F, false);
    RD53Interface::WriteChipReg(pRD53, "EN_CORE_COL_3", 0x3F, false);
    RD53Interface::SendCommand(pRD53, RD53BCmd::Clear{pRD53->getId()});

    RD53Interface::WriteChipReg(pRD53, "EN_CORE_COL_RESET_3", 0, false);
    RD53Interface::WriteChipReg(pRD53, "EN_CORE_COL_3", 0, false);
}

void RD53BInterface::WriteRD53Mask(RD53* pRD53, int writeMode, bool doDefault, size_t theRow, size_t theCol)
// ##################################
// # wireMode = 0 --> all pixels    #
// # wireMode = 1 --> sparse pixels #
// # wireMode = 2 --> single pixel  #
// ##################################
{
    this->setBoard(pRD53->getBeBoardId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);

    std::vector<uint16_t> commandList;
    const uint16_t        REGION_COL_ADDR = pRD53->getRegItem("REGION_COL").fAddress;
    const uint16_t        REGION_ROW_ADDR = pRD53->getRegItem("REGION_ROW").fAddress;
    const uint16_t        PIX_MODE_ADDR   = pRD53->getRegItem("PIX_MODE").fAddress;
    const uint16_t        PIX_PORTAL_ADDR = pRD53->getRegItem("PIX_PORTAL").fAddress;
    const uint8_t         chipID          = pRD53->getId();
    auto&                 mask            = doDefault == true ? pRD53->getPixelsMaskDefault() : pRD53->getPixelsMask();

    // ############
    // # PIX_MODE #
    // ############
    // bit[0]: enable auto-row
    // bit[1]: select mask(0) or TDAC(1)
    // bit[2]: enable broadcast

    // ########################
    // # Save original status #
    // ########################
    auto pixMode = pRD53->getRegMap().find("PIX_MODE")->second.fValue;

    if(writeMode == 0)
    {
        for(auto col = 0u; col < RD53B::NCOLS; col += 2)
        {
            // #######################
            // # Starting pixel cell #
            // #######################
            RD53BCmd::serialize(RD53BCmd::WrReg{chipID, REGION_COL_ADDR, col / 2}, commandList);

            // ####################
            // # Send pixels mask #
            // ####################
            std::vector<uint16_t> dColConfigMask;
            RD53BCmd::serialize(RD53BCmd::WrReg{chipID, REGION_ROW_ADDR, 0x0}, commandList);
            for(auto row = 0u; row < RD53B::NROWS; row++) dColConfigMask.push_back(RD53BInterface::GetPixelConfigMask(mask, row, col));

            RD53BCmd::serialize(RD53BCmd::WrReg{chipID, PIX_MODE_ADDR, 0x1}, commandList);
            RD53BCmd::serialize(RD53BCmd::WrRegLong{chipID, std::move(dColConfigMask)}, commandList);

            // ####################
            // # Send pixels TDAC #
            // ####################
            std::vector<uint16_t> dColConfigTDAC;
            RD53BCmd::serialize(RD53BCmd::WrReg{chipID, REGION_ROW_ADDR, 0x0}, commandList);
            for(auto row = 0u; row < RD53B::NROWS; row++) dColConfigTDAC.push_back(RD53BInterface::GetPixelConfigTDAC(mask, row, col));

            RD53BCmd::serialize(RD53BCmd::WrReg{chipID, PIX_MODE_ADDR, 0x3}, commandList);
            RD53BCmd::serialize(RD53BCmd::WrRegLong{chipID, std::move(dColConfigTDAC)}, commandList);
        }
    }
    else if(writeMode == 1)
    {
        // ############################
        // # Clear whole pixel matrix #
        // ############################
        for(auto col = 0u; col < RD53Constants::NROW_CORE; col += 2)
        {
            std::vector<uint16_t> dColConfigMask(RD53B::NROWS, 0x0);
            RD53BCmd::serialize(RD53BCmd::WrReg{chipID, REGION_COL_ADDR, col / 2}, commandList);
            RD53BCmd::serialize(RD53BCmd::WrReg{chipID, REGION_ROW_ADDR, 0x0}, commandList);
            RD53BCmd::serialize(RD53BCmd::WrReg{chipID, PIX_MODE_ADDR, 0x5}, commandList);
            RD53BCmd::serialize(RD53BCmd::WrRegLong{chipID, std::move(dColConfigMask)}, commandList);
        }
        RD53BCmd::serialize(RD53BCmd::WrReg{chipID, PIX_MODE_ADDR, 0x0}, commandList);

        for(auto col = 0u; col < RD53B::NCOLS; col += 2)
        {
            if((std::find(mask.Enable.begin() + (0 + RD53B::NROWS * col), mask.Enable.begin() + (RD53B::NROWS + RD53B::NROWS * col), true) ==
                (mask.Enable.begin() + (RD53B::NROWS + RD53B::NROWS * col))) &&
               (std::find(mask.Enable.begin() + (0 + RD53B::NROWS * (col + 1)), mask.Enable.begin() + (RD53B::NROWS + RD53B::NROWS * (col + 1)), true) ==
                (mask.Enable.begin() + (RD53B::NROWS + RD53B::NROWS * (col + 1)))) &&
               (std::find(mask.InjEn.begin() + (0 + RD53B::NROWS * col), mask.InjEn.begin() + (RD53B::NROWS + RD53B::NROWS * col), true) ==
                (mask.InjEn.begin() + (RD53B::NROWS + RD53B::NROWS * col))) &&
               (std::find(mask.InjEn.begin() + (0 + RD53B::NROWS * (col + 1)), mask.InjEn.begin() + (RD53B::NROWS + RD53B::NROWS * (col + 1)), true) ==
                (mask.InjEn.begin() + (RD53B::NROWS + RD53B::NROWS * (col + 1)))))
                continue;

            RD53BCmd::serialize(RD53BCmd::WrReg{chipID, REGION_COL_ADDR, col / 2}, commandList);

            for(auto row = 0u; row < RD53B::NROWS; row++)
                if((mask.Enable[row + RD53B::NROWS * col] == true) || (mask.Enable[row + RD53B::NROWS * (col + 1)] == true) || (mask.InjEn[row + RD53B::NROWS * col] == true) ||
                   (mask.InjEn[row + RD53B::NROWS * (col + 1)] == true))
                {
                    auto data = RD53BInterface::GetPixelConfig(mask, row, col);

                    RD53BCmd::serialize(RD53BCmd::WrReg{chipID, REGION_ROW_ADDR, row}, commandList);
                    RD53BCmd::serialize(RD53BCmd::WrReg{chipID, PIX_PORTAL_ADDR, data}, commandList);
                }
        }
    }
    else
    {
        auto data = RD53BInterface::GetPixelConfig(mask, theRow, theCol);

        RD53BCmd::serialize(RD53BCmd::WrReg{chipID, PIX_MODE_ADDR, 0x0}, commandList);
        RD53BCmd::serialize(RD53BCmd::WrReg{chipID, REGION_COL_ADDR, theCol / 2}, commandList);
        RD53BCmd::serialize(RD53BCmd::WrReg{chipID, REGION_ROW_ADDR, theRow}, commandList);
        RD53BCmd::serialize(RD53BCmd::WrReg{chipID, PIX_PORTAL_ADDR, data}, commandList);
    }

    RD53BInterface::SendChipCommandsWithSync(pRD53, commandList);

    // ###########################
    // # Restore original status #
    // ###########################
    RD53Interface::WriteChipReg(pRD53, "PIX_MODE", pixMode);
}

void RD53BInterface::SendChipCommandsWithSync(RD53* pRD53, const std::vector<uint16_t>& cmdStream)
{
    // #################################################################################################################################################################
    // # Compute number of 16-bit words to which we add NSYNC_WORDS sync words every RD53Constants::NWORDS_TO_SYNC:                                                    #
    // # nWordsPerPacketExclSync + 2 * nWordsPerPacketExclSync / RD53Constants::NWORDS_TO_SYNC = totaNumb16bitWords ( = 2 * (1 << RD53FWconstants::NBIT_SLOWCMD_FIFO)) #
    // #################################################################################################################################################################
    constexpr size_t nWordsPerPacketExclSync = 2 * ((1 << RD53FWconstants::NBIT_SLOWCMD_FIFO) - 1) / (1 + 2. / RD53Constants::NWORDS_TO_SYNC);
    auto             begin                   = cmdStream.begin();

    while(begin != cmdStream.end())
    {
        size_t                nWordsThisPacketExclSync = std::min(nWordsPerPacketExclSync, size_t(cmdStream.end() - begin));
        std::vector<uint16_t> cmdPacket;
        cmdPacket.reserve(std::ceil(nWordsThisPacketExclSync + 2. * nWordsThisPacketExclSync / RD53Constants::NWORDS_TO_SYNC));

        auto it = begin;
        while(it != begin + nWordsThisPacketExclSync)
        {
            auto next = std::min(cmdStream.end(), std::min(it + RD53Constants::NWORDS_TO_SYNC, begin + nWordsThisPacketExclSync));
            std::copy(it, next, std::back_inserter(cmdPacket));
            it = next;
            for(auto i = 0; i < RD53Constants::NSYNC_WORDS_S; i++) RD53BCmd::serialize(RD53BCmd::Sync{}, cmdPacket);
        }

        static_cast<RD53FWInterface*>(fBoardFW)->WriteChipCommands(cmdPacket, pRD53->getHybridId());
        begin = it;
    }
}

void RD53BInterface::PackWriteCommand(Chip* pChip, const std::string& regName, uint16_t data, std::vector<uint16_t>& chipCommandList, bool updateReg)
{
    RD53BCmd::serialize(RD53BCmd::WrReg{pChip->getId(), pChip->getRegItem(regName).fAddress, data}, chipCommandList);

    if(updateReg == true) pChip->setReg(regName, data);
}

void RD53BInterface::PackWriteBroadcastCommand(const BeBoard* pBoard, const std::string& regName, uint16_t data, std::vector<uint16_t>& chipCommandList, bool updateReg)
{
    RD53BCmd::serialize(RD53BCmd::WrReg{RD53Shared::firstChip->getFEtype()->broadcastChipId, RD53Shared::firstChip->getRegItem(regName).fAddress, data}, chipCommandList);

    if(updateReg == true)
        for(auto cOpticalGroup: *pBoard)
            for(auto cHybrid: *cOpticalGroup)
                for(auto cChip: *cHybrid) cChip->setReg(regName, data);
}

void RD53BInterface::WriteClockDataDelay(Chip* pChip, uint16_t value)
{
    this->setBoard(pChip->getBeBoardId());

    RD53Interface::WriteChipReg(pChip, "CLK_DATA_DELAY", value, false);
    static_cast<RD53FWInterface*>(fBoardFW)->WriteChipCommands(std::vector<uint16_t>(RD53Constants::NSYNC_WORDS_L, RD53BCmd::RD53BCmdEncoder::SYNC), -1);
    RD53Interface::WriteChipReg(pChip, "CLK_DATA_DELAY", value, true);
}

uint32_t RD53BInterface::ReadChipFuseID(Chip* pChip, uint8_t version)
{
    this->setBoard(pChip->getBeBoardId());

    bool     status    = RD53Interface::WriteChipReg(pChip, "EfusesConfig", 0x0F0F);
    uint16_t low       = RD53Interface::ReadChipReg(pChip, "EfusesReadData0");
    uint16_t high      = RD53Interface::ReadChipReg(pChip, "EfusesReadData1");
    uint32_t eFuseCode = low | (high << pChip->getNumberOfBits("EfusesReadData0"));

    if(static_cast<RD53*>(pChip)->geteFuseCode() < 0)
    {
        std::stringstream myString;
        myString << eFuseCode;
        throw std::out_of_range(myString.str().c_str());
    }
    else if(status == false)
    {
        std::error_code e{6, std::system_category()};
        throw std::system_error(e, "Problem reading chip e-fuse code");
    }
    else if(eFuseCode != static_cast<RD53*>(pChip)->geteFuseCode())
    {
        std::stringstream myString;
        myString << "Readout chip e-fuse code " << eFuseCode << " does not match value in xml file " << +static_cast<RD53*>(pChip)->geteFuseCode();
        throw std::runtime_error(myString.str());
    }

    return eFuseCode;
}

void RD53BInterface::SendBoardClear(const BeBoard* pBoard)
{
    this->setBoard(pBoard->getId());

    if(RD53Shared::firstChip->getFrontEndType() == FrontEndType::RD53Bv1)
        static_cast<RD53FWInterface*>(fBoardFW)->WriteChipCommands(serialize(RD53BCmd::Clear{RD53Shared::firstChip->getFEtype()->broadcastChipId}), -1);
    else
        RD53BInterface::SendGlobalPulseBroadcast(pBoard);
}

void RD53BInterface::SendGlobalPulse(Chip* pChip, uint16_t route, uint16_t pulseDuration)
{
    this->setBoard(pChip->getBeBoardId());

    std::vector<uint16_t> cmdStream;
    auto                  pRD53  = static_cast<RD53*>(pChip);
    const auto&           theMap = pRD53->getFEtype()->GlobalPulseConfMap;

    RD53BInterface::PackWriteCommand(pChip, "GlobalPulseConf", route, cmdStream);
    RD53BInterface::PackWriteCommand(pChip, "GlobalPulseWidth", pulseDuration, cmdStream);
    RD53BCmd::serialize(RD53BCmd::GlobalPulse{pChip->getId()}, cmdStream);
    RD53BInterface::PackWriteCommand(pChip,
                                     "GlobalPulseConf",
                                     (theMap.find("RstAuroraV1") != theMap.end() ? theMap.find("RstAuroraV1")->second : 0) |
                                         (theMap.find("RstSerializerV1") != theMap.end() ? theMap.find("RstSerializerV1")->second : 0) |
                                         (theMap.find("RstBCIDCnt") != theMap.end() ? theMap.find("RstBCIDCnt")->second : 0),
                                     cmdStream);
    static_cast<RD53FWInterface*>(fBoardFW)->WriteChipCommands(cmdStream, pChip->getHybridId());

    std::this_thread::sleep_for(std::chrono::nanoseconds(static_cast<int>((pulseDuration + 1.) / RD53Constants::ACCELERATOR_CLK * 1000.)));
}

void RD53BInterface::SendGlobalPulseBroadcast(const BeBoard* pBoard)
{
    this->setBoard(pBoard->getId());

    static_cast<RD53FWInterface*>(fBoardFW)->WriteChipCommands(serialize(RD53BCmd::GlobalPulse{RD53Shared::firstChip->getFEtype()->broadcastChipId}), -1);

    std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::DEEPSLEEP));
}

// ###########################
// # Dedicated to monitoring #
// ###########################

int RD53BInterface::getADCobservable(const std::string& observableName, bool& isCurrentNotVoltage, bool silentRunning)
// ############################################
// # Possible observable name values are also #
// # - INTERNAL_NTC                           #
// # - INTERNAL_NTC_VOLT                      #
// ############################################
{
    uint32_t voltageObservable(0), currentObservable(0);

    const std::unordered_map<std::string, uint32_t> currentMultiplexer = {{"Iref", 0x00},
                                                                          {"CDR_VCO_MAIN", 0x01},
                                                                          {"CDR_VCO_BUFFER", 0x02},
                                                                          {"CDR_CP_CURR", 0x03},
                                                                          {"CDR_CP_FD", 0x04},
                                                                          {"CDR_CP_BUFFER", 0x05},
                                                                          {"CML_DRIVER_TAP2_BIAS", 0x06},
                                                                          {"CML_DRIVER_TAP1_BIAS", 0x07},
                                                                          {"CML_DRIVER_MAIN", 0x08},
                                                                          {"NTC_CURR", 0x09},
                                                                          {"CAP_MEASURE_CIRC", 0x0A},
                                                                          {"CAP_MEASURE_PARASITIC", 0x0B},
                                                                          {"LIN_FE_PREAMP_MAIN", 0x0C},
                                                                          {"LIN_FE_COMPS_TAR", 0x0D},
                                                                          {"LIN_FE_COMPARATOR", 0x0E},
                                                                          {"LIN_FE_LDAC", 0x0F},
                                                                          {"LIN_FE_FC", 0x10},
                                                                          {"LIN_FE_KRUMCURR", 0x11},
                                                                          {"LIN_FE_PREAMP_LEFT", 0x13},
                                                                          {"LIN_FE_PREAMP_RIGHT", 0x15},
                                                                          {"LIN_FE_PREAMP_TOP_LEFT", 0x16},
                                                                          {"LIN_FE_PREAMP_TOP", 0x18},
                                                                          {"LIN_FE_PREAMP_TOP_RIGHT", 0x19},
                                                                          {"ANA_IN_CURR", 0x1C},
                                                                          {"ANA_SHUNT_CURR", 0x1D},
                                                                          {"DIG_IN_CURR", 0x1E},
                                                                          {"DIG_SHUNT_CURR", 0x1F}};

    const std::unordered_map<std::string, uint32_t> voltageMultiplexer = {{"Vref_ADC", 0x00},
                                                                          {"I_MUX", 0x01},
                                                                          {"NTC_VOLT", 0x02},
                                                                          {"Vref_CAL_DAC", 0x03},
                                                                          {"VDDA_CAPMEASURE", 0x04},
                                                                          {"POLY_TEMPSENS_TOP", 0x05},
                                                                          {"POLY_TEMPSENS_BOTTOM", 0x06},
                                                                          {"VCAL_HI", 0x07},
                                                                          {"VCAL_MD", 0x08},
                                                                          {"LIN_FE_REF_KRUMCURR", 0x09},
                                                                          {"LIN_FE_GDAC_MAIN", 0x0A},
                                                                          {"LIN_FE_GDAC_LEFT", 0x0B},
                                                                          {"LIN_FE_GDAC_RIGHT", 0x0C},
                                                                          {"RADSENS_ANA_SLDO", 0x0D},
                                                                          {"TEMPSENS_ANA_SLDO", 0x0E},
                                                                          {"RADSENS_DIG_SLDO", 0x0F},
                                                                          {"TEMPSENS_DIG_SLDO", 0x10},
                                                                          {"RADSENS_CENTER", 0x11},
                                                                          {"TEMPSENS_CENTER", 0x12},
                                                                          {"ANA_GND_0", 0x13},
                                                                          {"ANA_GND_1", 0x14},
                                                                          {"ANA_GND_2", 0x15},
                                                                          {"ANA_GND_3", 0x16},
                                                                          {"ANA_GND_4", 0x17},
                                                                          {"ANA_GND_5", 0x18},
                                                                          {"ANA_GND_6", 0x19},
                                                                          {"ANA_GND_7", 0x1A},
                                                                          {"ANA_GND_8", 0x1B},
                                                                          {"ANA_GND_9", 0x1C},
                                                                          {"ANA_GND_10", 0x1D},
                                                                          {"ANA_GND_11", 0x1E},
                                                                          {"Vref_CORE", 0x1F},
                                                                          {"Vref_PRE", 0x20},
                                                                          {"VINA", 0x21},
                                                                          {"VDDA", 0x22},
                                                                          {"VrefA", 0x23},
                                                                          {"VOFS", 0x24},
                                                                          {"VIND", 0x25},
                                                                          {"VDDD", 0x26},
                                                                          {"VrefD", 0x27}};

    auto search = currentMultiplexer.find(observableName);
    if(observableName == "INTERNAL_NTC")
    {
        currentObservable   = currentMultiplexer.find("NTC_CURR")->second;
        voltageObservable   = voltageMultiplexer.find("I_MUX")->second;
        isCurrentNotVoltage = true;
    }
    else if(observableName == "INTERNAL_NTC_VOLT")
    {
        voltageObservable   = voltageMultiplexer.find("NTC_VOLT")->second;
        isCurrentNotVoltage = false;
    }
    else if(search == currentMultiplexer.end())
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
        voltageObservable   = voltageMultiplexer.find("I_MUX")->second;
        isCurrentNotVoltage = true;
    }

    return bits::pack<1, 6, 6>(true, currentObservable, voltageObservable);
}

uint32_t RD53BInterface::measureADC(ReadoutChip* pChip, uint32_t data)
{
    this->setBoard(pChip->getBeBoardId());
    auto        pRD53  = static_cast<RD53*>(pChip);
    const auto& theMap = pRD53->getFEtype()->GlobalPulseConfMap;

    const uint16_t sampleNtimes  = pChip->getRegItem("SAMPLE_N_TIMES").fValue;
    const uint16_t waitMuxConfig = pChip->getRegItem("WAIT_MUX_CONFIG").fValue; // [ms]
    const uint16_t GlbPulseVal   = RD53Interface::ReadChipReg(pChip, "GlobalPulseConf");
    const uint16_t GlbPulseWidth = RD53Interface::ReadChipReg(pChip, "GlobalPulseWidth");

    RD53Interface::WriteChipReg(pChip, "MonitorConfig", 1 << 12 | data, false); // 13 bits: bit 12 enable, bits 6:11 I-Mon, bits 0:5 V-Mon
    // After the muxes have been configured, some time has to pass before the voltage is stable (RC circuit)
    // The amount of time depends on the particular signal and on the capacitance connected to VMUX/IMUX
    // 100 ms should be enough to properly sample all voltages from VMUX on UZH SCCs and on modules (22 nF)
    // On Bonn SCCs (100 nF), 100 ms are too short for RADSENS, and should be raised to 500 ms
    std::this_thread::sleep_for(std::chrono::milliseconds(waitMuxConfig));

    // ########################################################
    // # Sample data multiple times for better value estimate #
    // ########################################################
    uint32_t avgVal  = 0;
    uint16_t counter = 0;
    for(auto i = 0u; i < sampleNtimes; i++)
    {
        RD53BInterface::SendGlobalPulse(pChip, theMap.find("RstADC") != theMap.end() ? theMap.find("RstADC")->second : 0, 1);
        RD53BInterface::SendGlobalPulse(pChip, theMap.find("ADCstatConversion") != theMap.end() ? theMap.find("ADCstatConversion")->second : 0, 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Wait for end of conversion (at least 358.4 us according to manual)
        const uint16_t val = RD53Interface::ReadChipReg(pChip, "MonitoringDataADC");
        if(val != 0)
        {
            avgVal += val;
            counter++;
        }
    }
    avgVal /= (counter != 0 ? counter : 1);

    RD53Interface::WriteChipReg(pChip, "MonitorConfig", 0, false);         // Stop monitoring
    RD53Interface::WriteChipReg(pChip, "GlobalPulseConf", GlbPulseVal);    // Restore value in Global Pulse Route
    RD53Interface::WriteChipReg(pChip, "GlobalPulseWidth", GlbPulseWidth); // Restore value in Global Pulse Width

    return avgVal;
}

float RD53BInterface::measureTemperature(ReadoutChip* pChip, uint32_t data, const std::string& type, int beta)
{
    // ################################################################################################
    // #                     TEMPERATURE MEASUREMENT FOR TRANSISTOR-BASED SENSORS                     #
    // # Temperature measurement is done by measuring twice, once with high bias, once with low bias  #
    // # Temperature is calculated based on the difference of the two, with the formula on the bottom #
    // ################################################################################################

    this->setBoard(pChip->getBeBoardId());

    // #####################
    // # Natural constants #
    // #####################
    const float       T0C              = 273.15;                                      // [Kelvin]
    const float       T25C             = 298.15;                                      // [Kelvin]
    const float       R25C             = pChip->getRegItem("RNTCAT25C").fValue / 1e3; // [Ohm]
    const float       kb               = 1.38064852e-23;                              // [J/K]
    const float       e                = 1.6021766208e-19;                            // [C]
    const float       temperatureCoeff = 0.22e-2;                                     // By circuit design [dR/dT]
    const float       biasIratio       = 15;                                          // By circuit design
    const int         nDEM             = 16;                                          // Dynamic Element Matching
    const std::string regName          = (type.find("CENTER") != std::string::npos ? "MON_SENS_ACB" : "MON_SENS_SLDO");

    const std::unordered_map<std::string, std::string> observableToCalibrationConstant = {
        {"TEMPSENS_ANA_SLDO", "TEMPSENS_IDEAL_FACTOR_ANA"},
        {"TEMPSENS_DIG_SLDO", "TEMPSENS_IDEAL_FACTOR_DIG"},
        {"TEMPSENS_CENTER", "TEMPSENS_IDEAL_FACTOR"},
        {"RADSENS_ANA_SLDO", "RADSENS_IDEAL_FACTOR_ANA"},
        {"RADSENS_DIG_SLDO", "RADSENS_IDEAL_FACTOR_DIG"},
        {"RADSENS_CENTER", "RADSENS_IDEAL_FACTOR"},
        {"POLY_TEMPSENS_TOP", "TEMPSENS_OFFSET_TOP"},
        {"POLY_TEMPSENS_BOTTOM", "TEMPSENS_OFFSET_BOTTOM"},
        {"INTERNAL_NTC_VOLT", ""},
        {"INTERNAL_NTC", ""},
    };

    const auto iterator = observableToCalibrationConstant.find(type);
    if(iterator == observableToCalibrationConstant.end())
    {
        LOG(ERROR) << BOLDRED << "Invalid temperature sensor: " << BOLDYELLOW << type << RESET;
        return -HUGE_VALF; // Unphysically low temperature as error
    }

    const float idealityFactor = (iterator->second != "" ? pChip->getRegItem(iterator->second).fValue / 1e3 : 0);
    uint16_t    sensorConfigData; // Enable[5], DEM[4:1], SEL_BIAS[0] (x2 ... 10 bit in total for the sensors in each sensor config register)
    float       valueLow  = 0;
    float       valueHigh = 0;

    if(type.find("INTERNAL_NTC") != std::string::npos)
    {
        bool     isCurrentNotVoltage;
        uint32_t observable = RD53BInterface::getADCobservable("INTERNAL_NTC_VOLT", isCurrentNotVoltage);
        float    voltage    = RD53Interface::convertADC2VorI(pChip, RD53BInterface::measureADC(pChip, observable));
        observable          = RD53BInterface::getADCobservable("INTERNAL_NTC", isCurrentNotVoltage);
        float current       = RD53Interface::convertADC2VorI(pChip, RD53BInterface::measureADC(pChip, observable), true);

        // ###############################################
        // # Calculate temperature with NTC Beta formula #
        // ###############################################
        float resistance  = 1e3 * voltage / (current != 0 ? current : 1);           // [kOhm]
        float temperature = 1. / (1. / T25C + log(resistance / R25C) / beta) - T0C; // [Celsius]

        return temperature;
    }
    else if(type.find("POLY") != std::string::npos)
    {
        float voltage     = RD53Interface::convertADC2VorI(pChip, measureADC(pChip, data));
        float temperature = (voltage / idealityFactor - 1) / temperatureCoeff; // [Celsius]

        return temperature;
    }

    for(auto sensorDEM = 0; sensorDEM < nDEM; sensorDEM++)
    {
        // #########################
        // # Get high bias voltage #
        // #########################
        sensorConfigData = bits::pack<1, 4, 1>(true, sensorDEM, 0) << (type.find("DIG") != std::string::npos ? 6 : 0);
        RD53Interface::WriteChipReg(pChip, regName, sensorConfigData);
        valueLow += RD53Interface::convertADC2VorI(pChip, RD53BInterface::measureADC(pChip, data));

        // ########################
        // # Get low bias voltage #
        // ########################
        sensorConfigData = bits::pack<1, 4, 1>(true, sensorDEM, 1) << (type.find("DIG") != std::string::npos ? 6 : 0);
        RD53Interface::WriteChipReg(pChip, regName, sensorConfigData);
        valueHigh += RD53Interface::convertADC2VorI(pChip, RD53BInterface::measureADC(pChip, data));
    }

    // ####################
    // # Turn off sensing #
    // ####################
    RD53Interface::WriteChipReg(pChip, "MON_SENS_ACB", 0);
    RD53Interface::WriteChipReg(pChip, "MON_SENS_SLDO", 0);

    return e / (idealityFactor * kb * log(biasIratio)) * (valueHigh - valueLow) / nDEM - T0C;
}

} // namespace Ph2_HwInterface
