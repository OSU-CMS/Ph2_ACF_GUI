#include "DQMUtils/DQMHistogramOTBitErrorRateTest.h"
#include "HWDescription/lpGBT.h"
#include "RootUtils/RootContainerFactory.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/GenericDataArray.h"

#include "TFile.h"
#include "TH1F.h"
#include "TH1I.h"
#include "TH2F.h"

//========================================================================================================================
DQMHistogramOTBitErrorRateTest::DQMHistogramOTBitErrorRateTest() {}

//========================================================================================================================
DQMHistogramOTBitErrorRateTest::~DQMHistogramOTBitErrorRateTest() {}

//========================================================================================================================
void DQMHistogramOTBitErrorRateTest::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR CALIBRATION YOU DO NOT NEED THE FOLLOWING COMMENTED LINES
    // make fDetectorContainer ready to receive the information fromm the stream
    fDetectorContainer = &theDetectorStructure;
    // SoC utilities only - END

    fNumberOfLines = (theDetectorStructure.getFirstObject()->getFirstObject()->getFrontEndType() == FrontEndType::OuterTrackerPS) ? 7 : 6;

    auto setBitLabel = [this](TAxis* theXaxis)
    {
        std::vector<std::string> hybridSide{"FEHR", "FEHL"};
        for(uint8_t hybridId = 0; hybridId < hybridSide.size(); ++hybridId)
        {
            theXaxis->SetBinLabel(1 + hybridId * this->fNumberOfLines, (std::string("L1_") + hybridSide[hybridId]).c_str());
            for(size_t stubLine = 0; stubLine < size_t(this->fNumberOfLines) - 1; ++stubLine)
                theXaxis->SetBinLabel(stubLine + 2 + hybridId * this->fNumberOfLines, (std::string(Form("Stub%d_", int(stubLine))) + hybridSide[hybridId]).c_str());
        }
    };

    auto setBitLabelOpticalGroup = [this](TAxis* theXaxis)
    {
        theXaxis->SetBinLabel(1, "L1");
        for(size_t stubLine = 0; stubLine < size_t(this->fNumberOfLines) - 1; ++stubLine) theXaxis->SetBinLabel(stubLine + 2, Form("Stub%d", int(stubLine)));
    };

    HistContainer<TH2F> errorRatePhaseScanHistogram("BERTerrorRatePhaseScan", "BERT error rate phase scan", fNumberOfLines * 2, -0.5, fNumberOfLines * 2 - 0.5, 64, -0.5, 63.5);
    errorRatePhaseScanHistogram.fTheHistogram->GetYaxis()->SetTitle("Clock phase");
    errorRatePhaseScanHistogram.fTheHistogram->SetStats(false);
    setBitLabel(errorRatePhaseScanHistogram.fTheHistogram->GetXaxis());
    RootContainerFactory::bookOpticalGroupHistograms(theOutputFile, theDetectorStructure, fBERTerrorRatePhaseScanHistogram, errorRatePhaseScanHistogram);

    HistContainer<TH2F> bitCounterPhaseScanHistogram("BERTtestedBitCounterPhaseScan", "BERT tested bit counter phase scan", fNumberOfLines * 2, -0.5, fNumberOfLines * 2 - 0.5, 64, -0.5, 63.5);
    bitCounterPhaseScanHistogram.fTheHistogram->GetYaxis()->SetTitle("PRBS clock phase");
    bitCounterPhaseScanHistogram.fTheHistogram->SetStats(false);
    setBitLabel(bitCounterPhaseScanHistogram.fTheHistogram->GetXaxis());
    RootContainerFactory::bookOpticalGroupHistograms(theOutputFile, theDetectorStructure, fBERTbitCounterPhaseScanHistogram, bitCounterPhaseScanHistogram);

    HistContainer<TH1F> errorRateHistogram("BERTerrorRate", "BERT error rate", fNumberOfLines * 2, -0.5, fNumberOfLines * 2 - 0.5);
    errorRateHistogram.fTheHistogram->GetYaxis()->SetTitle("Error rate");
    errorRateHistogram.fTheHistogram->SetStats(false);
    setBitLabel(errorRateHistogram.fTheHistogram->GetXaxis());
    RootContainerFactory::bookOpticalGroupHistograms(theOutputFile, theDetectorStructure, fBERTerrorRateHistogram, errorRateHistogram);

    HistContainer<TH1F> bitCounterHistogram("BERTtestedBitCounter", "BERT tested bit counter", fNumberOfLines * 2, -0.5, fNumberOfLines * 2 - 0.5);
    bitCounterHistogram.fTheHistogram->GetYaxis()->SetTitle("Tested bits");
    bitCounterHistogram.fTheHistogram->SetStats(false);
    setBitLabel(bitCounterHistogram.fTheHistogram->GetXaxis());
    RootContainerFactory::bookOpticalGroupHistograms(theOutputFile, theDetectorStructure, fBERTbitCounterHistogram, bitCounterHistogram);

    HistContainer<TH1I> bestPhaseHistogram("BERTbestPhase", "Bert best phase", fNumberOfLines, -0.5, fNumberOfLines - 0.5);
    bestPhaseHistogram.fTheHistogram->GetYaxis()->SetTitle("Best phase");
    bestPhaseHistogram.fTheHistogram->SetStats(false);
    setBitLabelOpticalGroup(bestPhaseHistogram.fTheHistogram->GetXaxis());
    RootContainerFactory::bookOpticalGroupHistograms(theOutputFile, theDetectorStructure, fBestPhaseHistogram, bestPhaseHistogram);

    HistContainer<TH1I> fecCounterHistogram("FECerrorCounter", "FEC error counter", fNumberOfLines, -0.5, fNumberOfLines - 0.5);
    fecCounterHistogram.fTheHistogram->GetYaxis()->SetTitle("FEC counter");
    fecCounterHistogram.fTheHistogram->SetStats(false);
    setBitLabelOpticalGroup(fecCounterHistogram.fTheHistogram->GetXaxis());
    RootContainerFactory::bookOpticalGroupHistograms(theOutputFile, theDetectorStructure, fFECcounterHistogram, fecCounterHistogram);
}

