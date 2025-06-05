#include "DQMUtils/DQMHistogramOTPatternCheckerHelper.h"
#include "RootUtils/RootContainerFactory.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/GenericDataArray.h"

#include "TFile.h"
#include "TH1F.h"

//========================================================================================================================
DQMHistogramOTPatternCheckerHelper::DQMHistogramOTPatternCheckerHelper() {}

//========================================================================================================================
DQMHistogramOTPatternCheckerHelper::~DQMHistogramOTPatternCheckerHelper() {}

//========================================================================================================================
void DQMHistogramOTPatternCheckerHelper::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR CALIBRATION YOU DO NOT NEED THE FOLLOWING COMMENTED LINES
    // make fDetectorContainer ready to receive the information fromm the stream
    fDetectorContainer = &theDetectorStructure;
    // SoC utilities only - END

    fNumberOfLines = (theDetectorStructure.getFirstObject()->getFirstObject()->getFrontEndType() == FrontEndType::OuterTrackerPS) ? 6 : 5;

    auto setBitLabel = [this](TAxis* theXaxis)
    {
        std::vector<std::string> hybridSide{"FEHR", "FEHL"};
        for(uint8_t hybridId = 0; hybridId < hybridSide.size(); ++hybridId)
        {
            for(size_t stubLine = 0; stubLine < size_t(this->fNumberOfLines); ++stubLine)
                theXaxis->SetBinLabel(stubLine + 1 + hybridId * this->fNumberOfLines, (std::string(Form("Stub%d_", int(stubLine))) + hybridSide[hybridId]).c_str());
        }
    };

    HistContainer<TH1F> errorRateHistogram("PatternErrorRate", "Pattern error rate", fNumberOfLines * 2, -0.5, fNumberOfLines * 2 - 0.5);
    errorRateHistogram.fTheHistogram->GetYaxis()->SetTitle("Error rate");
    errorRateHistogram.fTheHistogram->SetStats(false);
    setBitLabel(errorRateHistogram.fTheHistogram->GetXaxis());
    RootContainerFactory::bookOpticalGroupHistograms(theOutputFile, theDetectorStructure, fPatternErrorRateHistogram, errorRateHistogram);

    HistContainer<TH1F> bitCounterHistogram("PatternTestedBitCounter", "Pattern tested bit counter", fNumberOfLines * 2, -0.5, fNumberOfLines * 2 - 0.5);
    bitCounterHistogram.fTheHistogram->GetYaxis()->SetTitle("Tested bits");
    bitCounterHistogram.fTheHistogram->SetStats(false);
    setBitLabel(bitCounterHistogram.fTheHistogram->GetXaxis());
    RootContainerFactory::bookOpticalGroupHistograms(theOutputFile, theDetectorStructure, fPatternBitCounterHistogram, bitCounterHistogram);
}

//========================================================================================================================
void DQMHistogramOTPatternCheckerHelper::process()
{
    // This step it is not necessary, unless you want to format / draw histograms,
    // otherwise they will be automatically saved
}

//========================================================================================================================
void DQMHistogramOTPatternCheckerHelper::reset(void)
{
    // Clear histograms if needed
}

//========================================================================================================================
void DQMHistogramOTPatternCheckerHelper::fillErrorCounter(DetectorDataContainer& theErrorCountainer, uint8_t line)
{
    for(auto theBoard: theErrorCountainer)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            auto theErrorCounterHistogram = fPatternErrorRateHistogram.getOpticalGroup(theBoard->getId(), theOpticalGroup->getId())->getSummary<HistContainer<TH1F>>().fTheHistogram;
            auto theBitCounterHistogram   = fPatternBitCounterHistogram.getOpticalGroup(theBoard->getId(), theOpticalGroup->getId())->getSummary<HistContainer<TH1F>>().fTheHistogram;
            for(auto theHybrid: *theOpticalGroup)
            {
                auto theErrorCounter = theHybrid->getSummary<GenericDataArray<uint64_t, 2>>();
                int  binNumber       = line + (theHybrid->getId() % 2) * fNumberOfLines;
                theBitCounterHistogram->SetBinContent(binNumber, theErrorCounter.at(0));
                theErrorCounterHistogram->SetBinContent(binNumber, theErrorCounter.at(0) > 0 ? float(theErrorCounter.at(1)) / float(theErrorCounter.at(0)) : 1.);
            }
        }
    }
}

//========================================================================================================================
bool DQMHistogramOTPatternCheckerHelper::fill(std::string& inputStream)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR CALIBRATION YOU DO NOT NEED THE FOLLOWING COMMENTED LINES

    // SoC utilities only - BEGIN
    ContainerSerialization theErrorCounterSerialization("TPatternCheckerHelperErrorCounter");

    if(theErrorCounterSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched OTBitErrorRateTest ErrorCounter!!!!\n";
        uint8_t               line;
        DetectorDataContainer theDetectorData =
            theErrorCounterSerialization.deserializeOpticalGroupContainer<EmptyContainer, EmptyContainer, GenericDataArray<uint64_t, 2>, EmptyContainer>(fDetectorContainer, line);
        fillErrorCounter(theDetectorData, line);
        return true;
    }

    return false;
    // SoC utilities only - END
}
