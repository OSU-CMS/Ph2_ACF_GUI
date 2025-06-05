#include "DQMUtils/DQMHistogramOTCICwordAlignment.h"
#include "RootUtils/RootContainerFactory.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/GenericDataArray.h"

#include "TAxis.h"
#include "TFile.h"
#include "TH2I.h"

//========================================================================================================================
DQMHistogramOTCICwordAlignment::DQMHistogramOTCICwordAlignment() {}

//========================================================================================================================
DQMHistogramOTCICwordAlignment::~DQMHistogramOTCICwordAlignment() {}

//========================================================================================================================
void DQMHistogramOTCICwordAlignment::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR CALIBRATION YOU DO NOT NEED THE FOLLOWING COMMENTED LINES
    // make fDetectorContainer ready to receive the information fromm the stream
    fDetectorContainer = &theDetectorStructure;
    // SoC utilities only - END

    bool        isPS       = theDetectorStructure.getFirstObject()->getFirstObject()->getFrontEndType() == FrontEndType::OuterTrackerPS;
    std::string chipName   = "CBC";
    std::string xAxisTitle = "CBC Id";
    int         idOffset   = 0;
    if(isPS)
    {
        xAxisTitle = "MPA Id";
        idOffset   = 8;
        chipName   = "MPA";
    }

    auto setLineBinLabels = [](TAxis* theHistogramAxis)
    {
        for(int line = 0; line < NUMBER_OF_LINES_PER_CIC_PORTS - 1; ++line) { theHistogramAxis->SetBinLabel(line + 1, Form("Stub%d", line)); }
    };

    HistContainer<TH2I> wordAlignmentDelayHistogram((chipName + "toCIC_WordAlignmentDelay").c_str(),
                                                    (chipName + " to CIC word alignment delay").c_str(),
                                                    NUMBER_OF_CIC_PORTS,
                                                    idOffset - 0.5,
                                                    idOffset + NUMBER_OF_CIC_PORTS - 0.5,
                                                    NUMBER_OF_LINES_PER_CIC_PORTS - 1,
                                                    -0.5,
                                                    NUMBER_OF_LINES_PER_CIC_PORTS - 1 - 0.5);
    wordAlignmentDelayHistogram.fTheHistogram->GetXaxis()->SetTitle(xAxisTitle.c_str());
    setLineBinLabels(wordAlignmentDelayHistogram.fTheHistogram->GetYaxis());
    wordAlignmentDelayHistogram.fTheHistogram->SetMinimum(-1);
    wordAlignmentDelayHistogram.fTheHistogram->SetMaximum(15);
    wordAlignmentDelayHistogram.fTheHistogram->SetStats(false);
    RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fWordAlignmentDelayHistogramContainer, wordAlignmentDelayHistogram);

    // Initialize to -1 to avoid confusing no entry and entry=0;
    for(auto board: fWordAlignmentDelayHistogramContainer)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                TH2I* wordAlignmentDelaPhaseHistogram = hybrid->getSummary<HistContainer<TH2I>>().fTheHistogram;
                for(size_t chipId = 0; chipId < NUMBER_OF_CIC_PORTS; ++chipId) // not using the chipID because I want always to read all phases
                {
                    for(size_t cLineId = 0; cLineId < NUMBER_OF_LINES_PER_CIC_PORTS - 1; cLineId++) { wordAlignmentDelaPhaseHistogram->SetBinContent(chipId + 1, cLineId + 1, -1); }
                }
            }
        }
    }
}
//========================================================================================================================
void DQMHistogramOTCICwordAlignment::fillWordAlignmentDelay(DetectorDataContainer& theWordAlignmentDelayContainer)
{
    for(auto board: theWordAlignmentDelayContainer)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                if(!hybrid->hasSummary()) continue;

                auto theWordAlignmentDelayVector = hybrid->getSummary<GenericDataArray<uint8_t, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS - 1>>();

                TH2I* wordAlignmentDelaPhaseHistogram =
                    fWordAlignmentDelayHistogramContainer.getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getSummary<HistContainer<TH2I>>().fTheHistogram;

                for(size_t chipId = 0; chipId < NUMBER_OF_CIC_PORTS; ++chipId) // not using the chipID because I want always to read all phases
                {
                    for(size_t cLineId = 0; cLineId < NUMBER_OF_LINES_PER_CIC_PORTS - 1; cLineId++)
                    {
                        wordAlignmentDelaPhaseHistogram->SetBinContent(chipId + 1, cLineId + 1, theWordAlignmentDelayVector.at(chipId).at(cLineId));
                    }
                }
            }
        }
    }
}

//========================================================================================================================
void DQMHistogramOTCICwordAlignment::process()
{
    // This step it is not necessary, unless you want to format / draw histograms,
    // otherwise they will be automatically saved
}

//========================================================================================================================
void DQMHistogramOTCICwordAlignment::reset(void)
{
    // Clear histograms if needed
}

//========================================================================================================================
bool DQMHistogramOTCICwordAlignment::fill(std::string& inputStream)
{
    // SoC utilities only - BEGIN
    ContainerSerialization theWordAlignmentDelayContainerSerialization("OTCICwordAlignmentWordAlignmentDelay");

    if(theWordAlignmentDelayContainerSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched OTCICwordAlignment WordAlignmentDelay!!!!\n";
        DetectorDataContainer theDetectorData =
            theWordAlignmentDelayContainerSerialization
                .deserializeOpticalGroupContainer<EmptyContainer, EmptyContainer, GenericDataArray<uint8_t, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS - 1>, EmptyContainer>(
                    fDetectorContainer);
        fillWordAlignmentDelay(theDetectorData);
        return true;
    }

    return false;
    // SoC utilities only - END
}
