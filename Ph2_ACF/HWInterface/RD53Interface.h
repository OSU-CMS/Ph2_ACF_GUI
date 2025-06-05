/*!
  \file                  RD53Interface.h
  \brief                 User interface to the RD53 readout chip
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef RD53Interface_H
#define RD53Interface_H

#include "HWDescription/ChipRegItem.h"
#include "HWDescription/RD53.h"
#include "HWDescription/RD53A.h"
#include "HWDescription/RD53B.h"
#include "HWInterface/BeBoardFWInterface.h"
#include "HWInterface/RD53FWInterface.h"
#include "HWInterface/ReadoutChipInterface.h"
#include "Utils/RD53ChannelGroupHandler.h"

namespace Ph2_HwInterface
{
class RD53Interface : public ReadoutChipInterface
{
  public:
    RD53Interface(const BeBoardFWMap& pBoardMap);

    // #############################
    // # Override member functions #
    // #############################
    bool    WriteChipReg(Ph2_HwDescription::Chip* pChip, const std::string& regName, uint16_t data, bool pVerify = true) override;
    void    WriteBoardBroadcastChipReg(const Ph2_HwDescription::BeBoard* pBoard, const std::string& regName, uint16_t data) override;
    bool    WriteChipAllLocalReg(Ph2_HwDescription::ReadoutChip* pChip, const std::string& regName, const ChipContainer& pValue, bool pVerify = true) override;
    void    ReadChipAllLocalReg(Ph2_HwDescription::ReadoutChip* pChip, const std::string& regName, ChipContainer& pValue) override;
    int32_t ReadChipReg(Ph2_HwDescription::Chip* pChip, const std::string& regName) override;
    bool    ConfigureChipOriginalMask(Ph2_HwDescription::ReadoutChip* pChip, bool pVerify = true, uint32_t pBlockSize = 310) override;
    bool    MaskAllChannels(Ph2_HwDescription::ReadoutChip* pChip, bool mask, bool pVerify = true) override;
    bool    maskChannelsAndSetInjectionSchema(Ph2_HwDescription::ReadoutChip* pChip, const std::shared_ptr<ChannelGroupBase> group, bool mask, bool inject, bool pVerify = false) override;
    void    DumpChipRegisters(Ph2_HwDescription::ReadoutChip* pChip) override;
    // #############################

    // ##################
    // # PRBS generator #
    // ##################
    void StartPRBSpattern(const Ph2_HwDescription::BeBoard* pBoard);
    void StopPRBSpattern(const Ph2_HwDescription::BeBoard* pBoard);

    virtual void InitRD53Downlink(const Ph2_HwDescription::BeBoard* pBoard)                                                                                                                    = 0;
    virtual void InitRD53Uplinks(Ph2_HwDescription::Chip* pChip)                                                                                                                               = 0;
    virtual void TAP0slaveOptimization(const Ph2_HwDescription::BeBoard* pBoard, const Ph2_HwDescription::Hybrid* pHybrid)                                                                     = 0;
    virtual void PackWriteCommand(Ph2_HwDescription::Chip* pChip, const std::string& regName, uint16_t data, std::vector<uint16_t>& chipCommandList, bool updateReg = true)                    = 0;
    virtual void PackWriteBroadcastCommand(const Ph2_HwDescription::BeBoard* pBoard, const std::string& regName, uint16_t data, std::vector<uint16_t>& chipCommandList, bool updateReg = true) = 0;
    virtual void WriteClockDataDelay(Ph2_HwDescription::Chip* pChip, uint16_t value)                                                                                                           = 0;
    virtual void WriteRD53Mask(Ph2_HwDescription::RD53* pRD53, int writeMode, bool doDefault, size_t theRow = 0, size_t theCol = 0)                                                            = 0;
    virtual void SendBoardClear(const Ph2_HwDescription::BeBoard* pBoard)                                                                                                                      = 0;
    virtual void SendRD53Clear(Ph2_HwDescription::RD53* pRD53)                                                                                                                                 = 0;

    void EnDisChip(Ph2_HwDescription::Chip* pChip, std::vector<uint16_t>& chipCommandList, bool enable);
    void ChipErrorReport(Ph2_HwDescription::Chip* pChip);
    void SendChipCommands(const Ph2_HwDescription::BeBoard* pBoard, const std::vector<uint16_t>& chipCommandList, int hybridId);
    void PackHybridCommands(const Ph2_HwDescription::BeBoard* pBoard, const std::vector<uint16_t>& chipCommandList, int hybridId, std::vector<uint32_t>& hybridCommandList);
    void SendHybridCommands(const Ph2_HwDescription::BeBoard* pBoard, const std::vector<uint32_t>& hybridCommandList);

    // ######################################################################################################
    // # SetSpecialRegister                                                                                 #
    // # Receives: any register (real or fake) and a value                                                  #
    // # Returns: real register name together with its value containing the input value in the proper field #
    // ######################################################################################################
    virtual std::pair<std::string, uint16_t> SetSpecialRegister(std::string regName, uint16_t value, Ph2_HwDescription::ChipRegMap& pRD53RegMap)      = 0;
    virtual uint16_t                         GetSpecialRegisterValue(std::string regName, uint16_t value, Ph2_HwDescription::ChipRegMap& pRD53RegMap) = 0;

    bool silentRunning{false};

  private:
    virtual std::vector<std::pair<uint16_t, uint16_t>> ReadRD53Reg(Ph2_HwDescription::ReadoutChip* pChip, const std::string& regName) = 0;

  protected:
    template <typename T>
    void SendCommand(Ph2_HwDescription::Chip* pChip, const T& cmd)
    {
        static_cast<RD53FWInterface*>(fBoardFW)->WriteChipCommands(serialize(cmd), pChip->getHybridId());
    }

    uint16_t SetFieldValue(uint16_t regValue, uint16_t fieldValue, uint8_t start, uint8_t size);
    uint16_t GetFieldValue(uint16_t regValue, uint8_t start, uint8_t size);

    // ###########################
    // # Dedicated to monitoring #
    // ###########################
  public:
    virtual int getADCobservable(const std::string& observableName, bool& isCurrentNotVoltage, bool silentRunning = false) = 0;
    void        ReadChipMonitor(Ph2_HwDescription::ReadoutChip* pChip, const std::vector<std::string>& args, bool silentRunning = false)
    {
        for(const auto& arg: args) ReadChipMonitor(pChip, arg, silentRunning);
    }
    float    ReadChipMonitor(Ph2_HwDescription::ReadoutChip* pChip, const std::string& observableName, bool silentRunning = false);
    uint32_t ReadChipADC(Ph2_HwDescription::ReadoutChip* pChip, const std::string& observableName);
    float    ReadHybridTemperature(Ph2_HwDescription::ReadoutChip* pChip, bool silentRunning = false);
    float    ReadHybridVoltage(Ph2_HwDescription::ReadoutChip* pChip, bool silentRunning = false);
    float    convertADC2VorI(Ph2_HwDescription::ReadoutChip* pChip, uint32_t value, bool isCurrentNotVoltage = false);

  private:
    virtual uint32_t measureADC(Ph2_HwDescription::ReadoutChip* pChip, uint32_t data)                                            = 0;
    virtual float    measureTemperature(Ph2_HwDescription::ReadoutChip* pChip, uint32_t data, const std::string& type, int beta) = 0;
    float            measureVoltageCurrent(Ph2_HwDescription::ReadoutChip* pChip, uint32_t data, bool isCurrentNotVoltage);
};

} // namespace Ph2_HwInterface

#endif
