/*!
  \file                  OccupancyAndPh.cc
  \brief                 Generic Occupancy and Pulse Height for DAQ
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#include "OccupancyAndPh.h"

void OccupancyAndPh::makeSummaryAverage(const std::vector<OccupancyAndPh>* theOccupancyVector, const std::vector<uint32_t>& theNumberOfEnabledChannelsList, const uint32_t numberOfEvents)
{
    if(theOccupancyVector->size() != theNumberOfEnabledChannelsList.size())
    {
        std::cout << __PRETTY_FUNCTION__ << " theOccupancyVector size = " << theOccupancyVector->size()
                  << " does not match theNumberOfEnabledChannelsList size = " << theNumberOfEnabledChannelsList.size() << std::endl;
        abort();
    }

    fOccupancy       = 0;
    fOccupancyMedian = 0;
    fPh              = 0;
    fPhError         = 0;

    std::vector<float> sortedOcc;
    size_t             totalNumberOfEnableChannels = 0;

    for(size_t iContainer = 0; iContainer < theOccupancyVector->size(); iContainer++)
    {
        fOccupancy += theOccupancyVector->at(iContainer).fOccupancy * theNumberOfEnabledChannelsList[iContainer];
        sortedOcc.insert(std::upper_bound(sortedOcc.begin(), sortedOcc.end(), theOccupancyVector->at(iContainer).fOccupancy * theNumberOfEnabledChannelsList[iContainer]),
                         theOccupancyVector->at(iContainer).fOccupancy * theNumberOfEnabledChannelsList[iContainer]);

        if(theOccupancyVector->at(iContainer).fPhError > 0)
        {
            fPh += theOccupancyVector->at(iContainer).fPh * theNumberOfEnabledChannelsList[iContainer] / (theOccupancyVector->at(iContainer).fPhError * theOccupancyVector->at(iContainer).fPhError);
            fPhError += theNumberOfEnabledChannelsList[iContainer] / (theOccupancyVector->at(iContainer).fPhError * theOccupancyVector->at(iContainer).fPhError);
        }

        totalNumberOfEnableChannels += theNumberOfEnabledChannelsList[iContainer];
    }

    fOccupancy /= (totalNumberOfEnableChannels > 0 ? totalNumberOfEnableChannels : 1);

    if(totalNumberOfEnableChannels != 0)
    {
        if(totalNumberOfEnableChannels % 2 == 0)
            fOccupancyMedian = (sortedOcc[sortedOcc.size() / 2 - 1] + sortedOcc[sortedOcc.size() / 2]) / 2;
        else
            fOccupancyMedian = sortedOcc[sortedOcc.size() / 2];
    }

    if(fPhError > 0)
    {
        fPh /= fPhError;
        fPhError = 1. / sqrt(fPhError);
    }
}

void OccupancyAndPh::normalize(const uint32_t numberOfEvents, bool doOnlyPh)
{
    fPh /= (fOccupancy > 0 ? fOccupancy : 1);
    fPhError = (fOccupancy > 1 ? sqrt((fPhError / fOccupancy - fPh * fPh) * fOccupancy / (fOccupancy - 1)) : 0);

    if(doOnlyPh == false) fOccupancy /= numberOfEvents;
}

template <>
void OccupancyAndPh::makeChannelAverage<OccupancyAndPh>(const ChipContainer*              theChipContainer,
                                                        std::shared_ptr<ChannelGroupBase> chipOriginalMask,
                                                        std::shared_ptr<ChannelGroupBase> cTestChannelGroup,
                                                        const uint32_t                    numberOfEvents)
{
    fOccupancy       = 0;
    fOccupancyMedian = 0;
    fPh              = 0;
    fPhError         = 0;

    std::vector<float> sortedOcc;
    size_t             numberOfEnabledChannels = 0;

    for(auto row = 0u; row < theChipContainer->getNumberOfRows(); row++)
        for(auto col = 0u; col < theChipContainer->getNumberOfCols(); col++)
            if(chipOriginalMask->isChannelEnabled(row, col) && cTestChannelGroup->isChannelEnabled(row, col))
            {
                fOccupancy += theChipContainer->getChannel<OccupancyAndPh>(row, col).fOccupancy;
                ;
                sortedOcc.insert(std::upper_bound(sortedOcc.begin(), sortedOcc.end(), theChipContainer->getChannel<OccupancyAndPh>(row, col).fOccupancy),
                                 theChipContainer->getChannel<OccupancyAndPh>(row, col).fOccupancy);

                if(theChipContainer->getChannel<OccupancyAndPh>(row, col).fPhError > 0)
                {
                    fPh += theChipContainer->getChannel<OccupancyAndPh>(row, col).fPh /
                           (theChipContainer->getChannel<OccupancyAndPh>(row, col).fPhError * theChipContainer->getChannel<OccupancyAndPh>(row, col).fPhError);
                    fPhError += 1. / (theChipContainer->getChannel<OccupancyAndPh>(row, col).fPhError * theChipContainer->getChannel<OccupancyAndPh>(row, col).fPhError);
                }

                numberOfEnabledChannels++;
            }

    fOccupancy /= (numberOfEnabledChannels > 0 ? numberOfEnabledChannels : 1);

    if(numberOfEnabledChannels != 0)
    {
        if(numberOfEnabledChannels % 2 == 0)
            fOccupancyMedian = (sortedOcc[sortedOcc.size() / 2 - 1] + sortedOcc[sortedOcc.size() / 2]) / 2;
        else
            fOccupancyMedian = sortedOcc[sortedOcc.size() / 2];
    }

    if(fPhError > 0)
    {
        fPh /= fPhError;
        fPhError = 1. / sqrt(fPhError);
    }
}
