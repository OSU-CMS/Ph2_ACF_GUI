#include "DQMUtils/DQMHistogramOTverifyCICdataWord.h"
#include "RootUtils/RootContainerFactory.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/GenericDataArray.h"

#include "TFile.h"
#include "TH2F.h"

//========================================================================================================================
DQMHistogramOTverifyCICdataWord::DQMHistogramOTverifyCICdataWord() {}

//========================================================================================================================
DQMHistogramOTverifyCICdataWord::~DQMHistogramOTverifyCICdataWord() {}

//========================================================================================================================
void DQMHistogramOTverifyCICdataWord::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR CALIBRATION YOU DO NOT NEED THE FOLLOWING COMMENTED LINES
    // make fDetectorContainer ready to receive the information fromm the stream
    fDetectorContainer = &theDetectorStructure;
    // SoC utilities only - END

    bool        isPS       = theDetectorStructure.getFirstObject()->getFirstObject()->getFrontEndType() == FrontEndType::OuterTrackerPS;
    std::string chipName   = "CBC";
    std::string xAxisTitle = "CBC Id";
    int         idOffset   = 0;
    if(isPS)
    {
        xAxisTitle = "MPA Id";
        idOffset   = 8;
        chipName   = "MPA";
    }

    HistContainer<TH2F> testedBitsHistogram((chipName + "toCIC_PatternMatchingTestedBits").c_str(),
                                            (chipName + " to CIC pattern matching tested bits").c_str(),
                                            NUMBER_OF_CIC_PORTS,
                                            idOffset - 0.5,
                                            idOffset + NUMBER_OF_CIC_PORTS - 0.5,
                                            2,
                                            -0.5,
                                            1.5);
    testedBitsHistogram.fTheHistogram->GetXaxis()->SetTitle(xAxisTitle.c_str());
    testedBitsHistogram.fTheHistogram->GetYaxis()->SetBinLabel(1, "L1");
    testedBitsHistogram.fTheHistogram->GetYaxis()->SetBinLabel(2, "Stubs");
    testedBitsHistogram.fTheHistogram->SetStats(false);
    RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fTestedBitsHistogramContainer, testedBitsHistogram);

    HistContainer<TH2F> bitErrorRateHistogram((chipName + "toCIC_PatternMatchingErrorRate").c_str(),
                                              (chipName + " to CIC pattern matching error rate").c_str(),
                                              NUMBER_OF_CIC_PORTS,
                                              idOffset - 0.5,
                                              idOffset + NUMBER_OF_CIC_PORTS - 0.5,
                                              2,
                                              -0.5,
                                              1.5);
    bitErrorRateHistogram.fTheHistogram->GetXaxis()->SetTitle(xAxisTitle.c_str());
    bitErrorRateHistogram.fTheHistogram->GetYaxis()->SetBinLabel(1, "L1");
    bitErrorRateHistogram.fTheHistogram->GetYaxis()->SetBinLabel(2, "Stubs");
    bitErrorRateHistogram.fTheHistogram->SetMinimum(0);
    bitErrorRateHistogram.fTheHistogram->SetMaximum(1);
    bitErrorRateHistogram.fTheHistogram->SetStats(false);
    RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fBitErrorRateHistogramContainer, bitErrorRateHistogram);
}

//========================================================================================================================
void DQMHistogramOTverifyCICdataWord::fillPatternMatchingEfficiencyResults(DetectorDataContainer& thePatternMatchingEfficiencyContainer)
{
    for(auto board: thePatternMatchingEfficiencyContainer)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                if(!hybrid->hasSummary()) continue;

                auto thePatternMatchingEfficiencyVector = hybrid->getSummary<GenericDataArray<float, NUMBER_OF_CIC_PORTS, 2, 2>>();

                TH2F* bitErroRateHistogram =
                    fBitErrorRateHistogramContainer.getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;

                TH2F* testedBitsHistogram =
                    fTestedBitsHistogramContainer.getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;

                for(size_t chipId = 0; chipId < NUMBER_OF_CIC_PORTS; ++chipId) // not using the chipID because I want always to read all phases
                {
                    for(size_t cLineId = 0; cLineId < 2; cLineId++)
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
void DQMHistogramOTverifyCICdataWord::process()
{
    // This step it is not necessary, unless you want to format / draw histograms,
    // otherwise they will be automatically saved
}

//========================================================================================================================
void DQMHistogramOTverifyCICdataWord::reset(void)
{
    // Clear histograms if needed
}

//========================================================================================================================
bool DQMHistogramOTverifyCICdataWord::fill(std::string& inputStream)
{
    // SoC utilities only - BEGIN
    ContainerSerialization thePatternMatchinEfficiencyContainerSerialization("OTverifyCICdataWordPatternMatchingEfficiency");

    if(thePatternMatchinEfficiencyContainerSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched OTverifyCICdataWord PatternMatchingEfficiency!!!!\n";
        DetectorDataContainer theDetectorData =
            thePatternMatchinEfficiencyContainerSerialization.deserializeOpticalGroupContainer<EmptyContainer, EmptyContainer, GenericDataArray<float, NUMBER_OF_CIC_PORTS, 2, 2>, EmptyContainer>(
                fDetectorContainer);
        fillPatternMatchingEfficiencyResults(theDetectorData);
        return true;
    }

    return false;
    // SoC utilities only - END
}
