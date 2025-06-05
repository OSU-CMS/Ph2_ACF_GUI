#include "Utils/ThresholdAndNoise.h"

void ThresholdAndNoise::makeSummaryAverage(const std::vector<ThresholdAndNoise>* theThresholdAndNoiseVector, const std::vector<uint32_t>& theNumberOfEnabledChannelsList, const uint32_t numberOfEvents)
{
    if(theThresholdAndNoiseVector->size() != theNumberOfEnabledChannelsList.size())
    {
        std::cout << __PRETTY_FUNCTION__ << " theThresholdAndNoiseVector size = " << theThresholdAndNoiseVector->size()
                  << " does not match theNumberOfEnabledChannelsList size = " << theNumberOfEnabledChannelsList.size() << std::endl;
        abort();
    }

    fThreshold      = 0;
    fThresholdError = 0;
    fNoise          = 0;
    fNoiseError     = 0;

    for(size_t iContainer = 0; iContainer < theThresholdAndNoiseVector->size(); ++iContainer)
    {
        if(theThresholdAndNoiseVector->at(iContainer).fThresholdError > 0)
        {
            fThreshold += (theThresholdAndNoiseVector->at(iContainer).fThreshold * float(theNumberOfEnabledChannelsList[iContainer])) /
                          (theThresholdAndNoiseVector->at(iContainer).fThresholdError * theThresholdAndNoiseVector->at(iContainer).fThresholdError);
            fThresholdError +=
                float(theNumberOfEnabledChannelsList[iContainer]) / (theThresholdAndNoiseVector->at(iContainer).fThresholdError * theThresholdAndNoiseVector->at(iContainer).fThresholdError);
        }

        if(theThresholdAndNoiseVector->at(iContainer).fNoiseError > 0)
        {
            fNoise += (theThresholdAndNoiseVector->at(iContainer).fNoise * float(theNumberOfEnabledChannelsList[iContainer])) /
                      (theThresholdAndNoiseVector->at(iContainer).fNoiseError * theThresholdAndNoiseVector->at(iContainer).fNoiseError);
            fNoiseError += float(theNumberOfEnabledChannelsList[iContainer]) / (theThresholdAndNoiseVector->at(iContainer).fNoiseError * theThresholdAndNoiseVector->at(iContainer).fNoiseError);
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
        fNoise /= fNoiseError;
    }
}
