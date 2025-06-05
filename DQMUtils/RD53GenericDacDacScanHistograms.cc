/*!
  \file                  RD53GenericDacDacScanHistograms.h
  \brief                 Implementation of generic DAC DAC scan histograms
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/05/21
  Support:               email to mauro.dinardo@cern.ch
*/

#include "RD53GenericDacDacScanHistograms.h"
#include "Utils/ContainerSerialization.h"

using namespace Ph2_HwDescription;

void GenericDacDacScanHistograms::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& settingsMap)
{
    fDetectorContainer = &theDetectorStructure;
    RD53Shared::setFirstChip(theDetectorStructure);

    // #######################
    // # Retrieve parameters #
    // #######################
    regNameDAC1    = this->findValueInSettings<std::string>(settingsMap, "RegNameDAC1");
    startValueDAC1 = this->findValueInSettings<double>(settingsMap, "StartValueDAC1");
    stopValueDAC1  = this->findValueInSettings<double>(settingsMap, "StopValueDAC1");
    stepDAC1       = this->findValueInSettings<double>(settingsMap, "StepDAC1");
    regNameDAC2    = this->findValueInSettings<std::string>(settingsMap, "RegNameDAC2");
    startValueDAC2 = this->findValueInSettings<double>(settingsMap, "StartValueDAC2");
    stopValueDAC2  = this->findValueInSettings<double>(settingsMap, "StopValueDAC2");
    stepDAC2       = this->findValueInSettings<double>(settingsMap, "StepDAC2");

    // #####################################
    // # Make proper axis for secial cases #
    // #####################################
    size_t            startValueX = startValueDAC1;
    size_t            stopValueX  = stopValueDAC1 + stepDAC1;
    size_t            startValueY = startValueDAC2;
    size_t            stopValueY  = stopValueDAC2 + stepDAC2;
    std::stringstream titleX("");
    std::stringstream titleY("");

    if(regNameDAC1.find("CAL_EDGE_FINE_DELAY") != std::string::npos)
    {
        const auto frontEnd = RD53Shared::firstChip->getFEtype(RD53Shared::firstChip->getNRows() / 2, RD53Shared::firstChip->getNCols() / 2);
        const auto unitTime =
            1. / RD53Constants::ACCELERATOR_CLK * 1000 / ((RD53Shared::setBits(RD53Shared::firstChip->getNumberOfBits("CAL_EDGE_FINE_DELAY")) + 1) / (2. / frontEnd->nLatencyBins2Span));
        titleX << "Injection Delay (" << unitTime << " ns)";
    }
    else if(regNameDAC2.find("CAL_EDGE_FINE_DELAY") != std::string::npos)
    {
        const auto frontEnd = RD53Shared::firstChip->getFEtype(RD53Shared::firstChip->getNRows() / 2, RD53Shared::firstChip->getNCols() / 2);
        const auto unitTime =
            1. / RD53Constants::ACCELERATOR_CLK * 1000 / ((RD53Shared::setBits(RD53Shared::firstChip->getNumberOfBits("CAL_EDGE_FINE_DELAY")) + 1) / (2. / frontEnd->nLatencyBins2Span));
        titleY << "Injection Delay (" << unitTime << " ns)";
    }

    if(regNameDAC1.find("VCAL") != std::string::npos)
    {
        const auto& regMap = RD53Shared::firstChip->getRegMap();
        titleX << "#DeltaVCal";
        startValueX -= regMap.find("VCAL_MED")->second.fValue;
        stopValueX -= regMap.find("VCAL_MED")->second.fValue;
    }
    else if(regNameDAC2.find("VCAL") != std::string::npos)
    {
        const auto& regMap = RD53Shared::firstChip->getRegMap();
        titleY << "#DeltaVCal";
        startValueY -= regMap.find("VCAL_MED")->second.fValue;
        stopValueY -= regMap.find("VCAL_MED")->second.fValue;
    }

    if(titleX.str() == "") titleX << regNameDAC1;
    if(titleY.str() == "") titleY << regNameDAC2;

    auto hGenericDac1 = CanvasContainer<TH1F>("GenericDac1", "Generic Dac1", (stopValueDAC1 - startValueDAC1) / stepDAC1 + 1, startValueDAC1, stopValueDAC1 + stepDAC1);
    bookChipImplementer(theOutputFile, theDetectorStructure, GenericDac1, hGenericDac1, regNameDAC1.c_str(), "Entries");

    auto hGenericDac2 = CanvasContainer<TH1F>("GenericDac2", "Generic Dac2", (stopValueDAC2 - startValueDAC2) / stepDAC2 + 1, startValueDAC2, stopValueDAC2 + stepDAC2);
    bookChipImplementer(theOutputFile, theDetectorStructure, GenericDac2, hGenericDac2, regNameDAC2.c_str(), "Entries");

    auto hOcc2D = CanvasContainer<TH2F>("GenericDacDacScanScan",
                                        "Generic DAC-DAC Scan",
                                        (stopValueDAC1 - startValueDAC1) / stepDAC1 + 1,
                                        startValueX,
                                        stopValueX,
                                        (stopValueDAC2 - startValueDAC2) / stepDAC2 + 1,
                                        startValueY,
                                        stopValueY);
    bookChipImplementer(theOutputFile, theDetectorStructure, Occupancy2D, hOcc2D, titleX.str().c_str(), titleY.str().c_str());

    AreHistoBooked = true;
}

