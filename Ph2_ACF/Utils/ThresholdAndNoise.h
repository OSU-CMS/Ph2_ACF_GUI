/*
  \file                          ThresholdAndNoise.h
  \brief                         Generic ThresholdAndNoise for DAQ
  \author                        Fabio Ravera, Lorenzo Uplegger
  \version                       1.0
  \date                          08/04/19
  Support :                      mail to : fabio.ravera@cern.ch
*/

#ifndef ThresholdAndNoise_H
#define ThresholdAndNoise_H

#include "Container.h"

#include <cmath>
#include <iostream>

class ThresholdAndNoise
{
  public:
    ThresholdAndNoise() : fThreshold(0), fThresholdError(0), fNoise(0), fNoiseError(0) {}

    void print(void) { std::cout << fThreshold << "\t" << fNoise << std::endl; }

    template <typename T>
    void makeChannelAverage(const ChipContainer*                    theChipContainer,
                            const std::shared_ptr<ChannelGroupBase> chipOriginalMask,
                            const std::shared_ptr<ChannelGroupBase> cTestChannelGroup,
                            const uint32_t                          numberOfEvents)
    {
    }
    void makeSummaryAverage(const std::vector<ThresholdAndNoise>* theThresholdAndNoiseVector, const std::vector<uint32_t>& theNumberOfEnabledChannelsList, const uint32_t numberOfEvents);
    void normalize(const uint32_t numberOfEvents) {}

    float fThreshold;
    float fThresholdError;

    float fNoise;
    float fNoiseError;

    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive& theArchive, const unsigned int version)
    {
        theArchive & fThreshold;
        theArchive & fThresholdError;
        theArchive & fNoise;
        theArchive & fNoiseError;
    }
};

template <>
inline void ThresholdAndNoise::makeChannelAverage<ThresholdAndNoise>(const ChipContainer*                    theChipContainer,
                                                                     const std::shared_ptr<ChannelGroupBase> chipOriginalMask,
                                                                     const std::shared_ptr<ChannelGroupBase> cTestChannelGroup,
                                                                     const uint32_t                          numberOfEvents)
{
    fThreshold      = 0;
    fThresholdError = 0;
    fNoise          = 0;
    fNoiseError     = 0;

    for(size_t row = 0; row < theChipContainer->getNumberOfRows(); ++row)
    {
        for(size_t col = 0; col < theChipContainer->getNumberOfCols(); ++col)
        {
            if(chipOriginalMask->isChannelEnabled(row, col) && cTestChannelGroup->isChannelEnabled(row, col))
            {
                if(theChipContainer->getChannel<ThresholdAndNoise>(row, col).fThresholdError > 0)
                {
                    fThreshold += theChipContainer->getChannel<ThresholdAndNoise>(row, col).fThreshold /
                                  (theChipContainer->getChannel<ThresholdAndNoise>(row, col).fThresholdError * theChipContainer->getChannel<ThresholdAndNoise>(row, col).fThresholdError);
                    fThresholdError += 1. / (theChipContainer->getChannel<ThresholdAndNoise>(row, col).fThresholdError * theChipContainer->getChannel<ThresholdAndNoise>(row, col).fThresholdError);
                }

                if(theChipContainer->getChannel<ThresholdAndNoise>(row, col).fNoiseError > 0)
                {
                    fNoise += theChipContainer->getChannel<ThresholdAndNoise>(row, col).fNoise /
                              (theChipContainer->getChannel<ThresholdAndNoise>(row, col).fNoiseError * theChipContainer->getChannel<ThresholdAndNoise>(row, col).fNoiseError);
                    fNoiseError += 1. / (theChipContainer->getChannel<ThresholdAndNoise>(row, col).fNoiseError * theChipContainer->getChannel<ThresholdAndNoise>(row, col).fNoiseError);
                }
            }
        }
    }

    if(fThresholdError > 0)
    {
        fThreshold /= fThresholdError;
        fThresholdError = sqrt(1. / fThresholdError);
    }

    if(fNoiseError > 0)
    {
        fNoise /= fNoiseError;
        fNoiseError = sqrt(1. / fNoiseError);
    }
}

#endif
