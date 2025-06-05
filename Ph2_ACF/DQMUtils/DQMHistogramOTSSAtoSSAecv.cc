#include "DQMUtils/DQMHistogramOTSSAtoSSAecv.h"
#include "RootUtils/RootContainerFactory.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/GenericDataArray.h"

#include "TFile.h"
#include "TH2F.h"

//========================================================================================================================
DQMHistogramOTSSAtoSSAecv::DQMHistogramOTSSAtoSSAecv() {}

//========================================================================================================================
DQMHistogramOTSSAtoSSAecv::~DQMHistogramOTSSAtoSSAecv() {}

//========================================================================================================================
void DQMHistogramOTSSAtoSSAecv::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR CALIBRATION YOU DO NOT NEED THE FOLLOWING COMMENTED LINES
    // make fDetectorContainer ready to receive the information fromm the stream
    fDetectorContainer = &theDetectorStructure;
    // SoC utilities only - END
    int numberOfSSA        = 8;
    int numberOfDirections = 2;

    auto setYaxisBinLabel = [this, numberOfSSA, numberOfDirections](TH2F* theHistogram)
    {
        auto theAxis = theHistogram->GetYaxis();
        for(int direction = 0; direction < numberOfDirections; ++direction)
        {
            std::pair<int, int> mpaRange = getMPArange(direction);
            for(int mpaId = mpaRange.first; mpaId < mpaRange.second; ++mpaId)
            {
                int ssaId = mpaId + (direction == 0 ? +1 : -1);
                theAxis->SetBinLabel(direction * (mpaRange.second - mpaRange.first) + (mpaId - mpaRange.first) + 1, Form("SSA%d#rightarrowSSA%d", ssaId, mpaId));
            }
        }
    };

    int totalNumberOfShifts = fMaximum320PhaseShift - fMinimum320PhaseShift + 1;

    auto setXaxisBinLabel = [numberOfSSA, totalNumberOfShifts, this](TH2F* theHistogram)
    {
        std::vector<std::string> edgeLabel = {"Falling", "Rising"};
        auto                     theAxis   = theHistogram->GetXaxis();
        for(size_t labelIndex = 0; labelIndex < edgeLabel.size(); ++labelIndex)
        {
            for(int shift = fMinimum320PhaseShift; shift <= fMaximum320PhaseShift; ++shift)
            {
                theAxis->SetBinLabel(shift + totalNumberOfShifts * labelIndex - fMinimum320PhaseShift + 1, Form("%s : %s%d", edgeLabel[labelIndex].c_str(), (shift > 0 ? "+" : ""), shift));
            }
        }
    };

    std::vector<float> listOfSSAslvsCurrents = convertStringToFloatList(findValueInSettings<std::string>(pSettingsMap, "OTSSAtoSSAecv_ListOfSSAslvsCurrents", "1, 4, 7"));

    for(auto slvsCurrent: listOfSSAslvsCurrents)
    {
        HistContainer<TH2F> phaseScanErrorRate(Form("SSAtoSSA_SamplingEdgeErrorRate_SSA_SLVScurrent_%d", int(slvsCurrent)),
                                               Form("SSA to SSA sampling edge error rate - SSA SLVS current = %d", int(slvsCurrent)),
                                               2 * totalNumberOfShifts,
                                               -0.5,
                                               2 * totalNumberOfShifts - 0.5,
                                               (numberOfSSA - 1) * numberOfDirections,
                                               -0.5,
                                               (numberOfSSA - 1) * numberOfDirections - 0.5);
        phaseScanErrorRate.fTheHistogram->GetXaxis()->SetTitle("Sampling egde : 320MHz clock shift");
        setXaxisBinLabel(phaseScanErrorRate.fTheHistogram);
        setYaxisBinLabel(phaseScanErrorRate.fTheHistogram);
        phaseScanErrorRate.fTheHistogram->SetMinimum(0);
        phaseScanErrorRate.fTheHistogram->SetMaximum(1);
        phaseScanErrorRate.fTheHistogram->SetStats(false);
        RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fPhaseScanErrorRate[slvsCurrent], phaseScanErrorRate);

        HistContainer<TH2F> phaseScanTestedBits(Form("SSAtoSSA_SamplingEdgeTestedBits_SSA_SLVScurrent_%d", int(slvsCurrent)),
                                                Form("SSA to SSA sampling edge tested bits - SSA SLVS current = %d", int(slvsCurrent)),
                                                2 * totalNumberOfShifts,
                                                -0.5,
                                                2 * totalNumberOfShifts - 0.5,
                                                (numberOfSSA - 1) * numberOfDirections,
                                                -0.5,
                                                (numberOfSSA - 1) * numberOfDirections - 0.5);
        phaseScanTestedBits.fTheHistogram->GetXaxis()->SetTitle("Sampling egde : 320MHz clock shift");
        setXaxisBinLabel(phaseScanTestedBits.fTheHistogram);
        setYaxisBinLabel(phaseScanTestedBits.fTheHistogram);
        phaseScanTestedBits.fTheHistogram->SetStats(false);
        RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fPhaseScanTestedBits[slvsCurrent], phaseScanTestedBits);
    }
}

