/*!
  \file                  GainFit.h
  \brief                 Generic Gain for DAQ
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef GainFit_H
#define GainFit_H

#include "Container.h"

#include <cmath>
#include <iostream>

class GainFit
{
  public:
    GainFit() : fSlopeHighQ(0), fSlopeHighQError(0), fInterceptHighQ(0), fInterceptHighQError(0), fSlopeLowQ(0), fSlopeLowQError(0), fInterceptLowQ(0), fInterceptLowQError(0), fChi2(0), fDoF(0) {}

    void print(void) { std::cout << fSlopeHighQ << "\t" << fInterceptHighQ << fSlopeLowQ << "\t" << fInterceptLowQ << "\t" << fChi2 << "\t" << fDoF << std::endl; }

    template <typename T>
    void
    makeChannelAverage(const ChipContainer* theChipContainer, std::shared_ptr<ChannelGroupBase> chipOriginalMask, std::shared_ptr<ChannelGroupBase> cTestChannelGroup, const uint32_t numberOfEvents)
    {
    }
    void makeSummaryAverage(const std::vector<GainFit>* theGainVector, const std::vector<uint32_t>& theNumberOfEnabledChannelsList, const uint32_t numberOfEvents);
    void normalize(const uint32_t numberOfEvents) {}

    float fSlopeHighQ;
    float fSlopeHighQError;

    float fInterceptHighQ;
    float fInterceptHighQError;

    float fSlopeLowQ;
    float fSlopeLowQError;

    float fInterceptLowQ;
    float fInterceptLowQError;

    float fChi2;
    float fDoF;

    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& theArchive, const unsigned int version)
    {
        theArchive & fSlopeHighQ;
        theArchive & fSlopeHighQError;
        theArchive & fInterceptHighQ;
        theArchive & fInterceptHighQError;
        theArchive & fSlopeLowQ;
        theArchive & fSlopeLowQError;
        theArchive & fInterceptLowQ;
        theArchive & fInterceptLowQError;
        theArchive & fChi2;
        theArchive & fDoF;
    }
};

template <>
inline void GainFit::makeChannelAverage<GainFit>(const ChipContainer*              theChipContainer,
                                                 std::shared_ptr<ChannelGroupBase> chipOriginalMask,
                                                 std::shared_ptr<ChannelGroupBase> cTestChannelGroup,
                                                 const uint32_t                    numberOfEvents)
{
    float cnt = 0;

    fSlopeHighQ          = 0;
    fSlopeHighQError     = 0;
    fInterceptHighQ      = 0;
    fInterceptHighQError = 0;
    fSlopeLowQ           = 0;
    fSlopeLowQError      = 0;
    fInterceptLowQ       = 0;
    fInterceptLowQError  = 0;
    fChi2                = 0;
    fDoF                 = 0;

    for(auto row = 0u; row < theChipContainer->getNumberOfRows(); row++)
        for(auto col = 0u; col < theChipContainer->getNumberOfCols(); col++)
            if(chipOriginalMask->isChannelEnabled(row, col) && cTestChannelGroup->isChannelEnabled(row, col))
            {
                if(theChipContainer->getChannel<GainFit>(row, col).fSlopeHighQError > 0)
                {
                    fSlopeHighQ += theChipContainer->getChannel<GainFit>(row, col).fSlopeHighQ /
                                   (theChipContainer->getChannel<GainFit>(row, col).fSlopeHighQError * theChipContainer->getChannel<GainFit>(row, col).fSlopeHighQError);
                    fSlopeHighQError += 1. / (theChipContainer->getChannel<GainFit>(row, col).fSlopeHighQError * theChipContainer->getChannel<GainFit>(row, col).fSlopeHighQError);
                }

                if(theChipContainer->getChannel<GainFit>(row, col).fInterceptHighQError > 0)
                {
                    fInterceptHighQ += theChipContainer->getChannel<GainFit>(row, col).fInterceptHighQ /
                                       (theChipContainer->getChannel<GainFit>(row, col).fInterceptHighQError * theChipContainer->getChannel<GainFit>(row, col).fInterceptHighQError);
                    fInterceptHighQError += 1. / (theChipContainer->getChannel<GainFit>(row, col).fInterceptHighQError * theChipContainer->getChannel<GainFit>(row, col).fInterceptHighQError);
                }

                if(theChipContainer->getChannel<GainFit>(row, col).fSlopeLowQError > 0)
                {
                    fSlopeLowQ += theChipContainer->getChannel<GainFit>(row, col).fSlopeLowQ /
                                  (theChipContainer->getChannel<GainFit>(row, col).fSlopeLowQError * theChipContainer->getChannel<GainFit>(row, col).fSlopeLowQError);
                    fSlopeLowQError += 1. / (theChipContainer->getChannel<GainFit>(row, col).fSlopeLowQError * theChipContainer->getChannel<GainFit>(row, col).fSlopeLowQError);
                }

                if(theChipContainer->getChannel<GainFit>(row, col).fInterceptLowQError > 0)
                {
                    fInterceptLowQ += theChipContainer->getChannel<GainFit>(row, col).fInterceptLowQ /
                                      (theChipContainer->getChannel<GainFit>(row, col).fInterceptLowQError * theChipContainer->getChannel<GainFit>(row, col).fInterceptLowQError);
                    fInterceptLowQError += 1. / (theChipContainer->getChannel<GainFit>(row, col).fInterceptLowQError * theChipContainer->getChannel<GainFit>(row, col).fInterceptLowQError);
                }

                if(theChipContainer->getChannel<GainFit>(row, col).fChi2 > 0) fChi2 += theChipContainer->getChannel<GainFit>(row, col).fChi2;
                if(theChipContainer->getChannel<GainFit>(row, col).fDoF > 0) fDoF += theChipContainer->getChannel<GainFit>(row, col).fDoF;

                cnt++;
            }

    if(fSlopeHighQError > 0)
    {
        fSlopeHighQ /= fSlopeHighQError;
        fSlopeHighQError = sqrt(1. / fSlopeHighQError);
    }

    if(fInterceptHighQError > 0)
    {
        fInterceptHighQ /= fInterceptHighQError;
        fInterceptHighQError = sqrt(1. / fInterceptHighQError);
    }

    if(fSlopeLowQError > 0)
    {
        fSlopeLowQ /= fSlopeLowQError;
        fSlopeLowQError = sqrt(1. / fSlopeLowQError);
    }

    if(fInterceptLowQError > 0)
    {
        fInterceptLowQ /= fInterceptLowQError;
        fInterceptLowQError = sqrt(1. / fInterceptLowQError);
    }

    if(fChi2 > 0) fChi2 /= cnt;
    if(fDoF > 0) fDoF /= cnt;
}

#endif
