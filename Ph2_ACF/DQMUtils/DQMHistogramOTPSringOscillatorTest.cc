#include "DQMUtils/DQMHistogramOTPSringOscillatorTest.h"
#include "HWDescription/ReadoutChip.h"
#include "RootUtils/RootContainerFactory.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/GenericDataArray.h"

#include "TFile.h"
#include "TH2I.h"

//========================================================================================================================
DQMHistogramOTPSringOscillatorTest::DQMHistogramOTPSringOscillatorTest() {}

//========================================================================================================================
DQMHistogramOTPSringOscillatorTest::~DQMHistogramOTPSringOscillatorTest() {}

//========================================================================================================================
void DQMHistogramOTPSringOscillatorTest::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR CALIBRATION YOU DO NOT NEED THE FOLLOWING COMMENTED LINES
    // make fDetectorContainer ready to receive the information fromm the stream
    fDetectorContainer = &theDetectorStructure;
    // SoC utilities only - END

    // Book MPA histograms
    auto        selectMPAfunction     = [](const ChipContainer* theChip) { return (static_cast<const Ph2_HwDescription::ReadoutChip*>(theChip)->getFrontEndType() == FrontEndType::MPA2); };
    std::string selectMPAfunctionName = "SelectMPAfunction";
    fDetectorContainer->addReadoutChipQueryFunction(selectMPAfunction, selectMPAfunctionName);

    auto formatMPAHistogram = [](TH2I* theHistogram)
    {
        theHistogram->SetStats(false);

        auto theXaxis = theHistogram->GetXaxis();
        theXaxis->SetTitle("MPA Id");
        for(int mpaId = 8; mpaId < 16; ++mpaId) { theXaxis->SetBinLabel(mpaId % 8 + 1, Form("%d", mpaId)); }

        auto theYaxis = theHistogram->GetYaxis();
        theYaxis->SetTitle("Ring oscillator");
        theYaxis->SetBinLabel(1, "Periphery");
        for(int row = 0; row < NMPAROWS; ++row) { theYaxis->SetBinLabel(row + 2, Form("Row%d", row)); }
    };

    HistContainer<TH2I> theMPAringOscillatorInverterHistogram("MPA_RingOscillatorInverterCounts", "MPA ring oscillator inverter counts", 8, -7.5, 15.5, NMPAROWS + 1, -0.5, NMPAROWS + 1 - 0.5);
    formatMPAHistogram(theMPAringOscillatorInverterHistogram.fTheHistogram);
    RootContainerFactory::bookHybridHistograms(theOutputFile, *fDetectorContainer, fMPAringOscillatorInverterContainer, theMPAringOscillatorInverterHistogram);

    HistContainer<TH2I> theMPAringOscillatorDelayHistogram("MPA_RingOscillatorDelayCounts", "MPA ring oscillator delay counts", 8, -7.5, 15.5, NMPAROWS + 1, -0.5, NMPAROWS + 1 - 0.5);
    formatMPAHistogram(theMPAringOscillatorDelayHistogram.fTheHistogram);
    RootContainerFactory::bookHybridHistograms(theOutputFile, *fDetectorContainer, fMPAringOscillatorDelayContainer, theMPAringOscillatorDelayHistogram);

    fDetectorContainer->removeReadoutChipQueryFunction(selectMPAfunctionName);

    // Book SSA histograms
    auto        selectSSAfunction     = [](const ChipContainer* theChip) { return (static_cast<const Ph2_HwDescription::ReadoutChip*>(theChip)->getFrontEndType() == FrontEndType::SSA2); };
    std::string selectSSAfunctionName = "SelectSSAfunction";
    fDetectorContainer->addReadoutChipQueryFunction(selectSSAfunction, selectSSAfunctionName);

    auto formatSSAHistogram = [](TH2I* theHistogram)
    {
        theHistogram->SetStats(false);

        auto theXaxis = theHistogram->GetXaxis();
        theXaxis->SetTitle("SSA Id");
        for(int ssaId = 0; ssaId < 8; ++ssaId) { theXaxis->SetBinLabel(ssaId + 1, Form("%d", ssaId)); }

        auto theYaxis = theHistogram->GetYaxis();
        theYaxis->SetTitle("Ring oscillator");
        theYaxis->SetBinLabel(1, "BL");
        theYaxis->SetBinLabel(2, "BC");
        theYaxis->SetBinLabel(3, "BR");
        theYaxis->SetBinLabel(4, "TR");
    };

    HistContainer<TH2I> theSSAringOscillatorInverterHistogram("SSA_RingOscillatorInverterCounts", "SSA ring oscillator inverter counts", 8, -7.5, 15.5, 4, -0.5, 3.5);
    formatSSAHistogram(theSSAringOscillatorInverterHistogram.fTheHistogram);
    RootContainerFactory::bookHybridHistograms(theOutputFile, *fDetectorContainer, fSSAringOscillatorInverterContainer, theSSAringOscillatorInverterHistogram);

    HistContainer<TH2I> theSSAringOscillatorDelayHistogram("SSA_RingOscillatorDelayCounts", "SSA ring oscillator delay counts", 8, -7.5, 15.5, 4, -0.5, 3.5);
    formatSSAHistogram(theSSAringOscillatorDelayHistogram.fTheHistogram);
    RootContainerFactory::bookHybridHistograms(theOutputFile, *fDetectorContainer, fSSAringOscillatorDelayContainer, theSSAringOscillatorDelayHistogram);

    fDetectorContainer->removeReadoutChipQueryFunction(selectSSAfunctionName);
}

//========================================================================================================================
void DQMHistogramOTPSringOscillatorTest::process()
{
    // This step it is not necessary, unless you want to format / draw histograms,
    // otherwise they will be automatically saved
}

