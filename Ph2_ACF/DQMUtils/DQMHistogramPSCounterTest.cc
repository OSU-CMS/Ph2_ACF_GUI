#include "DQMUtils/DQMHistogramPSCounterTest.h"
#include "HWDescription/ReadoutChip.h"
#include "RootUtils/RootContainerFactory.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/Occupancy.h"

#include "TFile.h"
#include "TH2F.h"

using namespace Ph2_HwDescription;

//========================================================================================================================
DQMHistogramPSCounterTest::DQMHistogramPSCounterTest() {}

//========================================================================================================================
DQMHistogramPSCounterTest::~DQMHistogramPSCounterTest() {}

//========================================================================================================================
void DQMHistogramPSCounterTest::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR CALIBRATION YOU DO NOT NEED THE FOLLOWING COMMENTED LINES
    // make fDetectorContainer ready to receive the information fromm the stream
    fDetectorContainer = &theDetectorStructure;
    // SoC utilities only - END

    auto        selectSSAfunction     = [](const ChipContainer* theChip) { return (static_cast<const ReadoutChip*>(theChip)->getFrontEndType() == FrontEndType::SSA2); };
    std::string selectSSAfunctionName = "SelectSSAfunction";

    auto        selectMPAfunction     = [](const ChipContainer* theChip) { return (static_cast<const ReadoutChip*>(theChip)->getFrontEndType() == FrontEndType::MPA2); };
    std::string selectMPAfunctionName = "SelectMPAfunction";

    fDetectorContainer->addReadoutChipQueryFunction(selectSSAfunction, selectSSAfunctionName);
    HistContainer<TH2F> theSSAscurveHistogram("SCurve", "SCurve", NSSACHANNELS, -0.5, NSSACHANNELS - 0.5, 256, -0.5, 255.5);
    theSSAscurveHistogram.fTheHistogram->GetXaxis()->SetTitle("channel");
    theSSAscurveHistogram.fTheHistogram->GetYaxis()->SetTitle("threshold");
    theSSAscurveHistogram.fTheHistogram->GetZaxis()->SetTitle("occupancy");
    theSSAscurveHistogram.fTheHistogram->SetStats(false);
    RootContainerFactory::bookChipHistograms<HistContainer<TH2F>>(theOutputFile, *fDetectorContainer, fSCurveContainer, theSSAscurveHistogram);
    fDetectorContainer->removeReadoutChipQueryFunction(selectSSAfunctionName);

    fDetectorContainer->addReadoutChipQueryFunction(selectMPAfunction, selectMPAfunctionName);
    HistContainer<TH2F> theMPAscurveHistogram("SCurve", "SCurve", NSSACHANNELS * NMPAROWS, -0.5, NSSACHANNELS * NMPAROWS - 0.5, 256, -0.5, 255.5);
    theMPAscurveHistogram.fTheHistogram->GetXaxis()->SetTitle("channel");
    theMPAscurveHistogram.fTheHistogram->GetYaxis()->SetTitle("threshold");
    theMPAscurveHistogram.fTheHistogram->GetZaxis()->SetTitle("occupancy");
    theMPAscurveHistogram.fTheHistogram->SetStats(false);
    RootContainerFactory::bookChipHistograms<HistContainer<TH2F>>(theOutputFile, *fDetectorContainer, fSCurveContainer, theMPAscurveHistogram);
    fDetectorContainer->removeReadoutChipQueryFunction(selectMPAfunctionName);
}

//========================================================================================================================
void DQMHistogramPSCounterTest::process()
{
    // This step it is not necessary, unless you want to format / draw histograms,
    // otherwise they will be automatically saved
}

//========================================================================================================================
void DQMHistogramPSCounterTest::reset(void)
{
    // Clear histograms if needed
}

//========================================================================================================================
void DQMHistogramPSCounterTest::fillSCurvePlots(uint16_t stripThreshold, uint16_t pixelThreshold, DetectorDataContainer& theOccupancyContainer)
{
    for(auto theBoard: theOccupancyContainer)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                for(auto theChip: *theHybrid)
                {
                    bool  isMPA         = theChip->getNumberOfRows() > 1 ? true : false;
                    TH2F* theChipSCurve = fSCurveContainer.getChip(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId(), theChip->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;
                    for(uint16_t row = 0; row < theChip->getNumberOfRows(); ++row)
                    {
                        for(uint16_t col = 0; col < theChip->getNumberOfCols(); ++col)
                        {
                            theChipSCurve->SetBinContent(
                                linearizeRowAndCols(row, col, theChip->getNumberOfCols()) + 1, (isMPA ? pixelThreshold : stripThreshold) + 1, theChip->getChannel<Occupancy>(row, col).fOccupancy);
                        }
                    }
                }
            }
        }
    }
}

//========================================================================================================================
bool DQMHistogramPSCounterTest::fill(std::string& inputStream)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR CALIBRATION YOU DO NOT NEED THE FOLLOWING COMMENTED LINES

    // As example, I'm expecting to receive a data stream from an uint32_t contained from calibration "PSCounterTest"
    ContainerSerialization myStreamer("PSCounterTest_SCurve");

    if(myStreamer.attachDeserializer(inputStream))
    {
        uint16_t stripThreshold, pixelThreshold;
        // Need to tell to the streamer what data are contained (in this case in every channel there is an object of type MyType)
        DetectorDataContainer theDetectorData = myStreamer.deserializeChipContainer<Occupancy, Occupancy>(fDetectorContainer, stripThreshold, pixelThreshold);
        // Filling the histograms
        fillSCurvePlots(stripThreshold, pixelThreshold, theDetectorData);
        return true;
    }
    // the stream does not match, the expected (DQM interface will try to check if other DQM istogrammers are looking
    //  for this stream)
    return false;
    // SoC utilities only - END
}
