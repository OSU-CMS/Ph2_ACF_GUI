#include "DQMUtils/DQMHistogramOTalignLpGBTinputsForBypass.h"
#include "RootUtils/RootContainerFactory.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/GenericDataArray.h"

#include "TFile.h"
#include "TH1I.h"
#include "TH2F.h"

//========================================================================================================================
DQMHistogramOTalignLpGBTinputsForBypass::DQMHistogramOTalignLpGBTinputsForBypass() {}

//========================================================================================================================
DQMHistogramOTalignLpGBTinputsForBypass::~DQMHistogramOTalignLpGBTinputsForBypass() {}

//========================================================================================================================
void DQMHistogramOTalignLpGBTinputsForBypass::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR CALIBRATION YOU DO NOT NEED THE FOLLOWING COMMENTED LINES
    // make fDetectorContainer ready to receive the information fromm the stream
    fDetectorContainer = &theDetectorStructure;
    // SoC utilities only - END

    uint8_t numberOfLines = 4;
    for(uint8_t phyPort = 0; phyPort < 12; ++phyPort)
    {
        HistContainer<TH2F> phaseScanMatchingBitErrorRate(Form("LpGBTforCICbypass_PhaseScanBitErrorRate_phyPort%d", phyPort),
                                                          Form("LpGBT for CIC bypass phase scan bit error rate - phyPort %d", phyPort),
                                                          15,
                                                          -0.5,
                                                          14.5,
                                                          numberOfLines,
                                                          -0.5,
                                                          numberOfLines - 0.5);
        phaseScanMatchingBitErrorRate.fTheHistogram->GetXaxis()->SetTitle("Phase");
        for(uint8_t line = 0; line < numberOfLines; ++line) phaseScanMatchingBitErrorRate.fTheHistogram->GetYaxis()->SetBinLabel(line + 1, Form("Stub%d", line + 1));
        phaseScanMatchingBitErrorRate.fTheHistogram->SetMinimum(0);
        phaseScanMatchingBitErrorRate.fTheHistogram->SetMaximum(1);
        phaseScanMatchingBitErrorRate.fTheHistogram->SetStats(false);
        RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fPhaseScanMatchingBitErrorRateContainer[phyPort], phaseScanMatchingBitErrorRate);

        HistContainer<TH2F> phaseScanMatchingTestedBits(Form("LpGBTforCICbypass_PhaseScanTestedBits_phyPort%d", phyPort),
                                                        Form("LpGBT for CIC bypass phase scan tested bits - phyPort %d", phyPort),
                                                        15,
                                                        -0.5,
                                                        14.5,
                                                        numberOfLines,
                                                        -0.5,
                                                        numberOfLines - 0.5);
        phaseScanMatchingTestedBits.fTheHistogram->GetXaxis()->SetTitle("Phase");
        for(uint8_t line = 0; line < numberOfLines; ++line) phaseScanMatchingTestedBits.fTheHistogram->GetYaxis()->SetBinLabel(line + 1, Form("Stub%d", line + 1));
        phaseScanMatchingTestedBits.fTheHistogram->SetStats(false);
        RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fPhaseScanMatchingTestedBitsContainer[phyPort], phaseScanMatchingTestedBits);

        HistContainer<TH1I> bestPhase(Form("LpGBTforCICbypass_BestPhase_phyPort%d", phyPort), Form("LpGBT for CIC bypass best phase - phyPort %d", phyPort), numberOfLines, -0.5, numberOfLines - 0.5);
        bestPhase.fTheHistogram->GetYaxis()->SetTitle("Phase");
        for(uint8_t line = 0; line < numberOfLines; ++line) bestPhase.fTheHistogram->GetXaxis()->SetBinLabel(line + 1, Form("Stub%d", line + 1));
        bestPhase.fTheHistogram->SetStats(false);
        RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fBestPhase[phyPort], bestPhase);
    }
}

