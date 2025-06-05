#include "DQMUtils/DQMHistogramOTverifyMPASSAdataWord.h"
#include "RootUtils/RootContainerFactory.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/GenericDataArray.h"

#include "TFile.h"
#include "TH2F.h"

//========================================================================================================================
DQMHistogramOTverifyMPASSAdataWord::DQMHistogramOTverifyMPASSAdataWord() {}

//========================================================================================================================
DQMHistogramOTverifyMPASSAdataWord::~DQMHistogramOTverifyMPASSAdataWord() {}

//========================================================================================================================
void DQMHistogramOTverifyMPASSAdataWord::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR CALIBRATION YOU DO NOT NEED THE FOLLOWING COMMENTED LINES
    // make fDetectorContainer ready to receive the information fromm the stream
    fDetectorContainer = &theDetectorStructure;
    // SoC utilities only - END

    HistContainer<TH2F> patternMatchingTestedBitsHistogram(
        "SSAtoMPA_PatternMatchingTestedBits", "SSA to MPA pattern matching tested bits", NUMBER_OF_CIC_PORTS, 8 - 0.5, 8 + NUMBER_OF_CIC_PORTS - 0.5, 9, -0.5, 8.5);
    patternMatchingTestedBitsHistogram.fTheHistogram->GetXaxis()->SetTitle("MPA Id");
    patternMatchingTestedBitsHistogram.fTheHistogram->GetYaxis()->SetBinLabel(1, "L1");
    for(size_t clusterLine = 0; clusterLine < 8; ++clusterLine) patternMatchingTestedBitsHistogram.fTheHistogram->GetYaxis()->SetBinLabel(clusterLine + 2, Form("Cluster%d", int(clusterLine)));
    patternMatchingTestedBitsHistogram.fTheHistogram->SetStats(false);
    RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fPatternMatchingTestedBitsHistogramContainer, patternMatchingTestedBitsHistogram);

    HistContainer<TH2F> patternMatchingErrorRateHistogram(
        "SSAtoMPA_PatternMatchingErrorRate", "SSA to MPA pattern matching error rate", NUMBER_OF_CIC_PORTS, 8 - 0.5, 8 + NUMBER_OF_CIC_PORTS - 0.5, 9, -0.5, 8.5);
    patternMatchingErrorRateHistogram.fTheHistogram->GetXaxis()->SetTitle("MPA Id");
    patternMatchingErrorRateHistogram.fTheHistogram->GetYaxis()->SetBinLabel(1, "L1");
    for(size_t clusterLine = 0; clusterLine < 8; ++clusterLine) patternMatchingErrorRateHistogram.fTheHistogram->GetYaxis()->SetBinLabel(clusterLine + 2, Form("Cluster%d", int(clusterLine)));
    patternMatchingErrorRateHistogram.fTheHistogram->SetMinimum(0);
    patternMatchingErrorRateHistogram.fTheHistogram->SetMaximum(1);
    patternMatchingErrorRateHistogram.fTheHistogram->SetStats(false);
    RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fPatternMatchingErrorRateHistogramContainer, patternMatchingErrorRateHistogram);
}

//========================================================================================================================
void DQMHistogramOTverifyMPASSAdataWord::fillPatternMatchingEfficiencyResults(DetectorDataContainer& thePatternMatchingEfficiencyContainer)
{
    for(auto board: thePatternMatchingEfficiencyContainer)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                if(!hybrid->hasSummary()) continue;

                auto thePatternMatchingEfficiencyVector = hybrid->getSummary<GenericDataArray<float, NUMBER_OF_CIC_PORTS, 9, 2>>();

                TH2F* bitErroRateHistogram = fPatternMatchingErrorRateHistogramContainer.getObject(board->getId())
                                                 ->getObject(opticalGroup->getId())
                                                 ->getObject(hybrid->getId())
                                                 ->getSummary<HistContainer<TH2F>>()
                                                 .fTheHistogram;

                TH2F* testedBitsHistogram = fPatternMatchingTestedBitsHistogramContainer.getObject(board->getId())
                                                ->getObject(opticalGroup->getId())
                                                ->getObject(hybrid->getId())
                                                ->getSummary<HistContainer<TH2F>>()
                                                .fTheHistogram;

                for(size_t chipId = 0; chipId < NUMBER_OF_CIC_PORTS; ++chipId) // not using the chipID because I want always to read all phases
                {
                    for(size_t cLineId = 0; cLineId < 9; cLineId++)
                    {
                        float bitCount   = thePatternMatchingEfficiencyVector.at(chipId).at(cLineId).at(0);
                        float errorCount = thePatternMatchingEfficiencyVector.at(chipId).at(cLineId).at(1);
                        testedBitsHistogram->SetBinContent(chipId + 1, cLineId + 1, bitCount);
                        bitErroRateHistogram->SetBinContent(chipId + 1, cLineId + 1, bitCount > 0 ? errorCount / bitCount : 1.);
                    }
                }
            }
        }
    }
}

//========================================================================================================================
void DQMHistogramOTverifyMPASSAdataWord::process()
{
    // This step it is not necessary, unless you want to format / draw histograms,
    // otherwise they will be automatically saved
}

//========================================================================================================================
void DQMHistogramOTverifyMPASSAdataWord::reset(void)
{
    // Clear histograms if needed
}

//========================================================================================================================
bool DQMHistogramOTverifyMPASSAdataWord::fill(std::string& inputStream)
{
    // SoC utilities only - BEGIN
    ContainerSerialization thePatternMatchinEfficiencyContainerSerialization("OTverifyMPASSAdataWordPatternMatchingEfficiency");

    if(thePatternMatchinEfficiencyContainerSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched OTverifyMPASSAdataWord PatternMatchingEfficiency!!!!\n";
        DetectorDataContainer theDetectorData =
            thePatternMatchinEfficiencyContainerSerialization.deserializeOpticalGroupContainer<EmptyContainer, EmptyContainer, GenericDataArray<float, NUMBER_OF_CIC_PORTS, 9>, EmptyContainer>(
                fDetectorContainer);
        fillPatternMatchingEfficiencyResults(theDetectorData);
        return true;
    }

    return false;
    // SoC utilities only - END
}
