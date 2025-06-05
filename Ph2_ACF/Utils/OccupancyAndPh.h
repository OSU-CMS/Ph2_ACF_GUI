/*!
  \file                  OccupancyAndPh.h
  \brief                 Generic Occupancy and Pulse Height for DAQ
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef OccupancyAndPh_H
#define OccupancyAndPh_H

#include "Container.h"
#include "EmptyContainer.h"

#include <algorithm>
#include <cmath>
#include <iostream>

class OccupancyAndPh
{
  public:
    OccupancyAndPh() : fOccupancy(0), fOccupancyMedian(0), fPh(0), fPhError(0), fStatus(0), readoutError(false) {}

    void print(void) { std::cout << fOccupancy << "\t" << fPh << std::endl; }

    template <typename T>
    void
    makeChannelAverage(const ChipContainer* theChipContainer, std::shared_ptr<ChannelGroupBase> chipOriginalMask, std::shared_ptr<ChannelGroupBase> cTestChannelGroup, const uint32_t numberOfEvents)
    {
    }
    void makeSummaryAverage(const std::vector<OccupancyAndPh>* theOccupancyVector, const std::vector<uint32_t>& theNumberOfEnabledChannelsList, const uint32_t numberOfEvents);
    void makeSummaryAverage(const std::vector<EmptyContainer>* theOccupancyVector, const std::vector<uint32_t>& theNumberOfEnabledChannelsList, const uint32_t numberOfEvents) {}
    void normalize(const uint32_t numberOfEvents, bool doOnlyPh = false);

    float fOccupancy;
    float fOccupancyMedian;

    float fPh;
    float fPhError;

    uint8_t fStatus;

    bool readoutError;

    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& theArchive, const unsigned int version)
    {
        theArchive & fOccupancy;
        theArchive & fOccupancyMedian;
        theArchive & fPh;
        theArchive & fPhError;
        theArchive & fStatus;
        theArchive & readoutError;
    }
};

template <>
void OccupancyAndPh::makeChannelAverage<OccupancyAndPh>(const ChipContainer*              theChipContainer,
                                                        std::shared_ptr<ChannelGroupBase> chipOriginalMask,
                                                        std::shared_ptr<ChannelGroupBase> cTestChannelGroup,
                                                        const uint32_t                    numberOfEvents);

#endif
