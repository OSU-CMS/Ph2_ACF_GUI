/*!
  \file                  GenericDataVector.h
  \brief                 Generic data vector for DAQ
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef GenericDataVector_H
#define GenericDataVector_H

#include "OccupancyAndPh.h"

class GenericDataVector : public OccupancyAndPh
{
  public:
    GenericDataVector() {}
    ~GenericDataVector() {}

    void print(void) { std::cout << data1.size() << "\t" << data2.size() << std::endl; }

    void makeSummaryAverage(const std::vector<GenericDataVector>* theOccupancyVector, const std::vector<uint32_t>& theNumberOfEnabledChannelsList, const uint32_t numberOfEvents) {}
    void makeSummaryAverage(const std::vector<EmptyContainer>* theOccupancyVector, const std::vector<uint32_t>& theNumberOfEnabledChannelsList, const uint32_t numberOfEvents)
    {
        OccupancyAndPh::makeSummaryAverage(theOccupancyVector, theNumberOfEnabledChannelsList, numberOfEvents);
    }

    std::vector<float> data1;
    std::vector<float> data2;

    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& theArchive, const unsigned int version)
    {
        theArchive & data1;
        theArchive & data2;
    }
};

#endif