bool GenericDacDacScanHistograms::fill(std::string& inputStream)
{
    ContainerSerialization theOccupancySerialization("GenericDacDacScanOccupancy");
    ContainerSerialization theDACDACSerialization("GenericDacDacDACDAC");

    if(theOccupancySerialization.attachDeserializer(inputStream))
    {
        DetectorDataContainer fDetectorData = theOccupancySerialization.deserializeChipContainer<EmptyContainer, std::vector<float>>(fDetectorContainer);
        GenericDacDacScanHistograms::fillOccupancy(fDetectorData);
        return true;
    }
    if(theDACDACSerialization.attachDeserializer(inputStream))
    {
        DetectorDataContainer fDetectorData = theDACDACSerialization.deserializeChipContainer<EmptyContainer, std::pair<uint16_t, uint16_t>>(fDetectorContainer);
        GenericDacDacScanHistograms::fillGenericDacDac(fDetectorData);
        return true;
    }
    return false;
}

void GenericDacDacScanHistograms::fillOccupancy(const DetectorDataContainer& OccupancyContainer)
{
    for(const auto cBoard: OccupancyContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    if(cChip->hasSummary() == false) continue;

                    auto* Occupancy2DHist = Occupancy2D.getObject(cBoard->getId())
                                                ->getObject(cOpticalGroup->getId())
                                                ->getObject(cHybrid->getId())
                                                ->getObject(cChip->getId())
                                                ->getSummary<CanvasContainer<TH2F>>()
                                                .fTheHistogram;

                    for(auto i = 0; i < Occupancy2DHist->GetNbinsX(); i++)
                        for(auto j = 0; j < Occupancy2DHist->GetNbinsY(); j++)
                            Occupancy2DHist->SetBinContent(i + 1, j + 1, cChip->getSummary<std::vector<float>>().at(i * Occupancy2DHist->GetNbinsY() + j));
                }
}

void GenericDacDacScanHistograms::fillGenericDacDac(const DetectorDataContainer& GenericDacDacContainer)
{
    for(const auto cBoard: GenericDacDacContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    if(cChip->hasSummary() == false) continue;

                    auto* GenericDac1Hist = GenericDac1.getObject(cBoard->getId())
                                                ->getObject(cOpticalGroup->getId())
                                                ->getObject(cHybrid->getId())
                                                ->getObject(cChip->getId())
                                                ->getSummary<CanvasContainer<TH1F>>()
                                                .fTheHistogram;
                    auto* GenericDac2Hist = GenericDac2.getObject(cBoard->getId())
                                                ->getObject(cOpticalGroup->getId())
                                                ->getObject(cHybrid->getId())
                                                ->getObject(cChip->getId())
                                                ->getSummary<CanvasContainer<TH1F>>()
                                                .fTheHistogram;

                    GenericDac1Hist->Fill(cChip->getSummary<std::pair<uint16_t, uint16_t>>().first);
                    GenericDac2Hist->Fill(cChip->getSummary<std::pair<uint16_t, uint16_t>>().second);
                }
}

void GenericDacDacScanHistograms::process()
{
    drawChip<TH2F>(Occupancy2D, "gcolz");
    drawChip<TH1F>(GenericDac1);
    drawChip<TH1F>(GenericDac2);
}
