/*!
  \file                  RD53GainOptimizationHistograms.cc
  \brief                 Implementation of Gain optimization calibration histograms
  \author                Alkiviadis PAPADOPOULOS
  \version               1.0
  \date                  28/06/18
  Support:               email to alkiviadis.papadopoulos@cern.ch
  Support:               email to mauro.dinardo@cern.ch
*/

#include "RD53GainOptimizationHistograms.h"
#include "Utils/ContainerSerialization.h"

using namespace Ph2_HwDescription;

void GainOptimizationHistograms::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& settingsMap)
{
    fDetectorContainer = &theDetectorStructure;
    RD53Shared::setFirstChip(theDetectorStructure);

    // #######################
    // # Retrieve parameters #
    // #######################
    auto           frontEnd      = RD53Shared::firstChip->getFEtype(RD53Shared::firstChip->getNCols() / 2, RD53Shared::firstChip->getNCols() / 2);
    const uint16_t rangeKrumCurr = RD53Shared::setBits(RD53Shared::firstChip->getNumberOfBits(frontEnd->gainReg)) + 1;

    auto hKrumCurr = CanvasContainer<TH1F>("KrumCurr", "Krummenacher current", rangeKrumCurr, 0, rangeKrumCurr);
    bookChipImplementer(theOutputFile, theDetectorStructure, KrumCurr, hKrumCurr, "Krummenacher Current", "Entries");

    AreHistoBooked = true;
}

bool GainOptimizationHistograms::fill(std::string& inputStream)
{
    ContainerSerialization theSCurveSerialization("GainOptimizationKrumCurr");

    if(theSCurveSerialization.attachDeserializer(inputStream))
    {
        DetectorDataContainer fDetectorData = theSCurveSerialization.deserializeChipContainer<EmptyContainer, uint16_t>(fDetectorContainer);
        GainOptimizationHistograms::fill(fDetectorData);
        return true;
    }
    return false;
}

void GainOptimizationHistograms::fill(const DetectorDataContainer& DataContainer)
{
    for(const auto cBoard: DataContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    if(cChip->hasSummary() == false) continue;

                    auto* hKrumCurr = KrumCurr.getObject(cBoard->getId())
                                          ->getObject(cOpticalGroup->getId())
                                          ->getObject(cHybrid->getId())
                                          ->getObject(cChip->getId())
                                          ->getSummary<CanvasContainer<TH1F>>()
                                          .fTheHistogram;

                    hKrumCurr->Fill(cChip->getSummary<uint16_t>());
                }
}

void GainOptimizationHistograms::process() { drawChip<TH1F>(KrumCurr); }
