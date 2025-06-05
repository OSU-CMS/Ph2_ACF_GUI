/*!
        \file                DQMHistogramRegisterTest.h
        \brief               base class to create and fill monitoring histograms
        \author              Fabio Ravera, Lorenzo Uplegger
        \version             1.0
        \date                6/5/19
        Support :            mail to : fabio.ravera@cern.ch
 */

#include "DQMUtils/DQMHistogramRegisterTest.h"
#include "RootUtils/RootContainerFactory.h"
#include "TCanvas.h"
#include "TF1.h"
#include "TFile.h"
#include "TH1F.h"
#include "TH2F.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"

#include "Utils/EmptyContainer.h"
#include "Utils/GenericDataArray.h"
#include "Utils/Utilities.h"

//========================================================================================================================
DQMHistogramRegisterTest::DQMHistogramRegisterTest() {}

//========================================================================================================================
DQMHistogramRegisterTest::~DQMHistogramRegisterTest() {}

//========================================================================================================================
void DQMHistogramRegisterTest::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    // need to get settings from settings map
    parseSettings(pSettingsMap);

    ContainerFactory::copyStructure(theDetectorStructure, fDetectorData);

    HistContainer<TH2F> hWriteMismatchesPg0("RegisterTestWritePg0", ";Number of Register Writes; Register Address", 1000 - 1, 1, 1000, 255, 0, 255);
    RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, fWriteMismatchesPg0, hWriteMismatchesPg0);

    HistContainer<TH2F> hWriteMismatchesPg1("RegisterTestWritePg1", ";Number of Register Writes; Register Address", 1000 - 1, 1, 1000, 255, 0, 255);
    RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, fWriteMismatchesPg1, hWriteMismatchesPg1);

    HistContainer<TH2F> hReadMismatchesPg0("RegisterTestReadPg0", ";Number of Register Reads; Register Address", 5000, 0, 5000, 255, 0, 255);
    RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, fReadMismatchesPg0, hReadMismatchesPg0);

    HistContainer<TH2F> hReadMismatchesPg1("RegisterTestReadPg1", ";Number of Register Reads; Register Address", 5000, 0, 5000, 255, 0, 255);
    RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, fReadMismatchesPg1, hReadMismatchesPg1);

    // mismatches on page1 after a page toggle
    HistContainer<TH2F> hWrMismatchesPg1("RegisterTestWrPg1", ";Number of Page Toggles [Write]; Register Address", 1000, 0, 1000, 255, 0, 255);
    RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, fWrMismatchesPg1, hWrMismatchesPg1);

    HistContainer<TH2F> hRdMismatchesPg1("RegisterTestRdPg1", ";Number of Page Toggles [Read]; Register Address", 1000, 0, 1000, 255, 0, 255);
    RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, fRdMismatchesPg1, hRdMismatchesPg1);

    HistContainer<TH2F> hRdValsMismatchesPg1("MismatchesRd", ";Expected Value; Actual Value", 255, 0, 255, 255, 0, 255);
    RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, fRdValsMismatchesPg1, hRdValsMismatchesPg1);

    HistContainer<TH2F> hWrValsMismatchesPg1("MismatchesWr", ";Expected Value; Actual Value", 255, 0, 255, 255, 0, 255);
    RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, fWrValsMismatchesPg1, hWrValsMismatchesPg1);

    HistContainer<TH1F> hWrCount("WrCount", ";; Count", 3, 0, 3);
    RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, fWrCnts, hWrCount);

    HistContainer<TH1F> hRdCount("RdCount", ";; Count", 3, 0, 3);
    RootContainerFactory::bookChipHistograms(theOutputFile, theDetectorStructure, fRdCnts, hRdCount);
}

//========================================================================================================================
bool DQMHistogramRegisterTest::fill(std::string& inputStream) { return false; }

