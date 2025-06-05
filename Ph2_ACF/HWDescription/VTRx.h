/*!
  \file                  VTRx.h
  \brief                 VTRx description class, config of the VTRx
  \author                Fabio Ravera
  \version               1.0
  \date                  11/07/24
  Support:               email to fabio.ravera@cern.ch
*/

#ifndef VTRx_H
#define VTRx_H

#include "HWDescription/Chip.h"

namespace Ph2_HwDescription
{

class lpGBT;
class VTRx : public Chip
{
  public:
    VTRx(uint8_t pBeBoardId, uint8_t FMCId, uint8_t pOpticalGroupId, uint8_t pChipId, const std::string& fileName, const std::string& configFilePath, lpGBT* theLpGBT);

    VTRx(const VTRx&) = delete;

    void initializeFreeRegisters() override;

    void loadfRegMap(const std::string& fileName) override;

    std::stringstream getRegMapStream() override;
    uint8_t           getNumberOfBits(const std::string& dacName) override { return 0; }

    lpGBT* fTheLpGBT;
};
} // namespace Ph2_HwDescription

#endif