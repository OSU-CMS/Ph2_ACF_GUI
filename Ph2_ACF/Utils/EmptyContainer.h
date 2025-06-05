/*

        \file                          EmptyContainer.h
        \brief                         Generic EmptyContainer for DAQ
        \author                        Fabio Ravera, Lorenzo Uplegger
        \version                       1.0
        \date                          08/04/19
        Support :                      mail to : fabio.ravera@cern.ch

 */

#ifndef __EMPTY_CONTAINER_H__
#define __EMPTY_CONTAINER_H__

#include "Utils/Container.h"
#include <iostream>

class EmptyContainer //: public streammable
{
  public:
    EmptyContainer() { ; }
    ~EmptyContainer() { ; }
    void print(void) { std::cout << "EmptyContainer" << std::endl; }
    template <typename T>
    uint32_t makeChannelAverage(const ChipContainer*                    theChipContainer,
                                const std::shared_ptr<ChannelGroupBase> chipOriginalMask,
                                const std::shared_ptr<ChannelGroupBase> cTestChannelGroup,
                                const uint32_t                          numberOfEvents)
    {
        return 0;
    }
    template <typename T>
    void makeSummaryAverage(const std::vector<T>* theEmptyContainerVector, const std::vector<uint32_t>& theNumberOfEnabledChannelsList, const uint32_t numberOfEvents)
    {
        ;
    }
    void normalize(uint32_t numberOfEvents) { ; }

    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive& theArchive, const unsigned int version)
    {
    }
};

#endif
