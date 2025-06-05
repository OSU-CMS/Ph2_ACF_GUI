/*!
        \file                DQMHistogramECV.cc
        \brief               class to create and fill data from ECV measurements
        \author              Stefan Maier
        \version             1.0
        \date                27/7/22
        Support :            mail to : s.maier@kit.edu
 */

#include "DQMUtils/DQMHistogramECV.h"
#include "RootUtils/RootContainerFactory.h"
#include "TCanvas.h"
#include "TF1.h"
#include "TFile.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TLine.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/EmptyContainer.h"
#include "Utils/GenericDataArray.h"
#include "Utils/Occupancy.h"
#include "Utils/ThresholdAndNoise.h"
#include "Utils/Utilities.h"

//========================================================================================================================
DQMHistogramECV::DQMHistogramECV() {}

//========================================================================================================================
DQMHistogramECV::~DQMHistogramECV() {}

//========================================================================================================================
void DQMHistogramECV::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    ContainerFactory::copyStructure(theDetectorStructure, fDetectorData);

    HistContainer<TH2F> hBitErrorScanPolarity0("1-BER_CIC_Clock_Polarity_0", "Polarity 0", 49, 0, 49, 75, 0, 75);
    RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fBitErrorScanPolarity0, hBitErrorScanPolarity0);
    HistContainer<TH2F> hBitErrorScanPolarity1("1-BER_CIC_Clock_Polarity_1", "Polarity 1", 49, 0, 49, 75, 0, 75);
    RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fBitErrorScanPolarity1, hBitErrorScanPolarity1);

    HistContainer<TH2F> hWordAlignmentScanPolarity0("WordAlignment_Polarity_0", "Polarity 0", 49, 0, 49, 75, 0, 75);
    RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fWordAlignmentScanPolarity0, hWordAlignmentScanPolarity0);
    HistContainer<TH2F> hWordAlignmentScanPolarity1("WordAlignment_Polarity_1", "Polarity 1", 49, 0, 49, 75, 0, 75);
    RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fWordAlignmentScanPolarity1, hWordAlignmentScanPolarity1);

    HistContainer<TH2F> hPhaseTrainingPolarity0("PhaseTraining_Polarity_0", "Polarity 0", 49, 0, 49, 5, 0, 5);
    RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fPhaseScanPolarity0, hPhaseTrainingPolarity0);
    HistContainer<TH2F> hPhaseTrainingPolarity1("PhaseTraining_Polarity_1", "Polarity 1", 49, 0, 49, 5, 0, 5);
    RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fPhaseScanPolarity1, hPhaseTrainingPolarity1);

    HistContainer<TH2F> hChosenPhasePolarity0("ChosenPhase_Polarity_0", "Polarity 0", 7, 0, 7, 5, 0, 5);
    RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fChosenPhasePolarity0, hChosenPhasePolarity0);
    HistContainer<TH2F> hChosenPhasePolarity1("ChosenPhase_Polarity_1", "Polarity 1", 7, 0, 7, 5, 0, 5);
    RootContainerFactory::bookHybridHistograms(theOutputFile, theDetectorStructure, fChosenPhasePolarity1, hChosenPhasePolarity1);
}

//========================================================================================================================
bool DQMHistogramECV::fill(std::string& inputStream) { return false; }

