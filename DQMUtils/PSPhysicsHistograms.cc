/*!
  \file                  PSPhysicsHistograms.cc
  \brief                 Implementation of Physics histograms
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#include "PSPhysicsHistograms.h"
#include "HWDescription/Definition.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"

#include "Utils/PSSync.h"

using namespace Ph2_HwDescription;

void PSPhysicsHistograms::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& settingsMap)
{
    fDetectorContainer = &theDetectorStructure;
    for(auto board: *fDetectorContainer)
        for(auto optical: *board)
            for(auto hybrid: *optical)
                for(auto chip: *hybrid)
                {
                    if(chip->getFrontEndType() == FrontEndType::MPA2) std::cout << "MPA2" << std::endl;
                    if(chip->getFrontEndType() == FrontEndType::SSA2) std::cout << "SSA2" << std::endl;
                }

    HistContainer<TH1F> theSClusterTemplateHistogram = HistContainer<TH1F>("S clusters", "S clusters", NSSACHANNELS, -0.5, NSSACHANNELS - 0.5);
    HistContainer<TH2F> thePClusterTemplateHistogram =
        HistContainer<TH2F>("P clusters", "P clusters", NSSACHANNELS, -0.5, NSSACHANNELS - 0.5, NMPAROWS * NSSACHANNELS / NSSACHANNELS, -0.5, float(NMPAROWS * NSSACHANNELS / NSSACHANNELS) - 0.5);
    HistContainer<TH2F> theStubTemplateHistogram =
        HistContainer<TH2F>("Stubs", "Stubs", NSSACHANNELS, -0.5, NSSACHANNELS - 0.5, NMPAROWS * NSSACHANNELS / NSSACHANNELS, -0.5, float(NMPAROWS * NSSACHANNELS / NSSACHANNELS) - 0.5);

    // auto mpaSelectFunction = [](const ChipContainer* theChip) { return (static_cast<const ReadoutChip*>(theChip)->getFrontEndType() == FrontEndType::MPA2); };
    // theDetectorStructure.setReadoutChipQueryFunction(mpaSelectFunction);
    RootContainerFactory::bookChipHistograms<HistContainer<TH2F>>(theOutputFile, theDetectorStructure, fStubHistogramContainer, theStubTemplateHistogram);
    RootContainerFactory::bookChipHistograms<HistContainer<TH2F>>(theOutputFile, theDetectorStructure, fOccupancyHistogramContainer, thePClusterTemplateHistogram);
    // theDetectorStructure.resetReadoutChipQueryFunction();

    // auto ssaSelectFunction = [](const ChipContainer* theChip) { return (static_cast<const ReadoutChip*>(theChip)->getFrontEndType() == FrontEndType::SSA2); };
    // theDetectorStructure.setReadoutChipQueryFunction(ssaSelectFunction);
    RootContainerFactory::bookChipHistograms<HistContainer<TH1F>>(theOutputFile, theDetectorStructure, fStripOccupancyHistogramContainer, theSClusterTemplateHistogram);
    // theDetectorStructure.resetReadoutChipQueryFunction();
}

// void PSPhysicsHistograms::fillSync(const DetectorDataContainer& DataContainer)
// {
//     for(const auto board: DataContainer)
// 	{
//         for(const auto opticalGroup: *board)
// 		{
//             for(const auto hybrid: *opticalGroup)
// 			{
//                 for(const auto chip: *hybrid)
//                 {
//                     TH1F* SClusterHistograms = fSClusterHistograms.getObject(board->getId())
//                                                         ->getObject(opticalGroup->getId())
//                                                         ->getObject(hybrid->getId())
//                                                         ->getObject(chip->getId())
//                                                         ->getSummary<HistContainer<TH1F>>()
//                                                         .fTheHistogram;
//                     TH2F* PClusterHistograms = fPClusterHistograms.getObject(board->getId())
//                                                         ->getObject(opticalGroup->getId())
//                                                         ->getObject(hybrid->getId())
//                                                         ->getObject(chip->getId())
//                                                         ->getSummary<HistContainer<TH2F>>()
//                                                         .fTheHistogram;
//                     TH2F* StubHistograms = fStubHistogramContainer.getObject(board->getId())
//                                                         ->getObject(opticalGroup->getId())
//                                                         ->getObject(hybrid->getId())
//                                                         ->getObject(chip->getId())
//                                                         ->getSummary<HistContainer<TH2F>>()
//                                                         .fTheHistogram;
//                     if(!chip->hasSummary()) continue;
//                     // else LOG(INFO) << BOLDBLUE << "Something received from chip " << chip->getId() << RESET;
// 					auto curPSSync = chip->getSummary<PSSync<MAX_NUMBER_OF_STRIP_CLUSTERS, MAX_NUMBER_OF_PIXEL_CLUSTERS,MAX_NUMBER_OF_STUB_CLUSTERS_PS>>();

//                     for(int pos=0; pos<MAX_NUMBER_OF_STUB_CLUSTERS_PS; ++pos)
//                     {
// 						StubHistograms->Fill(curPSSync.fStubs[pos].getPosition(),curPSSync.fStubs[pos].getRow());
// 		            }
//                     for(int pos=0; pos<MAX_NUMBER_OF_PIXEL_CLUSTERS; ++pos)
// 		            {
//                         if(curPSSync.fPClusters[pos].fAddress != 255u) std::cout<<"good pixel cluster"<<std::endl;
// 						PClusterHistograms->Fill(curPSSync.fPClusters[pos].fAddress ,curPSSync.fPClusters[pos].fZpos);
// 		            }
//                     for(int pos=0; pos<MAX_NUMBER_OF_STRIP_CLUSTERS; ++pos)
//                     {
//                         if(curPSSync.fSClusters[pos].fAddress != 255u) std::cout<<"good pixel cluster"<<std::endl;
// 						SClusterHistograms->Fill(curPSSync.fSClusters[pos].fAddress);
// 		            }

//                 }
//             }
//         }
//     }
// }

void PSPhysicsHistograms::fillOccupancy(const DetectorDataContainer& DataContainer)
{
    // std::cout<<__LINE__<<std::endl;
    for(const auto board: DataContainer)
    {
        // std::cout<<__LINE__<<std::endl;
        for(const auto opticalGroup: *board)
        {
            // std::cout<<__LINE__<<std::endl;
            for(const auto hybrid: *opticalGroup)
            {
                // std::cout<<__LINE__<<std::endl;
                for(const auto chip: *hybrid)
                {
                    // std::cout<<__LINE__<<std::endl;
                    if(chip->hasChannelContainer() == false) continue;

                    // std::cout<<__LINE__<<std::endl;
                    // std::cout<<"board = "<<board->getId()<<std::endl;
                    // std::cout<<"opticalGroup = "<<opticalGroup->getId()<<std::endl;
                    // std::cout<<"hybrid = "<<hybrid->getId()<<std::endl;
                    // std::cout<<"chip = "<<chip->getId()<<std::endl;
                    FrontEndType theFrontEndType =
                        fDetectorContainer->getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getObject(chip->getId())->getFrontEndType();
                    // std::cout<<__LINE__<<std::endl;
                    if(theFrontEndType == FrontEndType::MPA2)
                    {
                        // std::cout<<__LINE__<<std::endl;
                        TH2F* pixelClusterHistogram = fOccupancyHistogramContainer.getObject(board->getId())
                                                          ->getObject(opticalGroup->getId())
                                                          ->getObject(hybrid->getId())
                                                          ->getObject(chip->getId())
                                                          ->getSummary<HistContainer<TH2F>>()
                                                          .fTheHistogram;

                        // std::cout<<__LINE__<<std::endl;
                        for(int row = 0; row < NMPAROWS * NSSACHANNELS / NSSACHANNELS; ++row)
                        {
                            // std::cout<<__LINE__<<std::endl;
                            for(int col = 0; col < NSSACHANNELS; ++col)
                            {
                                // std::cout<<col<<" "<<row<<std::endl;
                                pixelClusterHistogram->Fill(col, row, chip->getChannel<float>(row, col));
                                // std::cout<<__LINE__<<std::endl;
                            }
                        }
                        // std::cout<<__LINE__<<std::endl;
                    }
                    else
                    {
                        // std::cout<<__LINE__<<std::endl;
                        // std::cout<<chip->getId()<<std::endl;
                        // std::cout<<hybrid->size()<<std::endl;

                        TH1F* stripClusterHistogram = fStripOccupancyHistogramContainer.getObject(board->getId())
                                                          ->getObject(opticalGroup->getId())
                                                          ->getObject(hybrid->getId())
                                                          ->getObject(chip->getId())
                                                          ->getSummary<HistContainer<TH1F>>()
                                                          .fTheHistogram;
                        // std::cout<<__LINE__<<std::endl;
                        for(int channel = 0; channel < NSSACHANNELS; ++channel) { stripClusterHistogram->Fill(channel, chip->getChannel<float>(0, channel)); }
                        // std::cout<<__LINE__<<std::endl;
                    }
                }
            }
        }
    }
}

void PSPhysicsHistograms::fillStub(const DetectorDataContainer& DataContainer)
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

                    FrontEndType theFrontEndType =
                        fDetectorContainer->getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getObject(chip->getId())->getFrontEndType();
                    if(theFrontEndType != FrontEndType::MPA2) continue;

                    TH2F* stubHistogram = fStubHistogramContainer.getObject(board->getId())
                                              ->getObject(opticalGroup->getId())
                                              ->getObject(hybrid->getId())
                                              ->getObject(chip->getId())
                                              ->getSummary<HistContainer<TH2F>>()
                                              .fTheHistogram;

                    for(int row = 0; row < NMPAROWS * NSSACHANNELS / NSSACHANNELS; ++row)
                    {
                        for(int col = 0; col < NSSACHANNELS; ++col) { stubHistogram->Fill(col, row, chip->getChannel<float>(row, col)); }
                    }
                }
            }
        }
    }
}

bool PSPhysicsHistograms::fill(std::string& inputStream)
{
    ContainerSerialization theOccupancySerialization("PSPhysicsOccupancy");
    ContainerSerialization theStubSerialization("PSPhysicsStub");

    if(theOccupancySerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched PSPhysics Occupancy!!!!!\n";
        DetectorDataContainer fDetectorData = theOccupancySerialization.deserializeHybridContainer<float, EmptyContainer, EmptyContainer>(fDetectorContainer);
        fillOccupancy(fDetectorData);
        return true;
    }
    if(theStubSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched PSPhysics Stub!!!!!\n";
        DetectorDataContainer fDetectorData = theStubSerialization.deserializeHybridContainer<float, EmptyContainer, EmptyContainer>(fDetectorContainer);
        fillStub(fDetectorData);
        return true;
    }
    return false;
}

void PSPhysicsHistograms::process()
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
