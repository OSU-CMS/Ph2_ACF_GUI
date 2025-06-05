/*!
        \file                DQMHistogramPhaseScan.h
        \brief               base class to create and fill monitoring histograms
        \author              Fabio Ravera, Lorenzo Uplegger
        \version             1.0
        \date                6/5/19
        Support :            mail to : fabio.ravera@cern.ch
 */

#include "../DQMUtils/DQMHistogramPhaseScan.h"
#include "../RootUtils/RootContainerFactory.h"
#include "../Utils/Container.h"
#include "../Utils/ContainerFactory.h"
#include "../Utils/EmptyContainer.h"
#include "../Utils/GenericDataArray.h"
#include "../Utils/Occupancy.h"
#include "../Utils/ThresholdAndNoise.h"
#include "../Utils/Utilities.h"
#include "TCanvas.h"
#include "TF1.h"
#include "TFile.h"
#include "TH1F.h"
#include "TH2F.h"

//========================================================================================================================
DQMHistogramPhaseScan::DQMHistogramPhaseScan()
{
    fStartPhase   = 0;
    fPhaseRange   = 1;
    fStartLatency = 999;
    fLatencyRange = 999;
}

//========================================================================================================================
DQMHistogramPhaseScan::~DQMHistogramPhaseScan() {}

//========================================================================================================================
void DQMHistogramPhaseScan::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    // need to get settings from settings map
    parseSettings(pSettingsMap);

    ContainerFactory::copyStructure(theDetectorStructure, fDetectorData);
    LOG(INFO) << "Setting histograms with range " << fPhaseRange << " and start value " << fStartPhase;

    HistContainer<TH1F> hPhase("PhaseValue", "Phase Value", fPhaseRange - 0.5, fStartPhase, fStartPhase + fPhaseRange - 0.5);
    RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, fPhaseHistograms, hPhase);

    HistContainer<TH2F> hLatencyVsPhase(
        "LatencyVsPhaseValue", "Latency vs Phase Value", fLatencyRange, fStartLatency - 0.5, fStartLatency + fLatencyRange - 0.5, fPhaseRange, fStartPhase - 0.5, fStartPhase + fPhaseRange - 0.5);
    RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, fLatencyVsPhaseHistograms, hLatencyVsPhase);

    HistContainer<TH2F> hTDCVsPhase("TDCVsPhaseValue", "TDC vs Phase Value", TDCBINS, -0.5, TDCBINS - 0.5, fPhaseRange, fStartPhase - 0.5, fStartPhase + fPhaseRange - 0.5);
    RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, fTDCVsPhaseHistograms, hTDCVsPhase);
}

//========================================================================================================================
bool DQMHistogramPhaseScan::fill(std::string& inputStream)
{
    // HybridContainerStream<EmptyContainer, EmptyContainer, GenericDataArray<uint16_t, VECSIZE>> thePhaseStream("PhaseScan");

    // if(thePhaseStream.attachBuffer(&dataBuffer))
    // {
    //     std::cout << "Matched Phase Stream!!!!!\n";
    //     thePhaseStream.decodeData(fDetectorData);
    //     // fillPhasePlots(fDetectorData);
    //     fDetectorData.cleanDataStored();
    //     return true;
    // }

    return false;
}

//========================================================================================================================
void DQMHistogramPhaseScan::process()
{
    // Phase plot
    for(auto board: fPhaseHistograms)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                std::string canvasName  = "Phase_B_" + std::to_string(board->getId()) + "_O_" + std::to_string(opticalGroup->getId()) + "_H_" + std::to_string(hybrid->getId());
                TCanvas*    PhaseCanvas = new TCanvas(canvasName.c_str(), canvasName.c_str(), 500, 500);
                // PhaseCanvas->DivideSquare(hybrid->size());
                PhaseCanvas->cd();
                TH1F* PhaseHistogram = hybrid->getSummary<HistContainer<TH1F>>().fTheHistogram;
                PhaseHistogram->GetXaxis()->SetTitle("Trigger Phase");
                PhaseHistogram->GetYaxis()->SetTitle("< Hit Occupancy >");
                PhaseHistogram->DrawCopy();
            }
        }
    }
}

