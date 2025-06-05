/*!
  \file                                            ReadoutChipInterface.h
  \brief                                           User Interface to the Chip, base class for, CBC, MPA, SSA, RD53
  \author                                          Fabio RAVERA
  \version                                         1.0
  \date                        25/02/19
  Support :                    mail to : fabio.ravera@cern.ch
*/

#include "HWInterface/ReadoutChipInterface.h"
#include "Utils/ConsoleColor.h"

#define DEV_FLAG 0

using namespace Ph2_HwDescription;

namespace Ph2_HwInterface
{
ReadoutChipInterface::ReadoutChipInterface(const BeBoardFWMap& pBoardMap) : ChipInterface(pBoardMap)
{
#ifdef COUNT_FLAG
    LOG(DEBUG) << "Counting number of Transactions!";
#endif
}

uint16_t ReadoutChipInterface::getMostFrequentLocalRegisterValue(const ChipContainer& theChipContainer)
{
    std::unordered_map<uint16_t, size_t> theValueFrequency;
    for(auto theChannel: *theChipContainer.getChannelContainer<uint16_t>()) { theValueFrequency[theChannel]++; }

    uint16_t maximumFrequencyValue = 0xFFFF;
    size_t   maximumFrequency      = 0;

    for(const auto& valueAndFrequency: theValueFrequency)
    {
        if(valueAndFrequency.second > maximumFrequency)
        {
            maximumFrequency      = valueAndFrequency.second;
            maximumFrequencyValue = valueAndFrequency.first;
        }
    }

    return maximumFrequencyValue;
}

ReadoutChipInterface::~ReadoutChipInterface() {}
} // namespace Ph2_HwInterface
