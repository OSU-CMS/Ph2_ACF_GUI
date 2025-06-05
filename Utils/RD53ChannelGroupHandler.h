/*!
  \file                  RD53ChannelGroupHandler.h
  \brief                 Channel container handler
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef RD53ChannelGroupHandler_H
#define RD53ChannelGroupHandler_H

#include "ChannelGroupHandler.h"
#include "HWDescription/RD53.h"

namespace RD53GroupType
{
constexpr uint8_t AllPixels      = 0;
constexpr uint8_t Groups         = 1;
constexpr uint8_t Custom         = 2;
constexpr uint8_t XtalkCoupled   = 3;
constexpr uint8_t XtalkDeCoupled = 4;
} // namespace RD53GroupType

class RD53ChannelGroup : public ChannelGroupBase
{
  public:
    RD53ChannelGroup(size_t nRows, size_t nCols, uint8_t groupType = RD53GroupType::AllPixels, bool initialValue = false)
        : ChannelGroupBase(nRows, nCols), groupType(groupType), storage(nRows * nCols, initialValue)
    {
    }

    uint32_t getNumberOfEnabledChannels(const std::shared_ptr<ChannelGroupBase> mask) const override { return getNumberOfEnabledChannels(mask.get()); }
    bool     isChannelEnabled(uint16_t row, uint16_t col) const override { return storage[row + numberOfRows_ * col]; }
    void     enableChannel(uint16_t row, uint16_t col) override { storage[row + numberOfRows_ * col] = true; }
    void     disableChannel(uint16_t row, uint16_t col) override { storage[row + numberOfRows_ * col] = false; }
    void     disableAllChannels() override { std::fill(storage.begin(), storage.end(), false); }
    void     enableAllChannels() override { std::fill(storage.begin(), storage.end(), true); }
    void     flipAllChannels() override { std::transform(storage.begin(), storage.end(), storage.begin(), std::logical_not<>{}); }
    bool     areAllChannelsEnabled() const override
    {
        return std::all_of(storage.begin(), storage.end(), [](auto x) { return x; });
    }
    void                     setCustomPattern(const ChannelGroupBase& customChannelGroupBase) {}
    const std::vector<bool>& getMask() const { return storage; }
    std::vector<bool>        getMaskNext(uint8_t groupType) const
    {
        std::vector<bool> nextCol(numberOfRows_ * numberOfCols_, false);

        for(auto col = 0; col < numberOfCols_; col++)
            for(auto row = 0; row < numberOfRows_; row++)
                if(storage[row + numberOfRows_ * col] == true)
                {
                    if(groupType == RD53GroupType::XtalkCoupled)
                    {
                        if((col % 2 == 0) && (col + 1 < numberOfCols_)) nextCol[row + numberOfRows_ * (col + 1)] = true;
                        if((col % 2 == 1) && (col - 1 >= 0)) nextCol[row + numberOfRows_ * (col - 1)] = true;
                    }
                    else if(groupType == RD53GroupType::XtalkDeCoupled)
                    {
                        if((col % 2 == 0) && (col + 1 < numberOfCols_) && (row - 1 >= 0)) nextCol[row - 1 + numberOfRows_ * (col + 1)] = true;
                        if((col % 2 == 1) && (col - 1 >= 0) && (row + 1 < numberOfRows_)) nextCol[row + 1 + numberOfRows_ * (col - 1)] = true;
                    }
                }

        return nextCol;
    }

    uint8_t groupType;

  protected:
    uint32_t getNumberOfEnabledChannels(const ChannelGroupBase* mask) const override
    {
        std::vector<bool> conjuction(storage.size());
        std::transform(storage.begin(), storage.end(), static_cast<const RD53ChannelGroup*>(mask)->storage.begin(), conjuction.begin(), std::logical_and<>{});
        return std::count(conjuction.begin(), conjuction.end(), true);
    }

  private:
    std::vector<bool> storage;
};

class RD53ChannelGroupHandler : public ChannelGroupHandler
{
  public:
    RD53ChannelGroupHandler(size_t rowStart, size_t rowStop, size_t colStart, size_t colStop, size_t nRows, size_t nCols, uint8_t groupType, size_t hitPerCol = 1, size_t onlyNGroups = 0);

    RD53ChannelGroup&                       getRegionOfInterest() { return regionOfInterest; }
    const std::shared_ptr<ChannelGroupBase> getTestGroup(int groupNumber) override;

  private:
    RD53ChannelGroup regionOfInterest;
    RD53ChannelGroup enabledGroups;
    uint8_t          groupType;
    size_t           hitPerCol;
    size_t           onlyNGroups;
};

#endif
