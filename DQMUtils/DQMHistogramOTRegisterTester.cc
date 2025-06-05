#include "DQMUtils/DQMHistogramOTRegisterTester.h"
#include "RootUtils/RootContainerFactory.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"

#include "TFile.h"
#include "TH1.h"

//========================================================================================================================
DQMHistogramOTRegisterTester::DQMHistogramOTRegisterTester() {}

//========================================================================================================================
DQMHistogramOTRegisterTester::~DQMHistogramOTRegisterTester() {}

//========================================================================================================================
void DQMHistogramOTRegisterTester::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR CALIBRATION YOU DO NOT NEED THE FOLLOWING COMMENTED LINES
    // make fDetectorContainer ready to receive the information fromm the stream
    fDetectorContainer = &theDetectorStructure;

    // SoC utilities only - END
    bool isPS = theDetectorStructure.getFirstObject()->getFirstObject()->getFrontEndType() == FrontEndType::OuterTrackerPS;

    int         numberOfBins = 8;
    std::string binLabel     = "CBC";
    if(isPS)
    {
        numberOfBins = 16;
        binLabel     = "SSA";
    }
    numberOfBins += 1; // adding CIC
    HistContainer<TH1F> patternMatchingEfficiencyHistogram("RegisterMatchingEfficiency", "Register matching efficiency", numberOfBins, -0.5, numberOfBins - 0.5);
    patternMatchingEfficiencyHistogram.fTheHistogram->GetYaxis()->SetTitle("Efficiency");

    for(uint8_t bin = 0; bin < numberOfBins; ++bin)
    {
        if(isPS && bin > 7) binLabel = "MPA";
        patternMatchingEfficiencyHistogram.fTheHistogram->GetXaxis()->SetBinLabel(bin + 1, Form("%s%d", binLabel.c_str(), bin));
        if(bin == numberOfBins - 1) patternMatchingEfficiencyHistogram.fTheHistogram->GetXaxis()->SetBinLabel(bin + 1, "CIC");
    }
    patternMatchingEfficiencyHistogram.fTheHistogram->SetMinimum(0);
    patternMatchingEfficiencyHistogram.fTheHistogram->SetMaximum(1.1);
    patternMatchingEfficiencyHistogram.fTheHistogram->SetStats(false);
    RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fPatternMatchingEfficiencyHistogramContainer, patternMatchingEfficiencyHistogram);
}
//========================================================================================================================
void DQMHistogramOTRegisterTester::fillPatternMatchingEfficiencyResults(DetectorDataContainer& thePatternMatchingEfficiencyContainer)
{
    for(auto board: thePatternMatchingEfficiencyContainer)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                if(!hybrid->hasSummary()) continue;

                auto thePatternMatchingEfficiencyVector = hybrid->getSummary<std::vector<float>>();

                TH1F* patternMatchingEfficiencyHistogram = fPatternMatchingEfficiencyHistogramContainer.getObject(board->getId())
                                                               ->getObject(opticalGroup->getId())
                                                               ->getObject(hybrid->getId())
                                                               ->getSummary<HistContainer<TH1F>>()
                                                               .fTheHistogram;

                for(size_t bin = 0; bin < thePatternMatchingEfficiencyVector.size(); ++bin) // not using the chipID because I want always to read all phases
                {
                    patternMatchingEfficiencyHistogram->SetBinContent(bin + 1, thePatternMatchingEfficiencyVector[bin]);
                }
            }
        }
    }
}
//========================================================================================================================
void DQMHistogramOTRegisterTester::process()
{
    // This step it is not necessary, unless you want to format / draw histograms,
    // otherwise they will be automatically saved
}

//========================================================================================================================
void DQMHistogramOTRegisterTester::reset(void)
{
    // Clear histograms if needed
}

//========================================================================================================================
bool DQMHistogramOTRegisterTester::fill(std::string& inputStream)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR CALIBRATION YOU DO NOT NEED THE FOLLOWING COMMENTED LINES

    ContainerSerialization thePatternMatchinEfficiencyContainerSerialization("OTRegisterTesterPatternMatchingEfficiency");

    if(thePatternMatchinEfficiencyContainerSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched OTverifyCICdataWord PatternMatchingEfficiency!!!!\n";
        DetectorDataContainer theDetectorData = thePatternMatchinEfficiencyContainerSerialization.deserializeHybridContainer<EmptyContainer, EmptyContainer, std::vector<float>>(fDetectorContainer);
        fillPatternMatchingEfficiencyResults(theDetectorData);
        return true;
    }
    // the stream does not match, the expected (DQM interface will try to check if other DQM istogrammers are looking
    //  for this stream)
    return false;
    // SoC utilities only - END
}
