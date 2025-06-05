#include "DQMUtils/DQMHistogramOTChipToCICecv.h"
#include "RootUtils/RootContainerFactory.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/GenericDataArray.h"

#include "TFile.h"
#include "TH2F.h"

//========================================================================================================================
DQMHistogramOTChipToCICecv::DQMHistogramOTChipToCICecv() {}

//========================================================================================================================
DQMHistogramOTChipToCICecv::~DQMHistogramOTChipToCICecv() {}

//========================================================================================================================
void DQMHistogramOTChipToCICecv::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR CALIBRATION YOU DO NOT NEED THE FOLLOWING COMMENTED LINES
    // make fDetectorContainer ready to receive the information fromm the stream
    fDetectorContainer = &theDetectorStructure;
    // SoC utilities only - END

    uint8_t numberOfPhases = 15; // 0 to 14

    bool isPS = fDetectorContainer->getFirstObject()->getFirstObject()->getFrontEndType() == FrontEndType::OuterTrackerPS;

    std::string nameOfListOfCurrentSetting;
    std::string defaultListOfCurrentSetting;
    std::string chipName;
    if(isPS)
    {
        nameOfListOfCurrentSetting  = "OTChipToCICecv_ListOfMPAslvsCurrents";
        defaultListOfCurrentSetting = "1, 4, 7";
        chipName                    = "MPA";
    }
    else
    {
        nameOfListOfCurrentSetting  = "OTChipToCICecv_ListOfCBCslvsCurrents";
        defaultListOfCurrentSetting = "0, 8, 14";
        chipName                    = "CBC";
    }

    std::vector<float> listOfSlvsCurrents = convertStringToFloatList(findValueInSettings<std::string>(pSettingsMap, nameOfListOfCurrentSetting, defaultListOfCurrentSetting));

    auto setYaxisLabel = [&chipName](TAxis* theYaxis)
    {
        for(uint8_t chipId = 0; chipId < NUMBER_OF_CIC_PORTS; ++chipId)
        {
            for(uint8_t stub = 0; stub < NUMBER_OF_LINES_PER_CIC_PORTS; ++stub)
            {
                if(stub == 0)
                    theYaxis->SetBinLabel(6 * chipId + stub + 1, Form("%s%d_L1", chipName.c_str(), chipId));
                else
                    theYaxis->SetBinLabel(6 * chipId + stub + 1, Form("%s%d_Stub%d", chipName.c_str(), chipId, stub - 1));
            }
        }
    };

    for(auto slvsCurrent: listOfSlvsCurrents)
    {
        HistContainer<TH2F> phaseScanMatchingErrorRate(Form("%stoCIC_PhaseScanErrorRate_%s_SLVScurrent_%d", chipName.c_str(), chipName.c_str(), int(slvsCurrent)),
                                                       Form("%s to CIC phase scan error rate %s SLVS current %d", chipName.c_str(), chipName.c_str(), int(slvsCurrent)),
                                                       numberOfPhases,
                                                       -0.5,
                                                       numberOfPhases - 0.5,
                                                       NUMBER_OF_CIC_PORTS * NUMBER_OF_LINES_PER_CIC_PORTS, // y-axis
                                                       0,
                                                       NUMBER_OF_CIC_PORTS * NUMBER_OF_LINES_PER_CIC_PORTS);
        phaseScanMatchingErrorRate.fTheHistogram->GetYaxis()->SetTitle("");
        phaseScanMatchingErrorRate.fTheHistogram->GetXaxis()->SetTitle("Phase");
        setYaxisLabel(phaseScanMatchingErrorRate.fTheHistogram->GetYaxis());
        phaseScanMatchingErrorRate.fTheHistogram->SetMinimum(0);
        phaseScanMatchingErrorRate.fTheHistogram->SetMaximum(1);
        phaseScanMatchingErrorRate.fTheHistogram->SetStats(false);
        RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fErrorRateContainerMap[slvsCurrent], phaseScanMatchingErrorRate);

        HistContainer<TH2F> phaseScanMatchingTestedBits(Form("%stoCIC_PhaseScanTestedBits_%s_SLVScurrent_%d", chipName.c_str(), chipName.c_str(), int(slvsCurrent)),
                                                        Form("%s to CIC phase scan tested bits %s SLVS current %d", chipName.c_str(), chipName.c_str(), int(slvsCurrent)),
                                                        numberOfPhases,
                                                        -0.5,
                                                        numberOfPhases - 0.5,
                                                        NUMBER_OF_CIC_PORTS * NUMBER_OF_LINES_PER_CIC_PORTS, // y-axis
                                                        0,
                                                        NUMBER_OF_CIC_PORTS * NUMBER_OF_LINES_PER_CIC_PORTS);
        phaseScanMatchingTestedBits.fTheHistogram->GetYaxis()->SetTitle("");
        phaseScanMatchingTestedBits.fTheHistogram->GetXaxis()->SetTitle("Phase");
        setYaxisLabel(phaseScanMatchingTestedBits.fTheHistogram->GetYaxis());
        phaseScanMatchingTestedBits.fTheHistogram->SetStats(false);
        RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fTestedBitsContainerMap[slvsCurrent], phaseScanMatchingTestedBits);
    }
}

