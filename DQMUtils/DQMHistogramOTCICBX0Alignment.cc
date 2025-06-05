#include "DQMUtils/DQMHistogramOTCICBX0Alignment.h"
#include "RootUtils/RootContainerFactory.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"

#include "TAxis.h"
#include "TFile.h"
#include "TH1I.h"

//========================================================================================================================
DQMHistogramOTCICBX0Alignment::DQMHistogramOTCICBX0Alignment() {}

//========================================================================================================================
DQMHistogramOTCICBX0Alignment::~DQMHistogramOTCICBX0Alignment() {}

//========================================================================================================================
void DQMHistogramOTCICBX0Alignment::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR CALIBRATION YOU DO NOT NEED THE FOLLOWING COMMENTED LINES
    // make fDetectorContainer ready to receive the information fromm the stream
    fDetectorContainer = &theDetectorStructure;
    // SoC utilities only - END
    HistContainer<TH1I> BX0AlignmentDelayHistogram("CICBX0AlignmentDelay", "CIC BX0 Alignment Delay", 1, 0, 1);
    BX0AlignmentDelayHistogram.fTheHistogram->GetXaxis()->SetTitle();
    BX0AlignmentDelayHistogram.fTheHistogram->GetYaxis()->SetTitle("Delay");
    BX0AlignmentDelayHistogram.fTheHistogram->SetMinimum(-1);
    BX0AlignmentDelayHistogram.fTheHistogram->SetMaximum(32);
    BX0AlignmentDelayHistogram.fTheHistogram->SetStats(false);
    RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fBX0AlignmentDelayHistogramContainer, BX0AlignmentDelayHistogram);

    HistContainer<TH1I> BX0AlignmentDelayVsRetimePixHistogram("CICBX0AlignmentDelayVsRetimePix", "CIC BX0 Alignment Delay Vs RetimePix", 8, -0.5, 7.5);
    BX0AlignmentDelayVsRetimePixHistogram.fTheHistogram->GetXaxis()->SetTitle("RetimePix");
    BX0AlignmentDelayVsRetimePixHistogram.fTheHistogram->GetYaxis()->SetTitle("Delay");
    BX0AlignmentDelayVsRetimePixHistogram.fTheHistogram->SetMinimum(-1);
    BX0AlignmentDelayVsRetimePixHistogram.fTheHistogram->SetMaximum(32);
    BX0AlignmentDelayVsRetimePixHistogram.fTheHistogram->SetStats(false);
    RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fBX0AlignmentDelayVsRetimePixHistogramContainer, BX0AlignmentDelayVsRetimePixHistogram);

    // Initialize to -1 to avoid confusing no entry and entry=0;
    for(auto board: fBX0AlignmentDelayHistogramContainer)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                TH1I* BX0AlignmentDelayPhaseHistogram = hybrid->getSummary<HistContainer<TH1I>>().fTheHistogram;

                BX0AlignmentDelayPhaseHistogram->SetBinContent(0, -1);
            }
        }
    }
}

//========================================================================================================================
void DQMHistogramOTCICBX0Alignment::fillBX0AlignmentDelay(DetectorDataContainer& theBX0AlignmentDelayContainer)
{
    for(auto board: theBX0AlignmentDelayContainer)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                if(!hybrid->hasSummary()) continue;

                auto theBX0AlignmentDelay = hybrid->getSummary<uint16_t>();

                TH1I* BX0AlignmentDelayHistogram =
                    fBX0AlignmentDelayHistogramContainer.getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getSummary<HistContainer<TH1I>>().fTheHistogram;
                BX0AlignmentDelayHistogram->SetBinContent(1, theBX0AlignmentDelay);
            }
        }
    }
}

//========================================================================================================================
void DQMHistogramOTCICBX0Alignment::fillBX0AlignmentDelayVsRetimePix(DetectorDataContainer& theBX0AlignmentDelayVsRetimePixContainer)
{
    for(auto board: theBX0AlignmentDelayVsRetimePixContainer)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                if(!hybrid->hasSummary()) continue;

                auto theBX0AlignmentDelayVsRetimePix = hybrid->getSummary<std::vector<uint16_t>>();

                TH1I* BX0AlignmentDelayHistogram = fBX0AlignmentDelayVsRetimePixHistogramContainer.getObject(board->getId())
                                                       ->getObject(opticalGroup->getId())
                                                       ->getObject(hybrid->getId())
                                                       ->getSummary<HistContainer<TH1I>>()
                                                       .fTheHistogram;

                for(size_t retimepix = 0; retimepix < theBX0AlignmentDelayVsRetimePix.size(); retimepix++)
                    BX0AlignmentDelayHistogram->SetBinContent(retimepix + 1, theBX0AlignmentDelayVsRetimePix[retimepix]);
            }
        }
    }
}

//========================================================================================================================
void DQMHistogramOTCICBX0Alignment::process()
{
    // This step it is not necessary, unless you want to format / draw histograms,
    // otherwise they will be automatically saved
}

//========================================================================================================================
void DQMHistogramOTCICBX0Alignment::reset(void)
{
    // Clear histograms if needed
}

//========================================================================================================================
bool DQMHistogramOTCICBX0Alignment::fill(std::string& inputStream)
{
    // SoC utilities only - BEGIN
    ContainerSerialization theBX0AlignmentDelayContainerSerialization("OTCICBX0AlignmentBX0AlignmentDelay");
    ContainerSerialization theBX0AlignmentDelayVsRetimePixContainerSerialization("OTCICBX0AlignmentBX0AlignmentDelayVsRetimePix");

    if(theBX0AlignmentDelayContainerSerialization.attachDeserializer(inputStream))
    {
        // It matched! Decoding data
        // std::cout << "Matched OTCICBX0Alignment  BX0AlignmentDelay!!!!!\n";
        DetectorDataContainer theDetectorData =
            theBX0AlignmentDelayContainerSerialization.deserializeOpticalGroupContainer<EmptyContainer, EmptyContainer, uint16_t, EmptyContainer>(fDetectorContainer);
        // Filling the histograms
        fillBX0AlignmentDelay(theDetectorData);
        return true;
    }

    if(theBX0AlignmentDelayVsRetimePixContainerSerialization.attachDeserializer(inputStream))
    {
        // It matched! Decoding data
        // std::cout << "Matched OTCICBX0Alignment  BX0AlignmentDelay VsRetimePix!!!!!\n";
        DetectorDataContainer theDetectorData =
            theBX0AlignmentDelayVsRetimePixContainerSerialization.deserializeOpticalGroupContainer<EmptyContainer, EmptyContainer, std::vector<uint16_t>, EmptyContainer>(fDetectorContainer);
        // Filling the histograms
        fillBX0AlignmentDelayVsRetimePix(theDetectorData);
        return true;
    }
    // the stream does not match, the expected (DQM interface will try to check if other DQM istogrammers are looking
    //  for this stream)
    return false;
    // SoC utilities only - END
}