//========================================================================================================================
void DQMHistogramOTalignLpGBTinputsForBypass::fillMatchingEfficiency(DetectorDataContainer& matchingEfficiencyContainer, uint8_t phyPort, uint8_t lpgbtPhase)
{
    for(auto theBoard: matchingEfficiencyContainer)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                if(!theHybrid->hasSummary()) continue;
                auto thePhaseScanErrorRateHistogram =
                    fPhaseScanMatchingBitErrorRateContainer[phyPort].getHybrid(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;
                auto thePhaseScanTestedBitsHistogram =
                    fPhaseScanMatchingTestedBitsContainer[phyPort].getHybrid(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;
                auto theTestedBitsAndErrorRateArray = theHybrid->getSummary<GenericDataArray<float, 4, 2>>();
                for(size_t line = 0; line < 4; ++line)
                {
                    const auto& theTestedBitsAndErrorRate = theTestedBitsAndErrorRateArray.at(line);
                    float       bitCount                  = theTestedBitsAndErrorRate.at(0);
                    float       errorCount                = theTestedBitsAndErrorRate.at(1);
                    thePhaseScanTestedBitsHistogram->SetBinContent(lpgbtPhase + 1, line + 1, bitCount);
                    thePhaseScanErrorRateHistogram->SetBinContent(lpgbtPhase + 1, line + 1, bitCount > 0 ? errorCount / bitCount : 1.);
                }
            }
        }
    }
}

//========================================================================================================================
void DQMHistogramOTalignLpGBTinputsForBypass::fillBestPhase(DetectorDataContainer& bestPhaseContainer, uint8_t phyPort)
{
    for(auto theBoard: bestPhaseContainer)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                if(!theHybrid->hasSummary()) continue;
                auto theBestPhaseHistogram = fBestPhase[phyPort].getHybrid(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId())->getSummary<HistContainer<TH1I>>().fTheHistogram;
                auto theBestPhaseArray     = theHybrid->getSummary<GenericDataArray<uint8_t, 4>>();
                for(size_t line = 0; line < 4; ++line) { theBestPhaseHistogram->SetBinContent(line + 1, theBestPhaseArray.at(line)); }
            }
        }
    }
}

//========================================================================================================================
void DQMHistogramOTalignLpGBTinputsForBypass::process()
{
    // This step it is not necessary, unless you want to format / draw histograms,
    // otherwise they will be automatically saved
}

//========================================================================================================================
void DQMHistogramOTalignLpGBTinputsForBypass::reset(void)
{
    // Clear histograms if needed
}

//========================================================================================================================
bool DQMHistogramOTalignLpGBTinputsForBypass::fill(std::string& inputStream)
{
    // SoC utilities only - BEGIN

    ContainerSerialization thePhaseScanMatchingEfficiencySerialization("OTalignLpGBTinputsForBypassMatchingEfficiency");
    ContainerSerialization theBestPhaseSerialization("OTalignLpGBTinputsForBypassBestPhase");

    if(thePhaseScanMatchingEfficiencySerialization.attachDeserializer(inputStream))
    {
        uint8_t               phyPort;
        uint8_t               lpgbtPhase;
        DetectorDataContainer theDetectorData =
            thePhaseScanMatchingEfficiencySerialization.deserializeHybridContainer<EmptyContainer, EmptyContainer, GenericDataArray<float, 4, 2>>(fDetectorContainer, phyPort, lpgbtPhase);
        fillMatchingEfficiency(theDetectorData, phyPort, lpgbtPhase);
        return true;
    }
    if(theBestPhaseSerialization.attachDeserializer(inputStream))
    {
        uint8_t               phyPort;
        DetectorDataContainer theDetectorData = theBestPhaseSerialization.deserializeHybridContainer<EmptyContainer, EmptyContainer, GenericDataArray<uint8_t, 4>>(fDetectorContainer, phyPort);
        fillBestPhase(theDetectorData, phyPort);
        return true;
    }

    return false;
    // SoC utilities only - END
}
