/*!
  Filename :                      Chip.cc
  Content :                       Chip Description class, config of the Chips
  Programmer :                    Lorenzo BIDEGAIN
  Version :                       1.0
  Date of Creation :              25/06/14
  Support :                       mail to : lorenzo.bidegain@gmail.com
*/

#include "ReadoutChip.h"

namespace Ph2_HwDescription
{
// C'tors with object FE Description
ReadoutChip::ReadoutChip(const FrontEndDescription& pFeDesc, uint8_t pChipId, uint16_t pMaxRegValue) : Chip(pFeDesc, pChipId, pMaxRegValue), ChipContainer(pChipId), fChipOriginalMask(nullptr) {}

// C'tors which take Board ID, Frontend ID/Hybrid ID, FMC ID, Chip ID
ReadoutChip::ReadoutChip(uint8_t pBeBoardId, uint8_t pFMCId, uint8_t pOpticalGroupId, uint8_t pHybridId, uint8_t pChipId, uint16_t pMaxRegValue)
    : Chip(pBeBoardId, pFMCId, pOpticalGroupId, pHybridId, pChipId, pMaxRegValue), ChipContainer(pChipId)
{
}

ReadoutChip::~ReadoutChip() {}
} // namespace Ph2_HwDescription
