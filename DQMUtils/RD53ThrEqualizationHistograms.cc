/*!
  \file                  RD53ThrEqualizationHistograms.cc
  \brief                 Implementation of ThrEqualization calibration histograms
  \author                Alkiviadis PAPADOPOULOS
  \version               1.0
  \date                  28/06/18
  Support:               email to alkiviadis.papadopoulos@cern.ch
  Support:               email to mauro.dinardo@cern.ch
*/

#include "RD53ThrEqualizationHistograms.h"
#include "Utils/ContainerSerialization.h"

using namespace Ph2_HwDescription;

void ThrEqualizationHistograms::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& settingsMap)
{
    fDetectorContainer = &theDetectorStructure;
    RD53Shared::setFirstChip(theDetectorStructure);

    nRows = RD53Shared::firstChip->getNRows();
    nCols = RD53Shared::firstChip->getNCols();

    // #######################
    // # Retrieve parameters #
    // #######################
    startValue            = this->findValueInSettings<double>(settingsMap, "TDACGainStart");
    stopValue             = this->findValueInSettings<double>(settingsMap, "TDACGainStop");
    nEvents               = this->findValueInSettings<double>(settingsMap, "nEvents");
    const size_t colStart = this->findValueInSettings<double>(settingsMap, "COLstart");
    const size_t colStop  = this->findValueInSettings<double>(settingsMap, "COLstop");
    frontEnd              = RD53Shared::firstChip->getFEtype(colStart, colStop);

    auto hThrEqualization = CanvasContainer<TH1F>("ThrEqualization", "Threshold Equalization", nEvents + 1, 0, 1 + 1. / nEvents);
    bookChipImplementer(theOutputFile, theDetectorStructure, ThrEqualization, hThrEqualization, "Efficiency", "Entries");

    auto hTDAC1D = CanvasContainer<TH1F>("TDAC1D", "TDAC distribution", frontEnd->nTDACvalues, 0, frontEnd->nTDACvalues);
    bookChipImplementer(theOutputFile, theDetectorStructure, TDAC1D, hTDAC1D, "TDAC", "Entries");

    auto hTDAC2D = CanvasContainer<TH2F>("TDAC2D", "TDAC map", nCols, 0, nCols, nRows, 0, nRows);
    bookChipImplementer(theOutputFile, theDetectorStructure, TDAC2D, hTDAC2D, "Column", "Row");

    auto hOcc1D = CanvasContainer<TH1F>("TDACGainScan", "TDAC Gain scan", stopValue - startValue + 1, startValue, stopValue + 1);
    bookChipImplementer(theOutputFile, theDetectorStructure, Occupancy1D, hOcc1D, "TDAC Gain", "Threshold Distribution (std.dev.)");

    auto hTDACGain = CanvasContainer<TH1F>("TDACGain", "TDAC Gain", stopValue - startValue + 1, startValue, stopValue + 1);
    bookChipImplementer(theOutputFile, theDetectorStructure, TDACGain, hTDACGain, "TDAC Gain", "Entries");

    AreHistoBooked = true;
}

bool ThrEqualizationHistograms::fill(std::string& inputStream)
{
    ContainerSerialization theOccupancySerialization("ThrEqualizationOccupancy");
    ContainerSerialization theTDACSerialization("ThrEqualizationTDAC");
    ContainerSerialization theOccupancyScanSerialization("ThrEqualizationOccupancyScan");
    ContainerSerialization theTDACGainSerialization("ThrEqualizationTDACGain");

    if(theOccupancySerialization.attachDeserializer(inputStream))
    {
        DetectorDataContainer fDetectorData = theOccupancySerialization.deserializeChipContainer<OccupancyAndPh, GenericDataVector>(fDetectorContainer);
        ThrEqualizationHistograms::fillOccupancy(fDetectorData);
        return true;
    }
    if(theTDACSerialization.attachDeserializer(inputStream))
    {
        DetectorDataContainer fDetectorData = theTDACSerialization.deserializeChipContainer<uint16_t, EmptyContainer>(fDetectorContainer);
        ThrEqualizationHistograms::fillTDAC(fDetectorData);
        return true;
    }
    if(theOccupancyScanSerialization.attachDeserializer(inputStream))
    {
        DetectorDataContainer fDetectorData = theOccupancyScanSerialization.deserializeChipContainer<EmptyContainer, std::vector<float>>(fDetectorContainer);
        ThrEqualizationHistograms::fillOccupancyScan(fDetectorData);
        return true;
    }
    if(theTDACGainSerialization.attachDeserializer(inputStream))
    {
        DetectorDataContainer fDetectorData = theTDACGainSerialization.deserializeChipContainer<EmptyContainer, uint16_t>(fDetectorContainer);
        ThrEqualizationHistograms::fillTDACGain(fDetectorData);
        return true;
    }
    return false;
}

