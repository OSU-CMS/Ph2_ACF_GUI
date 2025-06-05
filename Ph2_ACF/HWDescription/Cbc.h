/*!

  \file                   Cbc.h
  \brief                  Cbc Description class, config of the Cbcs
  \author                 Lorenzo BIDEGAIN
  \version                1.0
  \date                   25/06/14
  Support :               mail to : lorenzo.bidegain@gmail.com

*/

#ifndef Cbc_h__
#define Cbc_h__

#include "FrontEndDescription.h"
#include "ReadoutChip.h"
#include "Utils/Exception.h"
#include "Utils/Visitor.h"
#include <iostream>
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
using CbcRegPair = std::pair<std::string, ChipRegItem>;

/*!
 * \class Cbc
 * \brief Read/Write Cbc's registers on a file, contains a register map
 */
class Cbc : public ReadoutChip
{
  public:
    // C'tors which take BeBoardId, FMCId, HybridId, CbcId
    Cbc(uint8_t pBeBoardId, uint8_t pFMCId, uint8_t pOpticalGroupId, uint8_t pHybridId, uint8_t pChipId, const std::string& filename);

    // C'tors with object FE Description
    Cbc(const FrontEndDescription& pFeDesc, uint8_t pChipId, const std::string& filename);

    Cbc(const Cbc&) = delete;

    void initializeFreeRegisters() override;

    /*!
     * \brief acceptor method for HwDescriptionVisitor
     * \param pVisitor
     */
    virtual void accept(HwDescriptionVisitor& pVisitor) { pVisitor.visitChip(*this); }

    /*!
     * \brief Load RegMap from a file
     * \param filename
     */
    void loadfRegMap(const std::string& filename) override;

    // uint16_t getReg ( const std::string& pReg ) const override;
    // void setReg ( const std::string& pReg, uint16_t psetValue, bool pPrmptCfg = false) override;

    std::stringstream getRegMapStream() override;

    uint32_t getNumberOfChannels() const override { return NCHANNELS; }

    bool isDACLocal(const std::string& dacName) override
    {
        if(dacName.find("MaskChannel-", 0, 12) != std::string::npos || dacName.find("Channel", 0, 7) != std::string::npos)
            return true;
        else
            return false;
    }

    uint8_t getNumberOfBits(const std::string& dacName) override
    {
        if(dacName.find("MaskChannel-", 0, 12) != std::string::npos)
            return 1;
        else if(dacName == "VCth" || dacName == "Threshold")
            return 10;
        else if(dacName == "VCth2")
            return 2;
        else if(dacName == "TriggerLatency")
            return 9;
        else
            return 8;
    }

    bool                          isTopSensor(Ph2_HwDescription::ReadoutChip* pChip, uint16_t pLocalColumn) override;
    std::pair<uint16_t, uint16_t> getGlobalCoordinates(Ph2_HwDescription::ReadoutChip* pChip, uint16_t pLocalColumn, uint16_t pLocalRow) override;

    static uint8_t convertMIPtoInjectedCharge(float numberOfMIPs);
};
} // namespace Ph2_HwDescription

#endif
