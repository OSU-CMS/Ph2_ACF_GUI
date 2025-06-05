#include "DQMUtils/DQMHistogramOTCICphaseAlignment.h"
#include "HWDescription/Definition.h"
#include "RootUtils/RootContainerFactory.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/GenericDataArray.h"

#include "TFile.h"
#include "TH2F.h"
#include "TH2I.h"

//========================================================================================================================
DQMHistogramOTCICphaseAlignment::DQMHistogramOTCICphaseAlignment() {}

//========================================================================================================================
DQMHistogramOTCICphaseAlignment::~DQMHistogramOTCICphaseAlignment() {}

//========================================================================================================================
void DQMHistogramOTCICphaseAlignment::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
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
        chipName   = "MPA";
        xAxisTitle = "MPA Id";
        idOffset   = 8;
    }

    auto setLineBinLabels = [](TAxis* theHistogramAxis)
    {
        theHistogramAxis->SetBinLabel(1, "L1");
        for(int line = 1; line < NUMBER_OF_LINES_PER_CIC_PORTS; ++line) { theHistogramAxis->SetBinLabel(line + 1, Form("Stub%d", line - 1)); }
    };

    auto setPhaseHistogramBinLabels = [isPS](TAxis* theHistogramAxis)
    {
        for(int port = 0; port < NUMBER_OF_CIC_PORTS; ++port)
        {
            for(int line = 0; line < NUMBER_OF_LINES_PER_CIC_PORTS; ++line)
            {
                std::string binLabel;
                if(isPS) { binLabel = "MPA" + std::to_string(port + 8); }
                else { binLabel = "CBC" + std::to_string(port); }
                if(line == 0) { binLabel += "_L1"; }
                else
                {
                    binLabel += "_Stub";
                    binLabel += std::to_string(line - 1);
                }

                theHistogramAxis->SetBinLabel(port * NUMBER_OF_LINES_PER_CIC_PORTS + line + 1, binLabel.c_str());
            }
        }
    };

    HistContainer<TH2F> phaseHistogram((chipName + "toCIC_InputPhaseDistribution").c_str(),
                                       (chipName + " to CIC input phase distribution").c_str(),
                                       NUMBER_OF_CIC_PORTS * NUMBER_OF_LINES_PER_CIC_PORTS,
                                       -0.5,
                                       NUMBER_OF_CIC_PORTS * NUMBER_OF_LINES_PER_CIC_PORTS - 0.5,
                                       16,
                                       -0.5,
                                       16 - 0.5);
    setPhaseHistogramBinLabels(phaseHistogram.fTheHistogram->GetXaxis());
    phaseHistogram.fTheHistogram->GetYaxis()->SetTitle("Phase");
    phaseHistogram.fTheHistogram->SetMinimum(0);
    phaseHistogram.fTheHistogram->SetMaximum(1);
    phaseHistogram.fTheHistogram->SetStats(false);
    RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fPhaseHistogramContainer, phaseHistogram);

    HistContainer<TH2I> bestPhaseHistogram((chipName + "toCIC_BestInputPhases").c_str(),
                                           (chipName + " to CIC best input phases").c_str(),
                                           NUMBER_OF_CIC_PORTS,
                                           idOffset - 0.5,
                                           idOffset + NUMBER_OF_CIC_PORTS - 0.5,
                                           NUMBER_OF_LINES_PER_CIC_PORTS,
                                           -0.5,
                                           NUMBER_OF_LINES_PER_CIC_PORTS - 0.5);
    bestPhaseHistogram.fTheHistogram->GetXaxis()->SetTitle(xAxisTitle.c_str());
    setLineBinLabels(bestPhaseHistogram.fTheHistogram->GetYaxis());
    bestPhaseHistogram.fTheHistogram->SetMinimum(0);
    bestPhaseHistogram.fTheHistogram->SetMaximum(15);
    bestPhaseHistogram.fTheHistogram->SetStats(false);
    RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fBestPhaseHistogramContainer, bestPhaseHistogram);

    HistContainer<TH2F> lockingEfficiencyHistogram((chipName + "toCIC_LockingEfficiency").c_str(),
                                                   (chipName + " to CIC locking efficiency").c_str(),
                                                   NUMBER_OF_CIC_PORTS,
                                                   idOffset - 0.5,
                                                   idOffset + NUMBER_OF_CIC_PORTS - 0.5,
                                                   NUMBER_OF_LINES_PER_CIC_PORTS,
                                                   -0.5,
                                                   NUMBER_OF_LINES_PER_CIC_PORTS - 0.5);
    lockingEfficiencyHistogram.fTheHistogram->GetXaxis()->SetTitle(xAxisTitle.c_str());
    setLineBinLabels(lockingEfficiencyHistogram.fTheHistogram->GetYaxis());
    lockingEfficiencyHistogram.fTheHistogram->SetMinimum(0);
    lockingEfficiencyHistogram.fTheHistogram->SetMaximum(1);
    lockingEfficiencyHistogram.fTheHistogram->SetStats(false);
    RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fLockingEfficiencyHistogramContainer, lockingEfficiencyHistogram);
}

