/*!
  \file                  Physics2SHistograms.cc
  \brief                 Implementation of Physics histograms
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#include "Physics2SHistograms.h"
#include "HWDescription/Definition.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"

#include "Utils/Data2S.h"
#include "Utils/Occupancy.h"

using namespace Ph2_HwDescription;

void Physics2SHistograms::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& settingsMap)
{
    fDetectorContainer                                    = &theDetectorStructure;
    HistContainer<TH1F> theTopSensorOccupancyHistogram    = HistContainer<TH1F>("TopSensorOccupancy", "Top Sensor Occupancy", NCHANNELS / 2, -0.5, float(NCHANNELS / 2.) - 0.5);
    HistContainer<TH1F> theBottomSensorOccupancyHistogram = HistContainer<TH1F>("BottomSensorOccupancy", "Bottom Sensor Occupancy", NCHANNELS / 2, -0.5, float(NCHANNELS / 2.) - 0.5);
    HistContainer<TH1F> theStubPositionHistogram          = HistContainer<TH1F>("Stub Position", "Stub Position", NCHANNELS, -0.25, float(NCHANNELS / 2.) - 0.25);

    RootContainerFactory::bookChipHistograms<HistContainer<TH1F>>(theOutputFile, theDetectorStructure, fTopSensorHistogramContainer, theTopSensorOccupancyHistogram);
    RootContainerFactory::bookChipHistograms<HistContainer<TH1F>>(theOutputFile, theDetectorStructure, fBottomSensorHistogramContainer, theBottomSensorOccupancyHistogram);
    RootContainerFactory::bookChipHistograms<HistContainer<TH1F>>(theOutputFile, theDetectorStructure, fStubHistogramContainer, theStubPositionHistogram);
}

// void Physics2SHistograms::fillData(const DetectorDataContainer& DataContainer)
// {
//     for(const auto board: DataContainer)
// 	{
//         for(const auto opticalGroup: *board)
// 		{
//             for(const auto hybrid: *opticalGroup)
// 			{
//                 for(const auto chip: *hybrid)
//                 {
//                     TH1F* topClusterHistograms = fTopClusterHistograms.getObject(board->getId())
//                                                         ->getObject(opticalGroup->getId())
//                                                         ->getObject(hybrid->getId())
//                                                         ->getObject(chip->getId())
//                                                         ->getSummary<HistContainer<TH1F>>()
//                                                         .fTheHistogram;
//                     TH1F* bottomClusterHistograms = fBottomClusterHistograms.getObject(board->getId())
//                                                         ->getObject(opticalGroup->getId())
//                                                         ->getObject(hybrid->getId())
//                                                         ->getObject(chip->getId())
//                                                         ->getSummary<HistContainer<TH1F>>()
//                                                         .fTheHistogram;
//                     TH1F* stubHistograms = fStubPositionHistograms.getObject(board->getId())
//                                                         ->getObject(opticalGroup->getId())
//                                                         ->getObject(hybrid->getId())
//                                                         ->getObject(chip->getId())
//                                                         ->getSummary<HistContainer<TH1F>>()
//                                                         .fTheHistogram;
// 					auto data2S = chip->getSummary<Data2S<NCHANNELS, MAX_NUMBER_OF_STUB_CLUSTERS_2S>>();

//                     for(int pos=0; pos<MAX_NUMBER_OF_STUB_CLUSTERS_2S; ++pos)
//                     {
// 						stubHistograms->Fill(data2S.fStubs[pos].getPosition());
// 		            }
//                     for(int pos=0; pos<NCHANNELS; ++pos)
// 		            {
// 						if(data2S.fClusters[pos].getSensor() == 0)topClusterHistograms->Fill(data2S.fClusters[pos].getBaricentre());
// 						else bottomClusterHistograms->Fill(data2S.fClusters[pos].getBaricentre());
// 		            }

//                 }
//             }
//         }
//     }
// }

void Physics2SHistograms::fillOccupancy(const DetectorDataContainer& DataContainer)
{
    for(const auto board: DataContainer)
    {
        for(const auto opticalGroup: *board)
        {
            for(const auto hybrid: *opticalGroup)
            {
                for(const auto chip: *hybrid)
                {
                    if(chip->hasChannelContainer() == false) continue;

                    TH1F* topSensorHistogram = fTopSensorHistogramContainer.getObject(board->getId())
                                                   ->getObject(opticalGroup->getId())
                                                   ->getObject(hybrid->getId())
                                                   ->getObject(chip->getId())
                                                   ->getSummary<HistContainer<TH1F>>()
                                                   .fTheHistogram;

                    TH1F* bottomSensorHistogram = fBottomSensorHistogramContainer.getObject(board->getId())
                                                      ->getObject(opticalGroup->getId())
                                                      ->getObject(hybrid->getId())
                                                      ->getObject(chip->getId())
                                                      ->getSummary<HistContainer<TH1F>>()
                                                      .fTheHistogram;

                    uint16_t channelNumber = 0;
                    for(auto channel: *chip->getChannelContainer<Occupancy>())
                    {
                        if((int(channelNumber) % 2) == 0) { bottomSensorHistogram->Fill(int(channelNumber / 2) + 1, channel.fOccupancy); }
                        else { topSensorHistogram->Fill(int(channelNumber / 2) + 1, channel.fOccupancy); }
                        ++channelNumber;
                    }
                }
            }
        }
    }
}

void Physics2SHistograms::fillStub(const DetectorDataContainer& DataContainer)
{
    for(const auto board: DataContainer)
    {
        for(const auto opticalGroup: *board)
        {
            for(const auto hybrid: *opticalGroup)
            {
                for(const auto chip: *hybrid)
                {
                    if(chip->hasChannelContainer() == false) continue;

                    TH2F* stubHistogram = fStubHistogramContainer.getObject(board->getId())
                                              ->getObject(opticalGroup->getId())
                                              ->getObject(hybrid->getId())
                                              ->getObject(chip->getId())
                                              ->getSummary<HistContainer<TH2F>>()
                                              .fTheHistogram;

                    uint16_t channelNumber = 0;
                    for(auto channel: *chip->getChannelContainer<float>())
                    {
                        {
                            stubHistogram->Fill(float(channelNumber / 2.), channel);
                        }
                        ++channelNumber;
                    }
                }
            }
        }
    }
}

bool Physics2SHistograms::fill(std::string& inputStream)
{
    ContainerSerialization theOccupancySerialization("Physics2SOccupancy");
    ContainerSerialization theStubSerialization("Physics2SStub");

    if(theOccupancySerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched Physics2S Occupancy!!!!!\n";
        DetectorDataContainer fDetectorData = theOccupancySerialization.deserializeHybridContainer<Occupancy, EmptyContainer, EmptyContainer>(fDetectorContainer);
        fillOccupancy(fDetectorData);
        return true;
    }
    if(theStubSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched Physics2S Stub!!!!!\n";
        DetectorDataContainer fDetectorData = theStubSerialization.deserializeHybridContainer<float, EmptyContainer, EmptyContainer>(fDetectorContainer);
        fillStub(fDetectorData);
        return true;
    }
    return false;
}

void Physics2SHistograms::process()
{
    // This step it is not necessary, unless you want to format / draw histograms,
    // otherwise they will be automatically saved
    /*for(auto board: fOccupancy) // for on boards - begin
    {
        size_t boardId = board->getId();
        for(auto opticalGroup: *board) // for on opticalGroup - begin
        {
            size_t opticalGroupId = opticalGroup->getId();

            for(auto hybrid: *opticalGroup) // for on hybrid - begin
            {
                size_t hybridId = hybrid->getId();

                for(auto chip: *hybrid) // for on chip - begin
                {

                } // for on chip - end
            }     // for on hybrid - end
        }         // for on opticalGroup - end
    }             // for on boards - end*/
}