//========================================================================================================================
void DQMHistogramOTPSringOscillatorTest::reset(void)
{
    // Clear histograms if needed
}

//========================================================================================================================
void DQMHistogramOTPSringOscillatorTest::fillMPAringOscillator(const DetectorDataContainer& theRingOscillatorContainer, DetectorDataContainer& theMPAringOscillatorContainer)
{
    for(auto theBoard: theRingOscillatorContainer)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                auto theRingOscillatorHistogram =
                    theMPAringOscillatorContainer.getHybrid(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId())->getSummary<HistContainer<TH2I>>().fTheHistogram;

                for(auto theChip: *theHybrid)
                {
                    if(!theChip->hasSummary()) continue;
                    auto theRingOscillatorChipContainer = theChip->getSummary<GenericDataArray<uint16_t, NMPAROWS + 1>>();
                    for(int bin = 0; bin < NMPAROWS + 1; ++bin) { theRingOscillatorHistogram->SetBinContent(theChip->getId() % 8 + 1, bin + 1, theRingOscillatorChipContainer.at(bin)); }
                }
            }
        }
    }
}

//========================================================================================================================
void DQMHistogramOTPSringOscillatorTest::fillMPAringOscillatorInverter(const DetectorDataContainer& theRingOscillatorContainer)
{
    fillMPAringOscillator(theRingOscillatorContainer, fMPAringOscillatorInverterContainer);
}

//========================================================================================================================
void DQMHistogramOTPSringOscillatorTest::fillMPAringOscillatorDelay(const DetectorDataContainer& theRingOscillatorContainer)
{
    fillMPAringOscillator(theRingOscillatorContainer, fMPAringOscillatorDelayContainer);
}

//========================================================================================================================
void DQMHistogramOTPSringOscillatorTest::fillSSAringOscillator(const DetectorDataContainer& theRingOscillatorContainer, DetectorDataContainer& theSSAringOscillatorContainer)
{
    for(auto theBoard: theRingOscillatorContainer)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                auto theRingOscillatorHistogram =
                    theSSAringOscillatorContainer.getHybrid(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId())->getSummary<HistContainer<TH2I>>().fTheHistogram;

                for(auto theChip: *theHybrid)
                {
                    if(!theChip->hasSummary()) continue;
                    auto theRingOscillatorChipContainer = theChip->getSummary<GenericDataArray<uint16_t, 4>>();
                    for(int bin = 0; bin < 4; ++bin) { theRingOscillatorHistogram->SetBinContent(theChip->getId() % 8 + 1, bin + 1, theRingOscillatorChipContainer.at(bin)); }
                }
            }
        }
    }
}

//========================================================================================================================
void DQMHistogramOTPSringOscillatorTest::fillSSAringOscillatorInverter(const DetectorDataContainer& theRingOscillatorContainer)
{
    fillSSAringOscillator(theRingOscillatorContainer, fSSAringOscillatorInverterContainer);
}

//========================================================================================================================
void DQMHistogramOTPSringOscillatorTest::fillSSAringOscillatorDelay(const DetectorDataContainer& theRingOscillatorContainer)
{
    fillSSAringOscillator(theRingOscillatorContainer, fSSAringOscillatorDelayContainer);
}

//========================================================================================================================
bool DQMHistogramOTPSringOscillatorTest::fill(std::string& inputStream)
{
    ContainerSerialization theMPAringOscillatorInverterSerialization("OTPSringOscillatorTestMPAringOscillatorInverter");
    ContainerSerialization theMPAringOscillatorDelaySerialization("OTPSringOscillatorTestMPAringOscillatorDelay");

    ContainerSerialization theSSAringOscillatorInverterSerialization("OTPSringOscillatorTestSSAringOscillatorInverter");
    ContainerSerialization theSSAringOscillatorDelaySerialization("OTPSringOscillatorTestSSAringOscillatorDelay");

    if(theMPAringOscillatorInverterSerialization.attachDeserializer(inputStream))
    {
        DetectorDataContainer theDetectorData =
            theMPAringOscillatorInverterSerialization.deserializeHybridContainer<EmptyContainer, EmptyContainer, GenericDataArray<uint16_t, NMPAROWS + 1>>(fDetectorContainer);
        fillMPAringOscillatorInverter(theDetectorData);
        return true;
    }
    if(theMPAringOscillatorDelaySerialization.attachDeserializer(inputStream))
    {
        DetectorDataContainer theDetectorData =
            theMPAringOscillatorDelaySerialization.deserializeHybridContainer<EmptyContainer, EmptyContainer, GenericDataArray<uint16_t, NMPAROWS + 1>>(fDetectorContainer);
        fillMPAringOscillatorDelay(theDetectorData);
        return true;
    }
    if(theSSAringOscillatorInverterSerialization.attachDeserializer(inputStream))
    {
        DetectorDataContainer theDetectorData = theSSAringOscillatorInverterSerialization.deserializeHybridContainer<EmptyContainer, EmptyContainer, GenericDataArray<uint16_t, 4>>(fDetectorContainer);
        fillSSAringOscillatorInverter(theDetectorData);
        return true;
    }
    if(theSSAringOscillatorDelaySerialization.attachDeserializer(inputStream))
    {
        DetectorDataContainer theDetectorData = theSSAringOscillatorDelaySerialization.deserializeHybridContainer<EmptyContainer, EmptyContainer, GenericDataArray<uint16_t, 4>>(fDetectorContainer);
        fillSSAringOscillatorDelay(theDetectorData);
        return true;
    }

    return false;
    // SoC utilities only - END
}