void ThrEqualizationHistograms::fillOccupancy(const DetectorDataContainer& OccupancyContainer)
{
    for(const auto cBoard: OccupancyContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    if(OccupancyContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->hasChannelContainer() == false)
                        continue;

                    auto* hThrEqualization = ThrEqualization.getObject(cBoard->getId())
                                                 ->getObject(cOpticalGroup->getId())
                                                 ->getObject(cHybrid->getId())
                                                 ->getObject(cChip->getId())
                                                 ->getSummary<CanvasContainer<TH1F>>()
                                                 .fTheHistogram;

                    for(auto row = 0u; row < nRows; row++)
                        for(auto col = 0u; col < nCols; col++)
                            if(cChip->getChannel<OccupancyAndPh>(row, col).fStatus == RD53Shared::ISGOOD)
                                hThrEqualization->Fill(cChip->getChannel<OccupancyAndPh>(row, col).fOccupancy + hThrEqualization->GetBinWidth(1) / 2);
                }
}

void ThrEqualizationHistograms::fillTDAC(const DetectorDataContainer& TDACContainer)
{
    for(const auto cBoard: TDACContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    if(TDACContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->hasChannelContainer() == false) continue;

                    auto* hTDAC1D =
                        TDAC1D.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<CanvasContainer<TH1F>>().fTheHistogram;

                    auto* hTDAC2D =
                        TDAC2D.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<CanvasContainer<TH2F>>().fTheHistogram;

                    for(auto row = 0u; row < nRows; row++)
                        for(auto col = 0u; col < nCols; col++)
                            if(cChip->getChannel<uint16_t>(row, col) != frontEnd->nTDACvalues)
                            {
                                hTDAC1D->Fill(cChip->getChannel<uint16_t>(row, col));
                                hTDAC2D->SetBinContent(col + 1, row + 1, cChip->getChannel<uint16_t>(row, col));
                            }
                }
}

void ThrEqualizationHistograms::fillOccupancyScan(const DetectorDataContainer& OccupancyContainer)
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

                    const float TDACGainNSteps = cChip->getSummary<std::vector<float>>().size();
                    const float step           = (TDACGainNSteps != 0 ? (stopValue - startValue) / TDACGainNSteps : 0);
                    for(auto i = 0u; i < TDACGainNSteps; i++) Occupancy1DHist->SetBinContent(Occupancy1DHist->FindBin(startValue + step * i), cChip->getSummary<std::vector<float>>().at(i));
                }
}

void ThrEqualizationHistograms::fillTDACGain(const DetectorDataContainer& TDACGainContainer)
{
    for(const auto cBoard: TDACGainContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    if(cChip->hasSummary() == false) continue;

                    auto* TDACGainHist = TDACGain.getObject(cBoard->getId())
                                             ->getObject(cOpticalGroup->getId())
                                             ->getObject(cHybrid->getId())
                                             ->getObject(cChip->getId())
                                             ->getSummary<CanvasContainer<TH1F>>()
                                             .fTheHistogram;

                    TDACGainHist->Fill(cChip->getSummary<uint16_t>());
                }
}

void ThrEqualizationHistograms::process()
{
    drawChip<TH1F>(ThrEqualization);
    drawChip<TH1F>(TDAC1D);
    drawChip<TH2F>(TDAC2D, "gcolz");
    drawChip<TH1F>(Occupancy1D);
    drawChip<TH1F>(TDACGain);
}
