/*!
  \file                  RD53PixelAliveHistograms.cc
  \brief                 Implementation of PixelAlive calibration histograms
  \author                Alkiviadis PAPADOPOULOS
  \version               1.0
  \date                  28/06/18
  Support:               email to alkiviadis.papadopoulos@cern.ch
  Support:               email to mauro.dinardo@cern.ch
*/

#include "RD53PixelAliveHistograms.h"
#include "HWDescription/RD53A.h"
#include "HWDescription/RD53B.h"
#include "Utils/ContainerSerialization.h"

using namespace Ph2_HwDescription;

void PixelAliveHistograms::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& settingsMap)
{
    fDetectorContainer = &theDetectorStructure;
    RD53Shared::setFirstChip(theDetectorStructure);

    nRows = RD53Shared::firstChip->getNRows();
    nCols = RD53Shared::firstChip->getNCols();

    // #######################
    // # Retrieve parameters #
    // #######################
    nEvents                = this->findValueInSettings<double>(settingsMap, "nEvents");
    auto         frontEnd  = RD53Shared::firstChip->getFEtype(nCols / 2, nCols / 2);
    const size_t ToTsize   = frontEnd->maxToTvalue + 1;
    const size_t BCIDsize  = RD53Shared::firstChip->getMaxBCIDvalue() + 1;
    const size_t TrgIDsize = RD53Shared::firstChip->getMaxTRIGIDvalue() + 1;

    auto hOcc1D = CanvasContainer<TH1F>("Occ1D", "Occ1D", nEvents + 1, 0, 1 + 1. / nEvents);
    bookChipImplementer(theOutputFile, theDetectorStructure, Occupancy1D, hOcc1D, "Efficiency", "Entries");

    auto hOcc2D = CanvasContainer<TH2F>("PixelAlive", "Pixel Alive", nCols, 0, nCols, nRows, 0, nRows);
    bookChipImplementer(theOutputFile, theDetectorStructure, Occupancy2D, hOcc2D, "Columns", "Rows");

    auto hErrorReadOut2D = CanvasContainer<TH2F>("ReadoutErrors", "Readout arrors", nCols, 0, nCols, nRows, 0, nRows);
    bookChipImplementer(theOutputFile, theDetectorStructure, ErrorReadOut2D, hErrorReadOut2D, "Columns", "Rows");

    auto hMask1Dcol = CanvasContainer<TH1F>("Masked1Dcol", "Masked pixels projection", nCols, 0, nCols);
    bookChipImplementer(theOutputFile, theDetectorStructure, Mask1Dcol, hMask1Dcol, "Columns", "Entries");

    auto hMask1Drow = CanvasContainer<TH1F>("Masked1Drow", "Masked pixels projection", nRows, 0, nRows);
    bookChipImplementer(theOutputFile, theDetectorStructure, Mask1Drow, hMask1Drow, "Rows", "Entries");

    auto hToT1D = CanvasContainer<TH1F>("ToT1D", "<ToT> Distribution", ToTsize, 0, ToTsize);
    bookChipImplementer(theOutputFile, theDetectorStructure, ToT1D, hToT1D, "ToT", "Entries");

    auto hToT2D = CanvasContainer<TH2F>("ToT2D", "<ToT> map", nCols, 0, nCols, nRows, 0, nRows);
    bookChipImplementer(theOutputFile, theDetectorStructure, ToT2D, hToT2D, "Columns", "Rows");

    auto hBCID = CanvasContainer<TH1F>("BCID", "BCID", BCIDsize, 1, BCIDsize + 1);
    bookChipImplementer(theOutputFile, theDetectorStructure, BCID, hBCID, "#DeltaBCID", "Entries");

    auto hTriggerID = CanvasContainer<TH1F>("TriggerID", "TriggerID", TrgIDsize, 1, TrgIDsize + 1);
    bookChipImplementer(theOutputFile, theDetectorStructure, TriggerID, hTriggerID, "#DeltaTrigger-ID", "Entries");

    auto hMasked2D = CanvasContainer<TH2F>("Masked2D", "Masked pixels", nCols, 0, nCols, nRows, 0, nRows);
    bookChipImplementer(theOutputFile, theDetectorStructure, Masked2D, hMasked2D, "Columns", "Rows");

    AreHistoBooked = true;
}

