/*!

        \file                   SSA2.h
        \brief                  SSA2 Description class, config of the SSAs
        \author                 Marc Osherson (copying from SSA.h, SSA2 main difference is the implementation of the registers)
        \version                1.0
        \date                   28/12/2020
        Support :               mail to : oshersonmarc@gmail.com

 */

#ifndef SSA2_h__
#define SSA2_h__

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

namespace Ph2_HwDescription
{ // open namespace

using SSARegPair = std::pair<std::string, ChipRegItem>;
using CommentMap = std::map<int, std::string>;

class SSA2 : public ReadoutChip
{ // open class def
  public:
    // C'tors which take BeBoardId, FMCId, HybridId, ChipId
    SSA2(uint8_t pBeBoardId, uint8_t pFMCId, uint8_t pOpticalGroupId, uint8_t pHybridId, uint8_t pChipId, uint8_t pPartnerId, uint8_t pSSASide, const std::string& filename);
    // C'tors with object FE Description
    SSA2(const FrontEndDescription& pFeDesc, uint8_t pChipId, uint8_t pPartnerId, uint8_t pSSASide, const std::string& filename);
    SSA2(const SSA2&) = delete;

    void initializeFreeRegisters() override;
    void setReg(const std::string& pReg, uint16_t psetValue, uint8_t pStatusReg) override;

    uint8_t           fPartnerId;
    uint8_t           getPartid() { return fPartnerId; }
    virtual void      accept(HwDescriptionVisitor& pVisitor) { pVisitor.visitChip(*this); }
    void              loadfRegMap(const std::string& filename) override;
    std::stringstream getRegMapStream() override;
    uint32_t          getNumberOfChannels() const override { return NSSACHANNELS; }
    bool              isDACLocal(const std::string& dacName) override // FIXME: what does thsi do? Ask Kevin.
    {
        if((dacName.find("THTRIMMING_S", 0, 12) != std::string::npos) or (dacName.find("ThresholdTrim") != std::string::npos) or (dacName.find("DigCalibPattern", 0, 15) != std::string::npos))
            return true;
        else
            return false;
    }
    uint8_t getNumberOfBits(const std::string& dacName) override
    {
        if(dacName.find("THTRIMMING_S", 0, 12) != std::string::npos)
            return 5;
        else if(dacName.find("ThresholdTrim") != std::string::npos)
            return 5;
        else
            return 8;
    }

    bool                          isTopSensor(Ph2_HwDescription::ReadoutChip* pChip, uint16_t pLocalColumn = 0) override;
    std::pair<uint16_t, uint16_t> getGlobalCoordinates(Ph2_HwDescription::ReadoutChip* pChip, uint16_t pLocalColumn, uint16_t pLocalRow) override;
    static std::string            getStripRegisterName(const std::string& theRegisterName, uint16_t strip);
    static uint8_t                convertMIPtoInjectedCharge(float numberOfMIPs);

    void  setADCCalibrationValue(const std::string& theCalibrationName, float theCalibrationValue) override;
    float getADCCalibrationValue(const std::string& theCalibrationName) const override;

  protected:
    static std::vector<std::string> fListOfGlobalRegisters;
}; // close class def

} // namespace Ph2_HwDescription

#endif
