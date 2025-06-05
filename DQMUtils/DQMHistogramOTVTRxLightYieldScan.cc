#include "DQMUtils/DQMHistogramOTVTRxLightYieldScan.h"
#include "RootUtils/RootContainerFactory.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"

#include "TFile.h"
#include "TH2F.h"

//========================================================================================================================
DQMHistogramOTVTRxLightYieldScan::DQMHistogramOTVTRxLightYieldScan() {}

//========================================================================================================================
DQMHistogramOTVTRxLightYieldScan::~DQMHistogramOTVTRxLightYieldScan() {}

//========================================================================================================================
void DQMHistogramOTVTRxLightYieldScan::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR CALIBRATION YOU DO NOT NEED THE FOLLOWING COMMENTED LINES
    // make fDetectorContainer ready to receive the information fromm the stream
    fDetectorContainer = &theDetectorStructure;
    // SoC utilities only - END

    HistContainer<TH2F> lightYieldHistogram("VTRx_LightYieldScan", "VTRx light yield scan - Power (#muW)", 5, 38, 58, 5, 22, 42);
    lightYieldHistogram.fTheHistogram->GetXaxis()->SetTitle("Bias [DAC units]");
    lightYieldHistogram.fTheHistogram->GetYaxis()->SetTitle("Modulation [DAC units]");
    lightYieldHistogram.fTheHistogram->SetStats(false);
    RootContainerFactory::bookOpticalGroupHistograms(theOutputFile, theDetectorStructure, fLightYieldScanContainer, lightYieldHistogram);
}

//========================================================================================================================
void DQMHistogramOTVTRxLightYieldScan::process()
{
    // This step it is not necessary, unless you want to format / draw histograms,
    // otherwise they will be automatically saved
}

//========================================================================================================================
void DQMHistogramOTVTRxLightYieldScan::reset(void)
{
    // Clear histograms if needed
}

void DQMHistogramOTVTRxLightYieldScan::fillOpticalPower(DetectorDataContainer& theOpticalPowerContainer, uint8_t biasValue, uint8_t modulationValue)
{
    for(auto theBoard: theOpticalPowerContainer)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            if(!theOpticalGroup->hasSummary()) continue;
            float lightPower             = theOpticalGroup->getSummary<float>();
            TH2F* theLightYieldHistogram = fLightYieldScanContainer.getOpticalGroup(theBoard->getId(), theOpticalGroup->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;
            theLightYieldHistogram->SetBinContent(theLightYieldHistogram->GetXaxis()->FindBin(biasValue), theLightYieldHistogram->GetYaxis()->FindBin(modulationValue), lightPower);
        }
    }
}

//========================================================================================================================
bool DQMHistogramOTVTRxLightYieldScan::fill(std::string& inputStream)
{
    // SoC utilities only - BEGIN

    ContainerSerialization theLightYieldSerialization("OTVTRxLightYieldScanOpticalPower");

    if(theLightYieldSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched OTSSAtoMPAecv L1PatternMatchingEfficiency!!!!\n";
        uint8_t               biasValue, modulationValue;
        DetectorDataContainer theDetectorData =
            theLightYieldSerialization.deserializeBoardContainer<EmptyContainer, EmptyContainer, EmptyContainer, float, EmptyContainer>(fDetectorContainer, biasValue, modulationValue);
        fillOpticalPower(theDetectorData, biasValue, modulationValue);
        return true;
    }

    return false;
    // SoC utilities only - END
}
