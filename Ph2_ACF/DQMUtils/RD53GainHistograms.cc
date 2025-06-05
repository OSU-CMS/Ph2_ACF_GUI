/*!
  \file                  RD53GainHistograms.cc
  \brief                 Implementation of Gain calibration histograms
  \author                Alkiviadis PAPADOPOULOS
  \version               1.0
  \date                  28/06/18
  Support:               email to alkiviadis.papadopoulos@cern.ch
  Support:               email to mauro.dinardo@cern.ch
*/

#include "RD53GainHistograms.h"
#include "Utils/ContainerSerialization.h"

using namespace Ph2_HwDescription;

void GainHistograms::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& settingsMap)
{
    fDetectorContainer = &theDetectorStructure;
    RD53Shared::setFirstChip(theDetectorStructure);

    nRows = RD53Shared::firstChip->getNRows();
    nCols = RD53Shared::firstChip->getNCols();

    // #######################
    // # Retrieve parameters #
    // #######################
    nEvents               = this->findValueInSettings<double>(settingsMap, "nEvents");
    nSteps                = this->findValueInSettings<double>(settingsMap, "VCalHnsteps");
    startValue            = this->findValueInSettings<double>(settingsMap, "VCalHstart");
    stopValue             = this->findValueInSettings<double>(settingsMap, "VCalHstop");
    offset                = this->findValueInSettings<double>(settingsMap, "VCalMED");
    auto         frontEnd = RD53Shared::firstChip->getFEtype(nCols / 2, nCols / 2);
    const size_t ToTsize  = frontEnd->maxToTvalue + 1;
    const int    MAXCHI2  = 4; // @CONST@

    auto hOcc2D = CanvasContainer<TH2F>("Gain", "Gain", nSteps, startValue - offset, stopValue - offset, nEvents, 0, ToTsize);
    bookChipImplementer(theOutputFile, theDetectorStructure, Occupancy2D, hOcc2D, "#DeltaVCal", "ToT");

    auto hOcc3D = CanvasContainer<TH3F>("GainMap", "Gain map", nCols, 0, nCols, nRows, 0, nRows, nSteps, startValue - offset, stopValue - offset);
    bookChipImplementer(theOutputFile, theDetectorStructure, Occupancy3D, hOcc3D, "Column", "Row", "#DeltaVCal");

    auto hErrorReadOut2D = CanvasContainer<TH2F>("ReadoutErrors", "Readout errors", nCols, 0, nCols, nRows, 0, nRows);
    bookChipImplementer(theOutputFile, theDetectorStructure, ErrorReadOut2D, hErrorReadOut2D, "Columns", "Rows");

    auto hErrorFit2D = CanvasContainer<TH2F>("FitErrors", "Fit errors", nCols, 0, nCols, nRows, 0, nRows);
    bookChipImplementer(theOutputFile, theDetectorStructure, ErrorFit2D, hErrorFit2D, "Columns", "Rows");

    auto hInterceptHighQ1D = CanvasContainer<TH1F>("InterceptHighQ1D", "Intercept high Q 1D", NBINS_G, -INTERCEPT_HALFRANGE, INTERCEPT_HALFRANGE);
    bookChipImplementer(theOutputFile, theDetectorStructure, InterceptHighQ1D, hInterceptHighQ1D, "Intercept for high charge range (ToT)", "Entries");

    auto hSlopeHighQ1D = CanvasContainer<TH1F>("SlopeHighQ1D", "Slope high Q 1D", NBINS_G, 0, SLOPE_RANGE);
    bookChipImplementer(theOutputFile, theDetectorStructure, SlopeHighQ1D, hSlopeHighQ1D, "Slope for high charge range (ToT/VCal)", "Entries");

    auto hInterceptLowQ1D = CanvasContainer<TH1F>("InterceptLowQ1D", "Intercept low Q 1D", NBINS_G, -INTERCEPT_HALFRANGE, INTERCEPT_HALFRANGE);
    bookChipImplementer(theOutputFile, theDetectorStructure, InterceptLowQ1D, hInterceptLowQ1D, "Intercept for low charge range (ToT)", "Entries");

    auto hSlopeLowQ1D = CanvasContainer<TH1F>("SlopeLowQ1D", "Slope low Q 1D", NBINS_G, 0, SLOPE_RANGE);
    bookChipImplementer(theOutputFile, theDetectorStructure, SlopeLowQ1D, hSlopeLowQ1D, "Slope for low charge range (ToT/VCal)", "Entries");

    auto hChi2DoF1D = CanvasContainer<TH1F>("Chi2DoF1D", "Chi2/DoF", NBINS_G, 0, MAXCHI2);
    bookChipImplementer(theOutputFile, theDetectorStructure, Chi2DoF1D, hChi2DoF1D, "#chi^{2}/D.o.F.", "Entries");

    auto hInterceptHighQ2D = CanvasContainer<TH2F>("InterceptHighQ2D", "Intercept high Q map", nCols, 0, nCols, nRows, 0, nRows);
    bookChipImplementer(theOutputFile, theDetectorStructure, InterceptHighQ2D, hInterceptHighQ2D, "Column", "Row");

    auto hSlopeHighQ2D = CanvasContainer<TH2F>("SlopeHighQ2D", "Slope high Q map", nCols, 0, nCols, nRows, 0, nRows);
    bookChipImplementer(theOutputFile, theDetectorStructure, SlopeHighQ2D, hSlopeHighQ2D, "Column", "Row");

    auto hInterceptLowQ2D = CanvasContainer<TH2F>("InterceptLowQ2D", "Intercept low Q map", nCols, 0, nCols, nRows, 0, nRows);
    bookChipImplementer(theOutputFile, theDetectorStructure, InterceptLowQ2D, hInterceptLowQ2D, "Column", "Row");

    auto hSlopeLowQ2D = CanvasContainer<TH2F>("SlopeLowQ2D", "Slope low Q map", nCols, 0, nCols, nRows, 0, nRows);
    bookChipImplementer(theOutputFile, theDetectorStructure, SlopeLowQ2D, hSlopeLowQ2D, "Column", "Row");

    auto hChi2DoF2D = CanvasContainer<TH2F>("Chi2DoF2D", "Chi2/DoF map", nCols, 0, nCols, nRows, 0, nRows);
    bookChipImplementer(theOutputFile, theDetectorStructure, Chi2DoF2D, hChi2DoF2D, "Column", "Row");

    AreHistoBooked = true;
}

