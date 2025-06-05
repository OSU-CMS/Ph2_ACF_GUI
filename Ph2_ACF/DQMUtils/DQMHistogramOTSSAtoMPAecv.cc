#include "DQMUtils/DQMHistogramOTSSAtoMPAecv.h"
#include "RootUtils/RootContainerFactory.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/GenericDataArray.h"

#include "TFile.h"
#include "TH2F.h"

//========================================================================================================================
DQMHistogramOTSSAtoMPAecv::DQMHistogramOTSSAtoMPAecv() {}

//========================================================================================================================
DQMHistogramOTSSAtoMPAecv::~DQMHistogramOTSSAtoMPAecv() {}

//========================================================================================================================
void DQMHistogramOTSSAtoMPAecv::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    // SoC utilities only - BEGIN
    // THIS PART IT IS JUST TO SHOW HOW DATA ARE DECODED FROM THE TCP STREAM WHEN WE WILL GO ON THE SOC
    // IF YOU DO NOT WANT TO GO INTO THE SOC WITH YOUR CALIBRATION YOU DO NOT NEED THE FOLLOWING COMMENTED LINES
    // make fDetectorContainer ready to receive the information fromm the stream
    fDetectorContainer = &theDetectorStructure;
    // SoC utilities only - END

    std::vector<float> listOfSSAslvsCurrents = convertStringToFloatList(findValueInSettings<std::string>(pSettingsMap, "OTSSAtoMPAecv_ListOfSSAslvsCurrents", "1, 4, 7"));

    uint8_t numberOfMPA         = 8;
    uint8_t numberOfLinesPerMPA = 9;

    auto setYaxisBinLabel = [numberOfMPA, numberOfLinesPerMPA](TH2F* theHistogram)
    {
        auto theAxis = theHistogram->GetYaxis();
        for(uint8_t mpaId = 0; mpaId < numberOfMPA; ++mpaId)
        {
            theAxis->SetBinLabel(mpaId * numberOfLinesPerMPA + 1, Form("MPA%d_L1", mpaId + 8));
            for(uint8_t lineId = 1; lineId < numberOfLinesPerMPA; ++lineId) { theAxis->SetBinLabel(mpaId * numberOfLinesPerMPA + lineId + 1, Form("MPA%d_Stub%d", mpaId + 8, lineId)); }
        }
    };

    int totalNumberOfShifts = fMaximum320PhaseShift - fMinimum320PhaseShift + 1;

    auto setXaxisBinLabel = [numberOfMPA, totalNumberOfShifts, this](TH2F* theHistogram)
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

    for(auto slvsCurrent: listOfSSAslvsCurrents)
    {
        HistContainer<TH2F> phaseScanErrorRate(Form("SSAtoMPA_SamplingEdgeErrorRate_SSA_SLVScurrent_%d", int(slvsCurrent)),
                                               Form("SSA to MPA sampling edge error rate - SSA SLVS current = %d", int(slvsCurrent)),
                                               2 * totalNumberOfShifts,
                                               -0.5,
                                               2 * totalNumberOfShifts - 0.5,
                                               numberOfMPA * numberOfLinesPerMPA,
                                               -0.5,
                                               numberOfMPA * numberOfLinesPerMPA - 0.5);
        phaseScanErrorRate.fTheHistogram->GetXaxis()->SetTitle("Sampling egde : 320MHz clock shift");
        setXaxisBinLabel(phaseScanErrorRate.fTheHistogram);
        setYaxisBinLabel(phaseScanErrorRate.fTheHistogram);
        phaseScanErrorRate.fTheHistogram->SetMinimum(0);
        phaseScanErrorRate.fTheHistogram->SetMaximum(1);
        phaseScanErrorRate.fTheHistogram->SetStats(false);
        RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fPhaseScanErrorRate[slvsCurrent], phaseScanErrorRate);

        HistContainer<TH2F> phaseScanTestedBits(Form("SSAtoMPA_SamplingEdgeTestedBits_SSA_SLVScurrent_%d", int(slvsCurrent)),
                                                Form("SSA to MPA sampling edge tested bits - SSA SLVS current = %d", int(slvsCurrent)),
                                                2 * totalNumberOfShifts,
                                                -0.5,
                                                2 * totalNumberOfShifts - 0.5,
                                                numberOfMPA * numberOfLinesPerMPA,
                                                -0.5,
                                                numberOfMPA * numberOfLinesPerMPA - 0.5);
        phaseScanTestedBits.fTheHistogram->GetXaxis()->SetTitle("Sampling egde : 320MHz clock shift");
        setXaxisBinLabel(phaseScanTestedBits.fTheHistogram);
        setYaxisBinLabel(phaseScanTestedBits.fTheHistogram);
        phaseScanTestedBits.fTheHistogram->SetStats(false);
        RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fPhaseScanTestedBits[slvsCurrent], phaseScanTestedBits);
    }
}