//========================================================================================================================
void DQMHistogramOTChipToCICecv::process()
{
    // This step it is not necessary, unless you want to format / draw histograms,
    // otherwise they will be automatically saved
}

//========================================================================================================================
void DQMHistogramOTChipToCICecv::reset(void)
{
    // Clear histograms if needed
}

//========================================================================================================================
void DQMHistogramOTChipToCICecv::fillPhaseScanMatchingEfficiency(DetectorDataContainer& matchingEfficiencyContainer, uint8_t cicPhase, uint8_t slvsCurrent)
{
    for(auto theBoard: matchingEfficiencyContainer)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                auto theErrorRateHistogram =
                    fErrorRateContainerMap[slvsCurrent].getHybrid(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;
                auto theTestedBitsHistogram =
                    fTestedBitsContainerMap[slvsCurrent].getHybrid(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;

                for(auto theChip: *theHybrid)
                {
                    if(!theChip->hasSummary()) continue;
                    auto theChipLineTestedBitsAndErrorRate = theChip->getSummary<GenericDataArray<float, 6, 2>>();
                    for(uint8_t line = 0; line < NUMBER_OF_LINES_PER_CIC_PORTS; ++line)
                    {
                        auto testedBits = theChipLineTestedBitsAndErrorRate.at(line).at(0);
                        auto errorRate  = testedBits > 0 ? theChipLineTestedBitsAndErrorRate.at(line).at(1) / testedBits : 1.;
                        theErrorRateHistogram->SetBinContent(cicPhase + 1, (theChip->getId() % 8) * 6 + line + 1, errorRate);
                        theTestedBitsHistogram->SetBinContent(cicPhase + 1, (theChip->getId() % 8) * 6 + line + 1, testedBits);
                    }
                }
            }
        }
    }
}

//========================================================================================================================
bool DQMHistogramOTChipToCICecv::fill(std::string& inputStream)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR CALIBRATION YOU DO NOT NEED THE FOLLOWING COMMENTED LINES

    ContainerSerialization theMatchingEfficiencySerialization("OTChipToCICecvPhaseScanMatchingEfficiency");

    if(theMatchingEfficiencySerialization.attachDeserializer(inputStream))
    {
        uint8_t               cicPhase, slvsCurrent;
        DetectorDataContainer theDetectorData =
            theMatchingEfficiencySerialization.deserializeHybridContainer<EmptyContainer, EmptyContainer, GenericDataArray<float, 6, 2>>(fDetectorContainer, cicPhase, slvsCurrent);
        fillPhaseScanMatchingEfficiency(theDetectorData, cicPhase, slvsCurrent);
        return true;
    }
    return false;

    return false;
    // SoC utilities only - END
}