//========================================================================================================================
void DQMHistogramECV::process()
{
    for(auto board: fBitErrorScanPolarity0)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                std::string cCanvasName =
                    "ECV_BitterrorRate_HybridClockParity0_B_" + std::to_string(board->getId()) + "_O_" + std::to_string(opticalGroup->getId()) + "_H_" + std::to_string(hybrid->getId());
                TCanvas* berCanvas = new TCanvas(cCanvasName.c_str(), cCanvasName.c_str(), 500, 500);
                berCanvas->cd();
                TH2F* p0 = hybrid->getSummary<HistContainer<TH2F>>().fTheHistogram;
                p0->GetXaxis()->SetTitle("Channel : Hybrid clock strength");
                p0->GetYaxis()->SetTitle("lpGBT Phase : CIC Strength");
                int binNumber = 1;
                for(uint32_t phase = 0; phase < 15; phase++)
                {
                    for(uint32_t cicSignalStrength = 1; cicSignalStrength <= 5; cicSignalStrength++)
                    {
                        std::string s = convertToString(phase) + ":" + convertToString(cicSignalStrength);
                        p0->GetYaxis()->SetBinLabel(binNumber, s.c_str());
                        binNumber++;
                    }
                }
                binNumber = 1;

                for(uint32_t channel = 1; channel <= 7; channel++)
                {
                    for(uint32_t hybridClockStrength = 1; hybridClockStrength <= 7; hybridClockStrength++)
                    {
                        std::string s = convertToString(channel) + ":" + convertToString(hybridClockStrength);
                        p0->GetXaxis()->SetBinLabel(binNumber, s.c_str());
                        binNumber++;
                    }
                }
                p0->GetYaxis()->SetLabelSize(0.02);

                p0->LabelsOption("v", "X");
                p0->DrawCopy("text");
                berCanvas->SetGrid();
            }
        }
    }
    for(auto board: fBitErrorScanPolarity1)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                std::string cCanvasName =
                    "ECV_BitterrorRate_HybridClockParity1_B_" + std::to_string(board->getId()) + "_O_" + std::to_string(opticalGroup->getId()) + "_H_" + std::to_string(hybrid->getId());
                TCanvas* berCanvas = new TCanvas(cCanvasName.c_str(), cCanvasName.c_str(), 500, 500);
                berCanvas->cd();
                TH2F* p1 = hybrid->getSummary<HistContainer<TH2F>>().fTheHistogram;
                p1->GetXaxis()->SetTitle("Channel : Hybrid clock strength");
                p1->GetYaxis()->SetTitle("lpGBT Phase : CIC Strength");
                p1->GetYaxis()->SetLabelSize(0.02);

                berCanvas->SetGrid();
                int binNumber = 1;
                for(uint32_t phase = 0; phase < 15; phase++)
                {
                    for(uint32_t cicSignalStrength = 1; cicSignalStrength <= 5; cicSignalStrength++)
                    {
                        std::string s = convertToString(phase) + ":" + convertToString(cicSignalStrength);
                        p1->GetYaxis()->SetBinLabel(binNumber, s.c_str());
                        binNumber++;
                    }
                }
                binNumber = 1;

                for(uint32_t channel = 1; channel <= 7; channel++)
                {
                    for(uint32_t hybridClockStrength = 1; hybridClockStrength <= 7; hybridClockStrength++)
                    {
                        std::string s = convertToString(channel) + ":" + convertToString(hybridClockStrength);
                        p1->GetXaxis()->SetBinLabel(binNumber, s.c_str());
                        binNumber++;
                    }
                }
                p1->LabelsOption("v", "X");
                p1->DrawCopy("text");
                berCanvas->SetGrid();
                berCanvas->Update();
            }
        }
    }
    for(auto board: fWordAlignmentScanPolarity0)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                std::string cCanvasName =
                    "ECV_WordAlignment_CICClockParity0_B_" + std::to_string(board->getId()) + "_O_" + std::to_string(opticalGroup->getId()) + "_H_" + std::to_string(hybrid->getId());
                TCanvas* berCanvas = new TCanvas(cCanvasName.c_str(), cCanvasName.c_str(), 500, 500);
                berCanvas->cd();
                TH2F* p0 = hybrid->getSummary<HistContainer<TH2F>>().fTheHistogram;
                p0->GetXaxis()->SetTitle("Channel : CIC clock strength");
                p0->GetYaxis()->SetTitle("lpGBT Phase : CIC strength");
                int binNumber = 1;
                for(uint32_t phase = 0; phase < 15; phase++)
                {
                    for(uint32_t cicSignalStrength = 1; cicSignalStrength <= 5; cicSignalStrength++)
                    {
                        std::string s = convertToString(phase) + ":" + convertToString(cicSignalStrength);
                        p0->GetYaxis()->SetBinLabel(binNumber, s.c_str());
                        binNumber++;
                    }
                }
                binNumber = 1;

                for(uint32_t channel = 1; channel <= 7; channel++)
                {
                    for(uint32_t hybridClockStrength = 1; hybridClockStrength <= 7; hybridClockStrength++)
                    {
                        std::string s = convertToString(channel) + ":" + convertToString(hybridClockStrength);
                        p0->GetXaxis()->SetBinLabel(binNumber, s.c_str());
                        binNumber++;
                    }
                }
                p0->GetYaxis()->SetLabelSize(0.02);

                p0->LabelsOption("v", "X");
                p0->DrawCopy("text");
                berCanvas->SetGrid();
            }
        }
    }
    for(auto board: fWordAlignmentScanPolarity1)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                std::string cCanvasName =
                    "ECV_WordAlignment_CICClockParity1_B_" + std::to_string(board->getId()) + "_O_" + std::to_string(opticalGroup->getId()) + "_H_" + std::to_string(hybrid->getId());
                TCanvas* berCanvas = new TCanvas(cCanvasName.c_str(), cCanvasName.c_str(), 500, 500);
                berCanvas->cd();
                TH2F* p1 = hybrid->getSummary<HistContainer<TH2F>>().fTheHistogram;
                p1->GetXaxis()->SetTitle("Channel : CIC clock strength");
                p1->GetYaxis()->SetTitle("lpGBT Phase : CIC strength");
                p1->GetYaxis()->SetLabelSize(0.02);

                berCanvas->SetGrid();
                int binNumber = 1;
                for(uint32_t phase = 0; phase < 15; phase++)
                {
                    for(uint32_t cicSignalStrength = 1; cicSignalStrength <= 5; cicSignalStrength++)
                    {
                        std::string s = convertToString(phase) + ":" + convertToString(cicSignalStrength);
                        p1->GetYaxis()->SetBinLabel(binNumber, s.c_str());
                        binNumber++;
                    }
                }
                binNumber = 1;

                for(uint32_t channel = 1; channel <= 7; channel++)
                {
                    for(uint32_t hybridClockStrength = 1; hybridClockStrength <= 7; hybridClockStrength++)
                    {
                        std::string s = convertToString(channel) + ":" + convertToString(hybridClockStrength);
                        p1->GetXaxis()->SetBinLabel(binNumber, s.c_str());
                        binNumber++;
                    }
                }
                p1->LabelsOption("v", "X");
                p1->DrawCopy("text");
                berCanvas->SetGrid();
                berCanvas->Update();
            }
        }
    }
}

