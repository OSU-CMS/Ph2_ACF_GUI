/*!
  \file                  RD53SCurveHistograms.cc
  \brief                 Implementation of SCurve calibration histograms
  \author                Alkiviadis PAPADOPOULOS
  \version               1.0
  \date                  28/06/18
  Support:               email to alkiviadis.papadopoulos@cern.ch
  Support:               email to mauro.dinardo@cern.ch
*/

#include "RD53SCurveHistograms.h"
#include "Utils/ContainerSerialization.h"

using namespace Ph2_HwDescription;

void SCurveHistograms::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& settingsMap)
{
    fDetectorContainer = &theDetectorStructure;
    RD53Shared::setFirstChip(theDetectorStructure);

    nRows = RD53Shared::firstChip->getNRows();
    nCols = RD53Shared::firstChip->getNCols();

    // #######################
    // # Retrieve parameters #
    // #######################
    nEvents    = this->findValueInSettings<double>(settingsMap, "nEvents");
    nSteps     = this->findValueInSettings<double>(settingsMap, "VCalHnsteps");
    startValue = this->findValueInSettings<double>(settingsMap, "VCalHstart");
    stopValue  = this->findValueInSettings<double>(settingsMap, "VCalHstop");
    offset     = this->findValueInSettings<double>(settingsMap, "VCalMED");

    auto hOcc2D = CanvasContainer<TH2F>("SCurves", "SCurves", nSteps, startValue - offset, stopValue - offset, 2 * nEvents + 1, 0, 2 + 1. / nEvents);
    bookChipImplementer(theOutputFile, theDetectorStructure, Occupancy2D, hOcc2D, "#DeltaVCal", "Efficiency");

    auto hOcc3D = CanvasContainer<TH3F>("SCurveMap", "SCurve map", nCols, 0, nCols, nRows, 0, nRows, nSteps, startValue - offset, stopValue - offset);
    bookChipImplementer(theOutputFile, theDetectorStructure, Occupancy3D, hOcc3D, "Column", "Row", "#DeltaVCal");

    auto hErrorReadOut2D = CanvasContainer<TH2F>("ReadoutErrors", "Readout errors", nCols, 0, nCols, nRows, 0, nRows);
    bookChipImplementer(theOutputFile, theDetectorStructure, ErrorReadOut2D, hErrorReadOut2D, "Columns", "Rows");

    auto hErrorFit2D = CanvasContainer<TH2F>("FitErrors", "Fit errors", nCols, 0, nCols, nRows, 0, nRows);
    bookChipImplementer(theOutputFile, theDetectorStructure, ErrorFit2D, hErrorFit2D, "Columns", "Rows");

    auto hThreshold1D = CanvasContainer<TH1F>("Threshold1D", "Threshold distribution", NBINS_THR, startValue - offset, stopValue - offset);
    bookChipImplementer(theOutputFile, theDetectorStructure, Threshold1D, hThreshold1D, "Threshold (#DeltaVCal)", "Entries");

    auto hNoise1D = CanvasContainer<TH1F>("Noise1D", "Noise distribution", NBINS_NOISE, 0, NBINS_NOISE);
    bookChipImplementer(theOutputFile, theDetectorStructure, Noise1D, hNoise1D, "Noise (#DeltaVCal)", "Entries");

    auto hThreshold2D = CanvasContainer<TH2F>("Threshold2D", "Threshold map", nCols, 0, nCols, nRows, 0, nRows);
    bookChipImplementer(theOutputFile, theDetectorStructure, Threshold2D, hThreshold2D, "Column", "Row");

    auto hNoise2D = CanvasContainer<TH2F>("Noise2D", "Noise map", nCols, 0, nCols, nRows, 0, nRows);
    bookChipImplementer(theOutputFile, theDetectorStructure, Noise2D, hNoise2D, "Column", "Row");

    auto hToT2D = CanvasContainer<TH2F>("ToT2D", "Integrated ToT map", nCols, 0, nCols, nRows, 0, nRows);
    bookChipImplementer(theOutputFile, theDetectorStructure, ToT2D, hToT2D, "Columns", "Rows");

    auto hThrNoise2D = CanvasContainer<TH2F>("ThrNoise2D", "Noise vs Threshold scatter plot", stopValue - startValue, startValue - offset, stopValue - offset, NBINS_NOISE, 0, NBINS_NOISE);
    bookChipImplementer(theOutputFile, theDetectorStructure, ThrNoise2D, hThrNoise2D, "Threshold (#DeltaVCal)", "Noise (#DeltaVCal)");

    AreHistoBooked = true;
}

