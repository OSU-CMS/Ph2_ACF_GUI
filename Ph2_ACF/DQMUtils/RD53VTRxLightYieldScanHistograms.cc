/*!
  \file                  RD53VTRxLightYieldScanHistograms.h
  \brief                 Implementation of of VTRx light yield scan histograms
  \author                Mauro DINARDO
  \version               1.0
  \date                  29/11/24
  Support:               email to mauro.dinardo@cern.ch
*/

#include "RD53VTRxLightYieldScanHistograms.h"
#include "Utils/ContainerSerialization.h"

using namespace Ph2_HwDescription;

void VTRxLightYieldScanHistograms::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& settingsMap)
{
    fDetectorContainer = &theDetectorStructure;
    RD53Shared::setFirstChip(theDetectorStructure);

    // #######################
    // # Retrieve parameters #
    // #######################
    biasStart       = this->findValueInSettings<double>(settingsMap, "VTRxBiasStart");
    biasStop        = this->findValueInSettings<double>(settingsMap, "VTRxBiasStop");
    biasStep        = this->findValueInSettings<double>(settingsMap, "VTRxBiasStep");
    modulationStart = this->findValueInSettings<double>(settingsMap, "VTRxModulationStart");
    modulationStop  = this->findValueInSettings<double>(settingsMap, "VTRxModulationStop");
    modulationStep  = this->findValueInSettings<double>(settingsMap, "VTRxModulationStep");

    // #####################################
    // # Make proper axis for secial cases #
    // #####################################
    auto hInt2D = CanvasContainer<TH2F>("VTRxLightYieldScan",
                                        "VTRx Light Yield Scan",
                                        (biasStop - biasStart) / biasStep + 1,
                                        biasStart,
                                        biasStop + biasStep,
                                        (modulationStop - modulationStart) / modulationStep + 1,
                                        modulationStart,
                                        modulationStop + modulationStep);
    bookOpticalGroupImplementer(theOutputFile, theDetectorStructure, Intensity2D, hInt2D, "Bias (DAC units)", "Modulation (DAC units)");

    AreHistoBooked = true;
}

bool VTRxLightYieldScanHistograms::fill(std::string& inputStream)
{
    ContainerSerialization theIntensitySerialization("VTRxLightYieldScan");

    if(theIntensitySerialization.attachDeserializer(inputStream))
    {
        DetectorDataContainer fDetectorData = theIntensitySerialization.deserializeOpticalGroupContainer<EmptyContainer, EmptyContainer, EmptyContainer, std::vector<float>>(fDetectorContainer);
        VTRxLightYieldScanHistograms::fillIntensity(fDetectorData);
        return true;
    }
    return false;
}

void VTRxLightYieldScanHistograms::fillIntensity(const DetectorDataContainer& IntensityContainer)
{
    for(const auto cBoard: IntensityContainer)
        for(const auto cOpticalGroup: *cBoard)
        {
            if(cOpticalGroup->hasSummary() == false) continue;
            auto* Intensity2DHist = Intensity2D.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getSummary<CanvasContainer<TH2F>>().fTheHistogram;

            for(auto i = 0; i < Intensity2DHist->GetNbinsX(); i++)
                for(auto j = 0; j < Intensity2DHist->GetNbinsY(); j++)
                    Intensity2DHist->SetBinContent(i + 1, j + 1, cOpticalGroup->getSummary<std::vector<float>>().at(i * Intensity2DHist->GetNbinsY() + j));
        }
}

void VTRxLightYieldScanHistograms::process() { drawOpticalGroup<TH2F>(Intensity2D, "gcolz"); }
