#include "DQMUtils/DQMHistogramOTCICtoLpGBTecv.h"
#include "RootUtils/RootContainerFactory.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/GenericDataArray.h"

#include "TFile.h"
#include "TH2F.h"

//========================================================================================================================
DQMHistogramOTCICtoLpGBTecv::DQMHistogramOTCICtoLpGBTecv() {}

//========================================================================================================================
DQMHistogramOTCICtoLpGBTecv::~DQMHistogramOTCICtoLpGBTecv() {}

//========================================================================================================================
void DQMHistogramOTCICtoLpGBTecv::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR CALIBRATION YOU DO NOT NEED THE FOLLOWING COMMENTED LINES
    // make fDetectorContainer ready to receive the information fromm the stream
    fDetectorContainer = &theDetectorStructure;

    // Create a list of the possible CICSignalStrength and CICClockPolarity combinations. The left digit refers to the CICSignalStrength, the right digit refers to the CICClockPolarity
    std::vector<float> listOfLpGBTPhase    = convertStringToFloatList(findValueInSettings<std::string>(pSettingsMap, "OTCICtoLpGBTecv_LpGBTPhase", "0-14"));
    std::vector<float> listOfCICStrength   = convertStringToFloatList(findValueInSettings<std::string>(pSettingsMap, "OTCICtoLpGBTecv_CICStrength", "1, 3, 5"));
    std::vector<float> listOfClockPolarity = convertStringToFloatList(findValueInSettings<std::string>(pSettingsMap, "OTCICtoLpGBTecv_ClockPolarity", "0-1"));
    std::vector<float> listOfClockStrength = convertStringToFloatList(findValueInSettings<std::string>(pSettingsMap, "OTCICtoLpGBTecv_ClockStrength", "1, 4, 7"));
    size_t             numberOfLines       = (fDetectorContainer->getFirstObject()->getFirstObject()->getFrontEndType() == FrontEndType::OuterTrackerPS) ? 7 : 6;

    auto prepareHistogram = [&listOfLpGBTPhase](TH2F* theHistogram)
    {
        theHistogram->GetXaxis()->SetTitle("LpGBT Phase");
        theHistogram->SetStats(false);
        theHistogram->GetXaxis()->SetLabelSize(0.04);
        theHistogram->GetYaxis()->SetLabelSize(0.04);

        // Label the y axis with the line and clock strength
        int binNumber = 1;
        for(int channel = 1; channel <= theHistogram->GetYaxis()->GetNbins(); channel++)
        {
            std::string s = channel == 1 ? "L1" : "Stub" + convertToString(channel - 1);
            theHistogram->GetYaxis()->SetBinLabel(binNumber, s.c_str());
            binNumber++;
        }

        // Book the histograms
        theHistogram->DrawCopy("text");
    };

    // x-axis is LpGBT phase and y-axis is line
    for(auto polarity: listOfClockPolarity)
    {
        for(auto CICStrength: listOfCICStrength)
        {
            for(auto hybridClockStrength: listOfClockStrength)
            {
                // Declare histogram axes titles and number of bins
                size_t numberOfXaxisBins                   = listOfLpGBTPhase.size();
                size_t numberOfYaxisBins                   = numberOfLines;
                auto   ClockCICStrengthPolarityCombination = (hybridClockStrength * 100) + (CICStrength * 10) + polarity;

                HistContainer<TH2F> ECVTestedBitsHistogram(
                    Form("CICtoLpGBT_PatternMatchingTestedBits_CIC_SLVScurrent_%.0f_LpGBT_Clock_Polarity_%.0f_Clock_Strength_%.0f", CICStrength, polarity, hybridClockStrength),
                    Form("CIC to LpGBT pattern matching tested bits CIC SLVS current %.0f - LpGBT Clock Polarity %.0f Clock Strength %.0f", CICStrength, polarity, hybridClockStrength),
                    numberOfXaxisBins,
                    -0.5,
                    numberOfXaxisBins - 0.5,
                    numberOfYaxisBins,
                    0,
                    numberOfYaxisBins);
                prepareHistogram(ECVTestedBitsHistogram.fTheHistogram);
                RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fTestedBits[ClockCICStrengthPolarityCombination], ECVTestedBitsHistogram);

                HistContainer<TH2F> ECVErrorRateHistogram(
                    Form("CICtoLpGBT_PatternMatchingErrorRate_CIC_SLVScurrent_%.0f_LpGBT_Clock_Polarity_%.0f_Clock_Strength_%.0f", CICStrength, polarity, hybridClockStrength),
                    Form("CIC to LpGBT pattern matching error rate CIC SLVS current %.0f - LpGBT Clock Polarity %.0f Clock Strength %.0f", CICStrength, polarity, hybridClockStrength),
                    numberOfXaxisBins,
                    -0.5,
                    numberOfXaxisBins - 0.5,
                    numberOfYaxisBins,
                    0,
                    numberOfYaxisBins);
                prepareHistogram(ECVErrorRateHistogram.fTheHistogram);
                RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fErrorRate[ClockCICStrengthPolarityCombination], ECVErrorRateHistogram);
            }
        }
    }
}

