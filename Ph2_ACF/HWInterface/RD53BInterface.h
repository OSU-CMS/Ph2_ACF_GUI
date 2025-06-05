/*!
  \file                  RD53BInterface.h
  \brief                 User interface to the RD53B readout chip
  \author                Mauro DINARDO and Alkiviadis PAPADOPOULOS
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
  Support:               email to alkiviadis.papadopoulos@cern.ch
*/

#ifndef RD53BInterface_H
#define RD53BInterface_H

#include "HWDescription/RD53B.h"
#include "HWDescription/RD53BCommands.h"
#include "HWInterface/RD53Interface.h"

namespace Ph2_HwInterface
{
class RD53BInterface : public RD53Interface
{
  public:
    using RD53Interface::RD53Interface;

    // #############################
    // # Override member functions #
    // #############################
    bool     ConfigureChip(Ph2_HwDescription::Chip* pChip, bool pVerify = true, uint32_t pBlockSize = 310) override;
    void     InitRD53Downlink(const Ph2_HwDescription::BeBoard* pBoard) override;
    void     InitRD53Uplinks(Ph2_HwDescription::Chip* pChip) override;
    void     TAP0slaveOptimization(const Ph2_HwDescription::BeBoard* pBoard, const Ph2_HwDescription::Hybrid* pHybrid) override;
    void     PackWriteCommand(Ph2_HwDescription::Chip* pChip, const std::string& regName, uint16_t data, std::vector<uint16_t>& chipCommandList, bool updateReg = true) override;
    void     PackWriteBroadcastCommand(const Ph2_HwDescription::BeBoard* pBoard, const std::string& regName, uint16_t data, std::vector<uint16_t>& chipCommandList, bool updateReg = true) override;
    void     WriteClockDataDelay(Ph2_HwDescription::Chip* pChip, uint16_t value) override;
    uint32_t ReadChipFuseID(Ph2_HwDescription::Chip* pChip, uint8_t version = 1) override;
    void     WriteRD53Mask(Ph2_HwDescription::RD53* pRD53, int writeMode, bool doDefault, size_t theRow = 0, size_t theCol = 0) override;
    void     SendBoardClear(const Ph2_HwDescription::BeBoard* pBoard) override;
    void     SendRD53Clear(Ph2_HwDescription::RD53* pRD53) override { RD53Interface::SendCommand(pRD53, RD53BCmd::Clear{pRD53->getId()}); }

    std::pair<std::string, uint16_t> SetSpecialRegister(std::string regName, uint16_t value, Ph2_HwDescription::ChipRegMap& pRD53RegMap) override;
    uint16_t                         GetSpecialRegisterValue(std::string regName, uint16_t value, Ph2_HwDescription::ChipRegMap& pRD53RegMap) override;
    // #############################

  private:
    std::vector<std::pair<uint16_t, uint16_t>> ReadRD53Reg(Ph2_HwDescription::ReadoutChip* pChip, const std::string& regName) override;

    uint16_t GetPixelConfig(const Ph2_HwDescription::pixelMask& mask, uint16_t row, uint16_t col);
    uint16_t GetPixelConfigMask(const Ph2_HwDescription::pixelMask& mask, uint16_t row, uint16_t col);
    uint16_t GetPixelConfigTDAC(const Ph2_HwDescription::pixelMask& mask, uint16_t row, uint16_t col);
    void     SendGlobalPulse(Ph2_HwDescription::Chip* pChip, uint16_t route, uint16_t pulseDuration);
    void     SendGlobalPulseBroadcast(const Ph2_HwDescription::BeBoard* pBoard);
    void     SendChipCommandsWithSync(Ph2_HwDescription::RD53* pRD53, const std::vector<uint16_t>& cmdStream);
    void     ResetCoreColumns(Ph2_HwDescription::RD53* pRD53);

    // ###########################
    // # Dedicated to monitoring #
    // ###########################
    int      getADCobservable(const std::string& observableName, bool& isCurrentNotVoltage, bool silentRunning = false) override;
    uint32_t measureADC(Ph2_HwDescription::ReadoutChip* pChip, uint32_t data) override;
    float    measureTemperature(Ph2_HwDescription::ReadoutChip* pChip, uint32_t data, const std::string& type, int beta) override;
};

} // namespace Ph2_HwInterface

#endif
