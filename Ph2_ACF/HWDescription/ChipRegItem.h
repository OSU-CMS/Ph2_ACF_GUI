/*!
        \file            ChipRegItem.h
        \brief                   ChipRegItem description, contents of the structure ChipRegItem with is the value of the
   ChipRegMap \author                  Lorenzo BIDEGAIN \version                 1.0 \date                    25/06/14
        Support :                mail to : lorenzo.bidegain@cern.ch
 */

#ifndef ChipRegItem_H
#define ChipRegItem_H

#include <stdint.h>

namespace Ph2_HwDescription
{
struct ChipRegItem
{
    ChipRegItem() {};
    ChipRegItem(uint8_t pPage, uint16_t pAddress, uint16_t pDefValue, uint16_t pValue, uint8_t pStatus = 0)
        : fPage(pPage), fAddress(pAddress), fDefValue(pDefValue), fValue(pValue), fStatusReg(pStatus)
    {
    }

    ChipRegItem(const ChipRegItem&) = default;

    uint8_t  fPage       = 0;
    uint16_t fAddress    = 0;
    uint16_t fDefValue   = 0;
    uint16_t fValue      = 0;
    uint8_t  fStatusReg  = 0;
    bool     fPrmptCfg   = false;
    uint8_t  fBitSize    = 0;
    uint8_t  fControlReg = 0;

    bool operator==(const ChipRegItem& theChipRegItem) const { return fValue == theChipRegItem.fValue; }

    bool operator!=(const ChipRegItem& theChipRegItem) const { return !operator==(theChipRegItem); }
};
} // namespace Ph2_HwDescription

#endif