//========================================================================================================================

void DQMHistogramPhaseScan::reset(void) {}
void DQMHistogramPhaseScan::fillPhasePlots(uint16_t pLatency, uint16_t pPhase, DetectorDataContainer& pHits)
{
    for(auto board: pHits)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                uint32_t curch = 0;
                for(auto chip: *hybrid)
                {
                    TH2F* cTDCVsPhase = fTDCVsPhaseHistograms.getObject(board->getId())
                                            ->getObject(opticalGroup->getId())
                                            ->getObject(hybrid->getId())
                                            ->getObject(chip->getId())
                                            ->getSummary<HistContainer<TH2F>>()
                                            .fTheHistogram;
                    TH2F* cLatencyVsPhase = fLatencyVsPhaseHistograms.getObject(board->getId())
                                                ->getObject(opticalGroup->getId())
                                                ->getObject(hybrid->getId())
                                                ->getObject(chip->getId())
                                                ->getSummary<HistContainer<TH2F>>()
                                                .fTheHistogram;
                    TH1F* cPhase = fPhaseHistograms.getObject(board->getId())
                                       ->getObject(opticalGroup->getId())
                                       ->getObject(hybrid->getId())
                                       ->getObject(chip->getId())
                                       ->getSummary<HistContainer<TH1F>>()
                                       .fTheHistogram;
                    // TH1F* cPhase   = fPhaseHistograms.getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getSummary<HistContainer<TH1F>>().fTheHistogram;

                    for(uint32_t i = 0; i < TDCBINS; i++)
                    {
                        uint32_t hits = chip->getSummary<GenericDataArray<uint16_t, VECSIZE>>().at(i);

                        auto theTDCbin  = cTDCVsPhase->FindBin(float(i), float(pPhase));
                        auto thecontent = cTDCVsPhase->GetBinContent(theTDCbin);
                        cTDCVsPhase->SetBinContent(theTDCbin, thecontent + hits);

                        auto theLatencybin = cLatencyVsPhase->FindBin(float(pLatency), float(pPhase));
                        thecontent         = cLatencyVsPhase->GetBinContent(theLatencybin);
                        cLatencyVsPhase->SetBinContent(theLatencybin, thecontent + hits);

                        auto thePhasebin = cLatencyVsPhase->FindBin(float(pPhase));
                        thecontent       = cPhase->GetBinContent(thePhasebin);
                        cPhase->SetBinContent(thePhasebin, thecontent + hits);
                    }
                    curch += 1;
                }

                // cPhase
            }
        }
    }
}

void DQMHistogramPhaseScan::parseSettings(const Ph2_Parser::SettingsMap& pSettingsMap)
{
    auto cSetting = pSettingsMap.find("StartPhase");
    if(cSetting != std::end(pSettingsMap))
        fStartPhase = boost::any_cast<double>(cSetting->second);
    else
        fStartPhase = 0;

    cSetting = pSettingsMap.find("PhaseRange");
    if(cSetting != std::end(pSettingsMap))
        fPhaseRange = boost::any_cast<double>(cSetting->second);
    else
        fPhaseRange = 512;

    cSetting = pSettingsMap.find("PhaseStartLatency");
    if(cSetting != std::end(pSettingsMap))
        fStartLatency = boost::any_cast<double>(cSetting->second);
    else
        fStartLatency = 0;

    cSetting = pSettingsMap.find("PhaseLatencyRange");
    if(cSetting != std::end(pSettingsMap))
        fLatencyRange = boost::any_cast<double>(cSetting->second);
    else
        fLatencyRange = 512;
}
