/*!
  \file                  GainFit.cc
  \brief                 Generic Gain for DAQ
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#include "GainFit.h"

void GainFit::makeSummaryAverage(const std::vector<GainFit>* theGainVector, const std::vector<uint32_t>& theNumberOfEnabledChannelsList, const uint32_t numberOfEvents)
{
    if(theGainVector->size() != theNumberOfEnabledChannelsList.size())
    {
        std::cout << __PRETTY_FUNCTION__ << " theGainVector size = " << theGainVector->size() << " does not match theNumberOfEnabledChannelsList size = " << theNumberOfEnabledChannelsList.size()
                  << std::endl;
        abort();
    }

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

    for(size_t iContainer = 0; iContainer < theGainVector->size(); iContainer++)
    {
        if(theGainVector->at(iContainer).fSlopeHighQError > 0)
        {
            fSlopeHighQ += theGainVector->at(iContainer).fSlopeHighQ * theNumberOfEnabledChannelsList[iContainer] /
                           (theGainVector->at(iContainer).fSlopeHighQError * theGainVector->at(iContainer).fSlopeHighQError);
            fSlopeHighQError += theNumberOfEnabledChannelsList[iContainer] / (theGainVector->at(iContainer).fSlopeHighQError * theGainVector->at(iContainer).fSlopeHighQError);
        }

        if(theGainVector->at(iContainer).fInterceptHighQError > 0)
        {
            fInterceptHighQ += theGainVector->at(iContainer).fInterceptHighQ * theNumberOfEnabledChannelsList[iContainer] /
                               (theGainVector->at(iContainer).fInterceptHighQError * theGainVector->at(iContainer).fInterceptHighQError);
            fInterceptHighQError += theNumberOfEnabledChannelsList[iContainer] / (theGainVector->at(iContainer).fInterceptHighQError * theGainVector->at(iContainer).fInterceptHighQError);
        }

        if(theGainVector->at(iContainer).fSlopeLowQError > 0)
        {
            fSlopeLowQ +=
                theGainVector->at(iContainer).fSlopeLowQ * theNumberOfEnabledChannelsList[iContainer] / (theGainVector->at(iContainer).fSlopeLowQError * theGainVector->at(iContainer).fSlopeLowQError);
            fSlopeLowQError += theNumberOfEnabledChannelsList[iContainer] / (theGainVector->at(iContainer).fSlopeLowQError * theGainVector->at(iContainer).fSlopeLowQError);
        }

        if(theGainVector->at(iContainer).fInterceptLowQError > 0)
        {
            fInterceptLowQ += theGainVector->at(iContainer).fInterceptLowQ * theNumberOfEnabledChannelsList[iContainer] /
                              (theGainVector->at(iContainer).fInterceptLowQError * theGainVector->at(iContainer).fInterceptLowQError);
            fInterceptLowQError += theNumberOfEnabledChannelsList[iContainer] / (theGainVector->at(iContainer).fInterceptLowQError * theGainVector->at(iContainer).fInterceptLowQError);
        }

        if(theGainVector->at(iContainer).fChi2 > 0) fChi2 += theGainVector->at(iContainer).fChi2;
        if(theGainVector->at(iContainer).fDoF > 0) fDoF += theGainVector->at(iContainer).fDoF;

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