//========================================================================================================================
void DQMHistogramOTBitErrorRateTest::fillErrorCounterPhaseScan(DetectorDataContainer& theErrorCountainer, uint16_t phaseDelay, uint8_t line)
{
    for(auto theBoard: theErrorCountainer)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            auto theErrorCounterHistogram = fBERTerrorRatePhaseScanHistogram.getOpticalGroup(theBoard->getId(), theOpticalGroup->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;
            auto theBitCounterHistogram   = fBERTbitCounterPhaseScanHistogram.getOpticalGroup(theBoard->getId(), theOpticalGroup->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;
            for(auto theHybrid: *theOpticalGroup)
            {
                if(!theHybrid->hasSummary()) continue;
                auto theErrorCounterVector = theHybrid->getSummary<GenericDataArray<uint64_t, 2>>();
                int  binNumber             = 1 + line + (theHybrid->getId() % 2) * fNumberOfLines;
                theBitCounterHistogram->SetBinContent(binNumber, phaseDelay + 1, theErrorCounterVector.at(0));
                theErrorCounterHistogram->SetBinContent(binNumber, phaseDelay + 1, theErrorCounterVector.at(0) > 0 ? float(theErrorCounterVector.at(1)) / float(theErrorCounterVector.at(0)) : 1.);
            }
        }
    }
}

//========================================================================================================================
void DQMHistogramOTBitErrorRateTest::fillErrorCounter(DetectorDataContainer& theErrorCountainer, uint8_t line)
{
    for(auto theBoard: theErrorCountainer)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            auto theErrorCounterHistogram = fBERTerrorRateHistogram.getOpticalGroup(theBoard->getId(), theOpticalGroup->getId())->getSummary<HistContainer<TH1F>>().fTheHistogram;
            auto theBitCounterHistogram   = fBERTbitCounterHistogram.getOpticalGroup(theBoard->getId(), theOpticalGroup->getId())->getSummary<HistContainer<TH1F>>().fTheHistogram;
            for(auto theHybrid: *theOpticalGroup)
            {
                if(!theHybrid->hasSummary()) continue;
                auto theErrorCounter = theHybrid->getSummary<GenericDataArray<uint64_t, 2>>();
                int  binNumber       = 1 + line + (theHybrid->getId() % 2) * fNumberOfLines;
                theBitCounterHistogram->SetBinContent(binNumber, theErrorCounter.at(0));
                theErrorCounterHistogram->SetBinContent(binNumber, theErrorCounter.at(0) > 0 ? float(theErrorCounter.at(1)) / float(theErrorCounter.at(0)) : 1.);
            }
        }
    }
}

