#include "Utils/Occupancy.h"
#include <math.h>

void Occupancy::makeSummaryAverage(const std::vector<Occupancy>* theOccupancyVector, const std::vector<uint32_t>& theNumberOfEnabledChannelsList, const uint32_t numberOfEvents)
{
    if(theOccupancyVector->size() != theNumberOfEnabledChannelsList.size())
    {
        std::cout << __PRETTY_FUNCTION__ << " theOccupancyVector size = " << theOccupancyVector->size()
                  << " does not match theNumberOfEnabledChannelsList size = " << theNumberOfEnabledChannelsList.size() << std::endl;
        abort();
    }
    float totalNumberOfEnableChannels = 0;
    for(size_t iContainer = 0; iContainer < theOccupancyVector->size(); ++iContainer)
    {
        fOccupancy += (theOccupancyVector->at(iContainer).fOccupancy * float(theNumberOfEnabledChannelsList[iContainer]));
        totalNumberOfEnableChannels += theNumberOfEnabledChannelsList[iContainer];
    }

    fOccupancy /= float(totalNumberOfEnableChannels);
    if(fOccupancy > 1)
        fOccupancyError = sqrt(float(fOccupancy * (fOccupancy - 1) / numberOfEvents));
    else
        fOccupancyError = sqrt(float(fOccupancy * (1. - fOccupancy) / numberOfEvents));
}

void Occupancy::normalize(const uint32_t numberOfEvents)
{
    fOccupancy /= float(numberOfEvents);
    if(fOccupancy > 1)
        fOccupancyError = sqrt(float(fOccupancy * (fOccupancy - 1) / numberOfEvents));
    else
        fOccupancyError = sqrt(float(fOccupancy * (1. - fOccupancy) / numberOfEvents));
}
