#include "DQMUtils/DQMHistogramOTinjectionDelayOptimization.h"
#include "RootUtils/RootContainerFactory.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"

#include "TFile.h"
#include "TH1I.h"

//========================================================================================================================
DQMHistogramOTinjectionDelayOptimization::DQMHistogramOTinjectionDelayOptimization() {}

//========================================================================================================================
DQMHistogramOTinjectionDelayOptimization::~DQMHistogramOTinjectionDelayOptimization() {}

//========================================================================================================================
void DQMHistogramOTinjectionDelayOptimization::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR CALIBRATION YOU DO NOT NEED THE FOLLOWING COMMENTED LINES
    // make fDetectorContainer ready to receive the information fromm the stream
    fDetectorContainer = &theDetectorStructure;
    // SoC utilities only - END

    uint16_t maximumDelay = findValueInSettings<double>(pSettingsMap, "OTinjectionDelayOptimization_MaximumDelay", 150);
    fDelayStep            = findValueInSettings<double>(pSettingsMap, "OTinjectionDelayOptimization_DelayStep", 1);
    uint16_t numberOfBins = maximumDelay / fDelayStep;

    float bitSizeInNs = 1.;
    if(fDetectorContainer->getFirstObject()->getFirstObject()->getFrontEndType() == FrontEndType::OuterTrackerPS) { bitSizeInNs = 25. / 12.; }

    float minimumDelayInNs = bitSizeInNs / 2.;
    float maximumDelayInNs = (maximumDelay - 0.5) * bitSizeInNs;

    HistContainer<TH1I> thresholdVsDelayHistogram("ThresholdVsDelayScan", "Threshold Vs Delay Scan", numberOfBins, minimumDelayInNs, maximumDelayInNs);
    thresholdVsDelayHistogram.fTheHistogram->GetXaxis()->SetTitle("Delay [ns]");
    thresholdVsDelayHistogram.fTheHistogram->GetYaxis()->SetTitle("50% threshold [VcTh]");
    thresholdVsDelayHistogram.fTheHistogram->SetStats(false);
    RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, fThresholdVsDelayScanHistogramContainer, thresholdVsDelayHistogram);

    HistContainer<TH1I> bestThresholdAndDelayHistogram("BestThresholdAndDelay", "Best Threshold And Delay", numberOfBins, minimumDelayInNs, maximumDelayInNs);
    bestThresholdAndDelayHistogram.fTheHistogram->GetXaxis()->SetTitle("Best delay [ns]");
    bestThresholdAndDelayHistogram.fTheHistogram->GetYaxis()->SetTitle("Best threshold [VcTh]");
    bestThresholdAndDelayHistogram.fTheHistogram->SetStats(false);
    bestThresholdAndDelayHistogram.fTheHistogram->SetMarkerStyle(47);
    RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, fBestThresholdAndDelayHistogramContainer, bestThresholdAndDelayHistogram);
}

//========================================================================================================================
void DQMHistogramOTinjectionDelayOptimization::fillThresholdVsDelayScan(uint16_t delay, DetectorDataContainer& theThresholdContainer)
{
    for(auto theBoard: theThresholdContainer)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                for(auto theChip: *theHybrid)
                {
                    if(!theChip->hasSummary()) continue;
                    auto theHistogram = fThresholdVsDelayScanHistogramContainer.getChip(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId(), theChip->getId())
                                            ->getSummary<HistContainer<TH1I>>()
                                            .fTheHistogram;
                    theHistogram->SetBinContent((delay / fDelayStep) + 1, theChip->getSummary<uint16_t>());
                }
            }
        }
    }
}

//========================================================================================================================
void DQMHistogramOTinjectionDelayOptimization::fillBestThresholdAndDelay(DetectorDataContainer& theBestThresholdAndDelayContainer)
{
    for(auto theBoard: theBestThresholdAndDelayContainer)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                for(auto theChip: *theHybrid)
                {
                    if(!theChip->hasSummary()) continue;
                    auto theHistogram = fBestThresholdAndDelayHistogramContainer.getChip(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId(), theChip->getId())
                                            ->getSummary<HistContainer<TH1I>>()
                                            .fTheHistogram;
                    const auto& theBestThresholdAndDelay = theChip->getSummary<std::pair<uint16_t, uint16_t>>();
                    theHistogram->SetBinContent(theBestThresholdAndDelay.second + 1, theBestThresholdAndDelay.first);
                    theHistogram->SetBinError(theBestThresholdAndDelay.second + 1, 1);
                }
            }
        }
    }
}

//========================================================================================================================
void DQMHistogramOTinjectionDelayOptimization::process()
{
    // This step it is not necessary, unless you want to format / draw histograms,
    // otherwise they will be automatically saved
}

//========================================================================================================================
void DQMHistogramOTinjectionDelayOptimization::reset(void)
{
    // Clear histograms if needed
}

//========================================================================================================================
bool DQMHistogramOTinjectionDelayOptimization::fill(std::string& inputStream)
{
    // SoC utilities only - BEGIN
    ContainerSerialization theDelayScanSerialization("OTinjectionDelayOptimizationDelayScan");
    ContainerSerialization theBestValuesSerialization("OTinjectionDelayOptimizationBestValues");

    if(theDelayScanSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched OTinjectionDelayOptimization DelayScan!!!!!\n";
        uint16_t              delayValue;
        DetectorDataContainer theDetectorData = theDelayScanSerialization.deserializeOpticalGroupContainer<EmptyContainer, uint16_t, EmptyContainer, EmptyContainer>(fDetectorContainer, delayValue);
        fillThresholdVsDelayScan(delayValue, theDetectorData);
        return true;
    }
    if(theBestValuesSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched OTinjectionDelayOptimization BestValues!!!!!\n";
        DetectorDataContainer theDetectorData =
            theBestValuesSerialization.deserializeOpticalGroupContainer<EmptyContainer, std::pair<uint16_t, uint16_t>, EmptyContainer, EmptyContainer>(fDetectorContainer);
        fillBestThresholdAndDelay(theDetectorData);
        return true;
    }

    return false;
    // SoC utilities only - END
}