//========================================================================================================================
void DQMHistogramOTBitErrorRateTest::fillFECcounter(DetectorDataContainer& theFECContainer, uint8_t line)
{
    for(auto theBoard: theFECContainer)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            if(!theOpticalGroup->hasSummary()) continue;
            auto theFECCounterHistogram = fFECcounterHistogram.getOpticalGroup(theBoard->getId(), theOpticalGroup->getId())->getSummary<HistContainer<TH1I>>().fTheHistogram;
            theFECCounterHistogram->SetBinContent(1 + line, theOpticalGroup->getSummary<uint32_t>());
        }
    }
}

//========================================================================================================================
void DQMHistogramOTBitErrorRateTest::fillBERTbestPhase(DetectorDataContainer& BestPhaseContainer, uint8_t line)
{
    for(auto theBoard: BestPhaseContainer)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            if(!theOpticalGroup->hasSummary()) continue;
            auto theBestPhaseHistogram = fBestPhaseHistogram.getOpticalGroup(theBoard->getId(), theOpticalGroup->getId())->getSummary<HistContainer<TH1I>>().fTheHistogram;
            theBestPhaseHistogram->SetBinContent(1 + line, theOpticalGroup->getSummary<uint16_t>());
        }
    }
}

//========================================================================================================================
void DQMHistogramOTBitErrorRateTest::process()
{
    // This step it is not necessary, unless you want to format / draw histograms,
    // otherwise they will be automatically saved
}

//========================================================================================================================
void DQMHistogramOTBitErrorRateTest::reset(void)
{
    // Clear histograms if needed
}

//========================================================================================================================
bool DQMHistogramOTBitErrorRateTest::fill(std::string& inputStream)
{
    // SoC utilities only - BEGIN
    ContainerSerialization theErrorCounterSerialization("OTBitErrorRateTestErrorCounter");
    ContainerSerialization theBestPhaseSerialization("OTBitErrorRateTestBestPhase");
    ContainerSerialization theErrorCounterPhaseScanSerialization("OTBitErrorRateTestErrorCounterPhaseScan");
    ContainerSerialization theFECcounterSerialization("OTBitErrorRateTestFECcounter");

    if(theErrorCounterPhaseScanSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched OTBitErrorRateTest ErrorCounterPhaseScan!!!!\n";
        uint16_t              phase;
        uint8_t               line;
        DetectorDataContainer theDetectorData =
            theErrorCounterPhaseScanSerialization.deserializeOpticalGroupContainer<EmptyContainer, EmptyContainer, GenericDataArray<uint64_t, 2>, EmptyContainer>(fDetectorContainer, phase, line);
        fillErrorCounterPhaseScan(theDetectorData, phase, line);
        return true;
    }
    if(theBestPhaseSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched OTBitErrorRateTest BestPhase!!!!\n";
        uint8_t               line;
        DetectorDataContainer theDetectorData = theBestPhaseSerialization.deserializeOpticalGroupContainer<EmptyContainer, EmptyContainer, EmptyContainer, uint16_t>(fDetectorContainer, line);
        fillBERTbestPhase(theDetectorData, line);
        return true;
    }
    if(theErrorCounterSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched OTBitErrorRateTest ErrorCounter!!!!\n";
        uint8_t               line;
        DetectorDataContainer theDetectorData =
            theErrorCounterSerialization.deserializeOpticalGroupContainer<EmptyContainer, EmptyContainer, GenericDataArray<uint64_t, 2>, EmptyContainer>(fDetectorContainer, line);
        fillErrorCounter(theDetectorData, line);
        return true;
    }
    if(theFECcounterSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched OTBitErrorRateTest FECcounter!!!!\n";
        uint8_t               line;
        DetectorDataContainer theDetectorData = theFECcounterSerialization.deserializeOpticalGroupContainer<EmptyContainer, EmptyContainer, EmptyContainer, uint32_t>(fDetectorContainer, line);
        fillFECcounter(theDetectorData, line);
        return true;
    }

    return false;
    // SoC utilities only - END
}
