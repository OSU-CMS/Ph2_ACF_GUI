/*!
  \file                  SSAPhysicsHistograms.cc
  \brief                 Implementation of Physics histograms
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#include "SSAPhysicsHistograms.h"
#include "HWDescription/Definition.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/Occupancy.h"

using namespace Ph2_HwDescription;

void SSAPhysicsHistograms::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& settingsMap)
{
    fDetectorContainer                         = &theDetectorStructure;
    HistContainer<TH1F> theOcccupancyContainer = HistContainer<TH1F>("Occ2D", "Occupancy", NSSACHANNELS, -0.5, NSSACHANNELS - 0.5);
    RootContainerFactory::bookChipHistograms<HistContainer<TH1F>>(theOutputFile, theDetectorStructure, fOccupancy, theOcccupancyContainer);
}

bool SSAPhysicsHistograms::fill(std::string& inputStream)
{
    ContainerSerialization theOccupancySerialization("SSAPhysicsOccupancy");

    if(theOccupancySerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched SSAPhysics Occupancy!!!!!\n";
        DetectorDataContainer fDetectorData = theOccupancySerialization.deserializeHybridContainer<Occupancy, Occupancy, Occupancy>(fDetectorContainer);
        fillOccupancy(fDetectorData);
        return true;
    }
    return false;
}

void SSAPhysicsHistograms::fillOccupancy(const DetectorDataContainer& DataContainer)
{
    for(const auto cBoard: DataContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    if(cChip->hasChannelContainer() == false) continue;

                    auto* chipOccupancy = fOccupancy.getObject(cBoard->getId())
                                              ->getObject(cOpticalGroup->getId())
                                              ->getObject(cHybrid->getId())
                                              ->getObject(cChip->getId())
                                              ->getSummary<HistContainer<TH1F>>()
                                              .fTheHistogram;
                    uint channelBin = 1;

                    // Get channel data and fill the histogram
                    for(auto channel: *cChip->getChannelContainer<Occupancy>()) // for on channel - begin
                    {
                        chipOccupancy->Fill(channelBin++, channel.fOccupancy);
                    }
                }
}

void SSAPhysicsHistograms::process()
{
    // This step it is not necessary, unless you want to format / draw histograms,
    // otherwise they will be automatically saved
    for(auto board: fOccupancy) // for on boards - begin
    {
        size_t boardId = board->getId();
        for(auto opticalGroup: *board) // for on opticalGroup - begin
        {
            size_t opticalGroupId = opticalGroup->getId();

            for(auto hybrid: *opticalGroup) // for on hybrid - begin
            {
                size_t hybridId = hybrid->getId();

                // Create a canvas do draw the plots
                std::string occupancyCanvasName = "Occupancy_B_" + std::to_string(board->getId()) + "_O_" + std::to_string(opticalGroup->getId()) + "_H_" + std::to_string(hybrid->getId());
                TCanvas*    cOccupancy          = new TCanvas(occupancyCanvasName.c_str(), occupancyCanvasName.c_str(), 0, 0, 650, 650);
                cOccupancy->Divide(hybrid->size());

                for(auto chip: *hybrid) // for on chip - begin
                {
                    size_t chipId = chip->getId();
                    cOccupancy->cd(chipId + 1);
                    // Retreive the corresponging chip histogram:
                    TH1F* chipHitHistogram = fOccupancy.getObject(boardId)->getObject(opticalGroupId)->getObject(hybridId)->getObject(chipId)->getSummary<HistContainer<TH1F>>().fTheHistogram;

                    // Format the histogram (here you are outside from the SoC so you can use all the ROOT functions you
                    // need)
                    chipHitHistogram->SetStats(false);
                    chipHitHistogram->DrawCopy();
                } // for on chip - end
            } // for on hybrid - end
        } // for on opticalGroup - end
    } // for on boards - end
}
