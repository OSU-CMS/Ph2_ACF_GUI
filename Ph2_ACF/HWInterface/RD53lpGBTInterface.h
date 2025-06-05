/*!
  \file                  RD53lpGBTInterface.h
  \brief                 Interface to access and control the Low-power Gigabit Transceiver chip
  \author                Mauro DINARDO
  \version               1.0
  \date                  03/03/20
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef RD53lpGBTInterface_H
#define RD53lpGBTInterface_H

#include "HWInterface/lpGBTInterface.h"

namespace Ph2_HwInterface
{
class RD53lpGBTInterface : public lpGBTInterface
{
  public:
    RD53lpGBTInterface(const BeBoardFWMap& pBoardMap) : lpGBTInterface(pBoardMap) {}

    // #############################
    // # Override member functions #
    // #############################
    bool    WriteChipReg(Ph2_HwDescription::Chip* pChip, const std::string& pRegNode, uint16_t pValue, bool pVerify = true) override;
    bool    WriteChipMultReg(Ph2_HwDescription::Chip* pChip, const std::vector<std::pair<std::string, uint16_t>>& RegVec, bool pVerify = true) override;
    int32_t ReadChipReg(Ph2_HwDescription::Chip* pChip, const std::string& pRegNode) override;
    bool    ConfigureChip(Ph2_HwDescription::Chip* pChip, bool pVerify = true, uint32_t pBlockSize = 310) override;
    void
    PhaseAlignRx(Ph2_HwDescription::Chip* pChip, const Ph2_HwDescription::BeBoard* pBoard, const Ph2_HwDescription::OpticalGroup* pOpticalGroup, ReadoutChipInterface* pReadoutChipInterface) override;
    // #############################

    // ###################################
    // # RD53 specific routine functions #
    // ###################################
    void SetDownLinkMapping(const Ph2_HwDescription::OpticalGroup* pOpticalGroup);
    void SetUpLinkMapping(const Ph2_HwDescription::OpticalGroup* pOpticalGroup);

  private:
    bool     WriteReg(Ph2_HwDescription::Chip* pChip, uint16_t pAddress, uint16_t pValue, bool pVerify = true);
    uint16_t ReadReg(Ph2_HwDescription::Chip* pChip, uint16_t pAddress);
};

} // namespace Ph2_HwInterface
#endif
