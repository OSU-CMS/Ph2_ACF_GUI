#include "DQMUtils/DQMHistogramOTCBCtoCICecv.h"
#include "HWDescription/ReadoutChip.h"
#include "RootUtils/RootContainerFactory.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/GenericDataArray.h"

#include "TFile.h"
#include "TH1F.h"
#include "TH2F.h"

using namespace Ph2_HwDescription;

//========================================================================================================================
DQMHistogramOTCBCtoCICecv::DQMHistogramOTCBCtoCICecv() {}

//========================================================================================================================
DQMHistogramOTCBCtoCICecv::~DQMHistogramOTCBCtoCICecv() {}

//========================================================================================================================
void DQMHistogramOTCBCtoCICecv::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR CALIBRATION YOU DO NOT NEED THE FOLLOWING COMMENTED LINES
    // make fDetectorContainer ready to receive the information fromm the stream
    fDetectorContainer = &theDetectorStructure;
    // SoC utilities only - END

    uint8_t numberOfStubs  = 6; // Stub 1 to 5 and L1 is filled as Stub0
    uint8_t numberOfCBC    = 8;
    uint8_t numberOfPhases = 15; // 0 to 14

    std::vector<float> listOfCBCslvsCurrents = convertStringToFloatList(findValueInSettings<std::string>(pSettingsMap, "OTCBCtoCICecv_ListOfCBCslvsCurrents", "0, 8, 14"));

    for(auto cbcStrength: listOfCBCslvsCurrents)
    {
        HistContainer<TH2F> phaseScanMatchingEfficiency(Form("CBCtoCIC_PhaseScanEfficiency_CBCStrength%d", int(cbcStrength)),
                                                        Form("CBC to CIC phase scan efficiency CBC Strength %d", int(cbcStrength)),
                                                        numberOfPhases,
                                                        -0.5,
                                                        numberOfPhases - 0.5,
                                                        numberOfCBC * numberOfStubs, // y-axis
                                                        0,
                                                        numberOfCBC * numberOfStubs);
        phaseScanMatchingEfficiency.fTheHistogram->GetYaxis()->SetTitle("");
        phaseScanMatchingEfficiency.fTheHistogram->GetXaxis()->SetTitle("Phase");

        for(uint8_t cbcId = 1; cbcId <= numberOfCBC; ++cbcId)   // itr from 1 to 8
            for(uint8_t stub = 0; stub < numberOfStubs; ++stub) // itr from 0 to 5
            {
                if(stub == 0)
                    phaseScanMatchingEfficiency.fTheHistogram->GetYaxis()->SetBinLabel(6 * (cbcId - 1) + stub + 1, Form("CBC%d_L1", cbcId));
                else
                    phaseScanMatchingEfficiency.fTheHistogram->GetYaxis()->SetBinLabel(6 * (cbcId - 1) + stub + 1, Form("CBC%d_Stub%d", cbcId, stub - 1));
            }

        phaseScanMatchingEfficiency.fTheHistogram->SetMinimum(0);
        phaseScanMatchingEfficiency.fTheHistogram->SetMaximum(1);
        phaseScanMatchingEfficiency.fTheHistogram->SetStats(false);
        RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fPhaseScanMatchingEfficiencies[cbcStrength], phaseScanMatchingEfficiency);
    }
}

//========================================================================================================================
void DQMHistogramOTCBCtoCICecv::process()
{
    // This step it is not necessary, unless you want to format / draw histograms,
    // otherwise they will be automatically saved
}

//========================================================================================================================
void DQMHistogramOTCBCtoCICecv::reset(void)
{
    // Clear histograms if needed
}

//========================================================================================================================
bool DQMHistogramOTCBCtoCICecv::fill(std::string& inputStream)
{
    // SoC utilities only - BEGIN
    ContainerSerialization theMatchingEfficiencySerialization("OTCBCtoCICecvMatchingEfficiency");

    if(theMatchingEfficiencySerialization.attachDeserializer(inputStream))
    {
        uint8_t               cicPhase, cbcStrength;
        DetectorDataContainer theDetectorData =
            theMatchingEfficiencySerialization.deserializeHybridContainer<EmptyContainer, EmptyContainer, GenericDataArray<float, 6>>(fDetectorContainer, cicPhase, cbcStrength);
        fillMatchingEfficiency(theDetectorData, cicPhase, cbcStrength);
        return true;
    }
    return false;
    // SoC utilities only - END
}

//========================================================================================================================
void DQMHistogramOTCBCtoCICecv::fillMatchingEfficiency(DetectorDataContainer& matchingEfficiencyContainer, uint8_t cicPhase, uint8_t cbcStrength)
{
    for(auto theBoard: matchingEfficiencyContainer)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                auto thePhaseScanHistogram =
                    fPhaseScanMatchingEfficiencies[cbcStrength].getHybrid(theBoard->getId(), theOpticalGroup->getId(), theHybrid->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;
                for(auto theChip: *theHybrid)
                {
                    if(!theChip->hasSummary()) continue;
                    auto theChipLineMatchingEfficiency = theChip->getSummary<GenericDataArray<float, 6>>();
                    for(int line = 0; line < 6; ++line) { thePhaseScanHistogram->SetBinContent(cicPhase + 1, theChip->getId() * 6 + line + 1, theChipLineMatchingEfficiency.at(line)); }
                }
            }
        }
    }
}
