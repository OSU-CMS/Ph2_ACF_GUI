#include "DQMUtils/DQMHistogramOTverifyBoardDataWord.h"
#include "RootUtils/RootContainerFactory.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/GenericDataArray.h"

#include "TFile.h"
#include "TH1F.h"

//========================================================================================================================
DQMHistogramOTverifyBoardDataWord::DQMHistogramOTverifyBoardDataWord() {}

//========================================================================================================================
DQMHistogramOTverifyBoardDataWord::~DQMHistogramOTverifyBoardDataWord() {}

//========================================================================================================================
void DQMHistogramOTverifyBoardDataWord::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR CALIBRATION YOU DO NOT NEED THE FOLLOWING COMMENTED LINES
    // make fDetectorContainer ready to receive the information fromm the stream
    fDetectorContainer = &theDetectorStructure;
    // SoC utilities only - END

    size_t numberOfLines = (theDetectorStructure.getFirstObject()->getFirstObject()->getFrontEndType() == FrontEndType::OuterTrackerPS) ? 7 : 6;

    auto setBitLabel = [numberOfLines](TH1F* theHistogram)
    {
        theHistogram->GetXaxis()->SetBinLabel(1, "L1");
        for(size_t stubLine = 0; stubLine < numberOfLines - 1; ++stubLine) theHistogram->GetXaxis()->SetBinLabel(stubLine + 2, Form("Stub%d", int(stubLine)));
    };

    HistContainer<TH1F> errorRateHistogram("CICtoLpGBT_PatternMatchingErrorRate", "CIC to LpGBT pattern matching error rate", numberOfLines, -0.5, numberOfLines - 0.5);
    errorRateHistogram.fTheHistogram->GetYaxis()->SetTitle("Error rate");
    errorRateHistogram.fTheHistogram->SetStats(false);
    setBitLabel(errorRateHistogram.fTheHistogram);
    RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fMatchingErrorRateHistogramContainer, errorRateHistogram);

    HistContainer<TH1F> testedBitsHistogram("CICtoLpGBT_PatternMatchingTestedBits", "CIC to LpGBT pattern matching tested bits", numberOfLines, -0.5, numberOfLines - 0.5);
    testedBitsHistogram.fTheHistogram->GetYaxis()->SetTitle("Tested bits");
    testedBitsHistogram.fTheHistogram->SetStats(false);
    setBitLabel(testedBitsHistogram.fTheHistogram);
    RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fMatchingTestedBitsHistogramContainer, testedBitsHistogram);
}

//========================================================================================================================
void DQMHistogramOTverifyBoardDataWord::process()
{
    // This step it is not necessary, unless you want to format / draw histograms,
    // otherwise they will be automatically saved
}

//========================================================================================================================
void DQMHistogramOTverifyBoardDataWord::reset(void)
{
    // Clear histograms if needed
}

//========================================================================================================================

void DQMHistogramOTverifyBoardDataWord::fillPatternErrorRate(DetectorDataContainer& thePatternErrorRateContainer)
{
    for(auto board: thePatternErrorRateContainer)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                if(!hybrid->hasSummary()) continue;
                TH1F* hybridErrorRateHistogram =
                    fMatchingErrorRateHistogramContainer.getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getSummary<HistContainer<TH1F>>().fTheHistogram;
                TH1F* hybridTestedBitsHistogram =
                    fMatchingTestedBitsHistogramContainer.getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getSummary<HistContainer<TH1F>>().fTheHistogram;
                auto theHybridMatchingEfficiencyVector = hybrid->getSummary<std::vector<GenericDataArray<float, 2>>>();
                for(size_t lineId = 0; lineId < theHybridMatchingEfficiencyVector.size(); ++lineId)
                {
                    auto testedBits = theHybridMatchingEfficiencyVector.at(lineId).at(0);
                    auto errorRate  = testedBits > 0 ? theHybridMatchingEfficiencyVector.at(lineId).at(1) / testedBits : 1.;
                    hybridTestedBitsHistogram->SetBinContent(lineId + 1, testedBits);
                    hybridErrorRateHistogram->SetBinContent(lineId + 1, errorRate);
                }
            }
        }
    }
}

//========================================================================================================================
bool DQMHistogramOTverifyBoardDataWord::fill(std::string& inputStream)
{
    // SoC utilities only - BEGIN
    ContainerSerialization theMatchingEfficiencyContainerSerialization("OTverifyBoardDataWordMatchingEfficiency");

    if(theMatchingEfficiencyContainerSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched OTverifyBoardDataWord MatchingEfficiency!!!!\n";
        DetectorDataContainer theDetectorData =
            theMatchingEfficiencyContainerSerialization.deserializeOpticalGroupContainer<EmptyContainer, EmptyContainer, std::vector<GenericDataArray<float, 2>>, EmptyContainer>(fDetectorContainer);
        fillPatternErrorRate(theDetectorData);
        return true;
    }

    return false;
    // SoC utilities only - END
}