bool GainHistograms::fill(std::string& inputStream)
{
    ContainerSerialization theOccupancySerialization("GainOccupancy");
    ContainerSerialization theGainSerialization("GainGain");

    if(theOccupancySerialization.attachDeserializer(inputStream))
    {
        uint16_t              deltaVcal;
        DetectorDataContainer fDetectorData = theOccupancySerialization.deserializeChipContainer<OccupancyAndPh, OccupancyAndPh>(fDetectorContainer, deltaVcal);
        GainHistograms::fillOccupancy(fDetectorData, deltaVcal);
        return true;
    }
    if(theGainSerialization.attachDeserializer(inputStream))
    {
        DetectorDataContainer fDetectorData = theGainSerialization.deserializeChipContainer<GainFit, GainFit>(fDetectorContainer);
        GainHistograms::fillGain(fDetectorData);
        return true;
    }
    return false;
}

void GainHistograms::fillOccupancy(const DetectorDataContainer& OccupancyContainer, uint16_t DELTA_VCAL)
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

                    for(auto row = 0u; row < nRows; row++)
                        for(auto col = 0u; col < nCols; col++)
                        {
                            if(cChip->getChannel<OccupancyAndPh>(row, col).fStatus == RD53Shared::ISGOOD)
                            {
                                hOcc2D->Fill(DELTA_VCAL, cChip->getChannel<OccupancyAndPh>(row, col).fPh);
                                hOcc3D->SetBinContent(col + 1, row + 1, hOcc3D->GetZaxis()->FindBin(DELTA_VCAL), cChip->getChannel<OccupancyAndPh>(row, col).fPh);
                            }
                            if(cChip->getChannel<OccupancyAndPh>(row, col).readoutError == true) ErrorReadOut2DHist->Fill(col, row);
                        }
                }
}