//========================================================================================================================
void DQMHistogramRegisterTest::process()
{
    for(auto board: fWriteMismatchesPg0)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                for(auto chip: *hybrid)
                {
                    std::string cCanvasName  = "MismatchesPg0_" + std::to_string(chip->getId()) + std::to_string(hybrid->getId());
                    std::string cCanvasTitle = "Mismatches [Pg0] " + std::to_string(chip->getId()) + std::to_string(hybrid->getId());

                    TCanvas* cCanvas = new TCanvas(cCanvasName.c_str(), cCanvasTitle.c_str(), 500, 500);
                    cCanvas->cd();
                    auto& cHistogram = chip->getSummary<HistContainer<TH2F>>().fTheHistogram;
                    cHistogram->GetXaxis()->SetTitle("Number of Register Writes");
                    cHistogram->GetYaxis()->SetTitle("Register Address");
                    cHistogram->GetZaxis()->SetTitle("Mismatch");
                    cHistogram->DrawCopy();
                }
            }
        }
    }
    for(auto board: fWriteMismatchesPg1)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                for(auto chip: *hybrid)
                {
                    std::string cCanvasName  = "MismatchesPg1_" + std::to_string(chip->getId()) + std::to_string(hybrid->getId());
                    std::string cCanvasTitle = "Mismatches [Pg1] " + std::to_string(chip->getId()) + std::to_string(hybrid->getId());

                    TCanvas* cCanvas = new TCanvas(cCanvasName.c_str(), cCanvasTitle.c_str(), 500, 500);
                    cCanvas->cd();
                    auto& cHistogram = chip->getSummary<HistContainer<TH2F>>().fTheHistogram;
                    cHistogram->GetXaxis()->SetTitle("Number of Register Writes");
                    cHistogram->GetYaxis()->SetTitle("Register Address");
                    cHistogram->GetZaxis()->SetTitle("Mismatch");
                    cHistogram->DrawCopy();
                }
            }
        }
    }
    for(auto board: fReadMismatchesPg0)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                for(auto chip: *hybrid)
                {
                    std::string cCanvasName  = "RdMismatchesPg0_" + std::to_string(chip->getId()) + std::to_string(hybrid->getId());
                    std::string cCanvasTitle = "Read Mismatches [Pg0] " + std::to_string(chip->getId()) + std::to_string(hybrid->getId());

                    TCanvas* cCanvas = new TCanvas(cCanvasName.c_str(), cCanvasTitle.c_str(), 500, 500);
                    cCanvas->cd();
                    auto& cHistogram = chip->getSummary<HistContainer<TH2F>>().fTheHistogram;
                    cHistogram->GetXaxis()->SetTitle("Number of Register Reads");
                    cHistogram->GetYaxis()->SetTitle("Register Address");
                    cHistogram->GetZaxis()->SetTitle("Mismatch");
                    cHistogram->DrawCopy();
                }
            }
        }
    }
    for(auto board: fReadMismatchesPg1)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                for(auto chip: *hybrid)
                {
                    std::string cCanvasName  = "RdMismatchesPg1_" + std::to_string(chip->getId()) + std::to_string(hybrid->getId());
                    std::string cCanvasTitle = "Read Mismatches [Pg1] " + std::to_string(chip->getId()) + std::to_string(hybrid->getId());

                    TCanvas* cCanvas = new TCanvas(cCanvasName.c_str(), cCanvasTitle.c_str(), 500, 500);
                    cCanvas->cd();
                    auto& cHistogram = chip->getSummary<HistContainer<TH2F>>().fTheHistogram;
                    cHistogram->GetXaxis()->SetTitle("Number of Register Reads");
                    cHistogram->GetYaxis()->SetTitle("Register Address");
                    cHistogram->GetZaxis()->SetTitle("Mismatch");
                    cHistogram->DrawCopy();
                }
            }
        }
    }
    for(auto board: fWrCnts)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                for(auto chip: *hybrid)
                {
                    std::string cCanvasName  = "WriteCount_" + std::to_string(chip->getId()) + std::to_string(hybrid->getId());
                    std::string cCanvasTitle = "Transaction Count [Writes] " + std::to_string(chip->getId()) + std::to_string(hybrid->getId());

                    TCanvas* cCanvas = new TCanvas(cCanvasName.c_str(), cCanvasTitle.c_str(), 500, 500);
                    cCanvas->cd();
                    auto& cHistogram = chip->getSummary<HistContainer<TH1F>>().fTheHistogram;
                    cHistogram->GetXaxis()->SetTitle("");
                    cHistogram->GetYaxis()->SetTitle("Count");
                    cHistogram->GetXaxis()->SetBinLabel(cHistogram->FindBin(0.), "Write");
                    cHistogram->GetXaxis()->SetBinLabel(cHistogram->FindBin(1.), "Read");
                    cHistogram->GetXaxis()->SetBinLabel(cHistogram->FindBin(2.), "PgToggle");
                    cHistogram->DrawCopy();
                }
            }
        }
    }
    for(auto board: fRdCnts)
    {
        for(auto opticalGroup: *board)
        {
            for(auto hybrid: *opticalGroup)
            {
                for(auto chip: *hybrid)
                {
                    std::string cCanvasName  = "ReadCount_" + std::to_string(chip->getId()) + std::to_string(hybrid->getId());
                    std::string cCanvasTitle = "Transaction Count [Reads] " + std::to_string(chip->getId()) + std::to_string(hybrid->getId());

                    TCanvas* cCanvas = new TCanvas(cCanvasName.c_str(), cCanvasTitle.c_str(), 500, 500);
                    cCanvas->cd();
                    auto& cHistogram = chip->getSummary<HistContainer<TH1F>>().fTheHistogram;
                    cHistogram->GetXaxis()->SetTitle("");
                    cHistogram->GetYaxis()->SetTitle("Count");
                    cHistogram->GetXaxis()->SetBinLabel(cHistogram->FindBin(0.), "Write");
                    cHistogram->GetXaxis()->SetBinLabel(cHistogram->FindBin(1.), "Read");
                    cHistogram->GetXaxis()->SetBinLabel(cHistogram->FindBin(2.), "PgToggle");
                    cHistogram->DrawCopy();
                }
            }
        }
    }
}

