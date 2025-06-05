#include "DQMUtils/DQMHistogramOTPSADCCalibration.h"
#include "RootUtils/GraphContainer.h"
#include "RootUtils/HistContainer.h"
#include "RootUtils/RootContainerFactory.h"
#include "Utils/ADCSlope.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"

#include "TF1.h"
#include "TFile.h"
#include "TGraph.h"
#include "TH1F.h"

//========================================================================================================================
DQMHistogramOTPSADCCalibration::DQMHistogramOTPSADCCalibration() {}

//========================================================================================================================
DQMHistogramOTPSADCCalibration::~DQMHistogramOTPSADCCalibration() {}

//========================================================================================================================
void DQMHistogramOTPSADCCalibration::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR CALIBRATION YOU DO NOT NEED THE FOLLOWING COMMENTED LINES
    // make fDetectorContainer ready to receive the information fromm the stream
    fDetectorContainer = &theDetectorStructure;
    // SoC utilities only - END
    if(fDetectorContainer->getFirstObject()->getFirstObject()->getFrontEndType() == FrontEndType::OuterTracker2S) return;

    HistContainer<TH1F> theTH1FChipVref("VREF_DACtoV", "VREF DAC to Volts", 32, -0.5, 31.5);
    theTH1FChipVref.fTheHistogram->GetXaxis()->SetTitle("ADC_VREF register");
    theTH1FChipVref.fTheHistogram->GetYaxis()->SetTitle("VREF [V]");
    RootContainerFactory::bookChipHistograms<HistContainer<TH1F>>(theOutputFile, theDetectorStructure, fChipVrefHistograms, theTH1FChipVref);

    GraphContainer<TGraph> theTGraphChipSlope(fGraphSize);
    theTGraphChipSlope.setNameTitle("ADC_Slope", "ADC slope");
    theTGraphChipSlope.fTheGraph->SetMarkerStyle(20);
    theTGraphChipSlope.fTheGraph->GetXaxis()->SetTitle("ADC output [ADC]");
    theTGraphChipSlope.fTheGraph->GetYaxis()->SetTitle("ADC output [V]");
    RootContainerFactory::bookChipHistograms<GraphContainer<TGraph>>(theOutputFile, theDetectorStructure, fChipSlopeGraphs, theTGraphChipSlope);
}

//========================================================================================================================
void DQMHistogramOTPSADCCalibration::process()
{
    // This step it is not necessary, unless you want to format / draw histograms,
    // otherwise they will be automatically saved
}

