/*!
  \file                  RD53LatencyHistograms.cc
  \brief                 Implementation of Latency calibration histograms
  \author                Alkiviadis PAPADOPOULOS
  \version               1.0
  \date                  28/06/18
  Support:               email to alkiviadis.papadopoulos@cern.ch
  Support:               email to mauro.dinardo@cern.ch
*/

#include "RD53LatencyHistograms.h"
#include "Utils/ContainerSerialization.h"

using namespace Ph2_HwDescription;

void LatencyHistograms::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& settingsMap)
{
    fDetectorContainer = &theDetectorStructure;
    RD53Shared::setFirstChip(theDetectorStructure);

    // #######################
    // # Retrieve parameters #
    // #######################
    nTRIGxEvent = this->findValueInSettings<double>(settingsMap, "nTRIGxEvent");
    startValue  = this->findValueInSettings<double>(settingsMap, "LatencyStart");
    stopValue   = this->findValueInSettings<double>(settingsMap, "LatencyStop");

    auto hLatency = CanvasContainer<TH1F>("Latency", "Latency", stopValue - startValue + nTRIGxEvent, startValue, stopValue + nTRIGxEvent);
    bookChipImplementer(theOutputFile, theDetectorStructure, Latency, hLatency, "Latency (n.bx)", "Entries");

    auto hOcc1D = CanvasContainer<TH1F>("LatencyScan", "Latency scan", stopValue - startValue + nTRIGxEvent, startValue, stopValue + nTRIGxEvent);
    bookChipImplementer(theOutputFile, theDetectorStructure, Occupancy1D, hOcc1D, "Latency (n.bx)", "Efficiency");

    AreHistoBooked = true;
}

bool LatencyHistograms::fill(std::string& inputStream)
{
    ContainerSerialization theOccupancySerialization("LatencyOccupancy");
    ContainerSerialization theLatencySerialization("LatencyLatency");

    if(theOccupancySerialization.attachDeserializer(inputStream))
    {
        DetectorDataContainer fDetectorData = theOccupancySerialization.deserializeChipContainer<EmptyContainer, std::vector<float>>(fDetectorContainer);
        LatencyHistograms::fillOccupancy(fDetectorData);
        return true;
    }
    if(theLatencySerialization.attachDeserializer(inputStream))
    {
        DetectorDataContainer fDetectorData = theLatencySerialization.deserializeChipContainer<EmptyContainer, uint16_t>(fDetectorContainer);
        LatencyHistograms::fillLatency(fDetectorData);
        return true;
    }
    return false;
}

void LatencyHistograms::fillOccupancy(const DetectorDataContainer& OccupancyContainer)
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

                    for(size_t i = startValue; i <= stopValue; i += nTRIGxEvent)
                        Occupancy1DHist->SetBinContent(Occupancy1DHist->FindBin(i), cChip->getSummary<std::vector<float>>().at((i - startValue) / nTRIGxEvent));
                }
}

void LatencyHistograms::fillLatency(const DetectorDataContainer& LatencyContainer)
{
    for(const auto cBoard: LatencyContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    if(cChip->hasSummary() == false) continue;

                    auto* LatencyHist = Latency.getObject(cBoard->getId())
                                            ->getObject(cOpticalGroup->getId())
                                            ->getObject(cHybrid->getId())
                                            ->getObject(cChip->getId())
                                            ->getSummary<CanvasContainer<TH1F>>()
                                            .fTheHistogram;

                    for(auto i = 0u; i < nTRIGxEvent; i++) LatencyHist->Fill(cChip->getSummary<uint16_t>() - i);
                }
}

void LatencyHistograms::process()
{
    drawChip<TH1F>(Occupancy1D);
    drawChip<TH1F>(Latency);
}
