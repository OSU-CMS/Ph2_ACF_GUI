/*!
        \file            BeBoardRegItem.h
        \brief                   BeBoardRegItem description, contents of the structure BeBoardRegItem with is the value of the
   ChipRegMap \author                  Lorenzo BIDEGAIN \version                 1.0 \date                    25/06/14
        Support :                mail to : lorenzo.bidegain@cern.ch
 */

#ifndef BeBoardRegItem_H
#define BeBoardRegItem_H

#include <stdint.h>

namespace Ph2_HwDescription
{
struct BeBoardRegItem
{
    BeBoardRegItem() {};
    BeBoardRegItem(uint32_t pValue) : fValue(pValue) {}
    BeBoardRegItem(const BeBoardRegItem&) = default;

    uint32_t fValue    = 0;
    bool     fPrmptCfg = false;

    bool operator==(const BeBoardRegItem& theBeBoardRegItem) const { return fValue == theBeBoardRegItem.fValue; }

    bool operator!=(const BeBoardRegItem& theBeBoardRegItem) const { return !operator==(theBeBoardRegItem); }
};
} // namespace Ph2_HwDescription

#endif