//========================================================================================================================
void DQMHistogramOTPSADCCalibration::reset(void)
{
    // Clear histograms if needed
}
//========================================================================================================================
void DQMHistogramOTPSADCCalibration::fillDACPlots(DetectorDataContainer& theVrefContainer)
{
    LOG(DEBUG) << __PRETTY_FUNCTION__ << " Fill DAC Plots " << RESET;
    for(auto cBoard: theVrefContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    if(!cChip->hasSummary()) continue;
                    TH1F* theVrefHistograms = nullptr;

                    theVrefHistograms = fChipVrefHistograms.getObject(cBoard->getId())
                                            ->getObject(cOpticalGroup->getId())
                                            ->getObject(cHybrid->getId())
                                            ->getObject(cChip->getId())
                                            ->getSummary<HistContainer<TH1F>>()
                                            .fTheHistogram;
                    LOG(DEBUG) << BOLDBLUE << " Fill Chip " << RESET;
                    LOG(DEBUG) << BOLDBLUE << " DAC, VREF "
                               << +theVrefContainer.getObject(cBoard->getId())
                                       ->getObject(cOpticalGroup->getId())
                                       ->getObject(cHybrid->getId())
                                       ->getObject(cChip->getId())
                                       ->getSummary<std::pair<uint8_t, float>>()
                                       .first
                               << " "
                               << theVrefContainer.getObject(cBoard->getId())
                                      ->getObject(cOpticalGroup->getId())
                                      ->getObject(cHybrid->getId())
                                      ->getObject(cChip->getId())
                                      ->getSummary<std::pair<uint8_t, float>>()
                                      .second
                               << RESET;
                    theVrefHistograms->Fill(theVrefContainer.getObject(cBoard->getId())
                                                ->getObject(cOpticalGroup->getId())
                                                ->getObject(cHybrid->getId())
                                                ->getObject(cChip->getId())
                                                ->getSummary<std::pair<uint8_t, float>>()
                                                .first,
                                            theVrefContainer.getObject(cBoard->getId())
                                                ->getObject(cOpticalGroup->getId())
                                                ->getObject(cHybrid->getId())
                                                ->getObject(cChip->getId())
                                                ->getSummary<std::pair<uint8_t, float>>()
                                                .second);

                } // chip
            } // hybrid
        } // optical group
    }
}
//========================================================================================================================
void DQMHistogramOTPSADCCalibration::fillSlopePlots(DetectorDataContainer& theADCSlopeContainer)
{
    for(auto cBoard: theADCSlopeContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    if(!cChip->hasSummary()) continue;
                    TGraph* theADCSlopeGraphs = nullptr;
                    theADCSlopeGraphs         = fChipSlopeGraphs.getObject(cBoard->getId())
                                            ->getObject(cOpticalGroup->getId())
                                            ->getObject(cHybrid->getId())
                                            ->getObject(cChip->getId())
                                            ->getSummary<GraphContainer<TGraph>>()
                                            .fTheGraph;
                    auto cChipContainer =
                        theADCSlopeContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<ADCSlope>();

                    float ADCs[fGraphSize]     = {cChipContainer.fADC_GND, cChipContainer.fADC_VBG};
                    float voltages[fGraphSize] = {0, cChipContainer.fMeasured_VBG};
                    for(int i = 0; i < fGraphSize; i++)
                    {
                        theADCSlopeGraphs->SetPointX(i, ADCs[i]);
                        theADCSlopeGraphs->SetPointY(i, voltages[i]);
                    }
                    if(cChipContainer.fADC_VBG != 0)
                    {
                        theADCSlopeGraphs->Fit("pol1", "Q");
                        LOG(DEBUG) << BLUE << " cChipContainer.fOffset " << cChipContainer.fOffset << " cChipContainer.fSlope " << cChipContainer.fSlope << RESET;
                        TF1* thePol1 = theADCSlopeGraphs->GetFunction("pol1");
                        thePol1->SetParameter(0, cChipContainer.fOffset);
                        thePol1->SetParameter(1, cChipContainer.fSlope);
                        thePol1->SetRange(0, 4095);
                        thePol1->SetLineColor(kRed + 1);
                        thePol1->SetLineStyle(2);
                    }

                } // chip
            } // hybrid
        } // optical group
    } // board
}
//========================================================================================================================
void DQMHistogramOTPSADCCalibration::fillVDDPlots(DetectorDataContainer& theVDDContainer, bool isAVDD)
{
    LOG(DEBUG) << __PRETTY_FUNCTION__ << " Fill VDD Plots " << RESET;
    for(auto cBoard: theVDDContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    if(!cChip->hasSummary()) continue;
                    TH1F* theVDDHistograms = nullptr;

                    if(isAVDD)
                    {
                        LOG(DEBUG) << MAGENTA << " AVDD " << RESET;
                        theVDDHistograms = fChipAVDDHistograms.getObject(cBoard->getId())
                                               ->getObject(cOpticalGroup->getId())
                                               ->getObject(cHybrid->getId())
                                               ->getObject(cChip->getId())
                                               ->getSummary<HistContainer<TH1F>>()
                                               .fTheHistogram;
                    }
                    else
                    {
                        LOG(DEBUG) << MAGENTA << " DVDD " << RESET;
                        theVDDHistograms = fChipDVDDHistograms.getObject(cBoard->getId())
                                               ->getObject(cOpticalGroup->getId())
                                               ->getObject(cHybrid->getId())
                                               ->getObject(cChip->getId())
                                               ->getSummary<HistContainer<TH1F>>()
                                               .fTheHistogram;
                    }

                    LOG(DEBUG) << BOLDBLUE << " Fill " << RESET;
                    LOG(DEBUG) << BOLDBLUE << " ADC, volts "
                               << theVDDContainer.getObject(cBoard->getId())
                                      ->getObject(cOpticalGroup->getId())
                                      ->getObject(cHybrid->getId())
                                      ->getObject(cChip->getId())
                                      ->getSummary<std::pair<uint8_t, float>>()
                                      .first
                               << " "
                               << theVDDContainer.getObject(cBoard->getId())
                                      ->getObject(cOpticalGroup->getId())
                                      ->getObject(cHybrid->getId())
                                      ->getObject(cChip->getId())
                                      ->getSummary<std::pair<uint8_t, float>>()
                                      .second
                               << RESET;
                    theVDDHistograms->Fill(theVDDContainer.getObject(cBoard->getId())
                                               ->getObject(cOpticalGroup->getId())
                                               ->getObject(cHybrid->getId())
                                               ->getObject(cChip->getId())
                                               ->getSummary<std::pair<uint8_t, float>>()
                                               .first,
                                           theVDDContainer.getObject(cBoard->getId())
                                               ->getObject(cOpticalGroup->getId())
                                               ->getObject(cHybrid->getId())
                                               ->getObject(cChip->getId())
                                               ->getSummary<std::pair<uint8_t, float>>()
                                               .second);

                } // chip
            } // hybrid
        } // optical group
    } // board
}
//========================================================================================================================
bool DQMHistogramOTPSADCCalibration::fill(std::string& inputStream)
{
    // SoC utilities only - BEGIN
    ContainerSerialization theDACSerialization("OTPSADCCalibrationVrefDac");
    if(theDACSerialization.attachDeserializer(inputStream))
    {
        LOG(INFO) << BOLDMAGENTA << "Matched OTPSADCCalibration Vref DAC!!!!!" << RESET;
        DetectorDataContainer theVREFDACData = theDACSerialization.deserializeOpticalGroupContainer<EmptyContainer, std::pair<uint32_t, float>, EmptyContainer, EmptyContainer>(fDetectorContainer);
        fillDACPlots(theVREFDACData);
        return true;
    }

    ContainerSerialization theADCSlopeSerialization("OTPSADCCalibrationADCSlope");
    if(theADCSlopeSerialization.attachDeserializer(inputStream))
    {
        LOG(DEBUG) << BOLDMAGENTA << "Matched OTPSADCCalibration ADC slope!!!!!" << RESET;
        DetectorDataContainer theADCSlopeData = theADCSlopeSerialization.deserializeOpticalGroupContainer<EmptyContainer, ADCSlope, EmptyContainer, EmptyContainer>(fDetectorContainer);
        fillSlopePlots(theADCSlopeData);
        return true;
    }

    ContainerSerialization theAVDDSerialization("OTPSADCCalibrationAVDD");
    if(theAVDDSerialization.attachDeserializer(inputStream))
    {
        LOG(DEBUG) << BOLDMAGENTA << "Matched  OTPSADCCalibration AVDD!!!!!" << RESET;
        DetectorDataContainer theAVDDData = theAVDDSerialization.deserializeOpticalGroupContainer<EmptyContainer, std::pair<uint32_t, float>, EmptyContainer, EmptyContainer>(fDetectorContainer);

        fillVDDPlots(theAVDDData, true);
        return true;
    }
    ContainerSerialization theDVDDSerialization("OTPSADCCalibrationDVDD");
    if(theDVDDSerialization.attachDeserializer(inputStream))
    {
        LOG(DEBUG) << BOLDMAGENTA << "Matched OTPSADCCalibration DVDD!!!!!" << RESET;
        DetectorDataContainer theDVDDData = theDVDDSerialization.deserializeOpticalGroupContainer<EmptyContainer, std::pair<uint32_t, float>, EmptyContainer, EmptyContainer>(fDetectorContainer);

        fillVDDPlots(theDVDDData, false);
        return true;
    }

    return false;
    // SoC utilities only - END
}
