/*!
  \file                  RD53BERtestHistograms.cc
  \brief                 Implementation of BERtest calibration histograms
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#include "RD53BERtestHistograms.h"
#include "Utils/ContainerSerialization.h"

using namespace Ph2_HwDescription;

void BERtestHistograms::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& settingsMap)
{
    RD53Shared::setFirstChip(theDetectorStructure);

    auto hBERtest = CanvasContainer<TH1F>("BERtest", "Bit Error Rate Test", 1, 0, 1);
    bookChipImplementer(theOutputFile, theDetectorStructure, BERtest, hBERtest, "N.A.", "Bit Error Rate value");

    AreHistoBooked = true;
}

bool BERtestHistograms::fill(std::string& inputStream)
{
    ContainerSerialization theContainerSerialization("BERtest");

    if(theContainerSerialization.attachDeserializer(inputStream))
    {
        DetectorDataContainer fDetectorData = theContainerSerialization.deserializeChipContainer<EmptyContainer, double>(fDetectorContainer);
        BERtestHistograms::fillBERtest(fDetectorData);
        return true;
    }
    return false;
}

void BERtestHistograms::fillBERtest(const DetectorDataContainer& BERtestContainer)
{
    for(const auto cBoard: BERtestContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    if(cChip->hasSummary() == false) continue;

                    auto* BERtestHist = BERtest.getObject(cBoard->getId())
                                            ->getObject(cOpticalGroup->getId())
                                            ->getObject(cHybrid->getId())
                                            ->getObject(cChip->getId())
                                            ->getSummary<CanvasContainer<TH1F>>()
                                            .fTheHistogram;

                    BERtestHist->SetBinContent(1, cChip->getSummary<double>());
                }
}

void BERtestHistograms::process() { drawChip<TH1F>(BERtest); }
