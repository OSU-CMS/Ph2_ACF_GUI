/*
        \file                          Container.h
        \brief                         containers for DAQ
        \author                        Fabio Ravera, Lorenzo Uplegger
        \version                       1.0
        \date                          26/04/19
        Support :                      mail to : fabio.ravera@cern.ch

 */

#ifndef __CHANNELGROUPBASE_H__
#define __CHANNELGROUPBASE_H__

#include <bitset>
#include <iostream>
#include <map>
#include <memory>
#include <vector>

class ChannelGroupHandler;

class ChannelGroupBase
{
  public:
    ChannelGroupBase() {};
    ChannelGroupBase(uint16_t numberOfRows, uint16_t numberOfCols) : numberOfRows_(numberOfRows), numberOfCols_(numberOfCols), numberOfEnabledChannels_(numberOfRows * numberOfCols) {};
    virtual ~ChannelGroupBase() { ; }
    virtual void     makeTestGroup(std::shared_ptr<ChannelGroupBase>& currentChannelGroup,
                                   uint32_t                           groupNumber,
                                   uint32_t                           numberOfClustersPerGroup,
                                   uint16_t                           numberOfRowsPerCluster,
                                   uint16_t                           numberOfColsPerCluster) const {};
    uint32_t         getNumberOfRows(void) const { return numberOfRows_; }
    uint32_t         getNumberOfCols(void) const { return numberOfCols_; }
    uint32_t         getNumberOfEnabledChannels(void) const { return numberOfEnabledChannels_; }
    virtual uint32_t getNumberOfEnabledChannels(const std::shared_ptr<ChannelGroupBase> mask) const = 0;
    virtual bool     isChannelEnabled(uint16_t row, uint16_t col) const                             = 0;
    virtual void     enableChannel(uint16_t row, uint16_t col)                                      = 0;
    virtual void     disableChannel(uint16_t row, uint16_t col)                                     = 0;
    virtual void     disableAllChannels(void)                                                       = 0;
    virtual void     enableAllChannels(void)                                                        = 0;
    virtual void     flipAllChannels(void)                                                          = 0;
    virtual bool     areAllChannelsEnabled(void) const                                              = 0;
    virtual void     setCustomPattern(const ChannelGroupBase& customChannelGroupBase)               = 0;

  protected:
    virtual uint32_t getNumberOfEnabledChannels(const ChannelGroupBase* mask) const = 0;
    uint16_t         numberOfRows_;
    uint16_t         numberOfCols_;
    uint32_t         numberOfEnabledChannels_;
};

template <size_t R, size_t C>
class ChannelGroup : public ChannelGroupBase
{
  public:
    ChannelGroup() : ChannelGroupBase(R, C)
    {
        enableAllChannels();
        numberOfEnabledChannels_ = numberOfRows_ * numberOfCols_;
    };
    ChannelGroup(std::bitset<R * C>&& inputChannelsBitset) : ChannelGroupBase(R, C), channelsBitset_(inputChannelsBitset)
    {
        customPatternSet_        = true;
        numberOfEnabledChannels_ = channelsBitset_.count();
    };
    ChannelGroup(const ChannelGroup& theChannelGroup) : ChannelGroupBase(R, C)
    {
        channelsBitset_          = theChannelGroup.channelsBitset_;
        numberOfEnabledChannels_ = getNumberOfEnabledChannels(&theChannelGroup);
    }

    virtual ~ChannelGroup() { ; }

    inline bool isChannelEnabled(uint16_t row, uint16_t col) const override { return channelsBitset_[row + numberOfRows_ * col]; }
    inline void enableChannel(uint16_t row, uint16_t col) override
    {
        channelsBitset_[row + numberOfRows_ * col] = true;
        numberOfEnabledChannels_                   = channelsBitset_.count();
    }
    inline void disableChannel(uint16_t row, uint16_t col) override
    {
        channelsBitset_[row + numberOfRows_ * col] = false;
        numberOfEnabledChannels_                   = channelsBitset_.count();
    }
    inline void disableAllChannels(void) override
    {
        channelsBitset_.reset();
        numberOfEnabledChannels_ = channelsBitset_.count();
    }
    inline void enableAllChannels(void) override
    {
        channelsBitset_.set();
        numberOfEnabledChannels_ = channelsBitset_.count();
    }
    inline void flipAllChannels(void) override
    {
        channelsBitset_.flip();
        numberOfEnabledChannels_ = channelsBitset_.count();
    }
    inline bool areAllChannelsEnabled(void) const override { return channelsBitset_.all(); }

    inline uint32_t getNumberOfEnabledChannels(const std::shared_ptr<ChannelGroupBase> mask = std::make_shared<ChannelGroup<R, C>>()) const { return getNumberOfEnabledChannels(mask.get()); }

    inline std::bitset<R * C> getBitset(void) const { return channelsBitset_; }

    inline void setCustomPattern(const ChannelGroupBase& customChannelGroupBase) override
    {
        auto& customChannelGroup = static_cast<const ChannelGroup<R, C>&>(customChannelGroupBase);
        channelsBitset_          = customChannelGroup.channelsBitset_;
        customPatternSet_        = true;
        numberOfEnabledChannels_ = channelsBitset_.count();

        numberOfRows_ = customChannelGroup.numberOfRows_;
        numberOfCols_ = customChannelGroup.numberOfCols_;
    }