bool SCurveHistograms::fill(std::string& inputStream)
{
    ContainerSerialization theThresholdAndNoiseSerialization("SCurveThresholdAndNoise");
    ContainerSerialization theOccupancySerialization("SCurveOccupancy");

    if(theThresholdAndNoiseSerialization.attachDeserializer(inputStream))
    {
        DetectorDataContainer fDetectorData = theThresholdAndNoiseSerialization.deserializeChipContainer<ThresholdAndNoise, ThresholdAndNoise>(fDetectorContainer);
        SCurveHistograms::fillThrAndNoise(fDetectorData);
        return true;
    }
    if(theOccupancySerialization.attachDeserializer(inputStream))
    {
        uint16_t              deltaVcal;
        DetectorDataContainer fDetectorData = theOccupancySerialization.deserializeChipContainer<OccupancyAndPh, OccupancyAndPh>(fDetectorContainer, deltaVcal);
        SCurveHistograms::fillOccupancy(fDetectorData, deltaVcal);
        return true;
    }
    return false;
}

void SCurveHistograms::fillOccupancy(const DetectorDataContainer& OccupancyContainer, uint16_t DELTA_VCAL)
{
    for(const auto cBoard: OccupancyContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    if(OccupancyContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->hasChannelContainer() == false)
                        continue;

                    auto* hOcc2D = Occupancy2D.getObject(cBoard->getId())
                                       ->getObject(cOpticalGroup->getId())
                                       ->getObject(cHybrid->getId())
                                       ->getObject(cChip->getId())
                                       ->getSummary<CanvasContainer<TH2F>>()
                                       .fTheHistogram;
                    auto* hOcc3D = Occupancy3D.getObject(cBoard->getId())
                                       ->getObject(cOpticalGroup->getId())
                                       ->getObject(cHybrid->getId())
                                       ->getObject(cChip->getId())
                                       ->getSummary<CanvasContainer<TH3F>>()
                                       .fTheHistogram;
                    auto* ErrorReadOut2DHist = ErrorReadOut2D.getObject(cBoard->getId())
                                                   ->getObject(cOpticalGroup->getId())
                                                   ->getObject(cHybrid->getId())
                                                   ->getObject(cChip->getId())
                                                   ->getSummary<CanvasContainer<TH2F>>()
                                                   .fTheHistogram;
                    auto* ToT2DHist =
                        ToT2D.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<CanvasContainer<TH2F>>().fTheHistogram;

                    for(auto row = 0u; row < nRows; row++)
                        for(auto col = 0u; col < nCols; col++)
                        {
                            if(cChip->getChannel<OccupancyAndPh>(row, col).fStatus == RD53Shared::ISGOOD)
                            {
                                hOcc2D->Fill(DELTA_VCAL, cChip->getChannel<OccupancyAndPh>(row, col).fOccupancy + hOcc2D->GetYaxis()->GetBinWidth(1) / 2.);
                                hOcc3D->SetBinContent(col + 1, row + 1, hOcc3D->GetZaxis()->FindBin(DELTA_VCAL), cChip->getChannel<OccupancyAndPh>(row, col).fOccupancy);
                                ToT2DHist->SetBinContent(col + 1, row + 1, ToT2DHist->GetBinContent(col + 1, row + 1) + cChip->getChannel<OccupancyAndPh>(row, col).fPh);
                                ToT2DHist->SetBinError(col + 1,
                                                       row + 1,
                                                       sqrt(ToT2DHist->GetBinError(col + 1, row + 1) * ToT2DHist->GetBinError(col + 1, row + 1) +
                                                            cChip->getChannel<OccupancyAndPh>(row, col).fPhError * cChip->getChannel<OccupancyAndPh>(row, col).fPhError));
                            }
                            if(cChip->getChannel<OccupancyAndPh>(row, col).readoutError == true) ErrorReadOut2DHist->Fill(col, row);
                        }

                    hOcc2D->GetYaxis()->SetRangeUser(0, 1 + 1. / nEvents);
                }
}

