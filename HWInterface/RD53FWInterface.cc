/*!
  \file                  RD53FWInterface.h
  \brief                 RD53FWInterface to initialize and configure the FW
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#include "HWInterface/RD53FWInterface.h"
#include "HWDescription/BeBoard.h"
#include "HWDescription/BeBoardRegItem.h"
#include "HWInterface/RD53Interface.h"

using namespace Ph2_HwDescription;

namespace Ph2_HwInterface
{
const std::array<std::string, 8> RD53FWInterface::FastCommandsConfig::fastCmdWhiteList = {"user.ctrl_regs.fast_cmd_reg_2.trigger_source",
                                                                                          "user.ctrl_regs.fast_cmd_reg_2.init_ecr_en",
                                                                                          "user.ctrl_regs.fast_cmd_reg_2.backpressure_en",
                                                                                          "user.ctrl_regs.fast_cmd_reg_2.veto_en",
                                                                                          "user.ctrl_regs.fast_cmd_reg_3.triggers_to_accept",
                                                                                          "user.ctrl_regs.fast_cmd_reg_2.ext_trig_delay",
                                                                                          "user.ctrl_regs.fast_cmd_reg_2.trigger_duration",
                                                                                          "user.ctrl_regs.fast_cmd_reg_2.HitOr_enable_l12"}; // @CONST@

RD53FWInterface::RD53FWInterface(const std::string& pId, const std::string& pUri, const std::string& pAddressTable, BeBoard* theBoard)
    : BeBoardFWInterface(pId, pUri, pAddressTable, theBoard), ddr3Offset(0), FWinfo(0)
{
}

void RD53FWInterface::setFileHandler(FileHandler* pHandler)
{
    if(pHandler != nullptr)
    {
        this->fFileHandler = pHandler;
        this->fSaveToFile  = true;
    }
    else
        LOG(ERROR) << BOLDRED << "NULL FileHandler" << RESET;
}

void RD53FWInterface::ResetSequence(const std::string& refClockRate)
{
    LOG(INFO) << BOLDMAGENTA << "Resetting the backend board... it may take a while" << RESET;

    RD53FWInterface::TurnOffFMC();
    RD53FWInterface::TurnOnFMC();
    RD53FWInterface::ResetBoard();

    // ##############################
    // # Initialize clock generator #
    // ##############################
    RD53FWInterface::InitializeClockGenerator(refClockRate);

    // ###################################
    // # Reset optical link slow control #
    // ###################################
    RD53FWInterface::ResetOptoLinkSlowControl();

    // ######################
    // # Reset optical link #
    // ######################
    RD53FWInterface::ResetOptoLink();

    LOG(INFO) << BOLDMAGENTA << "Now you can start using the DAQ ... enjoy!" << RESET;
}

void RD53FWInterface::ConfigureBoard(const BeBoard* pBoard)
{
    const int clkSafeMargin = 1; // (Hz) @CONST@

    // ########################
    // # Print firmware infos #
    // ########################
    uint32_t cVersionMajor = RegManager::ReadReg("user.stat_regs.usr_ver.usr_ver_major");
    uint32_t cVersionMinor = RegManager::ReadReg("user.stat_regs.usr_ver.usr_ver_minor");
    this->FWinfo           = ((cVersionMajor << RD53FWconstants::NBIT_FWVER) | cVersionMinor);

    uint32_t cFWyear    = RegManager::ReadReg("user.stat_regs.fw_date.year");
    uint32_t cFWmonth   = RegManager::ReadReg("user.stat_regs.fw_date.month");
    uint32_t cFWday     = RegManager::ReadReg("user.stat_regs.fw_date.day");
    uint32_t cFWhour    = RegManager::ReadReg("user.stat_regs.fw_date.hour");
    uint32_t cFWminute  = RegManager::ReadReg("user.stat_regs.fw_date.minute");
    uint32_t cFWseconds = RegManager::ReadReg("user.stat_regs.fw_date.seconds");

    uint32_t cLinkType   = RegManager::ReadReg("user.stat_regs.global_reg.link_type");
    uint32_t cOptSpeed   = RegManager::ReadReg("user.stat_regs.global_reg.optical_speed");
    uint32_t cFEtype     = RegManager::ReadReg("user.stat_regs.global_reg.front_end_type");
    uint32_t cL12FMCtype = RegManager::ReadReg("user.stat_regs.global_reg.fmc_l12_type");
    uint32_t cL08FMCtype = RegManager::ReadReg("user.stat_regs.global_reg.fmc_l8_type");

    LOG(INFO) << BOLDBLUE << "\t--> SW commit number : " << BOLDYELLOW << RD53Shared::gitGitCommit() << RESET;
    LOG(INFO) << BOLDBLUE << "\t--> FW version : " << BOLDYELLOW << cVersionMajor << "." << cVersionMinor << BOLDBLUE << " -- Date (yy/mm/dd) : " << BOLDYELLOW << cFWyear << "/" << cFWmonth << "/"
              << cFWday << BOLDBLUE << " -- Time (hour:minute:sec) : " << BOLDYELLOW << cFWhour << ":" << cFWminute << ":" << cFWseconds << RESET;
    LOG(INFO) << BOLDBLUE << "\t--> Link type : " << BOLDYELLOW << (cLinkType == 0 ? "electrical" : "optical") << BOLDBLUE << " -- Optical speed : " << BOLDYELLOW
              << (cOptSpeed == 0 ? "10 Gbit/s" : "5 Gbit/s") << BOLDBLUE << " -- Frontend type : " << BOLDYELLOW << (cFEtype == 1 ? "RD53A" : "RD53B") << RESET;
    LOG(INFO) << BOLDBLUE << "\t--> L12 FMC type : " << BOLDYELLOW << cL12FMCtype << BOLDBLUE << " -- L08 FMC type : " << BOLDYELLOW << cL08FMCtype << BOLDBLUE
              << " (1=KSU, 2=CERN, 3=DIO5, 4=OPTO, 5=FERMI, 7=NONE, 0=Unspecified)" << RESET;

    if(cFEtype == 2) RegManager::WriteReg("user.ctrl_regs.reset_reg.enable_sync_word", 1);

    // #########################
    // # Set RD53 AURORA speed #
    // #########################
    RegManager::WriteStackReg({{"user.ctrl_regs.gtx_drp.aurora_speed", RD53FWconstants::AURORA_SPEED}, {"user.ctrl_regs.gtx_drp.set_aurora_speed", 1}, {"user.ctrl_regs.gtx_drp.set_aurora_speed", 0}});

    // ################
    // # Board resets #
    // ################
    RD53FWInterface::ResetFastCmdBlk();
    RD53FWInterface::ResetSlowCmdFIFO();
    RD53FWInterface::ResetReadBkFIFO();
    RD53FWInterface::ResetReadoutBlk();

    // ###############################################
    // # FW register initialization from config file #
    // ###############################################
    std::vector<std::pair<std::string, uint32_t>> cVecReg;

    // ##################
    // # Configure DIO5 #
    // ##################
    LOG(INFO) << GREEN << "Initializing DIO5" << RESET;
    RD53FWInterface::DIO5Config cfgDIO5;
    RD53FWInterface::ConfigureDIO5(pBoard, &cfgDIO5);
    RD53FWInterface::SendDIO5Cfg(&cfgDIO5);
    std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::DEEPSLEEP));
    LOG(INFO) << BOLDBLUE << "\t--> Done" << RESET;

    // #########################
    // # Configure DataMerging #
    // #########################
    size_t primaries[4]      = {0};
    size_t slaveEn           = 0;
    bool   enableDataMerging = false;
    bool   enableChipID      = false;
    for(const auto cOpticalGroup: *pBoard)
        for(const auto cHybrid: *cOpticalGroup)
            for(const auto cChip: *cHybrid)
            {
                // ###################################
                // # Check if DataMerging is enabled #
                // ###################################
                const auto lane = static_cast<RD53*>(cChip)->getChipLane();
                if(static_cast<RD53*>(cChip)->laneConfig.isPrimary == false)
                {
                    enableDataMerging = true;
                    slaveEn |= 1 << lane;
                    primaries[lane] = static_cast<RD53*>(cChip)->laneConfig.masterLane;
                }

                // ##############################
                // # Check if ChipID is enabled #
                // ##############################
                if(static_cast<RD53*>(cChip)->getDataFormatOptions().enableChipId == true) enableChipID = true;

                // ###############################
                // # Map chips into the firmware #
                // ###############################
                RegManager::WriteReg("user.ctrl_regs.i2c_block.chip" + std::to_string(lane) + "_id", cChip->getId() & 3);
                RegManager::WriteReg("user.ctrl_regs.i2c_block.chip" + std::to_string(lane) + "_primary", primaries[lane]);
            }

    RegManager::WriteStackReg(
        {{"user.ctrl_regs.Aurora_block.data_merging_en", enableDataMerging}, {"user.ctrl_regs.i2c_block.chip_id_en", enableChipID}, {"user.ctrl_regs.Aurora_block.slave_en", slaveEn}});

    // ################################
    // # Enabling hybrids and chips   #
    // # Hybrid_type hard coded in FW #
    // # 1 = single chip              #
    // # 2 = dual chip hybrid         #
    // # 4 = quad chip hybrid         #
    // ################################
    this->hybridType     = RegManager::ReadReg("user.stat_regs.aurora_rx.Hybrid_type");
    this->enabledHybrids = RD53FWInterface::GetBoardEnabledHybrids(pBoard);
    uint32_t chipsEn     = RD53FWInterface::GetBoardEnabledChips(pBoard);
    cVecReg.push_back({"user.ctrl_regs.Hybrids_en", this->enabledHybrids});
    cVecReg.push_back({"user.ctrl_regs.Chips_en", chipsEn});
    if(cVecReg.size() != 0) RegManager::WriteStackReg(cVecReg);

    // ########################
    // # Read clock generator #
    // ########################
    RD53FWInterface::ReadClockGenerator();

    // #########################################
    // # Read optical link slow control status #
    // #########################################
    uint32_t txIsReady = RegManager::ReadReg("user.stat_regs.lpgbt_sc_1.tx_ready");
    uint32_t rxIsReady = RegManager::ReadReg("user.stat_regs.lpgbt_sc_1.rx_empty");
    if(txIsReady == true)
        LOG(INFO) << GREEN << "Optical link tx slow control status: " << BOLDYELLOW << "ready" << RESET;
    else
        LOG(WARNING) << GREEN << "Optical link tx slow control status: " << BOLDRED << "not ready" << RESET;
    if(rxIsReady == true)
        LOG(INFO) << GREEN << "Optical link rx slow control status: " << BOLDYELLOW << "ready" << RESET;
    else
        LOG(WARNING) << GREEN << "Optical link rx slow control status: " << BOLDRED << "not ready" << RESET;

    // ###########################
    // # Check RD53 AURORA speed #
    // ###########################
    auto auroraSpeed = RD53FWInterface::ReadoutSpeed();
    LOG(INFO) << GREEN << "Aurora speed set to: " << BOLDYELLOW << (auroraSpeed == RD53FWconstants::ReadoutSpeed::x1280 ? "1.28 Gbit/s" : "640 Mbit/s") << RESET;

    // ###########################
    // # Print clock measurement #
    // ###########################
    uint32_t inputClk = RegManager::ReadReg("user.stat_regs.clkin_rate");
    uint32_t gtxClk   = RegManager::ReadReg("user.stat_regs.gtx_refclk_rate");
    LOG(INFO) << GREEN << std::fixed << std::setprecision(3) << "Input clock frequency (could be either internal or external, should be ~40 MHz): " << BOLDYELLOW << inputClk / 1000. << " MHz"
              << std::setprecision(-1) << RESET;
    if(fabs(inputClk / 1000. - 40) > clkSafeMargin)
    {
        LOG(ERROR) << BOLDRED << "Input clock frequency not nominal" << RESET;
        LOG(ERROR) << BOLDRED << "===== Aborting =====" << RESET;
        exit(EXIT_FAILURE);
    }
    LOG(INFO) << GREEN << std::fixed << std::setprecision(3) << "GTX receiver clock frequency (~160 MHz (~320 MHz) for electrical (optical) readout): " << BOLDYELLOW << gtxClk / 1000. << " MHz"
              << std::setprecision(-1) << RESET;
    if(!((fabs(gtxClk / 1000. - 160) < clkSafeMargin) || (fabs(gtxClk / 1000. - 320) < clkSafeMargin)))
    {
        LOG(ERROR) << BOLDRED << "GTX receiver clock frequency not nominal" << RESET;
        LOG(ERROR) << BOLDRED << "===== Aborting =====" << RESET;
        exit(EXIT_FAILURE);
    }

    // ##################
    // # Reset Metadata #
    // ##################
    RD53FWInterface::resetNcorruptedNevents();
    RD53FWInterface::resetNtrialsNevents();

    // #########################
    // # PortCard Test Adapter #
    // #########################
    if(pBoard->getEventType() == EventType::VRPCTestAdapter) RD53FWInterface::ConfigurePCTestAdapter(pBoard->getComment());
}

void RD53FWInterface::PrintFWstatus()
{
    LOG(INFO) << GREEN << "Checking firmware status:" << RESET;

    // #################################
    // # Check clock generator locking #
    // #################################
    if(RegManager::ReadReg("user.stat_regs.global_reg.clk_gen_lock") == 1)
        LOG(INFO) << BOLDBLUE << "\t--> Clock generator is " << BOLDYELLOW << "locked" << RESET;
    else
        LOG(ERROR) << BOLDRED << "\t--> Clock generator is not locked" << RESET;

    // ############################################################
    // # Check status registers associated wih fast command block #
    // ############################################################
    uint32_t fastCMDReg = RegManager::ReadReg("user.stat_regs.fast_cmd.trigger_source_o");
    LOG(INFO) << GREEN << "Fast command block trigger source: " << BOLDYELLOW << fastCMDReg << RESET << GREEN << " (1=IPBus, 2=Test-FSM, 3=TTC, 4=TLU, 5=External, 6=Hit-Or, 7=User-defined frequency)"
              << RESET;

    fastCMDReg = RegManager::ReadReg("user.stat_regs.fast_cmd.trigger_state");
    LOG(INFO) << GREEN << "Fast command block trigger state: " << BOLDYELLOW << fastCMDReg << RESET << GREEN << " (0=Idle, 2=Running)" << RESET;

    fastCMDReg = RegManager::ReadReg("user.stat_regs.fast_cmd.if_configured");
    LOG(INFO) << GREEN << "Fast command block check if configuraiton registers have been set: " << BOLDYELLOW << (fastCMDReg == true ? "configured" : "not configured") << RESET;

    fastCMDReg = RegManager::ReadReg("user.stat_regs.fast_cmd.error_code");
    LOG(INFO) << GREEN << "Fast command block error code (0=No error): " << BOLDYELLOW << fastCMDReg << RESET;

    // ###########################
    // # Check trigger registers #
    // ###########################
    uint32_t trigReg = RegManager::ReadReg("user.stat_regs.trigger_cntr");
    LOG(INFO) << GREEN << "Trigger counter: " << BOLDYELLOW << trigReg << RESET;

    // ##########################
    // # Check hybrid registers #
    // ##########################
    this->hybridType = RegManager::ReadReg("user.stat_regs.aurora_rx.Hybrid_type");
    LOG(INFO) << GREEN << "Hybrid type: " << BOLDYELLOW << +this->hybridType << RESET << GREEN " (1=Single chip, 2=Dual chip, 4=Quad chip)" << RESET;

    uint32_t hybrid = RegManager::ReadReg("user.stat_regs.aurora_rx.Nb_of_modules");
    LOG(INFO) << GREEN << "Number of hybrids which can be potentially readout: " << BOLDYELLOW << hybrid << RESET;
}

void RD53FWInterface::ConfigureFromXML(const BeBoard* pBoard)
{
    // ###############################################
    // # FW register initialization from config file #
    // ###############################################
    bool                                          gtxRxPolarity = false;
    bool                                          fastCmdReg1   = false;
    bool                                          extTluReg2    = false;
    bool                                          auroraSpeed   = false;
    std::vector<std::pair<std::string, uint32_t>> cVecReg;

    LOG(INFO) << GREEN << "Initializing board's registers" << RESET;

    for(const auto& it: pBoard->getBeBoardRegMap())
        if(it.second.fPrmptCfg == true)
        {
            LOG(INFO) << BOLDBLUE << "\t--> " << it.first << ": 0x" << BOLDYELLOW << std::hex << std::uppercase << it.second.fValue << std::dec << " (" << it.second.fValue << ")" << RESET;
            if(std::find(FastCommandsConfig::fastCmdWhiteList.begin(), FastCommandsConfig::fastCmdWhiteList.end(), it.first) == FastCommandsConfig::fastCmdWhiteList.end())
            {
                cVecReg.push_back({it.first, it.second.fValue});
                if(it.first.find("gtx_rx_polarity") != std::string::npos) gtxRxPolarity = true;
                if(it.first.find("fast_cmd_reg_1") != std::string::npos) fastCmdReg1 = true;
                if(it.first.find("ext_tlu_reg2") != std::string::npos) extTluReg2 = true;
                if(it.first.find("gtx_drp.aurora_speed") != std::string::npos) auroraSpeed = true;
            }
        }

    if(cVecReg.size() != 0)
    {
        RegManager::WriteStackReg(cVecReg);
        if(gtxRxPolarity == true) RD53FWInterface::ToggleRegister("user.ctrl_regs.gtx_rx_polarity.cmd_strobe");
        if(fastCmdReg1 == true) RD53FWInterface::SendBoardCommandWithStrobe("user.ctrl_regs.fast_cmd_reg_1.load_config");
        if(extTluReg2 == true) RD53FWInterface::ToggleRegister("user.ctrl_regs.ext_tlu_reg2.dio5_load_config");
        if(auroraSpeed == true) RD53FWInterface::ToggleRegister("user.ctrl_regs.gtx_drp.set_aurora_speed");
    }

    LOG(INFO) << BOLDBLUE << "\t--> Done" << RESET;
}

bool RD53FWInterface::WriteChipCommands(const std::vector<uint16_t>& data, int hybridId)
{
    std::vector<uint32_t> commandList;

    RD53FWInterface::ComposeAndPackChipCommands(data, hybridId, commandList);
    return RD53FWInterface::SendChipCommands(commandList);
}

void RD53FWInterface::ComposeAndPackChipCommands(const std::vector<uint16_t>& data, int hybridId, std::vector<uint32_t>& commandList)
{
    if(data.size() == 0) return;
    const size_t n32bitWords = (data.size() / 2) + (data.size() % 2);

    // ##########
    // # Header #
    // ##########
    commandList.emplace_back(bits::pack<6, 10, 16>(RD53FWconstants::HEADEAR_WRTCMD, (hybridId < 0 ? this->enabledHybrids : 1 << hybridId), n32bitWords));

    // ############
    // # Commands #
    // ############
    for(auto i = 1u; i < data.size(); i += 2) commandList.emplace_back(bits::pack<16, 16>(data[i - 1], data[i]));

    // If data.size() is not even, add a SYNC command
    if(data.size() % 2 != 0) commandList.emplace_back(bits::pack<16, 16>(data.back(), RD53ACmd::RD53ACmdEncoder::SYNC));
}

bool RD53FWInterface::SendChipCommands(const std::vector<uint32_t>& commandList)
{
    if(commandList.size() == 0) return true;
    bool returnValue = true;

    // ############################
    // # Check write-command FIFO #
    // ############################
    int nAttempts = 0;
    while(((RegManager::ReadReg("user.stat_regs.slow_cmd.error_flag") || !RegManager::ReadReg("user.stat_regs.slow_cmd.fifo_empty") || RegManager::ReadReg("user.stat_regs.slow_cmd.fifo_full")) ==
           true) &&
          (nAttempts < RD53Shared::MAXATTEMPTS))
    {
        if((RD53FWInterface::silentRunning == false) && (RegManager::ReadReg("user.stat_regs.slow_cmd.error_flag") == true)) LOG(ERROR) << BOLDRED << "Write-command FIFO error" << RESET;
        if((RD53FWInterface::silentRunning == false) && (RegManager::ReadReg("user.stat_regs.slow_cmd.fifo_empty") == false)) LOG(ERROR) << BOLDRED << "Write-command FIFO not empty" << RESET;
        if(((RD53FWInterface::silentRunning == false) && RegManager::ReadReg("user.stat_regs.slow_cmd.fifo_full") == true)) LOG(ERROR) << BOLDRED << "Write-command FIFO full" << RESET;

        nAttempts++;
        std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::READOUTSLEEP));
    }
    if((RD53FWInterface::silentRunning == false) && (nAttempts == RD53Shared::MAXATTEMPTS))
    {
        LOG(ERROR) << BOLDRED << "Error in the write-command FIFO, reached maximum number of attempts (" << BOLDYELLOW << +RD53Shared::MAXATTEMPTS << BOLDRED << ")" << RESET;
        returnValue = false;
    }

    // ###############################
    // # Send command(s) to the chip #
    // ###############################
    RegManager::WriteBlockReg("user.ctrl_regs.Slow_cmd_fifo_din", commandList);
    RD53FWInterface::ToggleRegister("user.ctrl_regs.Slow_cmd.dispatch_packet");

    // #####################################
    // # Check if commands were dispatched #
    // #####################################
    nAttempts = 0;
    while((RegManager::ReadReg("user.stat_regs.slow_cmd.fifo_packet_dispatched") == false) && (nAttempts < RD53Shared::MAXATTEMPTSCMDDISPATCH))
    {
        nAttempts++;
        std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::READOUTSLEEP));
    }
    if((RD53FWInterface::silentRunning == false) && (nAttempts == RD53Shared::MAXATTEMPTSCMDDISPATCH))
    {
        LOG(ERROR) << BOLDRED << "Error while dispatching chip register program, reached maximum number of attempts (" << BOLDYELLOW << RD53Shared::MAXATTEMPTSCMDDISPATCH << BOLDRED << ")" << RESET;
        returnValue = false;
    }

    return returnValue;
}

std::vector<std::pair<uint16_t, uint16_t>> RD53FWInterface::ReadChipRegisters(ReadoutChip* pChip)
{
    std::vector<std::pair<uint16_t, uint16_t>> regReadback;

    // #################################
    // # Compose chip-lane in readback #
    // #################################
    uint32_t chipLane = pChip->getHybridId();
    if(this->hybridType != 1) chipLane = (this->hybridType == 2 ? RD53FWconstants::NLANE_DUALHYBRID : RD53FWconstants::NLANE_QUADHYBRID) * chipLane + static_cast<RD53*>(pChip)->getChipLane();

    // #####################
    // # Read the register #
    // #####################
    if((RD53FWInterface::silentRunning == false) && (RegManager::ReadReg("user.stat_regs.readout1.register_fifo_full") == true)) LOG(ERROR) << BOLDRED << "Read-command FIFO full" << RESET;

    while(RegManager::ReadReg("user.stat_regs.readout1.register_fifo_empty") == false)
    {
        uint32_t readBackData = RegManager::ReadReg("user.stat_regs.Register_Rdback_fifo");

        uint16_t chipAddress, regAddress, regValue;
        std::tie(chipAddress, regAddress, regValue) = bits::unpack<6, 10, 16>(readBackData);

        if(chipAddress == chipLane) regReadback.emplace_back(regAddress, regValue);
    }

    if((regReadback.size() == 0) && (RD53FWInterface::silentRunning == false)) LOG(ERROR) << BOLDRED << "Read-command FIFO empty" << RESET;

    return regReadback;
}

bool RD53FWInterface::CheckChipCommunication(const BeBoard* pBoard)
{
    LOG(INFO) << GREEN << "Checking status communication RD53 --> FW" << RESET;
    isChipCommunicationOK = false;

    // ########################################
    // # Check communication with the chip(s) #
    // ########################################
    uint32_t chips_en = RD53FWInterface::GetBoardEnabledChips(pBoard, true);
    if(chips_en == 0)
    {
        LOG(ERROR) << "\t--> No data lane is enabled: aborting" << RESET;
        return isChipCommunicationOK;
    }
    LOG(INFO) << BOLDBLUE << "\t--> Total number of " << BOLDYELLOW << "required" << BOLDBLUE << " data lanes: " << BOLDYELLOW << RD53Shared::countBitsOne(chips_en) << BOLDBLUE << ", i.e. "
              << BOLDYELLOW << std::bitset<32>(chips_en) << RESET;

    uint32_t              channel_up;
    int                   nAttempts = 0;
    std::vector<uint16_t> initSequence(RD53Shared::firstChip->getLaneUpInitSequence());
    do {
        // ###############################################
        // # Send sequence to help frontend chip to lock #
        // ###############################################
        if(initSequence.size() != 0)
            for(const auto cOpticalGroup: *pBoard)
                for(const auto cHybrid: *cOpticalGroup) RD53FWInterface::WriteChipCommands(initSequence, cHybrid->getId());

        std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::DEEPSLEEP));
        channel_up = RegManager::ReadReg("user.stat_regs.aurora_rx_channel_up");
        LOG(INFO) << BOLDBLUE << "\t--> Total number of " << BOLDYELLOW << "active" << BOLDBLUE << " data lanes:   " << BOLDYELLOW << RD53Shared::countBitsOne(channel_up) << BOLDBLUE << ", i.e. "
                  << BOLDYELLOW << std::bitset<32>(channel_up) << RESET;

        if(chips_en & ~channel_up) LOG(INFO) << BOLDBLUE << "\t--> Some data lanes are enabled but inactive" << BOLDYELLOW << " -- > retry " << RESET;
        nAttempts++;
    } while((nAttempts < RD53Shared::MAXATTEMPTS) && (chips_en & ~channel_up));

    if(nAttempts == RD53Shared::MAXATTEMPTS)
    {
        LOG(ERROR) << BOLDRED << "\t--> Error, some data lanes are enabled but inactive, reached maximum number of attempts (" << BOLDYELLOW << +RD53Shared::MAXATTEMPTS << BOLDRED << ") " << RESET;
        return isChipCommunicationOK;
    }

    LOG(INFO) << BOLDBLUE << "\t--> All enabled data lanes are active" << RESET;
    isChipCommunicationOK = true;
    return isChipCommunicationOK;
}

RD53FWconstants::ReadoutSpeed RD53FWInterface::ReadoutSpeed()
// ###################
// # 0 = 1.28 Gbit/s #
// # 1 = 640 Mbit/s  #
// # 2 = 320 Mbit/s  #
// ###################
{
    return RegManager::ReadReg("user.stat_regs.aurora_rx.speed") == 0 ? RD53FWconstants::ReadoutSpeed::x1280 : RD53FWconstants::ReadoutSpeed::x640;
}

uint32_t RD53FWInterface::GetBoardEnabledChips(const BeBoard* pBoard, bool primariesOnly)
{
    uint32_t theChipsEn = 0;

    for(const auto cOpticalGroup: *pBoard)
        for(const auto cHybrid: *cOpticalGroup)
        {
            uint32_t       chips_en  = 0;
            const uint32_t hybrid_id = cHybrid->getId();

            if(this->hybridType == 1)
                chips_en = 1 << hybrid_id;
            else
                for(const auto cChip: *cHybrid)
                    if((primariesOnly == false) || (static_cast<Ph2_HwDescription::RD53*>(cChip)->laneConfig.isPrimary == true))
                    {
                        const uint32_t chip_lane =
                            ((this->hybridType == 2 ? RD53FWconstants::NLANE_DUALHYBRID : RD53FWconstants::NLANE_QUADHYBRID) * hybrid_id) + static_cast<RD53*>(cChip)->getChipLane();
                        chips_en |= 1 << chip_lane;
                    }

            theChipsEn |= chips_en;
        }

    return theChipsEn;
}

uint32_t RD53FWInterface::GetBoardEnabledHybrids(const BeBoard* pBoard)
{
    uint32_t theHybridsEn = 0;

    for(const auto cOpticalGroup: *pBoard)
        for(const auto cHybrid: *cOpticalGroup) theHybridsEn |= 1 << cHybrid->getId();

    return theHybridsEn;
}

void RD53FWInterface::Start(const BeBoard* pBoard)
{
    RD53Shared::chipInterface->SendBoardClear(pBoard);
    RD53FWInterface::ResetReadBkFIFO(); // @TMP@ : Temporary fix to avoid FIFO empty at readback
    RD53FWInterface::ResetReadoutBlk();
    RD53FWInterface::SendBoardCommandWithStrobe("user.ctrl_regs.fast_cmd_reg_1.start_trigger");
}

void RD53FWInterface::Stop() { RD53FWInterface::SendBoardCommandWithStrobe("user.ctrl_regs.fast_cmd_reg_1.stop_trigger"); }

void RD53FWInterface::Pause() { RD53FWInterface::SendBoardCommandWithStrobe("user.ctrl_regs.fast_cmd_reg_1.stop_trigger"); }

void RD53FWInterface::Resume() { RD53FWInterface::SendBoardCommandWithStrobe("user.ctrl_regs.fast_cmd_reg_1.start_trigger"); }

void RD53FWInterface::TurnOffFMC() { RegManager::WriteStackReg({{"system.ctrl_2.fmc_pg_c2m", 0}, {"system.ctrl_2.fmc_l8_pwr_en", 0}, {"system.ctrl_2.fmc_l12_pwr_en", 0}}); }

void RD53FWInterface::TurnOnFMC()
{
    RegManager::WriteStackReg({{"system.ctrl_2.fmc_l12_pwr_en", 1}, {"system.ctrl_2.fmc_l8_pwr_en", 1}, {"system.ctrl_2.fmc_pg_c2m", 1}});

    std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::DEEPSLEEP));
}

void RD53FWInterface::ResetBoard()
{
    // #######
    // # Set #
    // #######
    RegManager::WriteReg("user.ctrl_regs.reset_reg.aurora_rst", 0);
    RegManager::WriteReg("user.ctrl_regs.reset_reg.aurora_pma_rst", 0);
    RegManager::WriteReg("user.ctrl_regs.reset_reg.global_rst", 1);
    RegManager::WriteReg("user.ctrl_regs.reset_reg.clk_gen_rst", 1);
    RegManager::WriteReg("user.ctrl_regs.reset_reg.fmc_pll_rst", 0);
    RegManager::WriteReg("user.ctrl_regs.reset_reg.cmd_rst", 1);
    RegManager::WriteReg("user.ctrl_regs.reset_reg.i2c_rst", 1);

    // #########
    // # Reset #
    // #########
    RegManager::WriteReg("user.ctrl_regs.reset_reg.global_rst", 0);
    RegManager::WriteReg("user.ctrl_regs.reset_reg.clk_gen_rst", 0);
    RegManager::WriteReg("user.ctrl_regs.reset_reg.fmc_pll_rst", 1);
    RegManager::WriteReg("user.ctrl_regs.reset_reg.cmd_rst", 0);

    std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::DEEPSLEEP));

    RegManager::WriteReg("user.ctrl_regs.reset_reg.i2c_rst", 0);
    RegManager::WriteReg("user.ctrl_regs.reset_reg.aurora_pma_rst", 1);
    RegManager::WriteReg("user.ctrl_regs.reset_reg.aurora_rst", 1);

    // ##############################################
    // # Reset communication with the frontend chip #
    // ##############################################
    RD53FWInterface::ToggleRegister("user.ctrl_regs.reset_reg.chip_resync");

    // ########
    // # DDR3 #
    // ########
    LOG(INFO) << YELLOW << "Waiting for DDR3 calibration..." << RESET;
    while(RegManager::ReadReg("user.stat_regs.readout1.ddr3_initial_calibration_done") == false) std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::DEEPSLEEP));

    LOG(INFO) << BOLDBLUE << "\t--> DDR3 calibration done" << RESET;
}

void RD53FWInterface::ResetFastCmdBlk()
{
    RD53FWInterface::SendBoardCommandWithStrobe("user.ctrl_regs.fast_cmd_reg_1.ipb_reset");
    RegManager::WriteStackReg(
        {{"user.ctrl_regs.fast_cmd_reg_1.ipb_fast_duration", RD53FWconstants::IPBUS_FASTDURATION}, {"user.ctrl_regs.Aurora_block.event_stream_timeout", RD53FWconstants::EVENT_STREAM_TIMEOUT}});
}

void RD53FWInterface::ResetSlowCmdFIFO() { RD53FWInterface::ToggleRegister("user.ctrl_regs.Slow_cmd.fifo_reset"); }

void RD53FWInterface::ResetReadBkFIFO() { RD53FWInterface::ToggleRegister("user.ctrl_regs.Register_RdBack.fifo_reset"); }

void RD53FWInterface::ResetReadoutBlk()
{
    ddr3Offset = 0;
    RD53FWInterface::ToggleRegister("user.ctrl_regs.reset_reg.readout_block_rst");
}

void RD53FWInterface::ChipReset()
{
    RD53FWInterface::ToggleRegister("user.ctrl_regs.reset_reg.scc_rst");
    RD53FWInterface::ChipReSync();
}

void RD53FWInterface::ChipReSync() { RD53FWInterface::ToggleRegister("user.ctrl_regs.fast_cmd_reg_1.ipb_bcr"); }

uint32_t RD53FWInterface::ReadData(BeBoard* pBoard, bool pBreakTrigger, std::vector<uint32_t>& pData, bool pWait)
{
    uint32_t nWordsInMemoryOld, nWordsInMemory = 0;

    // #############################################
    // # Wait for a stable number of words to read #
    // #############################################
    nWordsInMemory = RegManager::ReadReg("user.stat_regs.words_to_read");
    do {
        nWordsInMemoryOld = nWordsInMemory;
        std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::READOUTSLEEP));
    } while(((nWordsInMemory = RegManager::ReadReg("user.stat_regs.words_to_read")) != nWordsInMemoryOld) && (pWait == true));
    // auto nTriggersReceived = RegManager::ReadReg("user.stat_regs.trigger_cntr");

    // #############
    // # Read DDR3 #
    // #############
    std::vector<uint32_t> theData;
    const auto            FIFOsize = (1 << RD53FWconstants::NBIT_DATA_FIFO);

    if(nWordsInMemory > (FIFOsize - ddr3Offset))
    {
        auto firstBlock(ReadBlockRegOffset("ddr3.fc7_daq_ddr3", FIFOsize - ddr3Offset, ddr3Offset));
        theData.insert(theData.end(), std::make_move_iterator(firstBlock.begin()), std::make_move_iterator(firstBlock.end()));
        auto secondBlock(ReadBlockRegOffset("ddr3.fc7_daq_ddr3", nWordsInMemory - (FIFOsize - ddr3Offset), 0));
        theData.insert(theData.end(), std::make_move_iterator(secondBlock.begin()), std::make_move_iterator(secondBlock.end()));
    }
    else
    {
        auto block(ReadBlockRegOffset("ddr3.fc7_daq_ddr3", nWordsInMemory, ddr3Offset));
        theData.insert(theData.end(), std::make_move_iterator(block.begin()), std::make_move_iterator(block.end()));
    }

    ddr3Offset += nWordsInMemory;
    ddr3Offset %= FIFOsize;
    pData.insert(pData.end(), std::make_move_iterator(theData.begin()), std::make_move_iterator(theData.end()));

    if((this->fSaveToFile == true) && (pData.size() != 0)) this->fFileHandler->setData(pData);
    return pData.size();
}

void RD53FWInterface::ReadNEvents(BeBoard* pBoard, uint32_t pNEvents, std::vector<uint32_t>& pData, bool pWait)
{
    int  nAttempts = 0;
    bool retry;

    RD53FWInterface::WriteArbitraryRegister("user.ctrl_regs.fast_cmd_reg_3.triggers_to_accept", RD53FWInterface::localCfgFastCmd.n_triggers = pNEvents);

    // @TMP@ : Autozero
    if(RD53FWInterface::localCfgFastCmd.autozero_source == AutozeroSource::FastCMDFSM)
        RD53FWInterface::WriteChipCommands(serialize(RD53ACmd::WrReg{RD53Shared::firstChip->getFEtype()->broadcastChipId,
                                                                     RD53Shared::firstChip->getRegItem("GlobalPulseConf").fAddress,
                                                                     RD53Shared::firstChip->getFEtype()->GlobalPulseConfMap.find("AcqureZeroSyncFE")->second}),
                                           -1);
    else if(RD53FWInterface::localCfgFastCmd.autozero_source == AutozeroSource::Software)
    {
        RD53FWInterface::WriteChipCommands(serialize(RD53ACmd::WrReg{RD53Shared::firstChip->getFEtype()->broadcastChipId,
                                                                     RD53Shared::firstChip->getRegItem("GlobalPulseConf").fAddress,
                                                                     RD53Shared::firstChip->getFEtype()->GlobalPulseConfMap.find("AcqureZeroSyncFE")->second}),
                                           -1);
        RD53FWInterface::WriteChipCommands(serialize(RD53ACmd::GlobalPulse{RD53Shared::firstChip->getFEtype()->broadcastChipId, 6}), -1);
        std::this_thread::sleep_for(std::chrono::microseconds(10));
        RD53FWInterface::WriteChipCommands(serialize(RD53ACmd::ECR{}), -1);
        std::this_thread::sleep_for(std::chrono::microseconds(20));
    }

    do {
        retry = false;
        nAttempts++;
        pData.clear();

        // ####################
        // # Readout sequence #
        // ####################
        RD53FWInterface::Start(pBoard);
        while(RegManager::ReadReg("user.stat_regs.trigger_cntr") < pNEvents * (1 + RD53FWInterface::localCfgFastCmd.trigger_duration))
            std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::READOUTSLEEP));
        RD53FWInterface::ReadData(pBoard, false, pData, pWait);
        RD53FWInterface::Stop();

        // ##################
        // # Error checking #
        // ##################
        RD53Event::decodedEvents.clear();
        uint32_t status = 0;

        // ###################
        // # Decoding events #
        // ###################
        RD53Event::DecodeEventsMultiThreads(pData, RD53Event::decodedEvents, status, RD53FWInterface::silentRunning); // Decode events with multiple threads
        // RD53Event::DecodeEvents(pData, RD53Event::decodedEvents, {}, status, RD53FWInterface::silentRunning); // Decode events with a single thread

        if((RD53FWInterface::silentRunning == false) && (RD53Event::EvtErrorHandler(status) == false))
        {
            NtrialsNevents++;
            retry = true;
            continue;
        }

        if(RD53Event::decodedEvents.size() != RD53FWInterface::localCfgFastCmd.n_triggers * (1 + RD53FWInterface::localCfgFastCmd.trigger_duration))
        {
            NtrialsNevents++;
            if(RD53FWInterface::silentRunning == false)
                LOG(ERROR) << BOLDRED << "Sent " << BOLDYELLOW << RD53FWInterface::localCfgFastCmd.n_triggers * (1 + RD53FWInterface::localCfgFastCmd.trigger_duration) << BOLDRED
                           << " triggers, but collected " << BOLDYELLOW << RD53Event::decodedEvents.size() << BOLDRED << " events" << BOLDYELLOW << " --> retry" << RESET;
            retry = true;
            continue;
        }

    } while((RD53FWInterface::silentRunning == false) && (retry == true) && (nAttempts < RD53Shared::MAXATTEMPTS));

    if(retry == true)
    {
        if(RD53FWInterface::silentRunning == false)
            LOG(ERROR) << BOLDRED << "\t--> Reached maximum number of attempts (" << BOLDYELLOW << +RD53Shared::MAXATTEMPTS << BOLDRED << ") without success" << RESET;
        pData.clear();
        NcorruptedNevents++;
    }

    // #################
    // # Show progress #
    // #################
    RD53RunProgress::update(pData.size(), showRunProgress);
}

void RD53FWInterface::SendBoardCommandWithStrobe(const std::string& cmdReg)
{
    RegManager::WriteStackReg({{cmdReg, 1}, {"user.ctrl_regs.fast_cmd_reg_1.cmd_strobe", 1}, {"user.ctrl_regs.fast_cmd_reg_1.cmd_strobe", 0}, {cmdReg, 0}});
}

void RD53FWInterface::ToggleRegister(const std::string& cmdReg, bool do1and0)
{
    if(do1and0 == true)
        RegManager::WriteStackReg({{cmdReg, 1}, {cmdReg, 0}});
    else
        RegManager::WriteStackReg({{cmdReg, 0}, {cmdReg, 1}});
}

void RD53FWInterface::SendFastCommands(const FastCommandsConfig* config)
{
    if(config == nullptr) config = &(RD53FWInterface::localCfgFastCmd);

    // @TMP@ : Prepare GlobalPulseConf to acquire zero level in SYNC FE
    if(config->autozero_source != AutozeroSource::Disabled)
        RD53FWInterface::WriteChipCommands(serialize(RD53ACmd::WrReg{RD53Shared::firstChip->getFEtype()->broadcastChipId,
                                                                     RD53Shared::firstChip->getRegItem("GlobalPulseConf").fAddress,
                                                                     RD53Shared::firstChip->getFEtype()->GlobalPulseConfMap.find("AcqureZeroSyncFE")->second}),
                                           -1);

    // ##################################
    // # Configuring fast command block #
    // ##################################
    RegManager::WriteStackReg({// ############################
                               // # General data for trigger #
                               // ############################
                               {"user.ctrl_regs.fast_cmd_reg_2.trigger_source", (uint32_t)config->trigger_source},
                               {"user.ctrl_regs.fast_cmd_reg_2.backpressure_en", (uint32_t)config->backpressure_en},
                               {"user.ctrl_regs.fast_cmd_reg_2.init_ecr_en", (uint32_t)config->initial_ecr_en},
                               {"user.ctrl_regs.fast_cmd_reg_2.veto_en", (uint32_t)config->veto_en},
                               {"user.ctrl_regs.fast_cmd_reg_2.ext_trig_delay", (uint32_t)config->ext_trigger_delay},
                               {"user.ctrl_regs.fast_cmd_reg_2.trigger_duration", (uint32_t)config->trigger_duration},
                               {"user.ctrl_regs.fast_cmd_reg_2.HitOr_enable_l12", (uint32_t)config->enable_hitor},
                               {"user.ctrl_regs.fast_cmd_reg_3.triggers_to_accept", (uint32_t)config->n_triggers},

                               // ##############################
                               // # Fast command configuration #
                               // ##############################
                               {"user.ctrl_regs.fast_cmd_reg_2.tp_fsm_ecr_en", (uint32_t)config->fast_cmd_fsm.ecr_en},
                               {"user.ctrl_regs.fast_cmd_reg_2.tp_fsm_test_pulse_en", (uint32_t)config->fast_cmd_fsm.first_cal_en},
                               {"user.ctrl_regs.fast_cmd_reg_2.tp_fsm_inject_pulse_en", (uint32_t)config->fast_cmd_fsm.second_cal_en},
                               {"user.ctrl_regs.fast_cmd_reg_2.tp_fsm_trigger_en", (uint32_t)(config->enable_hitor != 0 ? 0 : config->fast_cmd_fsm.trigger_en)},

                               {"user.ctrl_regs.fast_cmd_reg_6.delay_after_init_prime", (uint32_t)config->fast_cmd_fsm.delay_after_first_prime},
                               {"user.ctrl_regs.fast_cmd_reg_7.delay_after_ecr", (uint32_t)config->fast_cmd_fsm.delay_after_ecr},
                               {"user.ctrl_regs.fast_cmd_reg_6.delay_after_autozero", (uint32_t)config->fast_cmd_fsm.delay_after_autozero}, // @TMP@
                               {"user.ctrl_regs.fast_cmd_reg_4.cal_data_prime", (uint32_t)config->fast_cmd_fsm.first_cal_data},
                               {"user.ctrl_regs.fast_cmd_reg_4.delay_after_prime_pulse", (uint32_t)config->fast_cmd_fsm.delay_after_prime},
                               {"user.ctrl_regs.fast_cmd_reg_5.cal_data_inject", (uint32_t)config->fast_cmd_fsm.second_cal_data},
                               {"user.ctrl_regs.fast_cmd_reg_5.delay_after_inject_pulse", (uint32_t)config->fast_cmd_fsm.delay_after_inject},
                               {"user.ctrl_regs.fast_cmd_reg_6.delay_before_next_pulse", (uint32_t)config->fast_cmd_fsm.delay_after_trigger},

                               // ################################
                               // # @TMP@ Autozero configuration #
                               // ################################
                               {"user.ctrl_regs.fast_cmd_reg_2.autozero_source", (uint32_t)config->autozero_source},
                               {"user.ctrl_regs.fast_cmd_reg_7.glb_pulse_data", (uint32_t)bits::pack<4, 1, 4, 1>(RD53Shared::firstChip->getFEtype()->broadcastChipId, 0, 6, 0)}});

    RD53FWInterface::SendBoardCommandWithStrobe("user.ctrl_regs.fast_cmd_reg_1.load_config");
}

void RD53FWInterface::ConfigureFastCommands(const BeBoard*            pBoard,
                                            const uint32_t            nTRIGxEvent,
                                            const RD53Shared::INJtype injType,
                                            const uint32_t            injLatency,
                                            const uint32_t            nClkDelays,
                                            const bool                enableAutozero)
// ##################################################################################
// # Finite state machine                                                           #
// ##################################################################################
// # Idle --> Init_Prime --> Auto_Zero --> ECR --> Inject --> Trigger --> Prime --| #
// #  ^                          ^                                                | #
// #  |                          |------------------------------------------------| #
// #  |                                                                           | #
// #  |---------------------------------------------------------------------------| #
// ##################################################################################
{
    const size_t NbitsInitPrime = 10; // @CONST@
    enum INJdelay
    {
        AfterInjectCal = 32,
        BeforePrimeCal = 1,
        Loop           = 460,
        SelfTrigger    = 1500
    };

    // #############################
    // # Configuring FastCmd block #
    // #############################
    RD53FWInterface::localCfgFastCmd.n_triggers       = 0;
    RD53FWInterface::localCfgFastCmd.trigger_duration = nTRIGxEvent - 1;

    if(injType == RD53Shared::INJtype::Digital)
    {
        // #######################################
        // # Configuration for digital injection #
        // #######################################
        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.first_cal_data  = RD53Shared::firstChip->getCalCmd(1, 0, 10, 0, 0);
        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.second_cal_data = RD53Shared::firstChip->getCalCmd(0, 0, 2, 0, 0);

        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.delay_after_first_prime = (nClkDelays == 0 ? (uint32_t)INJdelay::Loop : nClkDelays) % (RD53Shared::setBits(NbitsInitPrime) + 1);
        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.delay_after_ecr         = 0;
        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.delay_after_inject      = (injLatency == 0 ? (uint32_t)INJdelay::AfterInjectCal : injLatency);
        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.delay_after_trigger     = INJdelay::BeforePrimeCal;
        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.delay_after_prime       = (nClkDelays == 0 ? (uint32_t)INJdelay::Loop : nClkDelays);

        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.first_cal_en  = true;
        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.second_cal_en = true;
        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.trigger_en    = true;
        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.ecr_en        = false;
    }
    else if((injType == RD53Shared::INJtype::Analog) || (injType == RD53Shared::INJtype::Custom) || (injType == RD53Shared::INJtype::XtalkCoupled) || (injType == RD53Shared::INJtype::XtalkDeCoupled))
    {
        // ###################################################################################
        // # Configuration for analog, custom, XtalkDeCoupled, abd XtalkDeCoupled injections #
        // ###################################################################################
        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.first_cal_data  = RD53Shared::firstChip->getCalCmd(1, 0, 0, 0, 0);
        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.second_cal_data = RD53Shared::firstChip->getCalCmd(0, 0, 2, 0, 0);

        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.delay_after_first_prime = (nClkDelays == 0 ? (uint32_t)INJdelay::Loop : nClkDelays) % (RD53Shared::setBits(NbitsInitPrime) + 1);
        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.delay_after_ecr         = 0;
        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.delay_after_inject      = (injLatency == 0 ? (uint32_t)INJdelay::AfterInjectCal : injLatency);
        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.delay_after_trigger     = INJdelay::BeforePrimeCal;
        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.delay_after_prime       = (nClkDelays == 0 ? (uint32_t)INJdelay::Loop : nClkDelays);

        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.first_cal_en  = true;
        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.second_cal_en = true;
        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.trigger_en    = true;
        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.ecr_en        = false;
    }
    else if(injType == RD53Shared::INJtype::SelfTrigger)
    {
        // ###################################
        // # Configuration for self triggers #
        // ###################################
        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.first_cal_data  = 0;
        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.second_cal_data = RD53Shared::firstChip->getCalCmd(1, 0, 16, 0, 0);

        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.delay_after_first_prime = (nClkDelays == 0 ? (uint32_t)INJdelay::Loop : nClkDelays) % (RD53Shared::setBits(NbitsInitPrime) + 1);
        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.delay_after_ecr         = 0;
        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.delay_after_inject      = (injLatency == 0 ? (uint32_t)INJdelay::AfterInjectCal : injLatency);
        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.delay_after_trigger     = INJdelay::SelfTrigger;
        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.delay_after_prime       = (nClkDelays == 0 ? (uint32_t)INJdelay::Loop : nClkDelays);

        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.first_cal_en  = false;
        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.second_cal_en = true;
        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.trigger_en    = false;
        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.ecr_en        = false;
    }
    else if(injType == RD53Shared::INJtype::None)
    {
        // ##################################
        // # Configuration for no injection #
        // ##################################
        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.first_cal_data  = 0;
        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.second_cal_data = 0;

        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.delay_after_first_prime = 0;
        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.delay_after_ecr         = 0;
        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.delay_after_inject      = 0;
        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.delay_after_trigger     = (nClkDelays == 0 ? (uint32_t)INJdelay::Loop : nClkDelays);
        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.delay_after_prime       = 0;

        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.first_cal_en  = false;
        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.second_cal_en = false;
        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.trigger_en    = true;
        RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.ecr_en        = false;
    }
    else
        LOG(ERROR) << BOLDRED << "Option not recognized " << BOLDYELLOW << +static_cast<uint8_t>(injType) << RESET;

    if(enableAutozero == true) // @TMP@
    {
        if(RD53FWInterface::localCfgFastCmd.trigger_source != TriggerSource::FastCMDFSM)
            RD53FWInterface::localCfgFastCmd.autozero_source = AutozeroSource::Software;
        else
        {
            RD53FWInterface::localCfgFastCmd.autozero_source                   = AutozeroSource::FastCMDFSM;
            RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.ecr_en               = true;
            RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.delay_after_ecr      = 512;
            RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.delay_after_autozero = 128;
        }
    }
    else
        RD53FWInterface::localCfgFastCmd.autozero_source = AutozeroSource::Disabled;

    LOG(INFO) << GREEN << "Internal trigger frequency (if enabled): " << BOLDYELLOW << std::fixed << std::setprecision(0)
              << RD53Constants::ACCELERATOR_CLK * 1e6 /
                     static_cast<float>(

                         // ((RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.first_cal_en == true) ? (RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.delay_after_first_prime + 1) * 4 + 7 : 0) +

                         ((RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.ecr_en == true) ? (RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.delay_after_ecr + 1) * 4 - 1 : 0) +
                         ((RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.second_cal_en == true) ? (RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.delay_after_inject + 1) * 4 + 7 : 0) +
                         ((RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.trigger_en == true) ? (RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.delay_after_trigger + 1) * 4 - 1 : 0) +
                         ((RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.first_cal_en == true) ? (RD53FWInterface::localCfgFastCmd.fast_cmd_fsm.delay_after_prime + 1) * 4 + 7 : 0) + 4)
              << std::setprecision(-1) << " Hz" << RESET;
    RD53Shared::resetDefaultFloat();

    // ##################################################
    // # Configure remaining parameters of FastCommands #
    // ##################################################
    for(const auto& it: pBoard->getBeBoardRegMap())
    {
        if((it.second.fPrmptCfg == true) &&
           (std::find(FastCommandsConfig::fastCmdWhiteList.begin(), FastCommandsConfig::fastCmdWhiteList.end(), it.first) != FastCommandsConfig::fastCmdWhiteList.end()))
        {
            if(it.first == FastCommandsConfig::fastCmdWhiteList[0])
                RD53FWInterface::localCfgFastCmd.trigger_source = static_cast<RD53FWInterface::TriggerSource>(it.second.fValue);
            else if(it.first == FastCommandsConfig::fastCmdWhiteList[1])
                RD53FWInterface::localCfgFastCmd.initial_ecr_en = it.second.fValue;
            else if(it.first == FastCommandsConfig::fastCmdWhiteList[2])
                RD53FWInterface::localCfgFastCmd.backpressure_en = it.second.fValue;
            else if(it.first == FastCommandsConfig::fastCmdWhiteList[3])
                RD53FWInterface::localCfgFastCmd.veto_en = it.second.fValue;
            else if(it.first == FastCommandsConfig::fastCmdWhiteList[4])
                RD53FWInterface::localCfgFastCmd.n_triggers = it.second.fValue;
            else if(it.first == FastCommandsConfig::fastCmdWhiteList[5])
                RD53FWInterface::localCfgFastCmd.ext_trigger_delay = it.second.fValue;
            else if(it.first == FastCommandsConfig::fastCmdWhiteList[6])
                RD53FWInterface::localCfgFastCmd.trigger_duration = it.second.fValue;
            else if(it.first == FastCommandsConfig::fastCmdWhiteList[7])
                RD53FWInterface::localCfgFastCmd.enable_hitor = it.second.fValue;
        }
    }
}

void RD53FWInterface::ConfigureDIO5(const BeBoard* pBoard, DIO5Config* config)
{
    std::map<std::string, uint32_t> register_overrides{};
    std::set<std::string>           valid_registers{
        "ext_clk_en", "trigger_source", "dio5_ch1_thr", "dio5_ch2_thr", "dio5_ch3_thr", "dio5_ch4_thr", "dio5_ch5_thr", "dio5_en", "dio5_term_50ohm_en", "dio5_ch_out_en"};

    for(const auto& it: pBoard->getBeBoardRegMap())
        if((it.second.fPrmptCfg == true) && (std::any_of(valid_registers.begin(), valid_registers.end(), [&](auto reg) { return it.first.find(reg) != std::string::npos; })))
        {
            if(it.first.find("ext_clk_en") != std::string::npos)
            {
                config->enable     = config->enable | it.second.fValue;
                config->ch_out_en  = config->ch_out_en & 0x0F;
                config->ext_clk_en = it.second.fValue;
            }
            else if(it.first.find("trigger_source") != std::string::npos)
            {
                if(static_cast<RD53FWInterface::TriggerSource>(it.second.fValue) == TriggerSource::External)
                {
                    LOG(INFO) << BOLDBLUE << "\t--> Trigger source was selected to be External" << RESET;
                    config->enable    = true;
                    config->ch_out_en = config->ch_out_en & 0x1D;
                }
                else if(static_cast<RD53FWInterface::TriggerSource>(it.second.fValue) == TriggerSource::TLU)
                {
                    LOG(INFO) << BOLDBLUE << "\t--> Trigger source was selected to be TLU" << RESET;
                    config->enable             = true;
                    config->ch_out_en          = config->ch_out_en | 0x0D;
                    config->tlu_en             = true;
                    config->tlu_handshake_mode = 0x02;
                }
            }
            else if(it.first.find("dio5_ch1_thr") != std::string::npos)
                config->ch1_thr = it.second.fValue;
            else if(it.first.find("dio5_ch2_thr") != std::string::npos)
                config->ch2_thr = it.second.fValue;
            else if(it.first.find("dio5_ch3_thr") != std::string::npos)
                config->ch3_thr = it.second.fValue;
            else if(it.first.find("dio5_ch4_thr") != std::string::npos)
                config->ch4_thr = it.second.fValue;
            else if(it.first.find("dio5_ch5_thr") != std::string::npos)
                config->ch5_thr = it.second.fValue;
            else if(it.first.find("dio5_en") != std::string::npos)
                register_overrides["dio5_en"] = it.second.fValue;
            else if(it.first.find("dio5_ch_out_en") != std::string::npos)
                register_overrides["dio5_ch_out_en"] = it.second.fValue;
            else if(it.first.find("dio5_term_50ohm_en") != std::string::npos)
                register_overrides["dio5_term_50ohm_en"] = it.second.fValue;
        }

    // ############################################
    // # Enable 50 Ohms termination on all inputs #
    // ############################################
    config->fiftyohm_en = 0x1f ^ config->ch_out_en;

    // ######################################################################
    // # Apply override values from XML file on automatically set registers #
    // ######################################################################
    auto override_warn = [](std::string regname, uint32_t val)
    { LOG(WARNING) << BOLDBLUE << "\t--> Overriding register " << BOLDYELLOW << regname << BOLDBLUE << " with user set value " << BOLDYELLOW << "0x" << std::hex << val << RESET; };

    auto oreg = register_overrides.end();
    if((oreg = register_overrides.find("dio5_en")) != register_overrides.end())
    {
        config->enable = oreg->second;
        override_warn(oreg->first, oreg->second);
    }
    if((oreg = register_overrides.find("dio5_ch_out_en")) != register_overrides.end())
    {
        config->ch_out_en = oreg->second;
        override_warn(oreg->first, oreg->second);
    }
    if((oreg = register_overrides.find("dio5_term_50ohm_en")) != register_overrides.end())
    {
        config->fiftyohm_en = oreg->second;
        override_warn(oreg->first, oreg->second);
    }
}

void RD53FWInterface::SendDIO5Cfg(const DIO5Config* config)
{
    if(RegManager::ReadReg("user.stat_regs.global_reg.dio5_not_ready") == true) LOG(ERROR) << BOLDRED << "DIO5 not ready" << RESET;

    if(RegManager::ReadReg("user.stat_regs.global_reg.dio5_error") == true) LOG(ERROR) << BOLDRED << "DIO5 is in error" << RESET;

    RegManager::WriteStackReg({{"user.ctrl_regs.ext_tlu_reg1.dio5_en", (uint32_t)config->enable},
                               {"user.ctrl_regs.ext_tlu_reg1.dio5_ch_out_en", (uint32_t)config->ch_out_en},
                               {"user.ctrl_regs.ext_tlu_reg1.dio5_term_50ohm_en", (uint32_t)config->fiftyohm_en},
                               {"user.ctrl_regs.ext_tlu_reg1.dio5_ch1_thr", (uint32_t)config->ch1_thr},
                               {"user.ctrl_regs.ext_tlu_reg1.dio5_ch2_thr", (uint32_t)config->ch2_thr},
                               {"user.ctrl_regs.ext_tlu_reg2.dio5_ch3_thr", (uint32_t)config->ch3_thr},
                               {"user.ctrl_regs.ext_tlu_reg2.dio5_ch4_thr", (uint32_t)config->ch4_thr},
                               {"user.ctrl_regs.ext_tlu_reg2.dio5_ch5_thr", (uint32_t)config->ch5_thr},
                               {"user.ctrl_regs.ext_tlu_reg2.tlu_en", (uint32_t)config->tlu_en},
                               {"user.ctrl_regs.ext_tlu_reg2.tlu_handshake_mode", (uint32_t)config->tlu_handshake_mode},
                               {"user.ctrl_regs.reset_reg.ext_clk_en", (uint32_t)config->ext_clk_en}});

    RD53FWInterface::SendBoardCommandWithStrobe("user.ctrl_regs.ext_tlu_reg2.dio5_load_config");
}

// ##################################
// # Optical Group member functions #
// ##################################

void RD53FWInterface::ResetOptoLinkSlowControl()
{
    RegManager::WriteStackReg({{"user.ctrl_regs.lpgbt_1.ic_tx_reset", 0x1}, {"user.ctrl_regs.lpgbt_1.ic_rx_reset", 0x1}});
    std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::DEEPSLEEP));
    RegManager::WriteStackReg({{"user.ctrl_regs.lpgbt_1.ic_tx_reset", 0x0}, {"user.ctrl_regs.lpgbt_1.ic_rx_reset", 0x0}});
}

void RD53FWInterface::ResetOptoLink()
{
    RegManager::WriteReg("user.ctrl_regs.lpgbt_1.mgt_reset", 0x1);
    std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::DEEPSLEEP));
    RegManager::WriteReg("user.ctrl_regs.lpgbt_1.mgt_reset", 0x0);
}

void RD53FWInterface::StatusOptoLink(uint32_t& txStatus, uint32_t& rxStatus, uint32_t& mgtStatus)
{
    txStatus  = RegManager::ReadReg("user.stat_regs.lpgbt_fpga.tx_ready");
    rxStatus  = RegManager::ReadReg("user.stat_regs.lpgbt_fpga.rx_ready");
    mgtStatus = RegManager::ReadReg("user.stat_regs.lpgbt_fpga.mgt_ready");

    LOG(INFO) << BOLDBLUE << "\t--> Optical link n. active LpGBT chip tx:  " << BOLDYELLOW << std::setw(3) << txStatus << BOLDBLUE << ", i.e.: " << BOLDYELLOW << std::bitset<8>(txStatus) << RESET;
    LOG(INFO) << BOLDBLUE << "\t--> Optical link n. active LpGBT chip rx:  " << BOLDYELLOW << std::setw(3) << rxStatus << BOLDBLUE << ", i.e.: " << BOLDYELLOW << std::bitset<8>(rxStatus) << RESET;
    LOG(INFO) << BOLDBLUE << "\t--> Optical link n. active LpGBT chip mgt: " << BOLDYELLOW << std::setw(3) << mgtStatus << BOLDBLUE << ", i.e.: " << BOLDYELLOW << std::bitset<8>(mgtStatus) << RESET;
}

bool RD53FWInterface::WriteOptoLinkRegister(const Chip* pChip, const uint32_t pAddress, const uint32_t pData, const bool pVerify)
{
    // OptoChip ID
    RD53FWInterface::selectLink(pChip->getOpticalGroupId());

    // Configure
    RegManager::WriteStackReg(
        {{"user.ctrl_regs.lpgbt_1.ic_tx_fifo_din", pData}, {"user.ctrl_regs.lpgbt_1.ic_chip_addr_tx", pChip->getChipAddress()}, {"user.ctrl_regs.lpgbt_2.ic_reg_addr_tx", pAddress}});

    // Perform operation
    RegManager::WriteStackReg({{"user.ctrl_regs.lpgbt_1.ic_tx_fifo_wr_en", 0x1},
                               {"user.ctrl_regs.lpgbt_1.ic_tx_fifo_wr_en", 0x0},
                               {"user.ctrl_regs.lpgbt_1.ic_send_wr_cmd", 0x1},
                               {"user.ctrl_regs.lpgbt_1.ic_send_wr_cmd", 0x0}});

    if(pVerify == true)
    {
        uint32_t cReadBack = RD53FWInterface::ReadOptoLinkRegister(pChip, pAddress);
        if(cReadBack != pData)
        {
            LOG(ERROR) << BOLDRED << "[RD53FWInterface::WriteOpticalLinkRegiser] Register readback failure for register 0x" << BOLDYELLOW << std::hex << std::uppercase << pAddress << std::dec
                       << RESET;
            return false;
        }
    }

    return true;
}

uint32_t RD53FWInterface::ReadOptoLinkRegister(const Chip* pChip, const uint32_t pAddress)
{
    // OptoChip ID
    RD53FWInterface::selectLink(pChip->getOpticalGroupId());

    // Configure
    RegManager::WriteStackReg({{"user.ctrl_regs.lpgbt_1.ic_chip_addr_tx", pChip->getChipAddress()}, {"user.ctrl_regs.lpgbt_2.ic_reg_addr_tx", pAddress}});

    // Perform operation
    RegManager::WriteStackReg({{"user.ctrl_regs.lpgbt_2.ic_nb_of_words_to_read", 0x1}, {"user.ctrl_regs.lpgbt_1.ic_send_rd_cmd", 0x1}, {"user.ctrl_regs.lpgbt_1.ic_send_rd_cmd", 0x0}});

    // Actual readback: one word at a time
    uint32_t cRead  = 0;
    uint8_t  nWords = (static_cast<const lpGBT*>(pChip)->getVersion() == 0 ? 7 : 6); // LpGBT-v0 --> 7th; LpGBT-v1 --> 6th
    for(uint8_t i = 0; i < nWords; i++)
    {
        RegManager::WriteStackReg({{"user.ctrl_regs.lpgbt_1.ic_rx_fifo_rd_en", 0x1}, {"user.ctrl_regs.lpgbt_1.ic_rx_fifo_rd_en", 0x0}});
        cRead = RegManager::ReadReg("user.stat_regs.lpgbt_sc_1.rx_fifo_dout");
    }

    return cRead;
}

void RD53FWInterface::SetDownLinkMapping(uint8_t TxLink, uint8_t TxGroup, uint8_t TxModuleId)
{
    RegManager::WriteStackReg({{"user.ctrl_regs.lpgbt_mapping.link_id", TxLink},
                               {"user.ctrl_regs.lpgbt_mapping.tx_elink_id", TxGroup},
                               {"user.ctrl_regs.lpgbt_mapping.module_id", TxModuleId},
                               {"user.ctrl_regs.lpgbt_mapping.update_downlink", 1},
                               {"user.ctrl_regs.lpgbt_mapping.update_downlink", 0}});
}

void RD53FWInterface::SetUpLinkMapping(uint8_t RxLink, uint8_t ModuleId, uint8_t ChipId, const std::vector<std::pair<uint8_t, uint8_t>>& RxGroupsChipLanes)
{
    // #######################
    // # Chip identification #
    // #######################
    std::vector<std::pair<std::string, uint32_t>> commands{
        {"user.ctrl_regs.lpgbt_mapping.link_id", RxLink}, {"user.ctrl_regs.lpgbt_mapping.module_id", ModuleId}, {"user.ctrl_regs.lpgbt_mapping.chip_id", ChipId}};

    // ###############################
    // # RxGroup to ChipLane mapping #
    // ###############################
    for(auto RxGroupChipLane: RxGroupsChipLanes) commands.push_back({"user.ctrl_regs.lpgbt_mapping.lane" + std::to_string(RxGroupChipLane.second) + "_elink_id", RxGroupChipLane.first});

    // ####################
    // # Toggle to update #
    // ####################
    commands.push_back({"user.ctrl_regs.lpgbt_mapping.update_uplink", 1});
    commands.push_back({"user.ctrl_regs.lpgbt_mapping.update_uplink", 0});

    // #################
    // # Send commands #
    // #################
    RegManager::WriteStackReg(commands);
    RegManager::WriteReg("user.ctrl_regs.i2c_block.active_lanes", RxGroupsChipLanes.size()); // @TMP@ : for the time being in the FW all chips can only have the same number of lanes
}

void RD53FWInterface::selectLink(const uint8_t pLinkId, uint32_t pWait_ms) { RegManager::WriteReg("user.ctrl_regs.lpgbt_1.active_link", pLinkId); }
void RD53FWInterface::SetOptoLinkVersion(uint8_t version) { RegManager::WriteReg("user.ctrl_regs.lpgbt_1.lpgbt_version", version); }

float RD53FWInterface::GetSFPParameter(std::string parameter, int channel)
{
    int  nAttempts = 0, error = 0;
    bool timeOut = false;

    if(parameter == "T")
        RegManager::WriteReg("user.ctrl_regs.cnfg_sfp_monitoring.reg_address", 96);
    else if(parameter == "V")
        RegManager::WriteReg("user.ctrl_regs.cnfg_sfp_monitoring.reg_address", 98);
    else if(parameter == "I")
        RegManager::WriteReg("user.ctrl_regs.cnfg_sfp_monitoring.reg_address", 100);
    else if(parameter == "TX")
        RegManager::WriteReg("user.ctrl_regs.cnfg_sfp_monitoring.reg_address", 102);
    else if(parameter == "RX")
        RegManager::WriteReg("user.ctrl_regs.cnfg_sfp_monitoring.reg_address", 104);
    else if(parameter == "raw")
        RegManager::WriteReg("user.ctrl_regs.cnfg_sfp_monitoring.reg_address", 96);

    RegManager::WriteReg("user.ctrl_regs.cnfg_sfp_monitoring.channel_number", channel);
    RegManager::WriteReg("user.ctrl_regs.cnfg_sfp_monitoring.enable", 1);

    while(RegManager::ReadReg("user.stat_regs.lpgbt_monitoring.sfp_i2c_busy") == true)
    {
        RegManager::WriteReg("user.ctrl_regs.cnfg_sfp_monitoring.enable", 0);
        std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::READOUTSLEEP));

        if(nAttempts > RD53Shared::MAXATTEMPTS)
        {
            timeOut = true;
            break;
        }
        else
            nAttempts++;
    }

    error = RegManager::ReadReg("user.stat_regs.lpgbt_monitoring.sfp_i2c_error");
    if(error != 0)
        throw std::runtime_error("Error occurred during communication with the SFP. The error code is: " + std::to_string(error));
    else if((error == 0) && (timeOut == true))
        throw std::runtime_error("Time out in reading from the SFP channel: " + std::to_string(channel));

    float result = RegManager::ReadReg("user.stat_regs.lpgbt_monitoring.sfp_i2c_data_out");
    if(parameter == "T")
    {
        result /= 256.0;
        LOG(DEBUG) << "The temperature of the SFP for channel " << channel << " is " << result << " Celsius" << RESET;
    }
    else if(parameter == "V")
    {
        result /= 10.0;
        LOG(DEBUG) << "The SFP's voltage for channel " << channel << " is " << result << " miliVolt" << RESET;
    }
    else if(parameter == "I")
    {
        result *= 0.002;
        LOG(DEBUG) << "The SFP's bias current for channel " << channel << " is " << result << " miliAmper" << RESET;
    }
    else if(parameter == "TX")
    {
        result *= 0.1;
        LOG(DEBUG) << "The SFP's transmited power for channel " << channel << " is " << result << " muWatt" << RESET;
    }
    else if(parameter == "RX")
    {
        result *= 0.1;
        LOG(DEBUG) << "The SFP's received power for channel " << channel << " is " << result << " muWatt" << RESET;
    }
    else if(parameter == "raw")
        LOG(DEBUG) << "The SFP's output for channel " << channel << " is " << result << RESET;

    return result;
}

void RD53FWInterface::ConfigurePCTestAdapter(const std::string& config)
{
    std::string configPath = expandEnvironmentVariables(config);
    LOG(INFO) << GREEN << "Starting configuration of PortCard Test Adapter with configuration: " << BOLDYELLOW << configPath << RESET;

    std::ifstream                            file(configPath.c_str(), std::ios::in);
    std::stringstream                        myString;
    std::string                              line, value, address;
    std::vector<std::pair<uint8_t, uint8_t>> fAddressValue;
    uint8_t                                  fValueReadBack;
    bool                                     errorFlag = false;

    if(file.is_open())
    {
        while(std::getline(file, line))
        {
            if(line.find_first_not_of(" \t") == std::string::npos || line.at(0) == '#' || line.at(0) == '*' || line.empty())
                continue;
            else
            {
                myString.str("");
                myString.clear();
                myString << line;
                myString >> address >> value;
                fAddressValue.push_back(std::make_pair(strtoul(address.c_str(), 0, 16), strtoul(value.c_str(), 0, 16)));
            }
        }

        file.close();
    }
    else
    {
        LOG(ERROR) << BOLDYELLOW << configPath << BOLDRED << " could not be opened. Please check file path" << RESET;
        throw std::runtime_error("File " + configPath + " not found. Error");
    }

    for(const auto& thePair: fAddressValue)
    {
        RegManager::WriteReg("user.ctrl_regs.cnf_portcard_adapter.reset", 1);
        std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::READOUTSLEEP));
        RegManager::WriteReg("user.ctrl_regs.cnf_portcard_adapter.reset", 0);

        LOG(INFO) << GREEN << "Setting: Address " << BOLDYELLOW << std::to_string(thePair.first) << RESET << GREEN << " Value " << BOLDYELLOW << std::to_string(thePair.second) << RESET;

        RegManager::WriteReg("user.ctrl_regs.cnf_portcard_adapter.switch_address", thePair.first);
        RegManager::WriteReg("user.ctrl_regs.cnf_portcard_adapter.switch_value", thePair.second);
        std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::READOUTSLEEP));
        RegManager::WriteReg("user.ctrl_regs.cnf_portcard_adapter.ctrl", 1); // Writing to Switch
        std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::READOUTSLEEP));

        RegManager::WriteReg("user.ctrl_regs.cnf_portcard_adapter.ctrl", 0);
        RegManager::WriteReg("user.ctrl_regs.cnf_portcard_adapter.reset", 1);
        std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::READOUTSLEEP));
        RegManager::WriteReg("user.ctrl_regs.cnf_portcard_adapter.reset", 0);

        std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::READOUTSLEEP));
        RegManager::WriteReg("user.ctrl_regs.cnf_portcard_adapter.ctrl", 2); // Reading from Switch
        std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::READOUTSLEEP));

        fValueReadBack = RegManager::ReadReg("user.stat_regs.stat_portcard_adapter.switch_value");
        RegManager::WriteReg("user.ctrl_regs.cnf_portcard_adapter.ctrl", 0);

        LOG(INFO) << GREEN << "Reading: Address " << BOLDYELLOW << std::to_string(thePair.first) << RESET << GREEN << " Value " << BOLDYELLOW << std::to_string(fValueReadBack) << RESET;
        if(fValueReadBack != thePair.second)
        {
            LOG(ERROR) << BOLDRED << "[RD53FWInterface::ConfigurePCTestAdapter] Mismatch between set value and read value!" << RESET;
            errorFlag = true;
        }
    }

    LOG(INFO) << GREEN << "Updating the matrix" << RESET;

    RegManager::WriteReg("user.ctrl_regs.cnf_portcard_adapter.reset", 1);
    std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::READOUTSLEEP));
    RegManager::WriteReg("user.ctrl_regs.cnf_portcard_adapter.reset", 0);

    LOG(INFO) << GREEN << "Setting: Address " << BOLDYELLOW << "1" << RESET << GREEN << " Value: " << BOLDYELLOW << "1" << RESET;
    RegManager::WriteReg("user.ctrl_regs.cnf_portcard_adapter.switch_address", (uint8_t)1);
    RegManager::WriteReg("user.ctrl_regs.cnf_portcard_adapter.switch_value", (uint8_t)1); // Updating Matrix
    std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::READOUTSLEEP));
    RegManager::WriteReg("user.ctrl_regs.cnf_portcard_adapter.ctrl", 1); // Writing to Switch
    std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::READOUTSLEEP));

    RegManager::WriteReg("user.ctrl_regs.cnf_portcard_adapter.ctrl", 0);
    RegManager::WriteReg("user.ctrl_regs.cnf_portcard_adapter.reset", 1);
    std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::READOUTSLEEP));
    RegManager::WriteReg("user.ctrl_regs.cnf_portcard_adapter.reset", 0);

    LOG(INFO) << GREEN << "Setting: Address " << BOLDYELLOW << "1" << RESET << GREEN << " Value: " << BOLDYELLOW << "0" << RESET;
    RegManager::WriteReg("user.ctrl_regs.cnf_portcard_adapter.switch_value", (uint8_t)0);
    std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::READOUTSLEEP));
    RegManager::WriteReg("user.ctrl_regs.cnf_portcard_adapter.ctrl", 1); // Writing to Switch
    std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::READOUTSLEEP));

    RegManager::WriteReg("user.ctrl_regs.cnf_portcard_adapter.ctrl", 0);

    LOG(INFO) << GREEN << "Reading back matrix values" << RESET;

    for(const auto& thePair: fAddressValue)
    {
        RegManager::WriteReg("user.ctrl_regs.cnf_portcard_adapter.reset", 1);
        std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::READOUTSLEEP));
        RegManager::WriteReg("user.ctrl_regs.cnf_portcard_adapter.reset", 0);

        RegManager::WriteReg("user.ctrl_regs.cnf_portcard_adapter.switch_address", (uint8_t)(thePair.first + 80));
        std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::READOUTSLEEP));
        RegManager::WriteReg("user.ctrl_regs.cnf_portcard_adapter.ctrl", 2); // Reading from Switch
        std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::READOUTSLEEP));

        fValueReadBack = RegManager::ReadReg("user.stat_regs.stat_portcard_adapter.switch_value");
        RegManager::WriteReg("user.ctrl_regs.cnf_portcard_adapter.ctrl", 0);

        LOG(INFO) << GREEN << "Reading: Address " << BOLDYELLOW << std::to_string(thePair.first + 80) << RESET << GREEN << " Value " << BOLDYELLOW << std::to_string(fValueReadBack) << RESET;
        if(fValueReadBack != thePair.second)
        {
            LOG(ERROR) << BOLDRED << "[RD53FWInterface::ConfigurePCTestAdapter] Mismatch between set value and read value" << RESET;
            errorFlag = true;
        }
    }

    if(errorFlag == true) throw std::runtime_error("A mismatch between set value and read value has occurred");
}

void RD53FWInterface::SelectBERcheckBitORFrame(const uint8_t bitORframe) { RegManager::WriteReg("user.ctrl_regs.PRBS_checker.error_cntr_sel", bitORframe); }

void RD53FWInterface::WriteArbitraryRegister(const std::string& regName, const uint32_t value, const BeBoard* pBoard, ReadoutChipInterface* pReadoutChipInterface, const bool doReset)
{
    RegManager::WriteReg(regName, value);
    RD53FWInterface::SendBoardCommandWithStrobe("user.ctrl_regs.fast_cmd_reg_1.load_config");

    if(doReset == true)
    {
        RD53FWInterface::ResetBoard();
        RD53FWInterface::ConfigureBoard(pBoard);
        static_cast<RD53Interface*>(pReadoutChipInterface)->InitRD53Downlink(pBoard);
    }
}

// ###################
// # Clock generator #
// ###################

void RD53FWInterface::InitializeClockGenerator(const std::string& refClockRate, bool doStoreInEEPROM)
// ############################
// # refClockRate = 160 [MHz] #
// # refClockRate = 320 [MHz] #
// ############################
{
    const uint32_t writeSPI(0x8FA38014);    // Write to SPI @CONST@
    const uint32_t writeEEPROM(0x8FA38014); // Write to EEPROM @CONST@
    uint32_t       SPIregSettings[] = {
        0xEB020320, // OUT0 --> This clock is not used, but it can be used as another GBT clock (160 MHz, LVDS, phase shift 0 deg)
        0xEB020321, // OUT1 --> GBT clock reference: 160 MHz, LVDS, phase shift 0 deg (0xEB820321: 320 MHz, LVDS, phase shift 0 deg)
        0xEB840302, // OUT2 --> DDR3 clock reference: 240 MHz, LVDS, phase shift 0 deg
        0xEB840303, // OUT3 --> Not used (240 MHz, LVDS, phase shift 0 deg)
        0xEB140334, // OUT4 --> Not used (40 MHz, LVDS, R4.1 = 1, ph4adjc = 0)
        0x10000E75, // Reference selection: 0x10000E75 primary reference, 0x10000EB5 secondary reference
        0x030E02E6, // VCO selection: 0xyyyyyyEy select VCO1 if CDCE reference is 40 MHz, 0xyyyyyyFy select VCO2 if CDCE reference is > 40 MHz
                    // VCO1, PS = 4, FD = 12, FB = 1, ChargePump 50 uA, Internal Filter, R6.20 = 0, AuxOut = enable, AuxOut = OUT2
        0xBD800DF7, // RC network parameters: C2 = 473.5 pF, R2 = 98.6 kOhm, C1 = 0 pF, C3 = 0 pF, R3 = 5 kOhm etc, SEL_DEL1 = 1, SEL_DEL2 = 1
        0x80001808  // SYNC command configuration
    };

    // 0xyy8403yy --> 240 MHz, LVDS, phase shift   0 deg
    // 0xyy8407yy --> 240 MHz, LVDS, phase shift  90 deg
    // 0xyy840Byy --> 240 MHz, LVDS, phase shift 180 deg
    // 0xyy840Fyy --> 240 MHz, LVDS, phase shift 270 deg

    // 0xyy1403yy --> 040 MHz
    // 0xyy0403yy --> 120 MHz
    // 0xyy0203yy --> 160 MHz
    // 0xyy8403yy --> 240 MHz
    // 0xyy8203yy --> 320 MHz
    // 0xyy8003yy --> 480 MHz

    if(refClockRate == "160")
        SPIregSettings[1] = 0xEB020321;
    else if(refClockRate == "320")
        SPIregSettings[1] = 0xEB820321;
    else
        throw Exception("[RD53FWInterface::InitializeClockGenerator] CDCE reference clock rate not recognized");

    for(const auto value: SPIregSettings)
    {
        RegManager::WriteReg("system.spi.tx_data", value);
        RegManager::WriteReg("system.spi.command", writeSPI);

        RegManager::ReadReg("system.spi.rx_data"); // Dummy read
        RegManager::ReadReg("system.spi.rx_data"); // Dummy read
    }

    // ############################################################################
    // # Load new settings otherwise CDCE uses whatever was in EEPROM at power up #
    // ############################################################################
    RD53FWInterface::ToggleRegister("system.ctrl.cdce_sync", false);

    // #########################
    // # Save config in EEPROM #
    // #########################
    if(doStoreInEEPROM == true)
    {
        RegManager::WriteReg("system.spi.tx_data", writeEEPROM);
        RegManager::WriteReg("system.spi.command", writeSPI);

        RegManager::ReadReg("system.spi.rx_data"); // Dummy read
        RegManager::ReadReg("system.spi.rx_data"); // Dummy read
    }
}

void RD53FWInterface::ReadClockGenerator()
{
    const uint32_t writeSPI(0x8FA38014);                                                       // Write to SPI @CONST@
    const uint32_t SPIreadCommands[] = {0x0E, 0x1E, 0x2E, 0x3E, 0x4E, 0x5E, 0x6E, 0x7E, 0x8E}; // @CONST@

    LOG(INFO) << GREEN << "Reading clock generator (CDCE62005) configuration" << RESET;
    for(const auto value: SPIreadCommands)
    {
        RegManager::WriteReg("system.spi.tx_data", value);
        RegManager::WriteReg("system.spi.command", writeSPI);

        RegManager::WriteReg("system.spi.tx_data", 0xAAAAAAAA); // Dummy write
        RegManager::WriteReg("system.spi.command", writeSPI);

        uint32_t          readback = RegManager::ReadReg("system.spi.rx_data");
        std::stringstream myString("");
        myString << std::right << std::setfill('0') << std::setw(8) << std::hex << std::uppercase << readback << std::dec;
        LOG(INFO) << BOLDBLUE << "\t--> SPI register content: 0x" << BOLDYELLOW << std::hex << std::uppercase << myString.str() << std::dec << RESET;
    }
}

// #################################################
// # FMC ADC measurements: temperature and voltage #
// #################################################

float RD53FWInterface::ReadHybridTemperature(int hybridId, bool silentRunning)
{
    const float measError = 4.0; // Current or Voltage measurement error due to MonitorConfig resolution [%] @CONST@

    RegManager::WriteReg("user.ctrl_regs.i2c_block.dp_addr", hybridId);
    std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::DEEPSLEEP));
    uint32_t sensor1 = RegManager::ReadReg("user.stat_regs.i2c_block_1.NTC1");
    std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::DEEPSLEEP));
    uint32_t sensor2 = RegManager::ReadReg("user.stat_regs.i2c_block_1.NTC2");
    std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::DEEPSLEEP));

    auto value = calcTemperature(sensor1, sensor2);
    if(silentRunning == false)
        LOG(INFO) << BOLDBLUE << "\t--> " << BOLDYELLOW << "Hybrid" << BOLDBLUE << " temperature: " << BOLDYELLOW << std::setprecision(3) << value << " +/- " << value * measError / 100 << BOLDBLUE
                  << " C" << std::setprecision(-1) << RESET;

    return value;
}

float RD53FWInterface::ReadHybridVoltage(int hybridId, bool silentRunning)
{
    const float measError = 4.0; // Current or Voltage measurement error due to MonitorConfig resolution [%] @CONST@

    RegManager::WriteReg("user.ctrl_regs.i2c_block.dp_addr", hybridId);
    std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::DEEPSLEEP));
    uint32_t senseVDD = RegManager::ReadReg("user.stat_regs.i2c_block_2.vdd_sense");
    std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::DEEPSLEEP));
    uint32_t senseGND = RegManager::ReadReg("user.stat_regs.i2c_block_2.gnd_sense");
    std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::DEEPSLEEP));

    auto value = calcVoltage(senseVDD, senseGND);
    if(silentRunning == false)
        LOG(INFO) << BOLDBLUE << "\t--> " << BOLDYELLOW << "Hybrid" << BOLDBLUE " voltage: " << BOLDYELLOW << std::setprecision(3) << value << " +/- " << value * measError / 100 << BOLDBLUE << " V"
                  << std::setprecision(-1) << RESET;

    return value;
}

float RD53FWInterface::calcTemperature(uint32_t sensor1, uint32_t sensor2, int beta)
{
    // #####################
    // # Natural constants #
    // #####################
    const float T0C  = 273.15; // [Kelvin]
    const float T25C = 298.15; // [Kelvin]
    const float R25C = 10;     // [kOhm]
    // For precise T measurements we should have individual -beta- for each temperature sensor
    // i.e. NTC thermistors = Negative Temperature Coefficient, measured in Kelvin

    // ##################################
    // # Voltage divider circuit on FMC #
    // ##################################
    const float Rdivider = 39.2; // [kOhm]
    const float Vdivider = 2.5;  // [V]

    // ###################
    // # Voltage per LSB #
    // ###################
    const float  safetyMargin       = 0.9;
    const float  minimumTemperature = -35;                                               // [Celsius]
    const size_t numberOfBits       = 12;                                                // Related to the ADC on the FMC
    const float  VrefADC            = 2.048;                                             // FMC's ADC refence voltage [V]
    const float  ADC_LSB            = VrefADC / (RD53Shared::setBits(numberOfBits) + 1); // [V/ADC]

    // #####################
    // # Calculate voltage #
    // #####################
    float voltage = (sensor1 - sensor2) * ADC_LSB;
    if((voltage > ((RD53Shared::setBits(numberOfBits) + 1.) * safetyMargin * ADC_LSB)) || (voltage >= Vdivider))
    {
        LOG(WARNING) << BOLDRED << "\t\t--> Thermistor measurement in saturation: either very cold or floating (voltage = " << BOLDYELLOW << std::setprecision(3) << voltage << std::setprecision(-1)
                     << BOLDRED << ")" << RESET;
        return minimumTemperature;
    }

    // ###############################################
    // # Calculate temperature with NTC Beta formula #
    // ###############################################
    float resistance  = voltage * Rdivider / (Vdivider - voltage);              // [kOhm]
    float temperature = 1. / (1. / T25C + log(resistance / R25C) / beta) - T0C; // [Celsius]

    return temperature;
}

float RD53FWInterface::calcVoltage(uint32_t senseVDD, uint32_t senseGND)
{
    // ##################################
    // # Voltage divider circuit on FMC #
    // ##################################
    const float R1divider      = 196;  // [kOhm]
    const float R2divider      = 39.2; // [kOhm]
    const float VdividerFactor = (R1divider + R2divider) / R2divider;

    // ###################
    // # Voltage per LSB #
    // ###################
    const size_t numberOfBits = 12;                                                 // Related to the ADC on the FMC
    const float  VrefADC      = 2.048;                                              // FMC's ADC refence voltage [V]
    const float  ADC_LSB      = VrefADC / (RD53Shared::setBits(numberOfBits) + 1.); // [V/ADC]

    // #####################
    // # Calculate voltage #
    // #####################
    float voltage = (senseVDD - senseGND) * ADC_LSB * VdividerFactor;
    if(voltage < ADC_LSB * VdividerFactor)
        LOG(WARNING) << BOLDRED << "\t\t--> Very low voltage: either floating VDD sense-line or FMC not powered (voltage = " << BOLDYELLOW << std::setprecision(3) << voltage << std::setprecision(-1)
                     << BOLDRED << ")" << RESET;
    else if(voltage > VrefADC * VdividerFactor)
        LOG(WARNING) << BOLDRED << "\t\t--> Measured voltage below reference: senseVDD = " << BOLDYELLOW << senseVDD << BOLDRED << "; senseGND = " << BOLDYELLOW << senseGND << RESET;

    return voltage;
}

// #######################
// # Bit Error Rate test #
// #######################

std::vector<double> RD53FWInterface::RunBERtest(bool given_time, double frames_or_time, const std::map<uint32_t, std::vector<uint8_t>>& optgroup_id_hybrid_id_chip_id_chip_lanes, uint8_t frontendSpeed)
// ####################
// # frontendSpeed    #
// # 1.28 Gbit/s  = 0 #
// # 640 Mbit/s   = 1 #
// # 320 Mbit/s   = 2 #
// ####################
{
    const double bitPerFrame      = 32. * std::pow(2, frontendSpeed); // Bits per frame
    const double fps              = 1.28e9 / bitPerFrame;             // Frames per second: 32-bit frame @ 1.28 Gbit/s, 64-bit frame @ 640 Mbit/s, 128-bit frame @ 320 Mbit/s
    const int    nPrints          = 10;                               // Only an indication, the real number of printouts will be driven by the length of the time steps @CONST@
    const double scaleByAuroraClk = 37.5 / 40;                        // @CONST@
    double       frames2run;
    double       time2run;
    uint32_t     cntr_lo;
    uint32_t     cntr_hi;
    uint64_t     nErrors;
    uint16_t     FECcounter;

    if(given_time == true)
    {
        time2run   = frames_or_time;
        frames2run = time2run * fps * scaleByAuroraClk;
    }
    else
    {
        frames2run = frames_or_time;
        time2run   = frames2run / fps;
    }

    // ##########################################################################
    // # Configure number of printouts and calculate the frequency of printouts #
    // ##########################################################################
    double time_per_step = std::min(std::max(time2run / nPrints, 1.), 3600.); // The runtime of the PRBS test will have a precision of one step (at most 1h and at least 1s)

    // ##################
    // # Reset counters #
    // ##################
    RegManager::WriteStackReg({{"user.ctrl_regs.PRBS_checker.reset_cntr", 1}, {"user.ctrl_regs.PRBS_checker.reset_cntr", 0}});

    // ##########################
    // # Set PRBS frames to run #
    // ##########################
    uint32_t lowFrames, highFrames;
    std::tie(highFrames, lowFrames) = bits::unpack<32, 32>(static_cast<long long>(frames2run));
    RegManager::WriteStackReg({{"user.ctrl_regs.prbs_frames_to_run_low", lowFrames}, {"user.ctrl_regs.prbs_frames_to_run_high", highFrames}});
    RD53FWInterface::ToggleRegister("user.ctrl_regs.PRBS_checker.load_config");

    // #########
    // # Start #
    // #########
    RD53FWInterface::ToggleRegister("user.ctrl_regs.lpgbt_1.fec_cntr_clear");
    RD53FWInterface::ToggleRegister("user.ctrl_regs.PRBS_checker.start_checker");

    // #########################################
    // # Read frame counters to check progress #
    // #########################################
    LOG(INFO) << BOLDGREEN << std::fixed << std::setprecision(0) << "===== BER run starting @ " << BOLDYELLOW << bitPerFrame << BOLDGREEN << "-bits/frame  =====" << RESET;
    bool     runDone, forceDone;
    int      idx          = 1;
    uint64_t frameCounter = 0;
    do {
        std::this_thread::sleep_for(std::chrono::seconds(static_cast<unsigned int>(time_per_step)));

        forceDone = true;
        for(const auto& thePair: optgroup_id_hybrid_id_chip_id_chip_lanes)
        {
            uint8_t hybrid_id = (thePair.first >> 8) & 0xFF;
            uint8_t chip_id   = thePair.first & 0xFF;
            LOG(INFO) << GREEN << "\t--> Hybrid Id " << BOLDYELLOW << +hybrid_id << RESET << GREEN << " Chip internal Id " << BOLDYELLOW << +chip_id << RESET;
            for(const auto& lane: thePair.second)
            {
                RegManager::WriteStackReg(
                    {{"user.ctrl_regs.PRBS_checker.module_addr", hybrid_id}, {"user.ctrl_regs.PRBS_checker.chip_address", chip_id}, {"user.ctrl_regs.PRBS_checker.lane_addr", lane}});

                cntr_hi = RegManager::ReadReg("user.stat_regs.prbs_frame_cntr_high");
                cntr_lo = RegManager::ReadReg("user.stat_regs.prbs_frame_cntr_low");
                nErrors = RegManager::ReadReg("user.stat_regs.prbs_ber_cntr");

                if(bits::pack<32, 32>(cntr_hi, cntr_lo) == 0)
                    LOG(WARNING) << BOLDRED << "No clock was detected for Hybrid Id " << BOLDYELLOW << +hybrid_id << BOLDRED << " Chip internal Id " << BOLDYELLOW << +chip_id << BOLDRED
                                 << " Chip Lane " << BOLDYELLOW << +lane << RESET;
                else
                {
                    frameCounter = bits::pack<32, 32>(cntr_hi, cntr_lo);
                    forceDone    = false;

                    LOG(INFO) << GREEN << "\t\t--> Frames with error(s) (Chip Lane: " << BOLDYELLOW << +lane << RESET << GREEN << "): " << BOLDYELLOW << nErrors << RESET << GREEN << " (" << BOLDYELLOW
                              << std::fixed << std::setprecision(3) << nErrors / frameCounter * 100 << RESET << GREEN << "% of the sent frames)" << std::setprecision(-1) << RESET;
                }
            }
        }

        LOG(INFO) << GREEN << "I've been running for " << BOLDYELLOW << time_per_step * idx << RESET << GREEN << "s (" << BOLDYELLOW << std::fixed << std::setprecision(3)
                  << frameCounter / frames2run * 100. << std::setprecision(-1) << RESET << GREEN << "% done)" << RESET;

        if(given_time == true)
            runDone = (time_per_step * idx >= time2run);
        else
            runDone = (frameCounter >= frames2run);
        idx++;
    } while((runDone == false) && (forceDone == false));
    LOG(INFO) << BOLDGREEN << "========= Finished =========" << RESET;

    // ########
    // # Stop #
    // ########
    RD53FWInterface::ToggleRegister("user.ctrl_regs.PRBS_checker.stop_checker");

    // ###########################
    // # Read PRBS frame counter #
    // ###########################
    std::vector<double> results;
    uint8_t             optgroup_id_old = 0;
    bool                optgroup_update = true;
    LOG(INFO) << BOLDGREEN << "===== BER test summary =====" << RESET;
    for(const auto& thePair: optgroup_id_hybrid_id_chip_id_chip_lanes)
    {
        uint8_t optgroup_id = (thePair.first >> 16) & 0xFF;
        uint8_t hybrid_id   = (thePair.first >> 8) & 0xFF;
        uint8_t chip_id     = thePair.first & 0xFF;

        RD53FWInterface::selectLink(optgroup_id);
        if((optgroup_update == false) && (optgroup_id != optgroup_id_old)) optgroup_update = true;
        if(optgroup_update == true)
        {
            FECcounter = RegManager::ReadReg("user.stat_regs.lpgbt_monitoring.fec_cntr");
            LOG(INFO) << GREEN << "OpticalGroup Id " << BOLDYELLOW << +optgroup_id << RESET << GREEN << " has Forward Error Correction (FEC) counter: " << BOLDYELLOW << FECcounter << RESET;
            optgroup_update = false;
            optgroup_id_old = optgroup_id;
        }

        for(const auto& lane: thePair.second)
        {
            RegManager::WriteStackReg({{"user.ctrl_regs.PRBS_checker.module_addr", hybrid_id}, {"user.ctrl_regs.PRBS_checker.chip_address", chip_id}, {"user.ctrl_regs.PRBS_checker.lane_addr", lane}});

            cntr_hi      = RegManager::ReadReg("user.stat_regs.prbs_frame_cntr_high");
            cntr_lo      = RegManager::ReadReg("user.stat_regs.prbs_frame_cntr_low");
            frameCounter = bits::pack<32, 32>(cntr_hi, cntr_lo);
            nErrors      = RegManager::ReadReg("user.stat_regs.prbs_ber_cntr");
            results.push_back(nErrors / frames2run);

            LOG(INFO) << BOLDGREEN << "Hybrid Id " << BOLDYELLOW << +hybrid_id << BOLDGREEN << " Chip internal Id " << BOLDYELLOW << +chip_id << BOLDGREEN << " Chip Lane " << BOLDYELLOW << +lane
                      << RESET;
            LOG(INFO) << GREEN << "Number of PRBS frames sent: " << BOLDYELLOW << frameCounter << RESET;
            LOG(INFO) << GREEN << "Frames with error(s): " << BOLDYELLOW << nErrors << RESET;
            LOG(INFO) << GREEN << "Frame Error Rate: " << BOLDYELLOW << nErrors / time2run << RESET << GREEN << " frames/s (" << BOLDYELLOW << std::fixed << std::setprecision(3)
                      << results.back() * 100 << RESET << GREEN << "%)" << std::setprecision(-1) << RESET;
            LOG(INFO) << GREEN << "BER test result: " << (nErrors == 0 ? BOLDYELLOW : BOLDRED) << (nErrors == 0 ? "PASSED" : "NOT PASSED") << RESET;
        }
    }
    LOG(INFO) << BOLDGREEN << "====== End of summary ======" << RESET;

    return results;
}

} // namespace Ph2_HwInterface