bool PixelAliveHistograms::fill(std::string& inputStream)
{
    ContainerSerialization theOccupancySerialization("PixelAliveOccupancy");
    ContainerSerialization theBCIDSerialization("PixelAliveBCID");
    ContainerSerialization theTrgIDSerialization("PixelAliveTrgID");

    if(theOccupancySerialization.attachDeserializer(inputStream))
    {
        DetectorDataContainer fDetectorData = theOccupancySerialization.deserializeChipContainer<OccupancyAndPh, OccupancyAndPh>(fDetectorContainer);
        PixelAliveHistograms::fill(fDetectorData);
        return true;
    }
    if(theBCIDSerialization.attachDeserializer(inputStream))
    {
        DetectorDataContainer fDetectorData = theBCIDSerialization.deserializeChipContainer<EmptyContainer, std::vector<uint16_t>>(fDetectorContainer);
        PixelAliveHistograms::fillBCID(fDetectorData);
        return true;
    }
    if(theTrgIDSerialization.attachDeserializer(inputStream))
    {
        DetectorDataContainer fDetectorData = theTrgIDSerialization.deserializeChipContainer<EmptyContainer, std::vector<uint16_t>>(fDetectorContainer);
        PixelAliveHistograms::fillTrgID(fDetectorData);
        return true;
    }
    return false;
}

void PixelAliveHistograms::fill(const DetectorDataContainer& DataContainer)
{
    for(const auto cBoard: DataContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    if(DataContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getChannelContainer<OccupancyAndPh>() ==
                       nullptr)
                        continue;

                    auto* Occupancy1DHist = Occupancy1D.getObject(cBoard->getId())
                                                ->getObject(cOpticalGroup->getId())
                                                ->getObject(cHybrid->getId())
                                                ->getObject(cChip->getId())
                                                ->getSummary<CanvasContainer<TH1F>>()
                                                .fTheHistogram;
                    auto* Occupancy2DHist = Occupancy2D.getObject(cBoard->getId())
                                                ->getObject(cOpticalGroup->getId())
                                                ->getObject(cHybrid->getId())
                                                ->getObject(cChip->getId())
                                                ->getSummary<CanvasContainer<TH2F>>()
                                                .fTheHistogram;
                    auto* ErrorReadOut2DHist = ErrorReadOut2D.getObject(cBoard->getId())
                                                   ->getObject(cOpticalGroup->getId())
                                                   ->getObject(cHybrid->getId())
                                                   ->getObject(cChip->getId())
                                                   ->getSummary<CanvasContainer<TH2F>>()
                                                   .fTheHistogram;
                    auto* ToT1DHist =
                        ToT1D.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<CanvasContainer<TH1F>>().fTheHistogram;
                    auto* ToT2DHist =
                        ToT2D.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<CanvasContainer<TH2F>>().fTheHistogram;
                    auto* Mask1DcolHist = Mask1Dcol.getObject(cBoard->getId())
                                              ->getObject(cOpticalGroup->getId())
                                              ->getObject(cHybrid->getId())
                                              ->getObject(cChip->getId())
                                              ->getSummary<CanvasContainer<TH1F>>()
                                              .fTheHistogram;
                    auto* Mask1DrowHist = Mask1Drow.getObject(cBoard->getId())
                                              ->getObject(cOpticalGroup->getId())
                                              ->getObject(cHybrid->getId())
                                              ->getObject(cChip->getId())
                                              ->getSummary<CanvasContainer<TH1F>>()
                                              .fTheHistogram;
                    auto* Masked2DHist = Masked2D.getObject(cBoard->getId())
                                             ->getObject(cOpticalGroup->getId())
                                             ->getObject(cHybrid->getId())
                                             ->getObject(cChip->getId())
                                             ->getSummary<CanvasContainer<TH2F>>()
                                             .fTheHistogram;

                    for(auto row = 0u; row < nRows; row++)
                        for(auto col = 0u; col < nCols; col++)
                        {
                            if(cChip->getChannel<OccupancyAndPh>(row, col).fOccupancy > 0)
                            {
                                Occupancy1DHist->Fill(cChip->getChannel<OccupancyAndPh>(row, col).fOccupancy + Occupancy1DHist->GetBinWidth(1) / 2);
                                Occupancy2DHist->SetBinContent(col + 1, row + 1, cChip->getChannel<OccupancyAndPh>(row, col).fOccupancy);
                                ToT1DHist->Fill(cChip->getChannel<OccupancyAndPh>(row, col).fPh);
                                ToT2DHist->SetBinContent(col + 1, row + 1, ToT2DHist->GetBinContent(col + 1, row + 1) + cChip->getChannel<OccupancyAndPh>(row, col).fPh);
                                ToT2DHist->SetBinError(col + 1,
                                                       row + 1,
                                                       sqrt(ToT2DHist->GetBinError(col + 1, row + 1) * ToT2DHist->GetBinError(col + 1, row + 1) +
                                                            cChip->getChannel<OccupancyAndPh>(row, col).fPhError * cChip->getChannel<OccupancyAndPh>(row, col).fPhError));
                            }

                            if(cChip->getChannel<OccupancyAndPh>(row, col).fStatus == RD53Shared::ISMASKED)
                            {
                                Mask1DrowHist->Fill(row);
                                Masked2DHist->Fill(col, row);
                            }

                            if(cChip->getChannel<OccupancyAndPh>(row, col).readoutError == true) ErrorReadOut2DHist->Fill(col, row);
                        }

                    for(auto col = 0u; col < nCols; col++)
                        for(auto row = 0u; row < nRows; row++)
                            if(cChip->getChannel<OccupancyAndPh>(row, col).fStatus == RD53Shared::ISMASKED) Mask1DcolHist->Fill(col);
                }
}

