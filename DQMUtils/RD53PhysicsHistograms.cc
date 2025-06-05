/*!
  \file                  RD53PhysicsHistograms.cc
  \brief                 Implementation of Physics histograms
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#include "RD53PhysicsHistograms.h"
#include "HWDescription/RD53A.h"
#include "HWDescription/RD53B.h"
#include "Utils/ContainerSerialization.h"

using namespace Ph2_HwDescription;

void PhysicsHistograms::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& settingsMap)
{
    fDetectorContainer = &theDetectorStructure;
    RD53Shared::setFirstChip(theDetectorStructure);

    nRows = RD53Shared::firstChip->getNRows();
    nCols = RD53Shared::firstChip->getNCols();

    // #######################
    // # Retrieve parameters #
    // #######################
    auto         frontEnd  = RD53Shared::firstChip->getFEtype(nCols / 2, nCols / 2);
    const size_t ToTsize   = frontEnd->maxToTvalue + 1;
    const size_t BCIDsize  = RD53Shared::firstChip->getMaxBCIDvalue() + 1;
    const size_t TrgIDsize = RD53Shared::firstChip->getMaxTRIGIDvalue() + 1;

    auto hToT1D = CanvasContainer<TH1F>("ToT1D", "<ToT> distribution", ToTsize, 0, ToTsize);
    bookChipImplementer(theOutputFile, theDetectorStructure, ToT1D, hToT1D, "ToT", "Entries");

    auto hToT2D = CanvasContainer<TH2F>("ToT2D", "<ToT> map", nCols, 0, nCols, nRows, 0, nRows);
    bookChipImplementer(theOutputFile, theDetectorStructure, ToT2D, hToT2D, "Columns", "Rows");

    auto hOcc2D = CanvasContainer<TH2F>("Occ2D", "Occupancy", nCols, 0, nCols, nRows, 0, nRows);
    bookChipImplementer(theOutputFile, theDetectorStructure, Occupancy2D, hOcc2D, "Columns", "Rows");

    auto hErrorReadOut2D = CanvasContainer<TH2F>("ReadoutErrors", "Readout errors", nCols, 0, nCols, nRows, 0, nRows);
    bookChipImplementer(theOutputFile, theDetectorStructure, ErrorReadOut2D, hErrorReadOut2D, "Columns", "Rows");

    auto hBCID = CanvasContainer<TH1F>("BCID", "BCID", BCIDsize, 1, BCIDsize + 1);
    bookChipImplementer(theOutputFile, theDetectorStructure, BCID, hBCID, "#DeltaBCID", "Entries");

    auto hTriggerID = CanvasContainer<TH1F>("TriggerID", "TriggerID", TrgIDsize, 1, TrgIDsize + 1);
    bookChipImplementer(theOutputFile, theDetectorStructure, TriggerID, hTriggerID, "#DeltaTrigger-ID", "Entries");

    AreHistoBooked = true;
}

bool PhysicsHistograms::fill(std::string& inputStream)
{
    ContainerSerialization theOccupancySerialization("PhysicsOccupancy");
    ContainerSerialization theBCIDSerialization("PhysicsBCID");
    ContainerSerialization theTrgIDSerialization("PhysicsTrgID");

    if(theOccupancySerialization.attachDeserializer(inputStream))
    {
        DetectorDataContainer fDetectorData = theOccupancySerialization.deserializeChipContainer<OccupancyAndPh, OccupancyAndPh>(fDetectorContainer);
        PhysicsHistograms::fill(fDetectorData);
        return true;
    }
    if(theBCIDSerialization.attachDeserializer(inputStream))
    {
        DetectorDataContainer fDetectorData = theBCIDSerialization.deserializeChipContainer<EmptyContainer, std::vector<uint16_t>>(fDetectorContainer);
        PhysicsHistograms::fillBCID(fDetectorData);
        return true;
    }
    if(theTrgIDSerialization.attachDeserializer(inputStream))
    {
        DetectorDataContainer fDetectorData = theTrgIDSerialization.deserializeChipContainer<EmptyContainer, std::vector<uint16_t>>(fDetectorContainer);
        PhysicsHistograms::fillTrgID(fDetectorData);
        return true;
    }
    return false;
}

void PhysicsHistograms::fill(const DetectorDataContainer& DataContainer)
{
    for(const auto cBoard: DataContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    if(DataContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->hasChannelContainer() == false) continue;

                    auto* ToT1DHist =
                        ToT1D.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<CanvasContainer<TH1F>>().fTheHistogram;
                    auto* ToT2DHist =
                        ToT2D.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<CanvasContainer<TH2F>>().fTheHistogram;
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

                    for(auto row = 0u; row < nRows; row++)
                        for(auto col = 0u; col < nCols; col++)
                        {
                            if(cChip->getChannel<OccupancyAndPh>(row, col).fOccupancy != 0)
                            {
                                Occupancy2DHist->SetBinContent(col + 1, row + 1, Occupancy2DHist->GetBinContent(col + 1, row + 1) + cChip->getChannel<OccupancyAndPh>(row, col).fOccupancy);
                                ToT1DHist->Fill(cChip->getChannel<OccupancyAndPh>(row, col).fPh);
                                ToT2DHist->SetBinContent(col + 1, row + 1, ToT2DHist->GetBinContent(col + 1, row + 1) + cChip->getChannel<OccupancyAndPh>(row, col).fPh);
                                ToT2DHist->SetBinError(col + 1, row + 1, ToT2DHist->GetBinContent(col + 1, row + 1) + cChip->getChannel<OccupancyAndPh>(row, col).fPhError);
                            }

                            if(cChip->getChannel<OccupancyAndPh>(row, col).readoutError == true) ErrorReadOut2DHist->Fill(col, row);
                        }
                }
}

void PhysicsHistograms::fillBCID(const DetectorDataContainer& DataContainer)
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

void PhysicsHistograms::fillTrgID(const DetectorDataContainer& DataContainer)
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

void PhysicsHistograms::process()
{
    drawChip<TH1F>(ToT1D);
    drawChip<TH2F>(ToT2D, "gcolz");
    drawChip<TH2F>(Occupancy2D, "gcolz");
    drawChip<TH2F>(ErrorReadOut2D, "gcolz");
    drawChip<TH1F>(BCID);
    drawChip<TH1F>(TriggerID);
}