//========================================================================================================================
void DQMHistogramOTCICtoLpGBTecv::process()
{
    // This step it is not necessary, unless you want to format / draw histograms,
    // otherwise they will be automatically saved
}

//========================================================================================================================
void DQMHistogramOTCICtoLpGBTecv::reset(void)
{
    // Clear histograms if needed
}

//========================================================================================================================
bool DQMHistogramOTCICtoLpGBTecv::fill(std::string& inputStream)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR CALIBRATION YOU DO NOT NEED THE FOLLOWING COMMENTED LINES

    // As example, I'm expecting to receive a data stream from an uint32_t contained from calibration "OTverifyECVlpGBTCIC"
    ContainerSerialization theECVlpGBTCICContainerSerialization("OTCICtoLpGBTecvEfficiencyHistogram");

    if(theECVlpGBTCICContainerSerialization.attachDeserializer(inputStream))
    {
        // It matched! Decoding data
        // Need to tell to the streamer what data are contained (in this case in every channel there is an object of type MyType)
        uint8_t               pClockPolarity, pClockStrengthIndex, pCicStrength, pPhaseIndex;
        DetectorDataContainer theDetectorData =
            theECVlpGBTCICContainerSerialization.deserializeOpticalGroupContainer<EmptyContainer, EmptyContainer, std::vector<GenericDataArray<float, 2>>, EmptyContainer>(
                fDetectorContainer, pClockPolarity, pClockStrengthIndex, pCicStrength, pPhaseIndex);

        // Filling the histograms
        fillEfficiency(pClockPolarity, pClockStrengthIndex, pCicStrength, pPhaseIndex, theDetectorData);
        return true;
    }

    // the stream does not match, the expected (DQM interface will try to check if other DQM istogrammers are looking
    //  for this stream)
    return false;
    // SoC utilities only - END
}

void DQMHistogramOTCICtoLpGBTecv::fillEfficiency(uint8_t pClockPolarity, uint8_t pClockStrength, uint8_t pCicStrength, uint8_t pPhaseIndex, DetectorDataContainer& theEfficiencyContainer)
{
    for(auto board: theEfficiencyContainer)
    {
        for(auto opticalGroup: *board)
        {
            for(auto theHybrid: *opticalGroup)
            {
                if(!theHybrid->hasSummary()) continue;
                auto efficiencies = theHybrid->getSummary<std::vector<GenericDataArray<float, 2>>>();

                // Select the correct histogram given the pClockPolarity and the pCicStrength
                uint8_t ClockCICStrengthPolarityCombination = (pClockStrength * 100) + (pCicStrength * 10) + pClockPolarity;
                auto    theErrorRateHistogram =
                    fErrorRate[ClockCICStrengthPolarityCombination].getHybrid(board->getId(), opticalGroup->getId(), theHybrid->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;
                auto theTestedBitsHistogram =
                    fTestedBits[ClockCICStrengthPolarityCombination].getHybrid(board->getId(), opticalGroup->getId(), theHybrid->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;
                // Fill the selected histogram with efficiency content
                uint8_t lineCounter = 0;
                for(auto efficiency: efficiencies)
                {
                    auto testedBits = efficiency.at(0);
                    auto errorRate  = testedBits > 0 ? efficiency.at(1) / testedBits : 1.;
                    theErrorRateHistogram->SetBinContent(pPhaseIndex, lineCounter + 1, errorRate);
                    theTestedBitsHistogram->SetBinContent(pPhaseIndex, lineCounter + 1, testedBits);
                    lineCounter++;
                }
            }
        }
    }
}