void SCurveHistograms::fillThrAndNoise(const DetectorDataContainer& ThrAndNoiseContainer)
{
    for(const auto cBoard: ThrAndNoiseContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    if(ThrAndNoiseContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->hasChannelContainer() == false)
                        continue;

                    auto* Threshold1DHist = Threshold1D.getObject(cBoard->getId())
                                                ->getObject(cOpticalGroup->getId())
                                                ->getObject(cHybrid->getId())
                                                ->getObject(cChip->getId())
                                                ->getSummary<CanvasContainer<TH1F>>()
                                                .fTheHistogram;
                    auto* Noise1DHist = Noise1D.getObject(cBoard->getId())
                                            ->getObject(cOpticalGroup->getId())
                                            ->getObject(cHybrid->getId())
                                            ->getObject(cChip->getId())
                                            ->getSummary<CanvasContainer<TH1F>>()
                                            .fTheHistogram;
                    auto* Threshold2DHist = Threshold2D.getObject(cBoard->getId())
                                                ->getObject(cOpticalGroup->getId())
                                                ->getObject(cHybrid->getId())
                                                ->getObject(cChip->getId())
                                                ->getSummary<CanvasContainer<TH2F>>()
                                                .fTheHistogram;
                    auto* Noise2DHist = Noise2D.getObject(cBoard->getId())
                                            ->getObject(cOpticalGroup->getId())
                                            ->getObject(cHybrid->getId())
                                            ->getObject(cChip->getId())
                                            ->getSummary<CanvasContainer<TH2F>>()
                                            .fTheHistogram;
                    auto* ErrorFit2DHist = ErrorFit2D.getObject(cBoard->getId())
                                               ->getObject(cOpticalGroup->getId())
                                               ->getObject(cHybrid->getId())
                                               ->getObject(cChip->getId())
                                               ->getSummary<CanvasContainer<TH2F>>()
                                               .fTheHistogram;
                    auto* ThrNoise2DHist = ThrNoise2D.getObject(cBoard->getId())
                                               ->getObject(cOpticalGroup->getId())
                                               ->getObject(cHybrid->getId())
                                               ->getObject(cChip->getId())
                                               ->getSummary<CanvasContainer<TH2F>>()
                                               .fTheHistogram;

                    for(auto row = 0u; row < nRows; row++)
                        for(auto col = 0u; col < nCols; col++)
                            if(cChip->getChannel<ThresholdAndNoise>(row, col).fNoise == RD53Shared::ISFITERROR)
                                ErrorFit2DHist->Fill(col, row);
                            else if(cChip->getChannel<ThresholdAndNoise>(row, col).fNoise != 0)
                            {
                                // #################
                                // # 1D histograms #
                                // #################
                                Threshold1DHist->Fill(cChip->getChannel<ThresholdAndNoise>(row, col).fThreshold);
                                Noise1DHist->Fill(cChip->getChannel<ThresholdAndNoise>(row, col).fNoise);

                                // #################
                                // # 2D histograms #
                                // #################
                                Threshold2DHist->SetBinContent(col + 1, row + 1, cChip->getChannel<ThresholdAndNoise>(row, col).fThreshold);
                                Threshold2DHist->SetBinError(col + 1, row + 1, cChip->getChannel<ThresholdAndNoise>(row, col).fThresholdError);
                                Noise2DHist->SetBinContent(col + 1, row + 1, cChip->getChannel<ThresholdAndNoise>(row, col).fNoise);
                                Noise2DHist->SetBinError(col + 1, row + 1, cChip->getChannel<ThresholdAndNoise>(row, col).fNoiseError);
                                ThrNoise2DHist->Fill(cChip->getChannel<ThresholdAndNoise>(row, col).fThreshold, cChip->getChannel<ThresholdAndNoise>(row, col).fNoise);
                            }
                }
}

void SCurveHistograms::process()
{
    drawChip<TH2F>(Occupancy2D, "gcolz logz", "electron", "Charge (electrons)");
    drawChip<TH3F>(Occupancy3D, "gcolz");
    drawChip<TH2F>(ErrorReadOut2D, "gcolz");
    drawChip<TH2F>(ErrorFit2D, "gcolz");
    drawChip<TH1F>(Threshold1D, "", "electron", "Threshold (electrons)");
    drawChip<TH1F>(Noise1D, "", "electron", "Noise (electrons)", true);
    drawChip<TH2F>(Threshold2D, "gcolz");
    drawChip<TH2F>(Noise2D, "gcolz");
    drawChip<TH2F>(ToT2D, "gcolz");
    drawChip<TH2F>(ThrNoise2D, "gcolz");
}