void GainHistograms::fillGain(const DetectorDataContainer& GainContainer)
{
    for(const auto cBoard: GainContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    if(GainContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->hasChannelContainer() == false) continue;

                    auto* InterceptHighQ1DHist = InterceptHighQ1D.getObject(cBoard->getId())
                                                     ->getObject(cOpticalGroup->getId())
                                                     ->getObject(cHybrid->getId())
                                                     ->getObject(cChip->getId())
                                                     ->getSummary<CanvasContainer<TH1F>>()
                                                     .fTheHistogram;
                    auto* SlopeHighQ1DHist = SlopeHighQ1D.getObject(cBoard->getId())
                                                 ->getObject(cOpticalGroup->getId())
                                                 ->getObject(cHybrid->getId())
                                                 ->getObject(cChip->getId())
                                                 ->getSummary<CanvasContainer<TH1F>>()
                                                 .fTheHistogram;
                    auto* InterceptLowQ1DHist = InterceptLowQ1D.getObject(cBoard->getId())
                                                    ->getObject(cOpticalGroup->getId())
                                                    ->getObject(cHybrid->getId())
                                                    ->getObject(cChip->getId())
                                                    ->getSummary<CanvasContainer<TH1F>>()
                                                    .fTheHistogram;
                    auto* SlopeLowQ1DHist = SlopeLowQ1D.getObject(cBoard->getId())
                                                ->getObject(cOpticalGroup->getId())
                                                ->getObject(cHybrid->getId())
                                                ->getObject(cChip->getId())
                                                ->getSummary<CanvasContainer<TH1F>>()
                                                .fTheHistogram;
                    auto* Chi2DoF1DHist = Chi2DoF1D.getObject(cBoard->getId())
                                              ->getObject(cOpticalGroup->getId())
                                              ->getObject(cHybrid->getId())
                                              ->getObject(cChip->getId())
                                              ->getSummary<CanvasContainer<TH1F>>()
                                              .fTheHistogram;

                    auto* InterceptHighQ2DHist = InterceptHighQ2D.getObject(cBoard->getId())
                                                     ->getObject(cOpticalGroup->getId())
                                                     ->getObject(cHybrid->getId())
                                                     ->getObject(cChip->getId())
                                                     ->getSummary<CanvasContainer<TH2F>>()
                                                     .fTheHistogram;
                    auto* SlopeHighQ2DHist = SlopeHighQ2D.getObject(cBoard->getId())
                                                 ->getObject(cOpticalGroup->getId())
                                                 ->getObject(cHybrid->getId())
                                                 ->getObject(cChip->getId())
                                                 ->getSummary<CanvasContainer<TH2F>>()
                                                 .fTheHistogram;
                    auto* InterceptLowQ2DHist = InterceptLowQ2D.getObject(cBoard->getId())
                                                    ->getObject(cOpticalGroup->getId())
                                                    ->getObject(cHybrid->getId())
                                                    ->getObject(cChip->getId())
                                                    ->getSummary<CanvasContainer<TH2F>>()
                                                    .fTheHistogram;
                    auto* SlopeLowQ2DHist = SlopeLowQ2D.getObject(cBoard->getId())
                                                ->getObject(cOpticalGroup->getId())
                                                ->getObject(cHybrid->getId())
                                                ->getObject(cChip->getId())
                                                ->getSummary<CanvasContainer<TH2F>>()
                                                .fTheHistogram;

                    auto* Chi2DoF2DHist = Chi2DoF2D.getObject(cBoard->getId())
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

                    for(auto row = 0u; row < nRows; row++)
                        for(auto col = 0u; col < nCols; col++)
                            if(cChip->getChannel<GainFit>(row, col).fChi2 == RD53Shared::ISFITERROR)
                                ErrorFit2DHist->Fill(col, row);
                            else if(cChip->getChannel<GainFit>(row, col).fChi2 != 0)
                            {
                                // #################
                                // # 1D histograms #
                                // #################
                                InterceptHighQ1DHist->Fill(cChip->getChannel<GainFit>(row, col).fInterceptHighQ);
                                SlopeHighQ1DHist->Fill(cChip->getChannel<GainFit>(row, col).fSlopeHighQ);
                                InterceptLowQ1DHist->Fill(cChip->getChannel<GainFit>(row, col).fInterceptLowQ);
                                SlopeLowQ1DHist->Fill(cChip->getChannel<GainFit>(row, col).fSlopeLowQ);
                                Chi2DoF1DHist->Fill(cChip->getChannel<GainFit>(row, col).fChi2 / cChip->getChannel<GainFit>(row, col).fDoF);

                                // #################
                                // # 2D histograms #
                                // #################
                                SlopeHighQ2DHist->SetBinContent(col + 1, row + 1, cChip->getChannel<GainFit>(row, col).fSlopeHighQ);
                                SlopeHighQ2DHist->SetBinError(col + 1, row + 1, cChip->getChannel<GainFit>(row, col).fSlopeHighQError);
                                InterceptHighQ2DHist->SetBinContent(col + 1, row + 1, cChip->getChannel<GainFit>(row, col).fInterceptHighQ);
                                InterceptHighQ2DHist->SetBinError(col + 1, row + 1, cChip->getChannel<GainFit>(row, col).fInterceptHighQError);
                                InterceptLowQ2DHist->SetBinContent(col + 1, row + 1, cChip->getChannel<GainFit>(row, col).fInterceptLowQ);
                                InterceptLowQ2DHist->SetBinError(col + 1, row + 1, cChip->getChannel<GainFit>(row, col).fInterceptLowQError);
                                SlopeLowQ2DHist->SetBinContent(col + 1, row + 1, cChip->getChannel<GainFit>(row, col).fSlopeLowQ);
                                SlopeLowQ2DHist->SetBinError(col + 1, row + 1, cChip->getChannel<GainFit>(row, col).fSlopeLowQError);
                                Chi2DoF2DHist->SetBinContent(col + 1, row + 1, cChip->getChannel<GainFit>(row, col).fChi2 / cChip->getChannel<GainFit>(row, col).fDoF);
                            }
                }
}

void GainHistograms::process()
{
    drawChip<TH2F>(Occupancy2D, "gcolz", "electron", "Charge (electrons)");
    drawChip<TH3F>(Occupancy3D, "gcolz");
    drawChip<TH2F>(ErrorReadOut2D, "gcolz");
    drawChip<TH2F>(ErrorFit2D, "gcolz");

    drawChip<TH1F>(InterceptHighQ1D);
    drawChip<TH1F>(SlopeHighQ1D, "", "electron", "Slope for high charge range (ToT/electrons)");
    drawChip<TH1F>(InterceptLowQ1D);
    drawChip<TH1F>(SlopeLowQ1D, "", "electron", "Slope for low charge range (ToT/electrons)");
    drawChip<TH1F>(Chi2DoF1D);

    drawChip<TH2F>(InterceptHighQ2D, "gcolz");
    drawChip<TH2F>(SlopeHighQ2D, "gcolz");
    drawChip<TH2F>(InterceptLowQ2D, "gcolz");
    drawChip<TH2F>(SlopeLowQ2D, "gcolz");
    drawChip<TH2F>(Chi2DoF2D, "gcolz");
}
