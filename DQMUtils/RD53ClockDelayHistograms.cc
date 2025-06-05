/*!
  \file                  RD53ClockDelayHistograms.cc
  \brief                 Implementation of ClockDelay calibration histograms
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#include "RD53ClockDelayHistograms.h"
#include "Utils/ContainerSerialization.h"

using namespace Ph2_HwDescription;

void ClockDelayHistograms::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& settingsMap)
{
    fDetectorContainer = &theDetectorStructure;
    RD53Shared::setFirstChip(theDetectorStructure);

    // #######################
    // # Retrieve parameters #
    // #######################
    const auto frontEnd = RD53Shared::firstChip->getFEtype(RD53Shared::firstChip->getNRows() / 2, RD53Shared::firstChip->getNCols() / 2);
    startValue          = 0;
    stopValue           = frontEnd->nLatencyBins2Span * (RD53Shared::setBits(RD53Shared::firstChip->getNumberOfBits("CLK_DATA_DELAY_CLK")) + 1) - 1;
    const auto unitTime = 1. / RD53Constants::ACCELERATOR_CLK * 1000 / ((RD53Shared::setBits(RD53Shared::firstChip->getNumberOfBits("CAL_EDGE_FINE_DELAY")) + 1) / (2. / frontEnd->nLatencyBins2Span));
    std::stringstream title;
    title << "Clock Delay (" << unitTime << " ns)";

    auto hClockDelay = CanvasContainer<TH1F>("ClockDelay", "Clock Delay", stopValue - startValue + 1, startValue, stopValue + 1);
    bookChipImplementer(theOutputFile, theDetectorStructure, ClockDelay, hClockDelay, title.str().c_str(), "Entries");

    auto hOcc1D = CanvasContainer<TH1F>("ClkDelayScan", "Clock Delay scan", stopValue - startValue + 1, startValue, stopValue + 1);
    bookChipImplementer(theOutputFile, theDetectorStructure, Occupancy1D, hOcc1D, title.str().c_str(), "Efficiency");

    AreHistoBooked = true;
}

bool ClockDelayHistograms::fill(std::string& inputStream)
{
    ContainerSerialization theOccupancySerialization("ClockDelayOccupancy");
    ContainerSerialization theClockDelaySerialization("ClockDelayClockDelay");

    if(theOccupancySerialization.attachDeserializer(inputStream))
    {
        DetectorDataContainer fDetectorData = theOccupancySerialization.deserializeChipContainer<EmptyContainer, std::vector<float>>(fDetectorContainer);
        ClockDelayHistograms::fillOccupancy(fDetectorData);
        return true;
    }
    if(theClockDelaySerialization.attachDeserializer(inputStream))
    {
        DetectorDataContainer fDetectorData = theClockDelaySerialization.deserializeChipContainer<EmptyContainer, uint16_t>(fDetectorContainer);
        ClockDelayHistograms::fillClockDelay(fDetectorData);
        return true;
    }
    return false;
}

void ClockDelayHistograms::fillOccupancy(const DetectorDataContainer& OccupancyContainer)
{
    for(const auto cBoard: OccupancyContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    if(cChip->hasSummary() == false) continue;

                    auto* Occupancy1DHist = Occupancy1D.getObject(cBoard->getId())
                                                ->getObject(cOpticalGroup->getId())
                                                ->getObject(cHybrid->getId())
                                                ->getObject(cChip->getId())
                                                ->getSummary<CanvasContainer<TH1F>>()
                                                .fTheHistogram;

                    for(size_t i = startValue; i <= stopValue; i++) Occupancy1DHist->SetBinContent(Occupancy1DHist->FindBin(i), cChip->getSummary<std::vector<float>>().at(i - startValue));
                }
}

void ClockDelayHistograms::fillClockDelay(const DetectorDataContainer& ClockDelayContainer)
{
    for(const auto cBoard: ClockDelayContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    if(cChip->hasSummary() == false) continue;

                    auto* ClockDelayHist = ClockDelay.getObject(cBoard->getId())
                                               ->getObject(cOpticalGroup->getId())
                                               ->getObject(cHybrid->getId())
                                               ->getObject(cChip->getId())
                                               ->getSummary<CanvasContainer<TH1F>>()
                                               .fTheHistogram;

                    ClockDelayHist->Fill(cChip->getSummary<uint16_t>());
                }
}

void ClockDelayHistograms::process()
{
    drawChip<TH1F>(Occupancy1D);
    drawChip<TH1F>(ClockDelay);
}
