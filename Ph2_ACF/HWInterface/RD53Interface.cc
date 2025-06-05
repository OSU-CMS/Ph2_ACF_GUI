/*!
  \file                  RD53Interface.cc
  \brief                 User interface to the RD53 readout chip
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#include "HWInterface/RD53Interface.h"

using namespace Ph2_HwDescription;

namespace Ph2_HwInterface
{
RD53Interface::RD53Interface(const BeBoardFWMap& pBoardMap) : ReadoutChipInterface(pBoardMap) {}

bool RD53Interface::WriteChipReg(Chip* pChip, const std::string& regName, const uint16_t data, bool pVerify)
{
    this->setBoard(pChip->getBeBoardId());
    std::unique_lock<std::recursive_mutex> theGuard(fBoardFW->fMutex);

    auto pRD53 = static_cast<RD53*>(pChip);

    const auto            nameAndValue(SetSpecialRegister(regName, data, pChip->getRegMap()));
    std::vector<uint16_t> cmdStream;
    PackWriteCommand(pChip, nameAndValue.first, nameAndValue.second, cmdStream, pVerify);
    if(static_cast<RD53FWInterface*>(fBoardFW)->WriteChipCommands(cmdStream, pChip->getHybridId()) == false)
    {
        static_cast<RD53FWInterface*>(fBoardFW)->ResetReadBkFIFO(); // @TMP@ : Temporary fix to avoid FIFO empty at readback
        SendRD53Clear(pRD53);                                       // @TMP@ : Temporary fix to avoid FIFO empty at readback
        std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::READOUTSLEEP));
    }

    if((regName == "VCAL_HIGH") || (regName == "VCAL_MED"))
        std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::firstChip->getFEtype(RD53Shared::firstChip->getNCols() / 2, RD53Shared::firstChip->getNCols() / 2)->VCalSleepTime));

    theGuard.unlock();

    bool    status      = true;
    int32_t actualValue = 0;
    if(pVerify == true)
    {
        if(regName == "PIX_PORTAL")
        {
            auto pixMode = RD53Interface::ReadChipReg(pChip, "PIX_MODE");
            if((pixMode & RD53Shared::firstChip->getFEtype(RD53Shared::firstChip->getNCols() / 2, RD53Shared::firstChip->getNCols() / 2)->AutoIncrementMask) == 0) // Check only auto-increment bits
            {
                auto regReadback = ReadRD53Reg(pRD53, regName);
                actualValue      = regReadback[0].second;
                auto row         = RD53Interface::ReadChipReg(pChip, "REGION_ROW");
                if((regReadback.size() == 0) || (regReadback[0].first != row) || (regReadback[0].second != data)) status = false;
            }
        }
        else
        {
            actualValue = RD53Interface::ReadChipReg(pChip, nameAndValue.first);
            if(nameAndValue.second != actualValue) status = false;
        }
    }

    if(RD53Interface::silentRunning == false)
    {
        if(status == false)
            LOG(ERROR) << BOLDRED << "Error when reading back what was written into RD53 id " << BOLDYELLOW << pChip->getId() << BOLDRED << " reg. " << BOLDYELLOW << regName << BOLDRED
                       << ": wrote = " << BOLDYELLOW << nameAndValue.second << BOLDRED << ", read = " << BOLDYELLOW << actualValue << RESET;
        else if((pVerify == true) && (status == true))
            LOG(DEBUG) << BOLDBLUE << "\t--> Succesfully configured chip register " << BOLDYELLOW << regName << RESET;
    }

    // #######################################
    // # Update both real and fake registers #
    // #######################################
    pChip->setReg(regName, data);
    pChip->setReg(nameAndValue.first, nameAndValue.second);

    return status;
}

void RD53Interface::WriteBoardBroadcastChipReg(const BeBoard* pBoard, const std::string& regName, const uint16_t data)
{
    this->setBoard(pBoard->getId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);

    auto                  nameAndValue(SetSpecialRegister(regName, data, RD53Shared::firstChip->getRegMap()));
    std::vector<uint16_t> cmdStream;
    PackWriteBroadcastCommand(pBoard, nameAndValue.first, nameAndValue.second, cmdStream);
    static_cast<RD53FWInterface*>(fBoardFW)->WriteChipCommands(cmdStream, -1);

    if((regName == "VCAL_HIGH") || (regName == "VCAL_MED"))
        std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::firstChip->getFEtype(RD53Shared::firstChip->getNCols() / 2, RD53Shared::firstChip->getNCols() / 2)->VCalSleepTime));
}

int32_t RD53Interface::ReadChipReg(Chip* pChip, const std::string& regName)
{
    this->setBoard(pChip->getBeBoardId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);

    auto pRD53 = static_cast<RD53*>(pChip);

    for(auto attempt = 0; attempt < RD53Shared::MAXATTEMPTS; attempt++)
    {
        auto regReadback = ReadRD53Reg(pRD53, regName);
        if(regReadback.size() == 0)
        {
            if(RD53Interface::silentRunning == false)
                LOG(WARNING) << BLUE << "Empty register readback from chip id " << BOLDYELLOW << pChip->getId() << RESET << BLUE << ", attempt n. " << BOLDYELLOW << attempt + 1 << RESET << BLUE << "/"
                             << BOLDYELLOW << +RD53Shared::MAXATTEMPTS << RESET;
            static_cast<RD53FWInterface*>(fBoardFW)->ResetReadBkFIFO(); // @TMP@ : Temporary fix to avoid FIFO empty at readback
            SendRD53Clear(pRD53);                                       // @TMP@ : Temporary fix to avoid FIFO empty at readback
            std::this_thread::sleep_for(std::chrono::microseconds(RD53Shared::READOUTSLEEP));
        }
        else
            return regReadback[0].second;
    }

    if(RD53Interface::silentRunning == false)
        LOG(ERROR) << BOLDRED << "Empty register (" << BOLDYELLOW << regName << BOLDRED << ") readback FIFO after " << BOLDYELLOW << +RD53Shared::MAXATTEMPTS << BOLDRED " attempts" << RESET;

    return -1;
}

bool RD53Interface::ConfigureChipOriginalMask(ReadoutChip* pChip, bool pVerify, uint32_t pBlockSize)
{
    auto pRD53 = static_cast<RD53*>(pChip);

    WriteRD53Mask(pRD53, false, true);

    return true;
}

bool RD53Interface::MaskAllChannels(ReadoutChip* pChip, bool mask, bool pVerify)
{
    auto pRD53 = static_cast<RD53*>(pChip);

    if(mask == true)
        pRD53->disableAllPixels();
    else
        pRD53->enableAllPixels();

    WriteRD53Mask(pRD53, false, false);

    return true;
}

bool RD53Interface::maskChannelsAndSetInjectionSchema(ReadoutChip* pChip, const std::shared_ptr<ChannelGroupBase> group, bool mask, bool inject, bool pVerify)
{
    auto  pRD53          = static_cast<RD53*>(pChip);
    auto& pixMaskDefault = pRD53->getPixelsMaskDefault();
    auto& pixMask        = pRD53->getPixelsMask();
    auto  pRD53group     = std::static_pointer_cast<RD53ChannelGroup>(group);

    // ##########
    // # Enable #
    // ##########
    if(mask == true)
    {
        std::transform(pixMaskDefault.Enable.begin(), pixMaskDefault.Enable.end(), pRD53group->getMask().begin(), pixMask.Enable.begin(), std::logical_and<>{});
        std::transform(pixMaskDefault.Enable.begin(), pixMaskDefault.Enable.end(), pRD53group->getMask().begin(), pixMask.InjEn.begin(), std::logical_and<>{});
        if(inject == false) std::transform(pixMask.InjEn.begin(), pixMask.InjEn.end(), pixMaskDefault.InjEn.begin(), pixMask.InjEn.begin(), std::logical_and<>{});
    }

    // ##########
    // # Inject #
    // ##########
    if((pRD53group->groupType == RD53GroupType::XtalkCoupled) || (pRD53group->groupType == RD53GroupType::XtalkDeCoupled))
        pixMask.InjEn = pRD53group->getMaskNext(pRD53group->groupType);
    else if(pRD53group->groupType == RD53GroupType::Custom)
        pixMask.InjEn = pixMaskDefault.InjEn;

    // #########
    // # Apply #
    // #########
    WriteRD53Mask(pRD53, true, false);

    return true;
}

void RD53Interface::DumpChipRegisters(ReadoutChip* pChip)
{
    this->setBoard(pChip->getBeBoardId());

    for(auto& cRegItem: pChip->getRegMap())
    {
        auto value = RD53Interface::ReadChipReg(pChip, cRegItem.first);
        std::cout << "\t--> Register " << std::left << std::setfill(' ') << std::setw(24) << cRegItem.first << " = " << std::setw(8) << std::dec << value << std::hex << "(0x" << value << ")"
                  << std::endl;
    }
}

void RD53Interface::EnDisChip(Chip* pChip, std::vector<uint16_t>& chipCommandList, bool enable)
{
    for(const auto& regName: static_cast<RD53*>(pChip)->getFEtype()->CoreColRegs)
    {
        const uint16_t value = (enable == true ? RD53Shared::setBits(pChip->getNumberOfBits(regName)) & pChip->getRegItem(regName).fDefValue : 0);
        PackWriteCommand(pChip, regName, value, chipCommandList, true);
    }
}

void RD53Interface::ChipErrorReport(Chip* pChip)
{
    this->setBoard(pChip->getBeBoardId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);

    const uint16_t baseAddrFakeCNT = 0x200; // @CONST@

    if(pChip->getRegItem("BCID_CNT").fAddress < baseAddrFakeCNT)
        LOG(INFO) << BOLDBLUE << "BCID_CNT            = " << BOLDYELLOW << std::setw(6) << std::fixed << RD53Interface::ReadChipReg(pChip, "BCID_CNT") << RESET;
    if(pChip->getRegItem("TRIG_CNT").fAddress < baseAddrFakeCNT)
        LOG(INFO) << BOLDBLUE << "TRIG_CNT            = " << BOLDYELLOW << std::setw(6) << std::fixed << RD53Interface::ReadChipReg(pChip, "TRIG_CNT") << RESET;
    if(pChip->getRegItem("READTRIG_CNT").fAddress < baseAddrFakeCNT)
        LOG(INFO) << BOLDBLUE << "READTRIG_CNT        = " << BOLDYELLOW << std::setw(6) << std::fixed << RD53Interface::ReadChipReg(pChip, "READTRIG_CNT") << RESET;
    if(pChip->getRegItem("LOCKLOSS_CNT").fAddress < baseAddrFakeCNT)
        LOG(INFO) << BOLDBLUE << "LOCKLOSS_CNT        = " << BOLDYELLOW << std::setw(6) << std::fixed << RD53Interface::ReadChipReg(pChip, "LOCKLOSS_CNT") << RESET;
    if(pChip->getRegItem("BITFLIP_WNG_CNT").fAddress < baseAddrFakeCNT)
        LOG(INFO) << BOLDBLUE << "BITFLIP_WNG_CNT     = " << BOLDYELLOW << std::setw(6) << std::fixed << RD53Interface::ReadChipReg(pChip, "BITFLIP_WNG_CNT") << RESET;
    if(pChip->getRegItem("BITFLIP_ERR_CNT").fAddress < baseAddrFakeCNT)
        LOG(INFO) << BOLDBLUE << "BITFLIP_ERR_CNT     = " << BOLDYELLOW << std::setw(6) << std::fixed << RD53Interface::ReadChipReg(pChip, "BITFLIP_ERR_CNT") << RESET;
    if(pChip->getRegItem("CMDERR_CNT").fAddress < baseAddrFakeCNT)
        LOG(INFO) << BOLDBLUE << "CMDERR_CNT          = " << BOLDYELLOW << std::setw(6) << std::fixed << RD53Interface::ReadChipReg(pChip, "CMDERR_CNT") << RESET;
    if(pChip->getRegItem("RDWRFIFOERROR_CNT").fAddress < baseAddrFakeCNT)
        LOG(INFO) << BOLDBLUE << "RDWRFIFOERROR_CNT   = " << BOLDYELLOW << std::setw(6) << std::fixed << RD53Interface::ReadChipReg(pChip, "RDWRFIFOERROR_CNT") << RESET;
    if(pChip->getRegItem("HITOR_0_CNT").fAddress < baseAddrFakeCNT)
        LOG(INFO) << BOLDBLUE << "HITOR_0_CNT         = " << BOLDYELLOW << std::setw(6) << std::fixed << RD53Interface::ReadChipReg(pChip, "HITOR_0_CNT") << RESET;
    if(pChip->getRegItem("HITOR_1_CNT").fAddress < baseAddrFakeCNT)
        LOG(INFO) << BOLDBLUE << "HITOR_1_CNT         = " << BOLDYELLOW << std::setw(6) << std::fixed << RD53Interface::ReadChipReg(pChip, "HITOR_1_CNT") << RESET;
    if(pChip->getRegItem("HITOR_2_CNT").fAddress < baseAddrFakeCNT)
        LOG(INFO) << BOLDBLUE << "HITOR_2_CNT         = " << BOLDYELLOW << std::setw(6) << std::fixed << RD53Interface::ReadChipReg(pChip, "HITOR_2_CNT") << RESET;
    if(pChip->getRegItem("HITOR_3_CNT").fAddress < baseAddrFakeCNT)
        LOG(INFO) << BOLDBLUE << "HITOR_3_CNT         = " << BOLDYELLOW << std::setw(6) << std::fixed << RD53Interface::ReadChipReg(pChip, "HITOR_3_CNT") << RESET;
    if(pChip->getRegItem("GatedHITOR_0_CNT").fAddress < baseAddrFakeCNT)
        LOG(INFO) << BOLDBLUE << "GatedHITOR_0_CNT    = " << BOLDYELLOW << std::setw(6) << std::fixed << RD53Interface::ReadChipReg(pChip, "GatedHITOR_0_CNT") << RESET;
    if(pChip->getRegItem("GatedHITOR_1_CNT").fAddress < baseAddrFakeCNT)
        LOG(INFO) << BOLDBLUE << "GatedHITOR_1_CNT    = " << BOLDYELLOW << std::setw(6) << std::fixed << RD53Interface::ReadChipReg(pChip, "GatedHITOR_1_CNT") << RESET;
    if(pChip->getRegItem("PIXELSEU_CNT").fAddress < baseAddrFakeCNT)
        LOG(INFO) << BOLDBLUE << "PIXELSEU_CNT        = " << BOLDYELLOW << std::setw(6) << std::fixed << RD53Interface::ReadChipReg(pChip, "PIXELSEU_CNT") << RESET;
    if(pChip->getRegItem("GLOBALCONFIGSEU_CNT").fAddress < baseAddrFakeCNT)
        LOG(INFO) << BOLDBLUE << "GLOBALCONFIGSEU_CNT = " << BOLDYELLOW << std::setw(6) << std::fixed << RD53Interface::ReadChipReg(pChip, "GLOBALCONFIGSEU_CNT") << RESET;
    if(pChip->getRegItem("SKIPPED_TRIGGER_CNT").fAddress < baseAddrFakeCNT)
        LOG(INFO) << BOLDBLUE << "SKIPPED_TRIGGER_CNT = " << BOLDYELLOW << std::setw(6) << std::fixed << RD53Interface::ReadChipReg(pChip, "SKIPPED_TRIGGER_CNT") << RESET;
}

uint16_t RD53Interface::SetFieldValue(uint16_t regValue, uint16_t fieldValue, uint8_t start, uint8_t size)
{
    uint16_t mask = ((1 << size) - 1) << start;
    return regValue ^ ((regValue ^ (fieldValue << start)) & mask);
}

uint16_t RD53Interface::GetFieldValue(uint16_t regValue, uint8_t start, uint8_t size)
{
    uint16_t mask = (1 << size) - 1;
    return (regValue >> start) & mask;
}

// ##################
// # PRBS generator #
// ##################

void RD53Interface::StartPRBSpattern(const BeBoard* pBoard) { RD53Interface::WriteBoardBroadcastChipReg(pBoard, "SER_SEL_OUT", RD53Constants::PATTERN_PRBS); }
void RD53Interface::StopPRBSpattern(const BeBoard* pBoard) { RD53Interface::WriteBoardBroadcastChipReg(pBoard, "SER_SEL_OUT", RD53Constants::PATTERN_AURORA); }

bool RD53Interface::WriteChipAllLocalReg(ReadoutChip* pChip, const std::string& regName, const ChipContainer& pValue, bool pVerify)
{
    auto pRD53 = static_cast<RD53*>(pChip);

    for(auto col = 0u; col < pRD53->getNCols(); col++)
        for(auto row = 0u; row < pRD53->getNRows(); row++) pRD53->setTDAC(row, col, pValue.getChannel<uint16_t>(row, col));

    WriteRD53Mask(pRD53, false, false);

    return true;
}

void RD53Interface::ReadChipAllLocalReg(ReadoutChip* pChip, const std::string& regName, ChipContainer& pValue)
{
    auto pRD53 = static_cast<RD53*>(pChip);
    for(auto col = 0u; col < pRD53->getNCols(); col++)
        for(auto row = 0u; row < pRD53->getNRows(); row++) pValue.getChannel<uint16_t>(row, col) = static_cast<RD53*>(pChip)->getTDAC(row, col);
}

void RD53Interface::SendChipCommands(const BeBoard* pBoard, const std::vector<uint16_t>& chipCommandList, int hybridId)
{
    this->setBoard(pBoard->getId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);
    static_cast<RD53FWInterface*>(fBoardFW)->WriteChipCommands(chipCommandList, hybridId);
}

void RD53Interface::PackHybridCommands(const BeBoard* pBoard, const std::vector<uint16_t>& chipCommandList, int hybridId, std::vector<uint32_t>& hybridCommandList)
{
    static_cast<RD53FWInterface*>(fBoardFW)->ComposeAndPackChipCommands(chipCommandList, hybridId, hybridCommandList);
}

void RD53Interface::SendHybridCommands(const BeBoard* pBoard, const std::vector<uint32_t>& hybridCommandList)
{
    this->setBoard(pBoard->getId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);
    static_cast<RD53FWInterface*>(fBoardFW)->SendChipCommands(hybridCommandList);
}

// ###########################
// # Dedicated to monitoring #
// ###########################

float RD53Interface::ReadChipMonitor(ReadoutChip* pChip, const std::string& observableName, bool silentRunning)
{
    this->setBoard(pChip->getBeBoardId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);

    const float measError = 4.0; // Current or Voltage measurement error due to MonitorConfig resolution [%]
    float       value;
    bool        isCurrentNotVoltage;
    uint32_t    observable = getADCobservable(observableName, isCurrentNotVoltage);

    if((observableName.find("TEMPSENS") != std::string::npos) || (observableName.find("RADSENS") != std::string::npos) || (observableName.find("INTERNAL_NTC") != std::string::npos))
    {
        value = measureTemperature(pChip, observable, observableName, pChip->getRegItem("NTCBETA").fValue);
        if(silentRunning == false)
            LOG(INFO) << BOLDBLUE << "\t--> " << BOLDYELLOW << observableName << BOLDBLUE << ": " << BOLDYELLOW << std::setprecision(3) << value << " +/- " << value * measError / 100 << BOLDBLUE
                      << " C" << std::setprecision(-1) << RESET;
    }
    else
    {
        value = RD53Interface::measureVoltageCurrent(pChip, observable, isCurrentNotVoltage);
        if(silentRunning == false)
            LOG(INFO) << BOLDBLUE << "\t--> " << BOLDYELLOW << observableName << BOLDBLUE << ": " << BOLDYELLOW << std::setprecision(3) << value << " +/- " << value * measError / 100 << BOLDBLUE
                      << (isCurrentNotVoltage == true ? " uA" : " V") << std::setprecision(-1) << RESET;
    }

    // std::this_thread::sleep_for(std::chrono::milliseconds(RD53Shared::SUPERDEEPSLEEP)); // @TMP@

    return value;
}

uint32_t RD53Interface::ReadChipADC(Ph2_HwDescription::ReadoutChip* pChip, const std::string& observableName)
{
    bool isCurrentNotVoltage;
    return measureADC(pChip, getADCobservable(observableName, isCurrentNotVoltage));
}

float RD53Interface::convertADC2VorI(ReadoutChip* pChip, uint32_t value, bool isCurrentNotVoltage)
// ######################################
// # Voltage output units: Volt         #
// # Current output units: micro-Ampere #
// ######################################
{
    // ################################################################################
    // # resistorI2V   = 0.01-0.005 [MOhm] Resistor for current to voltage conversion #
    // # ADCoffset     =  63 [1/10 mV]     Offset due to ground shift                 #
    // # actualVrefADC = 839 [mV]          Lower than VrefADC due to parasitics       #
    // ################################################################################

    const float resistorI2V   = pChip->getRegItem("RESISTORI2V").fValue / 1e6; // [MOhm]
    const float ADCoffset     = pChip->getRegItem("ADC_OFFSET_VOLT").fValue / 1e4;
    const float actualVrefADC = pChip->getRegItem("ADC_MAXIMUM_VOLT").fValue / 1e3;

    const float ADCslope = (actualVrefADC - ADCoffset) / (RD53Shared::setBits(pChip->getNumberOfBits("MonitoringDataADC")) + 1); // [V/ADC]
    const float voltage  = ADCoffset + ADCslope * value;

    return voltage / (isCurrentNotVoltage == true ? resistorI2V : 1);
}

float RD53Interface::measureVoltageCurrent(ReadoutChip* pChip, uint32_t data, bool isCurrentNotVoltage)
{
    const float safetyMargin = 0.9; // @CONST@

    auto ADC = measureADC(pChip, data);
    if(ADC > (RD53Shared::setBits(pChip->getNumberOfBits("MonitoringDataADC")) + 1.) * safetyMargin)
        LOG(WARNING) << BOLDRED << "\t\t--> ADC measurement in saturation (ADC = " << BOLDYELLOW << ADC << BOLDRED
                     << "): likely the IMUX resistor, that converts the current into a voltage, is not connected" << RESET;

    return RD53Interface::convertADC2VorI(pChip, ADC, isCurrentNotVoltage);
}

float RD53Interface::ReadHybridTemperature(ReadoutChip* pChip, bool silentRunning)
{
    this->setBoard(pChip->getBeBoardId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);
    return static_cast<RD53FWInterface*>(fBoardFW)->ReadHybridTemperature(pChip->getHybridId(), silentRunning);
}

float RD53Interface::ReadHybridVoltage(ReadoutChip* pChip, bool silentRunning)
{
    this->setBoard(pChip->getBeBoardId());
    std::lock_guard<std::recursive_mutex> theGuard(fBoardFW->fMutex);
    return static_cast<RD53FWInterface*>(fBoardFW)->ReadHybridVoltage(pChip->getHybridId(), silentRunning);
}

} // namespace Ph2_HwInterface
