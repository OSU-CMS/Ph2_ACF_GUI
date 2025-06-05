#include "DQMUtils/DQMHistogramOTalignLpGBTinputs.h"
#include "RootUtils/RootContainerFactory.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/LpGBTalignmentResult.h"

#include "TFile.h"
#include "TH1F.h"
#include "TH1I.h"
#include "TH2F.h"

//========================================================================================================================
DQMHistogramOTalignLpGBTinputs::DQMHistogramOTalignLpGBTinputs() {}

//========================================================================================================================
DQMHistogramOTalignLpGBTinputs::~DQMHistogramOTalignLpGBTinputs() {}

//========================================================================================================================
void DQMHistogramOTalignLpGBTinputs::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR CALIBRATION YOU DO NOT NEED THE FOLLOWING COMMENTED LINES
    // make fDetectorContainer ready to receive the information fromm the stream
    fDetectorContainer = &theDetectorStructure;
    // SoC utilities only - END

    fGroupAndChannelToBinNumber.clear();
    const auto theHybridGroupsAndChannels = theDetectorStructure.getFirstObject()->getFirstObject()->getLpGBTrxGroupsAndChannelsPerHybrid();

    int numberOfBins = theHybridGroupsAndChannels.size();
    for(const auto& groupAndChannels: theHybridGroupsAndChannels)
    {
        fGroupAndChannelToBinNumber[groupAndChannels.first.first][groupAndChannels.first.second] = groupAndChannels.second.first * numberOfBins / 2 + groupAndChannels.second.second + 1;
    }

    auto setBinLabels = [this, numberOfBins](TAxis* theHistogramAxis)
    {
        for(int binNumber = 0; binNumber < numberOfBins; ++binNumber)
        {
            std::string label = "FEH";
            label += (2 * binNumber / numberOfBins > 0) ? "L" : "R";
            if(2 * binNumber % numberOfBins == 0)
                label += "_L1";
            else
                label += ("_Stub" + std::to_string(binNumber % (numberOfBins / 2) - 1));
            theHistogramAxis->SetBinLabel(binNumber + 1, label.c_str());
        }
    };

    HistContainer<TH1F> alignmentSuccessHistogram("CICtoLpGBT_PhaseAlignmentEfficiency", "CIC to LpGBT phase alignment efficiency", numberOfBins, -0.5, numberOfBins - 0.5);
    alignmentSuccessHistogram.fTheHistogram->GetXaxis()->SetTitle("");
    setBinLabels(alignmentSuccessHistogram.fTheHistogram->GetXaxis());
    alignmentSuccessHistogram.fTheHistogram->GetYaxis()->SetTitle("Alignment efficiency");
    RootContainerFactory::bookOpticalGroupHistograms(theOutputFile, theDetectorStructure, fAlignmentSuccessHistogramContainer, alignmentSuccessHistogram);

    HistContainer<TH1I> bestPhaseHistogram("CICtoLpGBT_BestPhase", "CIC to LpGBT best phase", numberOfBins, -0.5, numberOfBins - 0.5);
    bestPhaseHistogram.fTheHistogram->GetXaxis()->SetTitle("");
    setBinLabels(bestPhaseHistogram.fTheHistogram->GetXaxis());
    bestPhaseHistogram.fTheHistogram->GetYaxis()->SetTitle("Best phase value");
    RootContainerFactory::bookOpticalGroupHistograms(theOutputFile, theDetectorStructure, fBestPhaseHistogramContainer, bestPhaseHistogram);

    HistContainer<TH2F> foundPhasesDistributionHistogram("CICtoLpGBT_FoundPhaseDistribution", "CIC to LpGBT found phase distribution", numberOfBins, -0.5, numberOfBins - 0.5, 16, -0.5, 15.5);
    foundPhasesDistributionHistogram.fTheHistogram->GetXaxis()->SetTitle("");
    setBinLabels(foundPhasesDistributionHistogram.fTheHistogram->GetXaxis());
    foundPhasesDistributionHistogram.fTheHistogram->GetYaxis()->SetTitle("Phase");
    RootContainerFactory::bookOpticalGroupHistograms(theOutputFile, theDetectorStructure, fFoundPhasesDistributionHistogramContainer, foundPhasesDistributionHistogram);
}

//========================================================================================================================

void DQMHistogramOTalignLpGBTinputs::fillPhaseAlignmentResults(DetectorDataContainer& thePhaseAlignmentResultContainer)
{
    for(auto board: thePhaseAlignmentResultContainer)
    {
        for(auto opticalGroup: *board)
        {
            if(!opticalGroup->hasSummary()) continue;

            auto theHybridRetryNumberVector = opticalGroup->getSummary<LpGBTalignmentResult>();

            TH1F* hybridAlignmentSuccessHistogram   = fAlignmentSuccessHistogramContainer.getObject(board->getId())->getObject(opticalGroup->getId())->getSummary<HistContainer<TH1F>>().fTheHistogram;
            TH1I* hybridBestPhaseHistogramHistogram = fBestPhaseHistogramContainer.getObject(board->getId())->getObject(opticalGroup->getId())->getSummary<HistContainer<TH1I>>().fTheHistogram;
            TH2F* hybridFoundPhasesDistributionHistogram =
                fFoundPhasesDistributionHistogramContainer.getObject(board->getId())->getObject(opticalGroup->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;

            for(const auto& theGroupResult: theHybridRetryNumberVector.fResultContainer)
            {
                for(const auto& theChannelResult: theGroupResult.second)
                {
                    float                       alignmentSuccessRate = std::get<0>(theChannelResult.second);
                    uint8_t                     bestPhaseValue       = std::get<1>(theChannelResult.second);
                    GenericDataArray<float, 16> foundPhaseHistogram  = std::get<2>(theChannelResult.second);
                    int                         currentBit           = fGroupAndChannelToBinNumber[theGroupResult.first][theChannelResult.first];

                    hybridAlignmentSuccessHistogram->SetBinContent(currentBit, alignmentSuccessRate);
                    hybridBestPhaseHistogramHistogram->SetBinContent(currentBit, bestPhaseValue);
                    for(size_t phaseValue = 0; phaseValue < foundPhaseHistogram.size(); ++phaseValue)
                        hybridFoundPhasesDistributionHistogram->SetBinContent(currentBit, phaseValue + 1, foundPhaseHistogram.at(phaseValue));
                }
            }
        }
    }
}

//========================================================================================================================
void DQMHistogramOTalignLpGBTinputs::process()
{
    // This step it is not necessary, unless you want to format / draw histograms,
    // otherwise they will be automatically saved
}

//========================================================================================================================
void DQMHistogramOTalignLpGBTinputs::reset(void)
{
    // Clear histograms if needed
}

//========================================================================================================================
bool DQMHistogramOTalignLpGBTinputs::fill(std::string& inputStream)
{
    // SoC utilities only - BEGIN
    ContainerSerialization theAlignmentResultsContainerSerialization("OTalignLpGBTinputsAlignmentResults");

    if(theAlignmentResultsContainerSerialization.attachDeserializer(inputStream))
    {
        std::cout << "Matched OTalignLpGBTinputs AlignmentResults!!!!\n";
        DetectorDataContainer theDetectorData =
            theAlignmentResultsContainerSerialization.deserializeOpticalGroupContainer<EmptyContainer, EmptyContainer, EmptyContainer, LpGBTalignmentResult>(fDetectorContainer);
        fillPhaseAlignmentResults(theDetectorData);
        return true;
    }

    return false;
    // SoC utilities only - END
}
