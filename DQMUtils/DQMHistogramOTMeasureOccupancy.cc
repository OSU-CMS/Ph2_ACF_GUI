#include "DQMUtils/DQMHistogramOTMeasureOccupancy.h"
#include "HWDescription/ReadoutChip.h"
#include "RootUtils/RootContainerFactory.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/Occupancy.h"

#include "TFile.h"
#include "TH1F.h"
#include "TH2F.h"

using namespace Ph2_HwDescription;

//========================================================================================================================
DQMHistogramOTMeasureOccupancy::DQMHistogramOTMeasureOccupancy() {}

//========================================================================================================================
DQMHistogramOTMeasureOccupancy::~DQMHistogramOTMeasureOccupancy() {}

//========================================================================================================================
void DQMHistogramOTMeasureOccupancy::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR CALIBRATION YOU DO NOT NEED THE FOLLOWING COMMENTED LINES
    // make fDetectorContainer ready to receive the information fromm the stream
    fDetectorContainer = &theDetectorStructure;
    // SoC utilities only - END

    double theNumberOfEvents    = findValueInSettings<double>(pSettingsMap, "OTMeasureOccupancy_NumberOfEvents", 10000);
    float  theCBCtestPulseValue = findValueInSettings<double>(pSettingsMap, "OTMeasureOccupancy_CBCtestPulseValue", 1.);
    float  theSSAtestPulseValue = findValueInSettings<double>(pSettingsMap, "OTMeasureOccupancy_SSAtestPulseValue", 1.);
    float  theMPAtestPulseValue = findValueInSettings<double>(pSettingsMap, "OTMeasureOccupancy_MPAtestPulseValue", 1.);
    bookPlotsForInjection(theOutputFile, theNumberOfEvents, theCBCtestPulseValue, theSSAtestPulseValue, theMPAtestPulseValue);
}

//========================================================================================================================

void DQMHistogramOTMeasureOccupancy::bookPlotsForInjection(TFile* theOutputFile,
                                                           double theNumberOfEvents,
                                                           float  theCBCtestPulseValue,
                                                           float  theSSAtestPulseValue,
                                                           float  theMPAtestPulseValue,
                                                           int    iteration)
{
    auto        selectCBCfunction     = [](const ChipContainer* theChip) { return (static_cast<const ReadoutChip*>(theChip)->getFrontEndType() == FrontEndType::CBC3); };
    std::string selectCBCfunctionName = "SelectCBCfunction";

    auto        selectSSAfunction     = [](const ChipContainer* theChip) { return (static_cast<const ReadoutChip*>(theChip)->getFrontEndType() == FrontEndType::SSA2); };
    std::string selectSSAfunctionName = "SelectSSAfunction";

    auto        selectMPAfunction     = [](const ChipContainer* theChip) { return (static_cast<const ReadoutChip*>(theChip)->getFrontEndType() == FrontEndType::MPA2); };
    std::string selectMPAfunctionName = "SelectMPAfunction";

    fDetectorContainer->addReadoutChipQueryFunction(selectCBCfunction, selectCBCfunctionName);
    HistContainer<TH1F> theCBCoccupancyHistogram(
        Form("ChannelOccupancy_Injection_%.3f_MIP", theCBCtestPulseValue), Form("Channel Occupancy - Injection = %.3f MIP", theCBCtestPulseValue), NCHANNELS, -0.5, NCHANNELS - 0.5);
    theCBCoccupancyHistogram.fTheHistogram->GetXaxis()->SetTitle("Channel");
    theCBCoccupancyHistogram.fTheHistogram->GetYaxis()->SetTitle("Occupancy");
    theCBCoccupancyHistogram.fTheHistogram->SetMaximum(1.2);
    theCBCoccupancyHistogram.fTheHistogram->SetMinimum(theCBCtestPulseValue > 0 ? 0. : 0.5 / theNumberOfEvents); // to allow go into log mode
    theCBCoccupancyHistogram.fTheHistogram->SetStats(false);
    RootContainerFactory::bookChipHistograms<HistContainer<TH1F>>(theOutputFile, *fDetectorContainer, fOccupancyHistogramContainer[iteration], theCBCoccupancyHistogram);
    fDetectorContainer->removeReadoutChipQueryFunction(selectCBCfunctionName);

    fDetectorContainer->addReadoutChipQueryFunction(selectSSAfunction, selectSSAfunctionName);
    HistContainer<TH1F> theSSAoccupancyHistogram(
        Form("ChannelOccupancy_Injection_%.3f_MIP", theSSAtestPulseValue), Form("Channel Occupancy - Injection = %.3f MIP", theSSAtestPulseValue), NSSACHANNELS, -0.5, NSSACHANNELS - 0.5);
    theSSAoccupancyHistogram.fTheHistogram->GetXaxis()->SetTitle("Channel");
    theSSAoccupancyHistogram.fTheHistogram->GetYaxis()->SetTitle("Occupancy");
    theSSAoccupancyHistogram.fTheHistogram->SetMaximum(1.2);
    theSSAoccupancyHistogram.fTheHistogram->SetMinimum(theSSAtestPulseValue > 0 ? 0. : 0.5 / theNumberOfEvents); // to allow go into log mode
    theSSAoccupancyHistogram.fTheHistogram->SetStats(false);
    RootContainerFactory::bookChipHistograms<HistContainer<TH1F>>(theOutputFile, *fDetectorContainer, fOccupancyHistogramContainer[iteration], theSSAoccupancyHistogram);
    fDetectorContainer->removeReadoutChipQueryFunction(selectSSAfunctionName);

    fDetectorContainer->addReadoutChipQueryFunction(selectMPAfunction, selectMPAfunctionName);
    HistContainer<TH2F> theMPAoccupancyHistogram(Form("ChannelOccupancy_Injection_%.3f_MIP", theMPAtestPulseValue),
                                                 Form("Channel Occupancy - Injection = %.3f MIP", theMPAtestPulseValue),
                                                 NSSACHANNELS,
                                                 -0.5,
                                                 NSSACHANNELS - 0.5,
                                                 NMPAROWS,
                                                 -0.5,
                                                 NMPAROWS - 0.5);
    theMPAoccupancyHistogram.fTheHistogram->GetXaxis()->SetTitle("Col");
    theMPAoccupancyHistogram.fTheHistogram->GetYaxis()->SetTitle("Row");
    theMPAoccupancyHistogram.fTheHistogram->SetMaximum(1.);
    theMPAoccupancyHistogram.fTheHistogram->SetMinimum(theMPAtestPulseValue > 0 ? 0. : 0.5 / theNumberOfEvents); // to allow go into log mode
    theMPAoccupancyHistogram.fTheHistogram->SetStats(false);
    RootContainerFactory::bookChipHistograms<HistContainer<TH2F>>(theOutputFile, *fDetectorContainer, fOccupancyHistogramContainer[iteration], theMPAoccupancyHistogram);
    fDetectorContainer->removeReadoutChipQueryFunction(selectMPAfunctionName);
}