//========================================================================================================================

void DQMHistogramRegisterTest::reset(void) {}

void DQMHistogramRegisterTest::fillRegisterWriteMismatches(DetectorDataContainer& pWrites, DetectorDataContainer& pToggles, DetectorDataContainer& pVals)
{
    for(auto board: pWrites)
    {
        auto& cBrdHistPg0  = fWriteMismatchesPg0.getObject(board->getId());
        auto& cBrdHistPg1  = fWriteMismatchesPg1.getObject(board->getId());
        auto& cHistToggles = fWrMismatchesPg1.getObject(board->getId());
        auto& cHistValues  = fWrValsMismatchesPg1.getObject(board->getId());
        auto& cPageToggles = pToggles.getObject(board->getId());
        auto& cVals        = pVals.getObject(board->getId());
        for(auto opticalGroup: *board)
        {
            auto& cOGHistPg0     = cBrdHistPg0->getObject(opticalGroup->getId());
            auto& cOGHistPg1     = cBrdHistPg1->getObject(opticalGroup->getId());
            auto& cOGHist        = cHistToggles->getObject(opticalGroup->getId());
            auto& cValsOGHist    = cHistValues->getObject(opticalGroup->getId());
            auto& cPageTogglesOG = cPageToggles->getObject(opticalGroup->getId());
            auto& cValsOG        = cVals->getObject(opticalGroup->getId());
            for(auto hybrid: *opticalGroup)
            {
                auto& cValsHybrdHist    = cValsOGHist->getObject(hybrid->getId());
                auto& cHybrdHistPg0     = cOGHistPg0->getObject(hybrid->getId());
                auto& cHybrdHistPg1     = cOGHistPg1->getObject(hybrid->getId());
                auto& cHybrdHist        = cOGHist->getObject(hybrid->getId());
                auto& cPageTogglesHybrd = cPageTogglesOG->getObject(hybrid->getId());
                auto& cValsHybrd        = cValsOG->getObject(hybrid->getId());
                for(auto chip: *hybrid)
                {
                    auto& cComparisons     = chip->getSummary<std::map<uint32_t, uint32_t>>();
                    auto& cPageTogglesChip = cPageTogglesHybrd->getObject(chip->getId())->getSummary<std::map<uint32_t, uint32_t>>();
                    TH2F* cChipHistPg0     = cHybrdHistPg0->getObject(chip->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;
                    TH2F* cChipHistPg1     = cHybrdHistPg1->getObject(chip->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;
                    TH2F* cChipHistToggles = cHybrdHist->getObject(chip->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;
                    for(auto cItem: cComparisons)
                    {
                        uint8_t cRegisterAddress = cItem.first & 0xFF;
                        uint8_t cRegisterPage    = cItem.first >> 8;
                        TH2F*   cChipHist        = (cRegisterPage == 0) ? cChipHistPg0 : cChipHistPg1;
                        auto    cBin             = cChipHist->FindBin(cItem.second + 1, cRegisterAddress);
                        auto    cBinContent      = cChipHist->GetBinContent(cBin);
                        cChipHist->SetBinContent(cBin, cBinContent + 1);
                        cChipHist->SetBinError(cBin, std::sqrt(cBinContent + 1));

                        if(cPageTogglesChip.find(cItem.first) != cPageTogglesChip.end())
                        {
                            float cNtoggles = cPageTogglesChip[cItem.first];
                            cBin            = cChipHistToggles->FindBin(cNtoggles, cRegisterAddress);
                            LOG(DEBUG) << BOLDYELLOW << "Register " << +cRegisterAddress << " on page " << +cRegisterPage << " first mis-match after " << +cNtoggles << " page toggles. Bin number is "
                                       << cBin << RESET;
                            cBinContent = cChipHistToggles->GetBinContent(cBin);
                            cChipHistToggles->SetBinContent(cBin, cBinContent + 1);
                            cChipHistToggles->SetBinError(cBin, std::sqrt(cBinContent + 1));
                        }
                    }

                    auto& cValsChip       = cValsHybrd->getObject(chip->getId())->getSummary<std::map<uint8_t, uint8_t>>();
                    TH2F* cChipHistValues = cValsHybrdHist->getObject(chip->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;
                    for(auto cItem: cValsChip)
                    {
                        auto cBin        = cChipHistValues->FindBin(cItem.first, cItem.second);
                        auto cBinContent = cChipHistValues->GetBinContent(cBin);
                        cChipHistValues->SetBinContent(cBin, cBinContent + 1);
                        cChipHistValues->SetBinError(cBin, std::sqrt(cBinContent + 1));
                    }
                }
            }
        }
    }

    // for(auto board: pWrites)
    // {
    //     auto& cBrdHistPg0 = fWriteMismatchesPg0.getObject(board->getId());
    //     auto& cBrdHistPg1 = fWriteMismatchesPg1.getObject(board->getId());
    //     for(auto opticalGroup: *board)
    //     {
    //         auto& cOGHistPg0 = cBrdHistPg0->getObject(opticalGroup->getId());
    //         auto& cOGHistPg1 = cBrdHistPg1->getObject(opticalGroup->getId());
    //         for(auto hybrid: *opticalGroup)
    //         {
    //             auto& cHybrdHistPg0 = cOGHistPg0->getObject(hybrid->getId());
    //             auto& cHybrdHistPg1 = cOGHistPg1->getObject(hybrid->getId());
    //             for(auto chip: *hybrid)
    //             {
    //                 auto&  cComparisons = chip->getSummary<std::map<uint32_t,uint32_t>>();
    //                 TH2F* cChipHistPg0 = cHybrdHistPg0->getObject(chip->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;
    //                 TH2F* cChipHistPg1 = cHybrdHistPg1->getObject(chip->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;
    //                 for( auto cItem : cComparisons)
    //                 {
    //                     uint8_t cRegisterAddress = cItem.first & 0xFF;
    //                     uint8_t cRegisterPage    = cItem.first >> 8;
    //                     TH2F* cChipHist = ( cRegisterPage == 0 ) ? cChipHistPg0 : cChipHistPg1;
    //                     // LOG (INFO) << BOLDYELLOW << "Register " << +cRegisterAddress << " on page " << +cRegisterPage << " last match after " << +cItem.second << RESET;
    //                     auto cBin = cChipHist->FindBin(cItem.second, cRegisterAddress);
    //                     auto cBinContent = cChipHist->GetBinContent(cBin);
    //                     cChipHist->SetBinContent(cBin, cBinContent + 1);
    //                     cChipHist->SetBinError(cBin, std::sqrt(cBinContent + 1));
    //                 }
    //             }
    //         }
    //     }
    // }
}
void DQMHistogramRegisterTest::fillRegisterReadMismatches(DetectorDataContainer& pReads, DetectorDataContainer& pToggles, DetectorDataContainer& pVals)
{
    for(auto board: pReads)
    {
        auto& cBrdHistPg0  = fReadMismatchesPg0.getObject(board->getId());
        auto& cBrdHistPg1  = fReadMismatchesPg1.getObject(board->getId());
        auto& cHistToggles = fRdMismatchesPg1.getObject(board->getId());
        auto& cHistValues  = fRdValsMismatchesPg1.getObject(board->getId());
        auto& cPageToggles = pToggles.getObject(board->getId());
        auto& cVals        = pVals.getObject(board->getId());
        for(auto opticalGroup: *board)
        {
            auto& cOGHistPg0     = cBrdHistPg0->getObject(opticalGroup->getId());
            auto& cOGHistPg1     = cBrdHistPg1->getObject(opticalGroup->getId());
            auto& cOGHist        = cHistToggles->getObject(opticalGroup->getId());
            auto& cValsOGHist    = cHistValues->getObject(opticalGroup->getId());
            auto& cPageTogglesOG = cPageToggles->getObject(opticalGroup->getId());
            auto& cValsOG        = cVals->getObject(opticalGroup->getId());
            for(auto hybrid: *opticalGroup)
            {
                auto& cValsHybrdHist    = cValsOGHist->getObject(hybrid->getId());
                auto& cHybrdHistPg0     = cOGHistPg0->getObject(hybrid->getId());
                auto& cHybrdHistPg1     = cOGHistPg1->getObject(hybrid->getId());
                auto& cHybrdHist        = cOGHist->getObject(hybrid->getId());
                auto& cPageTogglesHybrd = cPageTogglesOG->getObject(hybrid->getId());
                auto& cValsHybrd        = cValsOG->getObject(hybrid->getId());
                for(auto chip: *hybrid)
                {
                    auto& cComparisons     = chip->getSummary<std::map<uint32_t, uint32_t>>();
                    auto& cPageTogglesChip = cPageTogglesHybrd->getObject(chip->getId())->getSummary<std::map<uint32_t, uint32_t>>();
                    TH2F* cChipHistPg0     = cHybrdHistPg0->getObject(chip->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;
                    TH2F* cChipHistPg1     = cHybrdHistPg1->getObject(chip->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;
                    TH2F* cChipHistToggles = cHybrdHist->getObject(chip->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;
                    for(auto cItem: cComparisons)
                    {
                        uint8_t cRegisterAddress = cItem.first & 0xFF;
                        uint8_t cRegisterPage    = cItem.first >> 8;
                        TH2F*   cChipHist        = (cRegisterPage == 0) ? cChipHistPg0 : cChipHistPg1;
                        auto    cBin             = cChipHist->FindBin(cItem.second, cRegisterAddress);
                        auto    cBinContent      = cChipHist->GetBinContent(cBin);
                        cChipHist->SetBinContent(cBin, cBinContent + 1);
                        cChipHist->SetBinError(cBin, std::sqrt(cBinContent + 1));

                        if(cPageTogglesChip.find(cItem.first) != cPageTogglesChip.end())
                        {
                            float cNtoggles = cPageTogglesChip[cItem.first];
                            cBin            = cChipHistToggles->FindBin(cNtoggles, cRegisterAddress);
                            LOG(DEBUG) << BOLDYELLOW << "Register " << +cRegisterAddress << " on page " << +cRegisterPage << " first mis-match after " << +cNtoggles << " page toggles. Bin number is "
                                       << cBin << RESET;
                            cBinContent = cChipHistToggles->GetBinContent(cBin);
                            cChipHistToggles->SetBinContent(cBin, cBinContent + 1);
                            cChipHistToggles->SetBinError(cBin, std::sqrt(cBinContent + 1));
                        }
                    }

                    auto& cValsChip       = cValsHybrd->getObject(chip->getId())->getSummary<std::map<uint8_t, uint8_t>>();
                    TH2F* cChipHistValues = cValsHybrdHist->getObject(chip->getId())->getSummary<HistContainer<TH2F>>().fTheHistogram;
                    for(auto cItem: cValsChip)
                    {
                        auto cBin        = cChipHistValues->FindBin(cItem.first, cItem.second);
                        auto cBinContent = cChipHistValues->GetBinContent(cBin);
                        cChipHistValues->SetBinContent(cBin, cBinContent + 1);
                        cChipHistValues->SetBinError(cBin, std::sqrt(cBinContent + 1));
                    }
                }
            }
        }
    }
}
void DQMHistogramRegisterTest::fillRegisterReadCounts(DetectorDataContainer& pReads, DetectorDataContainer& pToggles)
{
    for(auto board: pReads)
    {
        auto& cBrdHist     = fRdCnts.getObject(board->getId());
        auto& cPageToggles = pToggles.getObject(board->getId());
        for(auto opticalGroup: *board)
        {
            auto& cOGHist        = cBrdHist->getObject(opticalGroup->getId());
            auto& cPageTogglesOG = cPageToggles->getObject(opticalGroup->getId());
            for(auto hybrid: *opticalGroup)
            {
                auto& cHybrdHist        = cOGHist->getObject(hybrid->getId());
                auto& cPageTogglesHybrd = cPageTogglesOG->getObject(hybrid->getId());
                for(auto chip: *hybrid)
                {
                    auto& cReads           = chip->getSummary<size_t>();
                    auto& cPageTogglesChip = cPageTogglesHybrd->getObject(chip->getId())->getSummary<size_t>();
                    TH1F* cChipHist        = cHybrdHist->getObject(chip->getId())->getSummary<HistContainer<TH1F>>().fTheHistogram;
                    LOG(INFO) << BOLDYELLOW << "Chip#" << +chip->getId() << " " << cReads << " reads; " << cPageTogglesChip << " page toggles " << RESET;
                    // number of read transactions
                    auto cBin        = cChipHist->FindBin(1.);
                    auto cBinContent = cChipHist->GetBinContent(cBin);
                    cChipHist->SetBinContent(cBin, cBinContent + cReads);
                    // number of toggles
                    cBin        = cChipHist->FindBin(2.);
                    cBinContent = cChipHist->GetBinContent(cBin);
                    cChipHist->SetBinContent(cBin, cBinContent + cPageTogglesChip);
                }
            }
        }
    }
}
void DQMHistogramRegisterTest::fillRegisterWriteCounts(DetectorDataContainer& pWrites, DetectorDataContainer& pReads, DetectorDataContainer& pToggles)
{
    for(auto board: pWrites)
    {
        auto& cBrdHist     = fWrCnts.getObject(board->getId());
        auto& cPageToggles = pToggles.getObject(board->getId());
        auto& cReads       = pReads.getObject(board->getId());
        for(auto opticalGroup: *board)
        {
            auto& cOGHist        = cBrdHist->getObject(opticalGroup->getId());
            auto& cPageTogglesOG = cPageToggles->getObject(opticalGroup->getId());
            auto& cReadsOG       = cReads->getObject(opticalGroup->getId());
            for(auto hybrid: *opticalGroup)
            {
                auto& cHybrdHist        = cOGHist->getObject(hybrid->getId());
                auto& cPageTogglesHybrd = cPageTogglesOG->getObject(hybrid->getId());
                auto& cReadsHybrd       = cReadsOG->getObject(hybrid->getId());
                for(auto chip: *hybrid)
                {
                    auto& cWrites          = chip->getSummary<size_t>();
                    auto& cPageTogglesChip = cPageTogglesHybrd->getObject(chip->getId())->getSummary<size_t>();
                    auto& cReadsChip       = cReadsHybrd->getObject(chip->getId())->getSummary<size_t>();
                    TH1F* cChipHist        = cHybrdHist->getObject(chip->getId())->getSummary<HistContainer<TH1F>>().fTheHistogram;
                    LOG(INFO) << BOLDYELLOW << "Chip#" << +chip->getId() << " " << cWrites << " writes; " << cReadsChip << " reads and " << cPageTogglesChip << " page toggles " << RESET;
                    // number of write transactions
                    auto cBin        = cChipHist->FindBin(0.);
                    auto cBinContent = cChipHist->GetBinContent(cBin);
                    cChipHist->SetBinContent(cBin, cBinContent + cWrites);
                    // number of read transactions
                    cBin        = cChipHist->FindBin(1.);
                    cBinContent = cChipHist->GetBinContent(cBin);
                    cChipHist->SetBinContent(cBin, cBinContent + cReadsChip);
                    // number of toggles
                    cBin        = cChipHist->FindBin(2.);
                    cBinContent = cChipHist->GetBinContent(cBin);
                    cChipHist->SetBinContent(cBin, cBinContent + cPageTogglesChip);
                }
            }
        }
    }
}
void DQMHistogramRegisterTest::parseSettings(const Ph2_Parser::SettingsMap& pSettingsMap)
{
    // auto cSetting = pSettingsMap.find("StartLatency");
    // if(cSetting != std::end(pSettingsMap))
    //     fStartLatency = cSetting->second;
    // else
    //     fStartLatency = 0;

    // cSetting = pSettingsMap.find("LatencyRange");
    // if(cSetting != std::end(pSettingsMap))
    //     fLatencyRange = cSetting->second;
    // else
    //     fLatencyRange = 512;
}