//========================================================================================================================

void DQMHistogramECV::reset(void) {}
void DQMHistogramECV::fillWordAlign(uint8_t pClockPolarity, uint8_t pClockStrength, uint8_t pCicStrength, uint8_t pPhase, uint8_t pHybridId, uint8_t pLine, DetectorDataContainer& pWordAlignSummary)
{
    for(auto board: pWordAlignSummary)
    {
        for(auto opticalGroup: *board)
        {
            bool  aligned = pWordAlignSummary.getObject(board->getId())->getObject(opticalGroup->getId())->getObject(pHybridId)->getSummary<bool>();
            TH2F* cAlignSummary;
            if(pClockPolarity == 0)
                cAlignSummary = fWordAlignmentScanPolarity0.getObject(board->getId())->getObject(opticalGroup->getId())->getObject(pHybridId)->getSummary<HistContainer<TH2F>>().fTheHistogram;
            else
                cAlignSummary = fWordAlignmentScanPolarity1.getObject(board->getId())->getObject(opticalGroup->getId())->getObject(pHybridId)->getSummary<HistContainer<TH2F>>().fTheHistogram;

            // LOG (INFO) << "Fill\t" << "hybrid: " << +pHybridId << " Line: " << +pLine << " aligned: " << aligned << RESET;

            cAlignSummary->SetBinContent(pLine * 7 + pClockStrength, pPhase * 5 + pCicStrength, (aligned) ? 1 : 0);
        }
    }
}
void DQMHistogramECV::fillBER(uint8_t pClockPolarity, uint8_t pClockStrength, uint8_t pCicStrength, uint8_t pPhase, uint8_t pHybridId, uint8_t pLine, DetectorDataContainer& pBERSummary)
{
    for(auto board: pBERSummary)
    {
        for(auto opticalGroup: *board)
        {
            float ber = pBERSummary.getObject(board->getId())->getObject(opticalGroup->getId())->getObject(pHybridId)->getSummary<float>();
            TH2F* cBERSummary;
            if(pClockPolarity == 0)
                cBERSummary = fBitErrorScanPolarity0.getObject(board->getId())->getObject(opticalGroup->getId())->getObject(pHybridId)->getSummary<HistContainer<TH2F>>().fTheHistogram;
            else
                cBERSummary = fBitErrorScanPolarity1.getObject(board->getId())->getObject(opticalGroup->getId())->getObject(pHybridId)->getSummary<HistContainer<TH2F>>().fTheHistogram;
            float value = 1 - ber;
            cBERSummary->SetBinContent(pLine * 7 + pClockStrength, pPhase * 5 + pCicStrength, value);
        }
    }
}

void DQMHistogramECV::fillPhases(uint8_t pClockPolarity, uint8_t pClockStrength, uint8_t pCicStrength, uint8_t pHybridId, uint8_t pLine, DetectorDataContainer& pPhase)
{
    for(auto board: pPhase)
    {
        for(auto opticalGroup: *board)
        {
            float phase = (float)pPhase.getObject(board->getId())->getObject(opticalGroup->getId())->getObject(pHybridId)->getSummary<uint8_t>();
            TH2F* cPhaseSummary;

            if(pClockPolarity == 0)
                cPhaseSummary = fPhaseScanPolarity0.getObject(board->getId())->getObject(opticalGroup->getId())->getObject(pHybridId)->getSummary<HistContainer<TH2F>>().fTheHistogram;
            else
                cPhaseSummary = fPhaseScanPolarity1.getObject(board->getId())->getObject(opticalGroup->getId())->getObject(pHybridId)->getSummary<HistContainer<TH2F>>().fTheHistogram;

            cPhaseSummary->SetBinContent(pLine * 7 + pClockStrength, pCicStrength, phase);
        }
    }
}

void DQMHistogramECV::fillChosenPhase(uint8_t pClockPolarity, uint8_t pClockStrength, uint8_t pCicStrength, DetectorDataContainer& pPhase)
{
    for(auto board: pPhase)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                float phase = (float)pPhase.getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getSummary<uint8_t>();
                TH2F* cPhaseSummary;

                if(pClockPolarity == 0)
                    cPhaseSummary = fChosenPhasePolarity0.getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;
                else
                    cPhaseSummary = fChosenPhasePolarity1.getObject(board->getId())->getObject(opticalGroup->getId())->getObject(hybrid->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;

                cPhaseSummary->SetBinContent(pClockStrength, pCicStrength, phase);
            }
        }
    }
}