//========================================================================================================================
void DQMHistogramOTMeasureOccupancy::fillOccupancy(const DetectorDataContainer& theOccupancyContainer, size_t iteration)
{
    for(auto theBoard: theOccupancyContainer)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                for(auto theChip: *theHybrid)
                {
                    if(!theChip->hasChannelContainer()) continue;
                    ReadoutChip* theReadoutChip = fDetectorContainer->getObject(theBoard->getId())->getObject(theOpticalGroup->getId())->getObject(theHybrid->getId())->getObject(theChip->getId());
                    const ChipDataContainer* theChipContainer = fOccupancyHistogramContainer.at(iteration).getChip(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId(), theChip->getId());
                    // using TH1F and TH2F inheritance from TH1
                    TH1* theOccupancyHistogram;
                    if(theReadoutChip->getFrontEndType() == FrontEndType::MPA2)
                        theOccupancyHistogram = theChipContainer->getSummary<HistContainer<TH2F>>().fTheHistogram;
                    else
                        theOccupancyHistogram = theChipContainer->getSummary<HistContainer<TH1F>>().fTheHistogram;

                    for(uint16_t row = 0; row < theChip->getNumberOfRows(); ++row)
                    {
                        for(uint16_t col = 0; col < theChip->getNumberOfCols(); ++col)
                        {
                            auto theOccupancy = theChip->getChannel<Occupancy>(row, col);
                            theOccupancyHistogram->SetBinContent(col + 1, row + 1, theOccupancy.fOccupancy);
                            theOccupancyHistogram->SetBinError(col + 1, row + 1, theOccupancy.fOccupancyError);
                        }
                    }
                }
            }
        }
    }
}

//========================================================================================================================
void DQMHistogramOTMeasureOccupancy::process()
{
    // This step it is not necessary, unless you want to format / draw histograms,
    // otherwise they will be automatically saved
}

//========================================================================================================================
void DQMHistogramOTMeasureOccupancy::reset(void)
{
    // Clear histograms if needed
}

//========================================================================================================================
bool DQMHistogramOTMeasureOccupancy::fill(std::string& inputStream)
{
    // SoC utilities only - BEGIN
    ContainerSerialization theOccupancySerialization("OTMeasureOccupancyOccupancy");

    if(theOccupancySerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched OTMeasureOccupancy Occupancy!!!!!\n";
        size_t                iteration;
        DetectorDataContainer theDetectorData = theOccupancySerialization.deserializeChipContainer<Occupancy, Occupancy>(fDetectorContainer, iteration);
        fillOccupancy(theDetectorData, iteration);
        return true;
    }
    return false;
    // SoC utilities only - END
}
