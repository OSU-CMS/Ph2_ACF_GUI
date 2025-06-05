/*!

        \file                   Cic.h
        \brief                  Cic Description class, config of the Cics
 */

#ifndef Cic_h__
#define Cic_h__

#include "Chip.h"
#include "FrontEndDescription.h"
#include "Utils/ConsoleColor.h"
#include "Utils/Exception.h"
#include "Utils/Visitor.h"
#include "Utils/easylogging++.h"

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
using CicRegPair = std::pair<std::string, ChipRegItem>;

/*!
 * \class Cic
 * \brief Read/Write Cic's registers on a file, contains a register map
 */
class Cic : public Chip
{
  public:
    // C'tors which take BeBoardId, FMCId, HybridId, CicId
    Cic(uint8_t pBeBoardId, uint8_t pFMCId, uint8_t pOpticalGroupId, uint8_t pHybridId, uint8_t pChipId, const std::string& filename);

    // C'tors with object FE Description
    Cic(const FrontEndDescription& pFeDesc, uint8_t pChipId, const std::string& filename);

    Cic(const Cic&) = delete;

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

    std::stringstream getRegMapStream() override;

    virtual uint8_t getNumberOfBits(const std::string& dacName) { return 8; };

    void setDriveStrength(uint8_t pDriveStrength);
    void setEdgeSelect(uint8_t pEdgeSel);

    void    setLpGBTphaseForCICbypass(uint8_t phyPort, uint8_t stubLine, uint8_t lpgbtPhase);
    uint8_t getLpGBTphaseForCICbypass(uint8_t phyPort, uint8_t stubLine) const;

    std::vector<uint8_t> getMapping();

  private:
    std::vector<uint8_t>                          fChipToCicMapping2S{0, 1, 2, 3, 7, 6, 5, 4};  // Index CIC FE Id , Value Hybrid FE Id
    std::vector<uint8_t>                          fChipToCicMappingPSR{6, 7, 3, 2, 1, 0, 4, 5}; // Index hybrid FE Id , Value CIC FE Id
    std::vector<uint8_t>                          fChipToCicMappingPSL{1, 0, 4, 5, 6, 7, 3, 2}; // Index hybrid FE Id , Value CIC FE Id
    void                                          initializeLpGBTphasesForCICbypassMap();
    static std::map<uint8_t, uint8_t>             fTxDriveStrength;
    std::map<uint8_t, std::map<uint8_t, uint8_t>> fLpGBTphasesForCICbypassMap;
};
} // namespace Ph2_HwDescription

#endif
