/*!
  \file                  RD53LpGBTeyeOpeningHistograms.h
  \brief                 Implementation of of VTRx light yield scan histograms
  \author                Mauro DINARDO
  \version               1.0
  \date                  29/11/24
  Support:               email to mauro.dinardo@cern.ch
*/

#include "RD53LpGBTeyeOpeningHistograms.h"
#include "Utils/ContainerSerialization.h"

using namespace Ph2_HwDescription;

void LpGBTeyeOpeningHistograms::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& settingsMap)
{
    fDetectorContainer = &theDetectorStructure;
    RD53Shared::setFirstChip(theDetectorStructure);

    // #######################
    // # Retrieve parameters #
    // #######################
    lpGBTattenuation = this->findValueInSettings<double>(settingsMap, "LpGBTattenuation");

    // #####################################
    // # Make proper axis for secial cases #
    // #####################################
    auto hInt2D =
        CanvasContainer<TH2F>("LpGBTeyeOpening", ("LpGBT Eye Opening Scan (attenuation = " + std::to_string(lpGBTattenuation) + ")").c_str(), TIMEMAX, 0, TIMEMAX - 1, VOLTMAX, 0, VOLTMAX - 1);
    bookOpticalGroupImplementer(theOutputFile, theDetectorStructure, Intensity2D, hInt2D, "Time (ps)", "Volt (mV)");

    AreHistoBooked = true;
}

bool LpGBTeyeOpeningHistograms::fill(std::string& inputStream)
{
    ContainerSerialization theIntensitySerialization("LpGBTeyeOpening");

    if(theIntensitySerialization.attachDeserializer(inputStream))
    {
        DetectorDataContainer fDetectorData =
            theIntensitySerialization.deserializeOpticalGroupContainer<EmptyContainer, EmptyContainer, EmptyContainer, GenericDataArray<uint16_t, TIMEMAX, VOLTMAX>>(fDetectorContainer);
        LpGBTeyeOpeningHistograms::fillIntensity(fDetectorData);
        return true;
    }
    return false;
}

void LpGBTeyeOpeningHistograms::fillIntensity(const DetectorDataContainer& IntensityContainer)
{
    for(const auto cBoard: IntensityContainer)
        for(const auto cOpticalGroup: *cBoard)
        {
            if(cOpticalGroup->hasSummary() == false) continue;

            const auto& theEyeArray     = cOpticalGroup->getSummary<GenericDataArray<uint16_t, TIMEMAX, VOLTMAX>>();
            auto*       Intensity2DHist = Intensity2D.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getSummary<CanvasContainer<TH2F>>().fTheHistogram;

            for(auto i = 0; i < Intensity2DHist->GetNbinsX(); i++)
                for(auto j = 0; j < Intensity2DHist->GetNbinsY(); j++) Intensity2DHist->SetBinContent(i + 1, j + 1, theEyeArray.at(i).at(j));
        }
}

void LpGBTeyeOpeningHistograms::process() { drawOpticalGroup<TH2F>(Intensity2D, "gcolz"); }