//========================================================================================================================
void DQMHistogramOTSSAtoMPAecv::process()
{
    // This step it is not necessary, unless you want to format / draw histograms,
    // otherwise they will be automatically saved
}

//========================================================================================================================
void DQMHistogramOTSSAtoMPAecv::reset(void)
{
    // Clear histograms if needed
}

//========================================================================================================================
void DQMHistogramOTSSAtoMPAecv::fillPatternEfficiencyScan(DetectorDataContainer& thePhaseMatchingEfficiency, uint8_t clockEdge, uint8_t slvsCurrent, int samplingPhaseOffset)
{
    int totalNumberOfShifts = fMaximum320PhaseShift - fMinimum320PhaseShift + 1;

    for(auto theBoard: thePhaseMatchingEfficiency)
    {
        for(auto theOpticalGroup: *theBoard)
        {
            for(auto theHybrid: *theOpticalGroup)
            {
                if(!theHybrid->hasSummary()) continue;

                auto thePatternMatchingEfficiencyVector = theHybrid->getSummary<GenericDataArray<float, NUMBER_OF_CIC_PORTS, 9, 2>>();

                TH2F* errorRateHistogram =
                    fPhaseScanErrorRate[slvsCurrent].getObject(theBoard->getId())->getObject(theOpticalGroup->getId())->getObject(theHybrid->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;
                TH2F* testedBitsHistogram =
                    fPhaseScanTestedBits[slvsCurrent].getObject(theBoard->getId())->getObject(theOpticalGroup->getId())->getObject(theHybrid->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;
                for(size_t chipId = 0; chipId < NUMBER_OF_CIC_PORTS; ++chipId) // not using the chipID because I want always to read all phases
                {
                    for(size_t line = 0; line < 9; line++)
                    {
                        auto testedBits = thePatternMatchingEfficiencyVector.at(chipId).at(line).at(0);
                        auto errorRate  = testedBits > 0 ? thePatternMatchingEfficiencyVector.at(chipId).at(line).at(1) / testedBits : 1.;
                        errorRateHistogram->SetBinContent(clockEdge * totalNumberOfShifts + samplingPhaseOffset - fMinimum320PhaseShift + 1, chipId * 9 + line + 1, errorRate);
                        testedBitsHistogram->SetBinContent(clockEdge * totalNumberOfShifts + samplingPhaseOffset - fMinimum320PhaseShift + 1, chipId * 9 + line + 1, testedBits);
                    }
                }
            }
        }
    }
}

//========================================================================================================================
bool DQMHistogramOTSSAtoMPAecv::fill(std::string& inputStream)
{
    // SoC utilities only - BEGIN
    ContainerSerialization thePatternMatchinEfficiencyContainerSerialization("OTSSAtoMPAecvPatternMatchingEfficiency");

    if(thePatternMatchinEfficiencyContainerSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched OTSSAtoMPAecv PatternMatchingEfficiency!!!!\n";
        uint8_t               clockEdge, slvsCurrent;
        int                   samplingPhaseOffset;
        DetectorDataContainer theDetectorData =
            thePatternMatchinEfficiencyContainerSerialization.deserializeHybridContainer<EmptyContainer, EmptyContainer, GenericDataArray<float, NUMBER_OF_CIC_PORTS, 9, 2>>(
                fDetectorContainer, clockEdge, slvsCurrent, samplingPhaseOffset);
        fillPatternEfficiencyScan(theDetectorData, clockEdge, slvsCurrent, samplingPhaseOffset);
        return true;
    }

    return false;
    // SoC utilities only - END
}