//========================================================================================================================
void DQMHistogramOTCICphaseAlignment::fillPhaseHistogramResults(DetectorDataContainer& thePhaseHistogramResultContainer)
{
    for(auto board: thePhaseHistogramResultContainer)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                if(!hybrid->hasSummary()) continue;

                auto thePhaseHistogramVector = hybrid->getSummary<GenericDataArray<float, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS, 16>>();

                TH2F* bestPhaseHistogram =
                    fPhaseHistogramContainer.getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;

                for(size_t chipId = 0; chipId < NUMBER_OF_CIC_PORTS; ++chipId) // not using the chipID because I want always to read all phases
                {
                    for(size_t cLineId = 0; cLineId < NUMBER_OF_LINES_PER_CIC_PORTS; cLineId++)
                    {
                        for(size_t phase = 0; phase < 16; phase++)
                        {
                            bestPhaseHistogram->SetBinContent(chipId * NUMBER_OF_LINES_PER_CIC_PORTS + cLineId + 1, phase + 1, thePhaseHistogramVector.at(chipId).at(cLineId).at(phase));
                        }
                    }
                }
            }
        }
    }
}

//========================================================================================================================
void DQMHistogramOTCICphaseAlignment::fillBestPhaseResults(DetectorDataContainer& thePhaseAlignmentResultContainer)
{
    for(auto board: thePhaseAlignmentResultContainer)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                if(!hybrid->hasSummary()) continue;

                auto theBestPhaseVector = hybrid->getSummary<GenericDataArray<uint8_t, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS>>();

                TH2I* bestPhaseHistogram =
                    fBestPhaseHistogramContainer.getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getSummary<HistContainer<TH2I>>().fTheHistogram;

                for(size_t chipId = 0; chipId < NUMBER_OF_CIC_PORTS; ++chipId) // not using the chipID because I want always to read all phases
                {
                    for(size_t cLineId = 0; cLineId < NUMBER_OF_LINES_PER_CIC_PORTS; cLineId++)
                    {
                        bestPhaseHistogram->SetBinContent(chipId + 1, cLineId + 1, theBestPhaseVector.at(chipId).at(cLineId));
                    }
                }
            }
        }
    }
}

//========================================================================================================================
void DQMHistogramOTCICphaseAlignment::fillLockingEfficiencyResults(DetectorDataContainer& theLockingEfficiencyContainer)
{
    for(auto board: theLockingEfficiencyContainer)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                if(!hybrid->hasSummary()) continue;

                auto theLockingEfficiencyVector = hybrid->getSummary<GenericDataArray<float, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS>>();

                TH2F* lockingEfficiencyHistogram =
                    fLockingEfficiencyHistogramContainer.getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;

                for(size_t chipId = 0; chipId < NUMBER_OF_CIC_PORTS; ++chipId) // not using the chipID because I want always to read all phases
                {
                    for(size_t cLineId = 0; cLineId < NUMBER_OF_LINES_PER_CIC_PORTS; cLineId++)
                    {
                        lockingEfficiencyHistogram->SetBinContent(chipId + 1, cLineId + 1, theLockingEfficiencyVector.at(chipId).at(cLineId));
                    }
                }
            }
        }
    }
}

//========================================================================================================================
void DQMHistogramOTCICphaseAlignment::process()
{
    // This step it is not necessary, unless you want to format / draw histograms,
    // otherwise they will be automatically saved
}

//========================================================================================================================
void DQMHistogramOTCICphaseAlignment::reset(void)
{
    // Clear histograms if needed
}

//========================================================================================================================
bool DQMHistogramOTCICphaseAlignment::fill(std::string& inputStream)
{
    // SoC utilities only - BEGIN
    ContainerSerialization thePhaseHistogramContainerSerialization("OTCICphaseAlignmentPhaseHistogram");
    ContainerSerialization theBestPhaseContainerSerialization("OTCICphaseAlignmentBestPhase");
    ContainerSerialization theLockingEfficiencyContainerSerialization("OTCICphaseAlignmentLockingEfficiency");

    if(thePhaseHistogramContainerSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched OTCICphaseAlignment PhaseHistogram!!!!\n";
        DetectorDataContainer theDetectorData =
            thePhaseHistogramContainerSerialization
                .deserializeOpticalGroupContainer<EmptyContainer, EmptyContainer, GenericDataArray<float, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS, 16>, EmptyContainer>(fDetectorContainer);
        fillPhaseHistogramResults(theDetectorData);
        return true;
    }
    if(theBestPhaseContainerSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched OTCICphaseAlignment BestPhase!!!!\n";
        DetectorDataContainer theDetectorData =
            theBestPhaseContainerSerialization
                .deserializeOpticalGroupContainer<EmptyContainer, EmptyContainer, GenericDataArray<uint8_t, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS>, EmptyContainer>(fDetectorContainer);
        fillBestPhaseResults(theDetectorData);
        return true;
    }
    if(theLockingEfficiencyContainerSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched OTCICphaseAlignment LockingEfficiency!!!!\n";
        DetectorDataContainer theDetectorData =
            theLockingEfficiencyContainerSerialization
                .deserializeOpticalGroupContainer<EmptyContainer, EmptyContainer, GenericDataArray<float, NUMBER_OF_CIC_PORTS, NUMBER_OF_LINES_PER_CIC_PORTS>, EmptyContainer>(fDetectorContainer);
        fillLockingEfficiencyResults(theDetectorData);
        return true;
    }

    return false;
    // SoC utilities only - END
}
