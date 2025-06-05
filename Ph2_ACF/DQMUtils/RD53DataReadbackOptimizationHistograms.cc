/*!
  \file                  RD53DataReadbackOptimizationHistograms.cc
  \brief                 Implementation of data readback optimization histograms
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#include "RD53DataReadbackOptimizationHistograms.h"
#include "Utils/ContainerSerialization.h"

using namespace Ph2_HwDescription;

void DataReadbackOptimizationHistograms::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& settingsMap)
{
    fDetectorContainer = &theDetectorStructure;
    RD53Shared::setFirstChip(theDetectorStructure);

    // #######################
    // # Retrieve parameters #
    // #######################
    startValueTAP0 = this->findValueInSettings<double>(settingsMap, "TAP0Start");
    stopValueTAP0  = this->findValueInSettings<double>(settingsMap, "TAP0Stop");
    startValueTAP1 = this->findValueInSettings<double>(settingsMap, "TAP1Start");
    stopValueTAP1  = this->findValueInSettings<double>(settingsMap, "TAP1Stop");
    startValueTAP2 = this->findValueInSettings<double>(settingsMap, "TAP2Start");
    stopValueTAP2  = this->findValueInSettings<double>(settingsMap, "TAP2Stop");

    size_t nSteps    = (stopValueTAP0 - startValueTAP0 + 1 >= RD53Shared::MAXSTEPS ? RD53Shared::MAXSTEPS : stopValueTAP0 - startValueTAP0 + 1);
    auto   hTAP0scan = CanvasContainer<TH1F>("TAP0scan", "TAP0 scan", nSteps, startValueTAP0, stopValueTAP0 + 1);
    bookChipImplementer(theOutputFile, theDetectorStructure, TAP0scan, hTAP0scan, "TAP0 - driver", "Bit Error Rate (frames-with-err / frames)");
    auto hTAP0 = CanvasContainer<TH1F>("TAP0", "TAP0 - driver", stopValueTAP0 - startValueTAP0 + 1, startValueTAP0, stopValueTAP0 + 1);
    bookChipImplementer(theOutputFile, theDetectorStructure, TAP0, hTAP0, "TAP0 - driver", "Entries");

    nSteps         = (stopValueTAP1 - startValueTAP1 + 1 >= RD53Shared::MAXSTEPS ? RD53Shared::MAXSTEPS : stopValueTAP1 - startValueTAP1 + 1);
    auto hTAP1scan = CanvasContainer<TH1F>("TAP1scan", "TAP1 scan", nSteps, startValueTAP1, stopValueTAP1 + 1);
    bookChipImplementer(theOutputFile, theDetectorStructure, TAP1scan, hTAP1scan, "TAP1 - pre-emphasis-1", "Bit Error Rate (frames-with-err / frames)");
    auto hTAP1 = CanvasContainer<TH1F>("TAP1", "TAP1 - pre-emphasis-1", stopValueTAP1 - startValueTAP1 + 1, startValueTAP1, stopValueTAP1 + 1);
    bookChipImplementer(theOutputFile, theDetectorStructure, TAP1, hTAP1, "TAP1 - pre-emphasis-1", "Entries");

    nSteps         = (stopValueTAP2 - startValueTAP2 + 1 >= RD53Shared::MAXSTEPS ? RD53Shared::MAXSTEPS : stopValueTAP2 - startValueTAP2 + 1);
    auto hTAP2scan = CanvasContainer<TH1F>("TAP2scan", "TAP2 scan", nSteps, startValueTAP2, stopValueTAP2 + 1);
    bookChipImplementer(theOutputFile, theDetectorStructure, TAP2scan, hTAP2scan, "TAP2 - pre-emphasis-2", "Bit Error Rate (frames-with-err / frames)");
    auto hTAP2 = CanvasContainer<TH1F>("TAP2", "TAP2 - pre-emphasis-2", stopValueTAP2 - startValueTAP2 + 1, startValueTAP2, stopValueTAP2 + 1);
    bookChipImplementer(theOutputFile, theDetectorStructure, TAP2, hTAP2, "TAP2 - pre-emphasis-2", "Entries");

    AreHistoBooked = true;
}

bool DataReadbackOptimizationHistograms::fill(std::string& inputStream)
{
    ContainerSerialization theTAP0scanSerialization("DataReadbackOptimizationTAP0scan");
    ContainerSerialization theTAP0Serialization("DataReadbackOptimizationTAP0");

    ContainerSerialization theTAP1scanSerialization("DataReadbackOptimizationTAP1scan");
    ContainerSerialization theTAP1Serialization("DataReadbackOptimizationTAP1");

    ContainerSerialization theTAP2scanSerialization("DataReadbackOptimizationTAP2scan");
    ContainerSerialization theTAP2Serialization("DataReadbackOptimizationTAP2");

    if(theTAP0scanSerialization.attachDeserializer(inputStream))
    {
        DetectorDataContainer fDetectorData = theTAP0scanSerialization.deserializeChipContainer<EmptyContainer, std::vector<double>>(fDetectorContainer);
        DataReadbackOptimizationHistograms::fillScanTAP0(fDetectorData);
        return true;
    }
    if(theTAP0Serialization.attachDeserializer(inputStream))
    {
        DetectorDataContainer fDetectorData = theTAP0Serialization.deserializeChipContainer<EmptyContainer, uint16_t>(fDetectorContainer);
        DataReadbackOptimizationHistograms::fillTAP0(fDetectorData);
        return true;
    }
    if(theTAP1scanSerialization.attachDeserializer(inputStream))
    {
        DetectorDataContainer fDetectorData = theTAP1scanSerialization.deserializeChipContainer<EmptyContainer, std::vector<double>>(fDetectorContainer);
        DataReadbackOptimizationHistograms::fillScanTAP1(fDetectorData);
        return true;
    }
    if(theTAP1Serialization.attachDeserializer(inputStream))
    {
        DetectorDataContainer fDetectorData = theTAP1Serialization.deserializeChipContainer<EmptyContainer, uint16_t>(fDetectorContainer);
        DataReadbackOptimizationHistograms::fillTAP1(fDetectorData);
        return true;
    }
    if(theTAP2scanSerialization.attachDeserializer(inputStream))
    {
        DetectorDataContainer fDetectorData = theTAP2scanSerialization.deserializeChipContainer<EmptyContainer, std::vector<double>>(fDetectorContainer);
        DataReadbackOptimizationHistograms::fillScanTAP2(fDetectorData);
        return true;
    }
    if(theTAP2Serialization.attachDeserializer(inputStream))
    {
        DetectorDataContainer fDetectorData = theTAP2Serialization.deserializeChipContainer<EmptyContainer, uint16_t>(fDetectorContainer);
        DataReadbackOptimizationHistograms::fillTAP2(fDetectorData);
        return true;
    }
    return false;
}

void DataReadbackOptimizationHistograms::fillScanTAP0(const DetectorDataContainer& TAP0scanContainer)
{
    for(const auto cBoard: TAP0scanContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    if(cChip->hasSummary() == false) continue;

                    auto* TAP0scanHist = TAP0scan.getObject(cBoard->getId())
                                             ->getObject(cOpticalGroup->getId())
                                             ->getObject(cHybrid->getId())
                                             ->getObject(cChip->getId())
                                             ->getSummary<CanvasContainer<TH1F>>()
                                             .fTheHistogram;

                    for(auto i = 0; i < TAP0scanHist->GetNbinsX(); i++) TAP0scanHist->SetBinContent(i + 1, cChip->getSummary<std::vector<double>>().at(i));
                }
}

void DataReadbackOptimizationHistograms::fillTAP0(const DetectorDataContainer& TAP0Container)
{
    for(const auto cBoard: TAP0Container)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    if(cChip->hasSummary() == false) continue;

                    auto* TAP0Hist =
                        TAP0.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<CanvasContainer<TH1F>>().fTheHistogram;

                    TAP0Hist->Fill(cChip->getSummary<uint16_t>());
                }
}

void DataReadbackOptimizationHistograms::fillScanTAP1(const DetectorDataContainer& TAP1scanContainer)
{
    for(const auto cBoard: TAP1scanContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    if(cChip->hasSummary() == false) continue;

                    auto* TAP1scanHist = TAP1scan.getObject(cBoard->getId())
                                             ->getObject(cOpticalGroup->getId())
                                             ->getObject(cHybrid->getId())
                                             ->getObject(cChip->getId())
                                             ->getSummary<CanvasContainer<TH1F>>()
                                             .fTheHistogram;

                    for(auto i = 0; i < TAP1scanHist->GetNbinsX(); i++) TAP1scanHist->SetBinContent(i + 1, cChip->getSummary<std::vector<double>>().at(i));
                }
}

void DataReadbackOptimizationHistograms::fillTAP1(const DetectorDataContainer& TAP1Container)
{
    for(const auto cBoard: TAP1Container)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    if(cChip->hasSummary() == false) continue;

                    auto* TAP1Hist =
                        TAP1.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<CanvasContainer<TH1F>>().fTheHistogram;

                    TAP1Hist->Fill(cChip->getSummary<uint16_t>());
                }
}

void DataReadbackOptimizationHistograms::fillScanTAP2(const DetectorDataContainer& TAP2scanContainer)
{
    for(const auto cBoard: TAP2scanContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    if(cChip->hasSummary() == false) continue;

                    auto* TAP2scanHist = TAP2scan.getObject(cBoard->getId())
                                             ->getObject(cOpticalGroup->getId())
                                             ->getObject(cHybrid->getId())
                                             ->getObject(cChip->getId())
                                             ->getSummary<CanvasContainer<TH1F>>()
                                             .fTheHistogram;

                    for(auto i = 0; i < TAP2scanHist->GetNbinsX(); i++) TAP2scanHist->SetBinContent(i + 1, cChip->getSummary<std::vector<double>>().at(i));
                }
}

void DataReadbackOptimizationHistograms::fillTAP2(const DetectorDataContainer& TAP2Container)
{
    for(const auto cBoard: TAP2Container)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    if(cChip->hasSummary() == false) continue;

                    auto* TAP2Hist =
                        TAP2.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<CanvasContainer<TH1F>>().fTheHistogram;

                    TAP2Hist->Fill(cChip->getSummary<uint16_t>());
                }
}

void DataReadbackOptimizationHistograms::process()
{
    drawChip<TH1F>(TAP0scan);
    drawChip<TH1F>(TAP0);
    drawChip<TH1F>(TAP1scan);
    drawChip<TH1F>(TAP1);
    drawChip<TH1F>(TAP2scan);
    drawChip<TH1F>(TAP2);
}
