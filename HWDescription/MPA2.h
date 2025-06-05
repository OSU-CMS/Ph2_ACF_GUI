/*!

        \file                   MPA2.h
        \brief                  MPA2 Description class, config of the MPA2s
        \author                 Kevin Nash
        \version                1.0
        \date                   12/06/21
        Support :               mail to : knash201@gmail.com

 */

#ifndef MPA2_h__
#define MPA2_h__

#include "ChipRegItem.h"
#include "FrontEndDescription.h"
#include "ReadoutChip.h"
#include "Utils/Exception.h"
#include "Utils/Visitor.h"
#include "Utils/easylogging++.h"
#include <iostream>
#include <map>
#include <set>
#include <stdint.h>
#include <string>
#include <utility>

/*!
 * \namespace Ph2_HwDescription
 * \brief Namespace regrouping all the hardware description
 */
namespace Ph2_HwDescription
{
using MPARegPair = std::pair<std::string, ChipRegItem>;
using CommentMap = std::map<int, std::string>;

class MPA2 : public ReadoutChip
{
  public:
    static constexpr size_t nRows = NSSACHANNELS;
    static constexpr size_t nCols = NMPAROWS;

    MPA2(uint8_t pBeBoardId, uint8_t pFMCId, uint8_t pOpticalGroupId, uint8_t pHybridId, uint8_t pChipId, uint8_t pPartnerId, const std::string& filename);
    // C'tors with object FE Description
    MPA2(const FrontEndDescription& pFeDesc, uint8_t pChipId, uint8_t pPartnerId, const std::string& filename);

    MPA2(const MPA2&) = delete;

    void setReg(const std::string& pReg, uint16_t psetValue, uint8_t pStatusReg) override;

    void initializeFreeRegisters() override;

    using MPARegPair = std::pair<std::string, ChipRegItem>;
    uint8_t           fPartnerId;
    uint8_t           getPartid() { return fPartnerId; }
    void              loadfRegMap(const std::string& filename) override;
    std::stringstream getRegMapStream() override;

    bool isDACLocal(const std::string& dacName) override
    {
        if((dacName.find("TrimDAC", 0, 7) != std::string::npos) or (dacName.find("ThresholdTrim") != std::string::npos) or (dacName.find("DigPattern", 0, 10) != std::string::npos))
            return true;
        else
            return false;
    }
    uint8_t getNumberOfBits(const std::string& dacName) override
    {
        if((dacName.find("TrimDAC_C", 0, 9) != std::string::npos) or (dacName.find("ThresholdTrim") != std::string::npos))
            return 5;
        else
            return 8;
    }

    // row, col starts at index 0, global pix number starts at number 1

    // std::pair<uint32_t, uint32_t> PNlocal(const uint32_t PN) { return std::pair<uint32_t, uint32_t>((PN + 1) / 120 + 1, ((PN - 1) % 120) + 1); }

    uint32_t getNumberOfChannels() const override { return NMPAROWS * NSSACHANNELS; }

    bool                          isTopSensor(Ph2_HwDescription::ReadoutChip* pChip, uint16_t pLocalColumn = 0) override;
    std::pair<uint16_t, uint16_t> getGlobalCoordinates(Ph2_HwDescription::ReadoutChip* pChip, uint16_t pLocalColumn, uint16_t pLocalRow) override;

    // uint32_t PNglobal(std::pair<uint32_t, uint32_t> PC) { return (PC.first - 1) * 120 + (PC.second - 1) + 1; }
    static std::string getPixelRegisterName(const std::string& theRegisterName, uint16_t row, uint16_t col);
    static std::string getRowRegisterName(const std::string& theRegisterName, uint16_t row);
    static uint8_t     convertMIPtoInjectedCharge(float numberOfMIPs);

    float getADCCalibrationValue(const std::string& theCalibrationName) const override;
    void  setADCCalibrationValue(const std::string& theCalibrationName, float theCalibrationValue) override;

  protected:
    static std::vector<std::string> fListOfGlobalPixelRegisters;
    static std::vector<std::string> fListOfGlobalRowRegisters;
};

struct MPA2RegItemComparer // Irene
{
    bool operator()(const MPARegPair& pRegItem1, const MPARegPair& pRegItem2) const;
};
} // namespace Ph2_HwDescription

#endif