void PixelAliveHistograms::fillBCID(const DetectorDataContainer& DataContainer)
{
    for(const auto cBoard: DataContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    if(cChip->hasSummary() == false) continue;

                    auto* BCIDHist =
                        BCID.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<CanvasContainer<TH1F>>().fTheHistogram;

                    for(auto i = 0u; i < cChip->getSummary<std::vector<uint16_t>>().size(); i++)
                    {
                        auto bin = (i == 0 ? BCIDHist->GetNbinsX() : i);
                        BCIDHist->SetBinContent(bin, BCIDHist->GetBinContent(bin) + cChip->getSummary<std::vector<uint16_t>>().at(i));
                    }
                }
}

void PixelAliveHistograms::fillTrgID(const DetectorDataContainer& DataContainer)
{
    for(const auto cBoard: DataContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    if(cChip->hasSummary() == false) continue;

                    auto* TriggerIDHist = TriggerID.getObject(cBoard->getId())
                                              ->getObject(cOpticalGroup->getId())
                                              ->getObject(cHybrid->getId())
                                              ->getObject(cChip->getId())
                                              ->getSummary<CanvasContainer<TH1F>>()
                                              .fTheHistogram;

                    for(auto i = 0u; i < cChip->getSummary<std::vector<uint16_t>>().size(); i++)
                    {
                        auto bin = (i == 0 ? TriggerIDHist->GetNbinsX() : i);
                        TriggerIDHist->SetBinContent(bin, TriggerIDHist->GetBinContent(bin) + cChip->getSummary<std::vector<uint16_t>>().at(i));
                    }
                }
}

void PixelAliveHistograms::process()
{
    drawChip<TH1F>(Occupancy1D);
    drawChip<TH2F>(Occupancy2D, "gcolz");
    drawChip<TH2F>(ErrorReadOut2D, "gcolz");
    drawChip<TH1F>(Mask1Dcol);
    drawChip<TH1F>(Mask1Drow);
    drawChip<TH1F>(ToT1D);
    drawChip<TH2F>(ToT2D, "gcolz");
    drawChip<TH1F>(BCID);
    drawChip<TH1F>(TriggerID);
    drawChip<TH2F>(Masked2D, "gcolz");
}
