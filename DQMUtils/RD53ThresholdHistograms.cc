/*!
  \file                  RD53ThresholdHistograms.cc
  \brief                 Implementation of Threshold calibration histograms
  \author                Alkiviadis PAPADOPOULOS
  \version               1.0
  \date                  28/06/18
  Support:               email to alkiviadis.papadopoulos@cern.ch
  Support:               email to mauro.dinardo@cern.ch
*/

#include "RD53ThresholdHistograms.h"
#include "Utils/ContainerSerialization.h"

using namespace Ph2_HwDescription;

void ThresholdHistograms::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& settingsMap)
{
    fDetectorContainer = &theDetectorStructure;
    RD53Shared::setFirstChip(theDetectorStructure);

    // #######################
    // # Retrieve parameters #
    // #######################
    auto           frontEnd       = RD53Shared::firstChip->getFEtype(RD53Shared::firstChip->getNCols() / 2, RD53Shared::firstChip->getNCols() / 2);
    const uint16_t rangeThreshold = RD53Shared::setBits(RD53Shared::firstChip->getNumberOfBits(frontEnd->thresholdRegs[0])) + 1;

    auto hThreshold = CanvasContainer<TH1F>("Threshold", "Threshold", rangeThreshold, 0, rangeThreshold);
    bookChipImplementer(theOutputFile, theDetectorStructure, Threshold, hThreshold, "Threshold", "Entries");

    AreHistoBooked = true;
}

bool ThresholdHistograms::fill(std::string& inputStream)
{
    ContainerSerialization theContainerSerialization("ThrAdjustmentThreshold");

    if(theContainerSerialization.attachDeserializer(inputStream))
    {
        DetectorDataContainer fDetectorData = theContainerSerialization.deserializeChipContainer<EmptyContainer, uint16_t>(fDetectorContainer);
        ThresholdHistograms::fill(fDetectorData);
        return true;
    }
    return false;
}

void ThresholdHistograms::fill(const DetectorDataContainer& DataContainer)
{
    for(const auto cBoard: DataContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    if(cChip->hasSummary() == false) continue;

                    auto* hThreshold = Threshold.getObject(cBoard->getId())
                                           ->getObject(cOpticalGroup->getId())
                                           ->getObject(cHybrid->getId())
                                           ->getObject(cChip->getId())
                                           ->getSummary<CanvasContainer<TH1F>>()
                                           .fTheHistogram;

                    hThreshold->Fill(cChip->getSummary<uint16_t>());
                }
}

void ThresholdHistograms::process() { drawChip<TH1F>(Threshold); }