    virtual void makeTestGroup(std::shared_ptr<ChannelGroupBase>& currentChannelGroup,
                               uint32_t                           groupNumber,
                               uint32_t                           numberOfClustersPerGroup,
                               uint16_t                           numberOfRowsPerCluster,
                               uint16_t                           numberOfColsPerCluster) const override
    {
        if(numberOfClustersPerGroup * numberOfRowsPerCluster * numberOfColsPerCluster >= numberOfEnabledChannels_)
        {
            static_cast<ChannelGroup<R, C>*>(currentChannelGroup.get())->setCustomPattern(*this);
            return;
        }
        static_cast<ChannelGroup*>(currentChannelGroup.get())->disableAllChannels();

        uint32_t numberOfClusterToSkip = numberOfEnabledChannels_ / (numberOfRowsPerCluster * numberOfColsPerCluster * numberOfClustersPerGroup) - 1;
        if(numberOfEnabledChannels_ % (numberOfRowsPerCluster * numberOfColsPerCluster * numberOfClustersPerGroup) > 0) ++numberOfClusterToSkip;

        uint32_t clusterSkipped = numberOfClusterToSkip - groupNumber;
        for(uint16_t col = 0; col < numberOfCols_; col += numberOfColsPerCluster)
        {
            for(uint16_t row = 0; row < numberOfRows_; row += numberOfRowsPerCluster)
            {
                if(clusterSkipped == numberOfClusterToSkip)
                    clusterSkipped = 0;
                else
                {
                    ++clusterSkipped;
                    continue;
                }
                if(isChannelEnabled(row, col))
                {
                    for(uint16_t clusterRow = 0; clusterRow < numberOfRowsPerCluster; ++clusterRow)
                    {
                        for(uint16_t clusterCol = 0; clusterCol < numberOfColsPerCluster; ++clusterCol)
                        {
                            static_cast<ChannelGroup<R, C>*>(currentChannelGroup.get())->enableChannel(row + clusterRow, col + clusterCol);
                        }
                    }
                }
            }
        }
    }

  private:
    inline uint32_t getNumberOfEnabledChannels(const ChannelGroupBase* mask) const
    {
        std::bitset<R * C> tmpBitset;
        tmpBitset = this->channelsBitset_ & static_cast<const ChannelGroup<R, C>*>(mask)->channelsBitset_;
        return tmpBitset.count();
    }

    std::bitset<R * C> channelsBitset_;
    bool               customPatternSet_ = false;
};

class ChannelGroupHandler
{
  public:
    class ChannelGroupIterator
    {
      public:
        using iterator_category = std::output_iterator_tag;
        using value_type        = uint32_t;
        explicit ChannelGroupIterator(ChannelGroupHandler& channelGroupHandler, uint32_t groupNumber) : channelGroupHandler_(channelGroupHandler), groupNumber_(groupNumber) { ; }
        const std::shared_ptr<ChannelGroupBase> operator*() const { return channelGroupHandler_.getTestGroup(groupNumber_); }
        ChannelGroupIterator&                   operator++()
        {
            ++groupNumber_;
            return *this;
        }
        ChannelGroupIterator& operator++(int i) { return ++(*this); }

        bool operator!=(const ChannelGroupIterator& rhs) const { return groupNumber_ != rhs.groupNumber_; }

      protected:
        ChannelGroupHandler& channelGroupHandler_;
        uint32_t             groupNumber_;
    };

    ChannelGroupHandler() {};
    ChannelGroupHandler(const ChannelGroupHandler&) = delete;
    ChannelGroupHandler(ChannelGroupHandler&&)      = default;
    virtual ~ChannelGroupHandler() {};

    virtual void setChannelGroupParameters(uint32_t numberOfClustersPerGroup, uint32_t numberOfRowsPerCluster, uint32_t numberOfColsPerCluster);

    void setCustomChannelGroup(ChannelGroupBase& customChannelGroup) { allChannelGroup_->setCustomPattern(customChannelGroup); }

    virtual ChannelGroupIterator begin() { return ChannelGroupIterator(*this, 0); }

    virtual ChannelGroupIterator end() { return ChannelGroupIterator(*this, numberOfGroups_); }

    const std::shared_ptr<ChannelGroupBase> allChannelGroup() const { return allChannelGroup_; }

    virtual const std::shared_ptr<ChannelGroupBase> getTestGroup(int groupNumber)
    {
        if(groupNumber < 0) return allChannelGroup();
        if(groupNumber > getNumberOfGroups()) return std::shared_ptr<ChannelGroupBase>();
        allChannelGroup_->makeTestGroup(currentChannelGroup_, groupNumber, numberOfClustersPerGroup_, numberOfRowsPerCluster_, numberOfColsPerCluster_);
        return currentChannelGroup_;
    }

    uint16_t getNumberOfGroups() const { return numberOfGroups_; };

  protected:
    uint32_t                          numberOfGroups_;
    uint32_t                          numberOfClustersPerGroup_;
    uint32_t                          numberOfRowsPerCluster_;
    uint32_t                          numberOfColsPerCluster_;
    std::shared_ptr<ChannelGroupBase> allChannelGroup_;
    std::shared_ptr<ChannelGroupBase> currentChannelGroup_;
};

#endif