//========================================================================================================================
void DQMHistogramOTSSAtoSSAecv::fillStubPatternEfficiencyScan(DetectorDataContainer& thePatternMatchingEfficiency, uint8_t clockEdge, uint8_t slvsCurrent, int samplingPhaseOffset)
{
    int totalNumberOfShifts = fMaximum320PhaseShift - fMinimum320PhaseShift + 1;

    for(auto theBoard: thePatternMatchingEfficiency)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                if(!theHybrid->hasSummary()) continue;

                auto thePatternMatchingEfficiencyVector = theHybrid->getSummary<GenericDataArray<float, NUMBER_OF_CIC_PORTS, 2, 2>>();

                TH2F* errorRateHistogram =
                    fPhaseScanErrorRate[slvsCurrent].getObject(theBoard->getId())->getObject(theOpticalGroup->getId())->getObject(theHybrid->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;
                TH2F* testedBitsHistogram =
                    fPhaseScanTestedBits[slvsCurrent].getObject(theBoard->getId())->getObject(theOpticalGroup->getId())->getObject(theHybrid->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;

                int binCounter = 1;
                for(size_t stubPatternCounter = 0; stubPatternCounter < 2; ++stubPatternCounter)
                {
                    for(uint8_t mpaId = 0; mpaId < NUMBER_OF_CIC_PORTS; ++mpaId)
                    {
                        if(stubPatternCounter == 0 && mpaId == NUMBER_OF_CIC_PORTS - 1) continue;
                        if(stubPatternCounter == 1 && mpaId == 0) continue;

                        auto testedBits = thePatternMatchingEfficiencyVector.at(mpaId).at(stubPatternCounter).at(0);
                        auto errorRate  = testedBits > 0 ? thePatternMatchingEfficiencyVector.at(mpaId).at(stubPatternCounter).at(1) / testedBits : 1.;

                        testedBitsHistogram->SetBinContent(clockEdge * totalNumberOfShifts + samplingPhaseOffset - fMinimum320PhaseShift + 1, binCounter, testedBits);
                        errorRateHistogram->SetBinContent(clockEdge * totalNumberOfShifts + samplingPhaseOffset - fMinimum320PhaseShift + 1, binCounter, errorRate);

                        ++binCounter;
                    }
                }
            }
        }
    }
}

//========================================================================================================================
void DQMHistogramOTSSAtoSSAecv::process()
{
    // This step it is not necessary, unless you want to format / draw histograms,
    // otherwise they will be automatically saved
}

//========================================================================================================================
void DQMHistogramOTSSAtoSSAecv::reset(void)
{
    // Clear histograms if needed
}

//========================================================================================================================
bool DQMHistogramOTSSAtoSSAecv::fill(std::string& inputStream)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR CALIBRATION YOU DO NOT NEED THE FOLLOWING COMMENTED LINES

    ContainerSerialization theStubPatternMatchinEfficiencyContainerSerialization("OTSSAtoSSAecvStubPatternMatchingEfficiency");

    if(theStubPatternMatchinEfficiencyContainerSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched OTSSAtoSSAecv StubPatternMatchingEfficiency!!!!\n";
        uint8_t               clockEdge, slvsCurrent;
        int                   samplingPhaseOffset;
        DetectorDataContainer theDetectorData =
            theStubPatternMatchinEfficiencyContainerSerialization.deserializeHybridContainer<EmptyContainer, EmptyContainer, GenericDataArray<float, NUMBER_OF_CIC_PORTS, 2, 2>>(
                fDetectorContainer, clockEdge, slvsCurrent, samplingPhaseOffset);
        fillStubPatternEfficiencyScan(theDetectorData, clockEdge, slvsCurrent, samplingPhaseOffset);
        return true;
    }

    return false;
    // SoC utilities only - END
}

//========================================================================================================================
std::pair<int, int> DQMHistogramOTSSAtoSSAecv::getMPArange(uint8_t stubPattern) const
{
    std::pair<int, int> mpaRange;
    if(stubPattern == 0)
    {
        mpaRange.first  = 0;
        mpaRange.second = 7;
    }
    else
    {
        mpaRange.first  = 1;
        mpaRange.second = 8;
    }
    return mpaRange;
}
