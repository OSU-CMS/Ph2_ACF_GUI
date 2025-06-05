#include "HybridTester.h"
#ifdef __USE_ROOT__
#include <ctime>

// fill the Histograms, count the hits and increment Vcth
struct HistogramFiller : public HwDescriptionVisitor
{
    TH1F*  fBotHist;
    TH1F*  fTopHist;
    Event* fEvent;

    HistogramFiller(TH1F* pBotHist, TH1F* pTopHist, Event* pEvent) : fBotHist(pBotHist), fTopHist(pTopHist), fEvent(pEvent) {}

    void visit(ChipContainer* pCbc)
    {
        ReadoutChip*      theCbc         = static_cast<ReadoutChip*>(pCbc);
        std::vector<bool> cDataBitVector = fEvent->DataBitVector(theCbc->getHybridId(), theCbc->getId());

        for(uint32_t cId = 0; cId < NCHANNELS; cId++)
        {
            if(cDataBitVector.at(cId))
            {
                uint32_t globalChannel = (theCbc->getId() * 254) + cId;

                //              LOG(INFO) << "Channel " << globalChannel << " VCth " << int(pCbc.getReg( "VCth" )) ;
                // find out why histograms are not filling!
                if(globalChannel % 2 == 0)
                    fBotHist->Fill(globalChannel / 2);
                else
                    fTopHist->Fill((globalChannel - 1) / 2);
            }
        }
    }
};

// Default C'tor
HybridTester::HybridTester() : Tool() {}
// Default D'tor
HybridTester::~HybridTester() {}

void HybridTester::ReconfigureCBCRegisters(std::string pDirectoryName)
{
    for(auto cBoard: *fDetectorContainer)
    {
        BeBoard* theBoard = static_cast<BeBoard*>(cBoard);
        fBeBoardInterface->ChipReset(theBoard);

        trigSource = fBeBoardInterface->ReadBoardReg(theBoard, "fc7_daq_cnfg.fast_command_block.trigger_source");
        LOG(INFO) << int(trigSource);

        trigSource = fBeBoardInterface->ReadBoardReg(theBoard, "fc7_daq_cnfg.fast_command_block.trigger_source");
        LOG(INFO) << int(trigSource);

        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cCbc: *cHybrid)
                {
                    ReadoutChip* theCbc = static_cast<ReadoutChip*>(cCbc);
                    std::string  pRegFile;
                    char         buffer[120];

                    if(pDirectoryName.empty())
                        sprintf(buffer, "%s/FE%dCBC%d.txt", fDirectoryName.c_str(), cHybrid->getId(), cCbc->getId());
                    else
                        sprintf(buffer, "%s/FE%dCBC%d.txt", pDirectoryName.c_str(), cHybrid->getId(), cCbc->getId());

                    pRegFile = buffer;
                    theCbc->loadfRegMap(pRegFile);
                    fReadoutChipInterface->ConfigureChip(theCbc);
                    LOG(INFO) << GREEN << "\t\t Successfully reconfigured CBC" << int(cCbc->getId()) << "'s regsiters from " << pRegFile << " ." << RESET;
                }
            }
        }

        fBeBoardInterface->ChipReSync(theBoard);
    }
}

void HybridTester::InitializeHists()
{
    uint32_t cNBinsX = (fNCbc * 254) / 2;
    TString  cFrontName("fHistTop");
    fHistTop = (TH1F*)(gROOT->FindObject(cFrontName));

    if(fHistTop) delete fHistTop;

    fHistTop = new TH1F(cFrontName, "Front Pad Channels; Pad Number; Occupancy [%]", cNBinsX, -0.5, cNBinsX - 0.5);
    fHistTop->SetFillColor(4);
    fHistTop->SetFillStyle(3001);

    TString cBackName("fHistBottom");
    fHistBottom = (TH1F*)(gROOT->FindObject(cBackName));

    if(fHistBottom) delete fHistBottom;

    fHistBottom = new TH1F(cBackName, "Back Pad Channels; Pad Number; Occupancy [%]", cNBinsX, -0.5, cNBinsX - 0.5);
    fHistBottom->SetFillColor(4);
    fHistBottom->SetFillStyle(3001);

    TString cFrontNameMerged("fHistTopMerged");
    fHistTopMerged = (TH1F*)(gROOT->FindObject(cFrontNameMerged));

    if(fHistTopMerged) delete fHistTopMerged;

    fHistTopMerged = new TH1F(cFrontNameMerged, "Front Pad Channels; Pad Number; Occupancy [%]", cNBinsX, -0.5, cNBinsX - 0.5);
    fHistTopMerged->SetFillColor(4);
    fHistTopMerged->SetFillStyle(3001);

    TString cBackNameMerged("fHistBottomMerged");
    fHistBottomMerged = (TH1F*)(gROOT->FindObject(cBackNameMerged));

    if(fHistBottomMerged) delete fHistBottomMerged;

    fHistBottomMerged = new TH1F(cBackNameMerged, "Back Pad Channels; Pad Number; Occupancy [%]", cNBinsX, -0.5, cNBinsX - 0.5);
    fHistBottomMerged->SetFillColor(4);
    fHistBottomMerged->SetFillStyle(3001);

    TString cOccupancyBottom("fHistOccupancyBottom");
    fHistOccupancyBottom = (TH1F*)(gROOT->FindObject(cOccupancyBottom));

    if(fHistOccupancyBottom) delete fHistOccupancyBottom;

    fHistOccupancyBottom = new TH1F(cOccupancyBottom, "Back Pad Channels.", (int)(110 / 1.0), -0.5, 110.0 - 0.5);
    fHistOccupancyBottom->SetStats(1);
    fHistOccupancyBottom->SetFillColor(4);
    fHistOccupancyBottom->SetLineColor(4);
    fHistOccupancyBottom->SetFillStyle(3004);
    fHistOccupancyBottom->GetYaxis()->SetTitle("Number of Strips");
    fHistOccupancyBottom->GetXaxis()->SetTitle("Occupancy (%)");
    fHistOccupancyBottom->GetXaxis()->SetRangeUser(0.0, 101.0);

    TString cOccupancyTop("fHistOccupancyTop");
    fHistOccupancyTop = (TH1F*)(gROOT->FindObject(cOccupancyTop));

    if(fHistOccupancyTop) delete fHistOccupancyTop;

    fHistOccupancyTop = new TH1F(cOccupancyTop, "Top Pad Channels.", (int)(110 / 1.0), -0.5, 110.0 - 0.5);
    fHistOccupancyTop->SetStats(1);
    fHistOccupancyTop->SetFillColor(3);
    fHistOccupancyTop->SetLineColor(3);
    fHistOccupancyTop->SetFillStyle(3004);
    fHistOccupancyTop->GetYaxis()->SetTitle("Number of Strips");
    fHistOccupancyTop->GetXaxis()->SetTitle("Occupancy (%)");
    fHistOccupancyTop->GetXaxis()->SetRangeUser(0.0, 101.0);

    // Now the Histograms for SCurves
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                uint32_t cHybridId = cHybrid->getId();
                uint16_t cMaxRange = 1023;
                fType              = static_cast<OuterTrackerHybrid*>(cHybrid)->getFrontEndType();

                for(auto cCbc: *cHybrid)
                {
                    uint32_t cCbcId = cCbc->getId();

                    TString  cName   = Form("SCurve_Fe%d_Cbc%d", cHybridId, cCbcId);
                    TObject* cObject = static_cast<TObject*>(gROOT->FindObject(cName));

                    if(cObject) delete cObject;

                    TH1F* cTmpScurve = new TH1F(cName, Form("Noise Occupancy Chip%d; VCth; Counts", cCbcId), cMaxRange, 0, cMaxRange);
                    cTmpScurve->SetMarkerStyle(8);
                    bookHistogram(cCbc, "Scurve", cTmpScurve);
                    fSCurveMap[cCbc] = cTmpScurve;

                    cName   = Form("SCurveFit_Fe%d_Cbc%d", cHybridId, cCbcId);
                    cObject = static_cast<TObject*>(gROOT->FindObject(cName));

                    if(cObject) delete cObject;

                    TF1* cTmpFit = new TF1(cName, MyErf, 0, cMaxRange, 2);
                    bookHistogram(cCbc, "ScurveFit", cTmpFit);

                    fFitMap[cCbc] = cTmpFit;
                }
            }
        }
    }
}

void HybridTester::InitialiseSettings()
{
    auto cSetting       = fSettingsMap.find("Threshold_NSigmas");
    fSigmas             = (cSetting != std::end(fSettingsMap)) ? boost::any_cast<double>(cSetting->second) : 4;
    cSetting            = fSettingsMap.find("Nevents");
    fTotalEvents        = (cSetting != std::end(fSettingsMap)) ? boost::any_cast<double>(cSetting->second) : 999;
    cSetting            = fSettingsMap.find("HoleMode");
    fHoleMode           = (cSetting != std::end(fSettingsMap)) ? boost::any_cast<double>(cSetting->second) : true;
    cSetting            = fSettingsMap.find("TestPulsePotentiometer");
    fTestPulseAmplitude = (cSetting != std::end(fSettingsMap)) ? boost::any_cast<double>(cSetting->second) : 0x7F;

    // cSetting = fSettingsMap.find ( "TriggerSource" );
    // if ( cSetting != std::end ( fSettingsMap ) ) trigSource = boost::any_cast<double>(cSetting->second);
    // LOG (INFO)  <<int (trigSource);

    for(auto cBoard: *fDetectorContainer)
    {
        trigSource = fBeBoardInterface->ReadBoardReg(static_cast<BeBoard*>(cBoard), "fc7_daq_cnfg.fast_command_block.trigger_source");
        LOG(INFO) << int(trigSource);
    }
    // LOG(INFO) << "Read the f llowing Settings: " ;
    // LOG(INFO) << "Hole Mode: " << fHoleMode << std::endl << "NEvents: " << fTotalEvents << std::endl << "NSigmas: "
    // << fSigmas ;
}

void HybridTester::Initialize(bool pThresholdScan)
{
    fThresholdScan = pThresholdScan;
    gStyle->SetOptStat(000000);
    // gStyle->SetTitleOffset( 1.3, "Y" );
    //  special Visito class to count objects
    Counter cCbcCounter;
    accept(cCbcCounter);
    fNCbc = cCbcCounter.getNChip();

    fDataCanvas = new TCanvas("fDataCanvas", "SingleStripEfficiency", 10, 0, 500, 500);
    fDataCanvas->Divide(2);

    fSummaryCanvas = new TCanvas("fSummaryCanvas", "Summarizing Hybrid Efficiency", 10, 0, 500, 500);
    fSummaryCanvas->Divide(2);

    if(fThresholdScan)
    {
        fSCurveCanvas = new TCanvas("fSCurveCanvas", "Noise Occupancy as function of VCth");
        fSCurveCanvas->Divide(fNCbc);
    }

    InitializeHists();
    InitialiseSettings();

    fNoisyChannelsTop.clear();
    fNoisyChannelsBottom.clear();
    fDeadChannelsTop.clear();
    fDeadChannelsBottom.clear();
}

uint32_t HybridTester::fillSCurves(BeBoard* pBoard, Event* pEvent, uint16_t pValue)
{
    uint32_t cHitCounter = 0;

    for(auto cOpticalGroup: *pBoard)
    {
        for(auto cHybrid: *cOpticalGroup)
        {
            for(auto cCbc: *cHybrid)
            {
                // SS
                /*TH1F* sCurveHist = static_cast<TH1F*>( getHist( cCbc, "Scurve" ) );
                uint32_t cbcEventCounter = 0;
                for ( uint32_t cId = 0; cId < NCHANNELS; cId++ )
                {
                    if ( pEvent->DataBit( cCbc->getHybridId(), cCbc->getId(), 0, cId ) )
                    {
                        sCurveHist->Fill( pValue );
                        cHitCounter++;
                        cbcEventCounter++;
                    }
                }*/

                auto cScurve = fSCurveMap.find(cCbc);

                if(cScurve == fSCurveMap.end())
                    LOG(INFO) << "Error: could not find an Scurve object for Chip " << int(cCbc->getId());
                else
                {
                    // for ( uint32_t cId = 0; cId < NCHANNELS; cId++ )
                    //{
                    // if ( pEvent->DataBit ( cCbc->getHybridId(), cCbc->getId(), 0, cId ) )
                    //{
                    // cScurve->second->Fill ( pValue );
                    // cHitCounter++;
                    //}
                    //}
                    // experimental

                    auto cHits = pEvent->GetHits(cHybrid->getId(), cCbc->getId());
                    cHitCounter += cHits.size();

                    for(__attribute__((unused)) auto cHit: cHits) cScurve->second->Fill(pValue);
                }
            }
        }
    }

    return cHitCounter;
}

void HybridTester::ScanThresholds()
{
    LOG(INFO) << "Mesuring Efficiency per Strip ... ";
    LOG(INFO) << "Taking data with " << fTotalEvents << " Events!";

    int      cVcthStep = 2;
    uint16_t cMaxValue = 0x03FF;
    uint16_t cVcth     = (fHoleMode) ? cMaxValue : 0x00;
    int      cStep     = (fHoleMode) ? (-1 * cVcthStep) : cVcthStep;

    int iVcth = +(cVcth);
    // LOG(INFO) << RED << "Vcth = " <<  iVcth << RESET ;

    // simple VCth loop
    ThresholdVisitor cVisitor(fReadoutChipInterface, 0);

    while(0 <= iVcth && iVcth <= cMaxValue)
    {
        cVisitor.setThreshold(iVcth);
        accept(cVisitor);

        fHistTop->GetYaxis()->SetRangeUser(0, fTotalEvents);
        fHistBottom->GetYaxis()->SetRangeUser(0, fTotalEvents);

        for(auto pBoard: *fDetectorContainer)
        {
            BeBoard* theBoard = static_cast<BeBoard*>(pBoard);
            uint32_t cN       = 1;
            uint32_t cNthAcq  = 0;

            fBeBoardInterface->Start(theBoard);

            while(cN <= fTotalEvents)
            {
                // Run( theBoard, cNthAcq );
                ReadData(theBoard);
                const std::vector<Event*>& events = GetEvents();

                // Loop over Events from this Acquisition
                for(auto& cEvent: events)
                {
                    HistogramFiller cFiller(fHistBottom, fHistTop, cEvent);
                    theBoard->accept(cFiller);

                    fillSCurves(theBoard, cEvent, cVcth);

                    if(cN % 100 == 0)
                    {
                        updateSCurveCanvas(theBoard);
                        UpdateHists();
                    }

                    cN++;
                }

                cNthAcq++;
            }

            fBeBoardInterface->Stop(theBoard);
        }

        fHistTop->Scale(100 / double_t(fTotalEvents));
        fHistTop->GetYaxis()->SetRangeUser(0, 100);
        fHistBottom->Scale(100 / double_t(fTotalEvents));
        fHistBottom->GetYaxis()->SetRangeUser(0, 100);
        UpdateHists();

        // LOG(INFO) << RED << "Vcth = " <<  iVcth << RESET << " " ;
        // LOG(INFO) << GREEN << "Mean occupancy at the Top side: " << fHistTop->Integral()/(double)(fNCbc*127) << RESET
        // << " "; LOG(INFO) << BLUE << "Mean occupancy at the Bottom side: " <<
        // fHistBottom->Integral()/(double)(fNCbc*127) << RESET ;
        iVcth = +(cVcth + cStep);
        cVcth += cStep;
    }

    // Fit and save the SCurve & Fit - extract the right threshold
    // TODO
    processSCurves(fTotalEvents);

    // normalize noise scan
    /*for ( BeBoard* pBoard : fBoardVector )
    {
        for ( auto cHybrid : pBoard->fModuleVector )
        {
            for ( auto cCbc : cHybrid->fReadoutChipVector )
            {
                fSCurveCanvas->cd(cCbc->getId()+1);
                TH1F* sCurveHist = static_cast<TH1F*>( getHist( cCbc, "Scurve" ) );
                sCurveHist->Scale(100./(NCHANNELS*fTotalEvents));
                sCurveHist->GetYaxis()->SetTitle("Occupancy (%)");
                sCurveHist->GetYaxis()->SetTitleOffset(1.2);
                sCurveHist->DrawCopy("P0");

                sCurveHist->Write( sCurveHist->GetName(), TObject::kOverwrite );
                fSCurveCanvas->cd(cCbc->getId()+1)->Update();
            }
        }
    }*/
}

void HybridTester::ScanThreshold()
{
    LOG(INFO) << "Scanning noise Occupancy to find threshold for test with external source ... ";

    // Necessary variables
    uint32_t cEventsperVcth = fTotalEvents;
    bool     cNonZero       = false;
    bool     cAllOne        = false;
    uint32_t cAllOneCounter = 0;
    uint16_t cDoubleVcth    = 0;
    uint16_t cMaxValue      = 0x03FF;
    uint16_t cVcth          = (fHoleMode) ? cMaxValue : 0x00;
    int      cStep          = (fHoleMode) ? -10 : 10;

    // Adaptive VCth loop
    ThresholdVisitor cVisitor(fReadoutChipInterface, 0);

    while(0x00 == cVcth && cVcth <= cMaxValue)
    {
        if(cAllOne) break;

        if(cVcth == cDoubleVcth)
        {
            cVcth += cStep;
            continue;
        }

        // Set current Vcth value on all Chip's
        cVisitor.setThreshold(cVcth);
        accept(cVisitor);
        uint32_t cN          = 1;
        uint32_t cNthAcq     = 0;
        uint32_t cHitCounter = 0;

        // maybe restrict to pBoard? instead of looping?
        if(cAllOne) break;

        for(auto pBoard: *fDetectorContainer)
        {
            BeBoard* theBoard = static_cast<BeBoard*>(pBoard);

            fBeBoardInterface->Start(theBoard);

            while(cN <= cEventsperVcth)
            {
                // Run( theBoard, cNthAcq );
                ReadData(theBoard);
                const std::vector<Event*>& events = GetEvents();

                // Loop over Events from this Acquisition
                for(auto& cEvent: events)
                {
                    // loop over Hybrids & Cbcs and count hits separately
                    cHitCounter += fillSCurves(theBoard, cEvent, cVcth);
                    cN++;
                }

                cNthAcq++;
            }

            fBeBoardInterface->Stop(theBoard);
            // LOG(INFO) << +cVcth << " " << cHitCounter ;
            // Draw the thing after each point
            updateSCurveCanvas(theBoard);

            // check if the hitcounter is all ones

            if(cNonZero == false && cHitCounter != 0)
            {
                cDoubleVcth = cVcth;
                cNonZero    = true;
                cVcth -= 2 * cStep;
                cStep /= 10;
                continue;
            }

            if(cNonZero && (cHitCounter != 0))
            {
                // check if all Cbcs have reached full occupancy
                if(cHitCounter > 0.95 * cEventsperVcth * fNCbc * NCHANNELS) cAllOneCounter++;

                // add a second check if the global SCurve slope is 0 for 10 consecutive Vcth values
            }

            if(cAllOneCounter >= 10) cAllOne = true;

            // if ( cSlopeZeroCounter >= 10 ) cSlopeZero = true;

            if(cAllOne)
            {
                LOG(INFO) << "All strips firing -- ending the scan at VCth " << +cVcth;
                break;
            }

            // else if ( cSlopeZero )
            // {
            //   LOG(INFO) << "Slope of SCurve 0 -- ending the scan at VCth " << +cVcth ;
            //  break;
            // }

            cVcth += cStep;
        }
    }

    // Fit and save the SCurve & Fit - extract the right threshold
    // TODO
    processSCurves(cEventsperVcth);
}

void HybridTester::processSCurves(uint32_t pEventsperVcth)
{
    for(auto cScurve: fSCurveMap)
    {
        fSCurveCanvas->cd(cScurve.first->getId() + 1);

        cScurve.second->Scale(1.0 / double_t(pEventsperVcth * NCHANNELS));
        cScurve.second->Draw("P");
        // Write to file
        cScurve.second->Write(cScurve.second->GetName(), TObject::kOverwrite);

        // Estimate parameters for the Fit
        double cFirstNon0(0);
        double cFirst1(0);

        // Not Hole Mode
        if(!fHoleMode)
        {
            for(Int_t cBin = 1; cBin <= cScurve.second->GetNbinsX(); cBin++)
            {
                double cContent = cScurve.second->GetBinContent(cBin);

                if(!cFirstNon0)
                {
                    if(cContent) cFirstNon0 = cScurve.second->GetBinCenter(cBin);
                }
                else if(cContent == 1)
                {
                    cFirst1 = cScurve.second->GetBinCenter(cBin);
                    break;
                }
            }
        }
        // Hole mode
        else
        {
            for(Int_t cBin = cScurve.second->GetNbinsX(); cBin >= 1; cBin--)
            {
                double cContent = cScurve.second->GetBinContent(cBin);

                if(!cFirstNon0)
                {
                    if(cContent) cFirstNon0 = cScurve.second->GetBinCenter(cBin);
                }
                else if(cContent == 1)
                {
                    cFirst1 = cScurve.second->GetBinCenter(cBin);
                    break;
                }
            }
        }

        // Get rough midpoint & width
        double cMid   = (cFirst1 + cFirstNon0) * 0.5;
        double cWidth = (cFirst1 - cFirstNon0) * 0.5;

        // find the corresponding fit
        auto cFit = fFitMap.find(cScurve.first);

        if(cFit == std::end(fFitMap))
            LOG(INFO) << "Error: could not find Fit for Chip " << int(cScurve.first->getId());
        else
        {
            // Fit
            cFit->second->SetParameter(0, cMid);
            cFit->second->SetParameter(1, cWidth);

            cScurve.second->Fit(cFit->second, "RNQ+");
            cScurve.second->SetLineStyle(2);
            cFit->second->Draw("same");

            // Write to File
            cFit->second->Write(cFit->second->GetName(), TObject::kOverwrite);

            // TODO
            // Set new VCth - for the moment each Chip gets his own Vcth - I shold add a mechanism to take one that
            // works for all!
            double_t pedestal = cFit->second->GetParameter(0);
            double_t noise    = cFit->second->GetParameter(1);

            uint16_t cThreshold = ceil(pedestal + fSigmas * fabs(noise));

            LOG(INFO) << "Identified a noise Occupancy of 50% at VCth " << static_cast<int>(pedestal) << " -- increasing by " << fSigmas << " sigmas (=" << fabs(noise) << ") to " << +cThreshold
                      << " for Chip " << int(cScurve.first->getId());

            TLine* cLine = new TLine(cThreshold, 0, cThreshold, 1);
            cLine->SetLineWidth(3);
            cLine->SetLineColor(kCyan);
            cLine->Draw("same");

            ThresholdVisitor cVisitor(fReadoutChipInterface, cThreshold);
            static_cast<ReadoutChip*>(cScurve.first)->accept(cVisitor);
        }
    }

    fSCurveCanvas->Update();

    // Write and Save the Canvas as PDF
    fSCurveCanvas->Write(fSCurveCanvas->GetName(), TObject::kOverwrite);
    std::string cPdfName = fDirectoryName + "/NoiseOccupancy.pdf";
    fSCurveCanvas->SaveAs(cPdfName.c_str());
}

void HybridTester::updateSCurveCanvas(BeBoard* pBoard)
{
    /*for ( auto cHybrid : pBoard->fModuleVector )
    {
        for ( auto cCbc : cHybrid->fReadoutChipVector )
        {
            fSCurveCanvas->cd(cCbc->getId()+1);
            TH1F* sCurveHist = static_cast<TH1F*>( getHist( cCbc, "Scurve" ) );
            sCurveHist->DrawCopy("P0");
            fSCurveCanvas->cd(cCbc->getId()+1)->Update();
        }
    }*/

    // Here iterate over the fScurveMap and update
    fSCurveCanvas->cd();

    for(auto cOpticalGroup: *pBoard)
    {
        for(auto cHybrid: *cOpticalGroup)
        {
            for(auto cCbc: *cHybrid)
            {
                uint32_t cCbcId  = cCbc->getId();
                auto     cScurve = fSCurveMap.find(cCbc);

                if(cScurve == fSCurveMap.end())
                    LOG(INFO) << "Error: could not find an Scurve object for Chip " << int(cCbc->getId());
                else
                {
                    fSCurveCanvas->cd(cCbcId + 1);
                    cScurve->second->DrawCopy("P");
                }
            }
        }
    }

    fSCurveCanvas->Update();
    this->HttpServerProcess();
}

void HybridTester::TestRegisters()
{
    // This method has to be followed by a configure call, otherwise the CBCs will be in an undefined state
    struct RegTester : public HwDescriptionVisitor
    {
        ChipInterface*                            fInterface;
        std::map<uint32_t, std::set<std::string>> fBadRegisters;
        RegTester(ChipInterface* pInterface, uint32_t pNCbc) : fInterface(pInterface)
        {
            std::set<std::string> tempset;
            uint32_t              cCbcIterator = 0;

            for(cCbcIterator = 0; cCbcIterator < pNCbc; cCbcIterator++) fBadRegisters[cCbcIterator] = tempset;
        }

        void visit(ChipContainer* pCbc)
        {
            uint8_t cFirstBitPattern  = 0xAA;
            uint8_t cSecondBitPattern = 0x55;

            ReadoutChip* theCbc = static_cast<ReadoutChip*>(pCbc);

            ChipRegMap cMap = theCbc->getRegMap();

            for(const auto& cReg: cMap)
            {
                if(!fInterface->WriteChipReg(theCbc, cReg.first, cFirstBitPattern, true)) fBadRegisters[pCbc->getId()].insert(cReg.first);

                if(!fInterface->WriteChipReg(theCbc, cReg.first, cSecondBitPattern, true)) fBadRegisters[pCbc->getId()].insert(cReg.first);
            }
        }

        void dumpResult(std::string fDirectoryName)
        {
            std::ofstream report(fDirectoryName + "/registers_test.txt"); // Creates a file in the current directory
            report << "Testing Chip Registers one-by-one with complimentary bit-patterns (0xAA, 0x55)" << std::endl;

            for(const auto& cCbc: fBadRegisters)
            {
                report << "Malfunctioning Registers on Chip " << cCbc.first << " : " << std::endl;

                for(const auto& cReg: cCbc.second) report << cReg << std::endl;
            }

            report.close();
            LOG(INFO) << "Channels diagnosis report written to: " + fDirectoryName + "/registers_test.txt";
        }
    };

    // This should probably be done in the top level application but there I do not have access to the settings map
    time_t start_time = time(0);
    char*  start      = ctime(&start_time);
    LOG(INFO) << "start: " << start;
    LOG(INFO) << std::endl << "Running registers testing tool ... ";
    RegTester cRegTester(fReadoutChipInterface, fNCbc);
    accept(cRegTester);
    cRegTester.dumpResult(fDirectoryName);
    LOG(INFO) << "Done testing registers, re-configuring to calibrated state!";
    start_time = time(0);
    char* stop = ctime(&start_time);
    LOG(INFO) << "stop: " << stop;
    ConfigureHw();
}

void HybridTester::DisplayGroupsContent(std::array<std::vector<std::array<int, 5>>, 8> pShortedGroupsArray)
{
    std::stringstream ss;

    for(int i = 0; i < 8; i++)
    {
        ss << "TP group ID: " << i << std::endl;

        for(auto cShortsVector: pShortedGroupsArray[i])
        {
            for(auto i: cShortsVector) ss << i << ' ';

            ss << std::endl;
        }

        ss << std::endl;
    }

    LOG(INFO) << ss.str();
}

std::vector<std::array<int, 2>> HybridTester::MergeShorts(std::vector<std::array<int, 2>> pShortA, std::vector<std::array<int, 2>> pShortB)
{
    std::stringstream ss;

    for(auto cChannel: pShortA)
    {
        if(!CheckChannelInShortPresence(cChannel, pShortB)) pShortB.push_back(cChannel);
    }

    for(auto cMemberChannel: pShortB)
    {
        for(auto i: cMemberChannel) ss << i << ' ';
    }

    ss << std::endl;
    LOG(INFO) << ss.str();

    return pShortB;
}

bool HybridTester::CheckShortsConnection(std::vector<std::array<int, 2>> pShortA, std::vector<std::array<int, 2>> pShortB)
{
    for(auto cChannel: pShortA)
    {
        if(CheckChannelInShortPresence(cChannel, pShortB)) return true;
    }

    return false;
}

bool HybridTester::CheckChannelInShortPresence(std::array<int, 2> pShortedChannel, std::vector<std::array<int, 2>> pShort)
{
    for(auto cChannel: pShort)
    {
        if(cChannel[0] == pShortedChannel[0] && cChannel[1] == pShortedChannel[1]) return true;
    }

    return false;
}

void HybridTester::ReconstructShorts(std::array<std::vector<std::array<int, 5>>, 8> pShortedGroupsArray)
{
    std::stringstream ss;
    ss << std::endl << "---------Creating shorted pairs-----------------" << std::endl;
    std::vector<std::vector<std::array<int, 2>>> cShortsVector;
    std::vector<std::array<int, 2>>              cShort;
    std::array<int, 2>                           temp_shorted_channel;

    std::array<int, 2> best_candidate;
    int                best_candidate_index  = 0;
    int                index                 = 0;
    int                sub_index             = 0;
    int                cross_side_punishment = 2;
    int                smallest_distance     = 10000;
    bool               matching_channel_found;

    for(auto cShortedChannelsGroup: pShortedGroupsArray)
    {
        for(auto cShortedChannelInfo: cShortedChannelsGroup)
        {
            temp_shorted_channel[0] = cShortedChannelInfo[0];
            temp_shorted_channel[1] = cShortedChannelInfo[1];

            cShort.push_back(temp_shorted_channel);
            cShort.push_back(temp_shorted_channel);

            index                  = 0;
            matching_channel_found = false;

            for(auto cCandidate: pShortedGroupsArray[cShortedChannelInfo[2]])
            {
                if(cShortedChannelInfo[3] == cCandidate[2])
                {
                    if(smallest_distance > (cross_side_punishment * (cShort[0][0] - cCandidate[0]) * (cShort[0][0] - cCandidate[0]) + (cShort[0][1] - cCandidate[1]) * (cShort[0][1] - cCandidate[1])))
                    {
                        smallest_distance = (cross_side_punishment * (cShort[0][0] - cCandidate[0]) * (cShort[0][0] - cCandidate[0]) + (cShort[0][1] - cCandidate[1]) * (cShort[0][1] - cCandidate[1]));
                        best_candidate[0] = cCandidate[0];
                        best_candidate[1] = cCandidate[1];
                        best_candidate_index   = index;
                        matching_channel_found = true;
                    }
                }

                index++;
            }

            if(matching_channel_found)
            {
                if(best_candidate[0] <= cShort[0][0] && best_candidate[1] < cShort[0][1])
                {
                    cShort[0][0] = best_candidate[0];
                    cShort[0][1] = best_candidate[1];
                }

                else
                {
                    cShort[1][0] = best_candidate[0];
                    cShort[1][1] = best_candidate[1];
                }

                pShortedGroupsArray[cShortedChannelInfo[2]].erase(pShortedGroupsArray[cShortedChannelInfo[2]].begin() + best_candidate_index);

                for(auto cMemberChannel: cShort)
                {
                    for(auto i: cMemberChannel) ss << i << ' ';
                }

                ss << "smallest distance: " << smallest_distance << std::endl;
                // DisplayGroupsContent(pShortedGroupsArray);

                cShortsVector.push_back(cShort);
            }
            else
                ss << "ERROR: No matching channel found for detected short (watch the level of noise)!" << std::endl;

            smallest_distance = 10000;
            cShort.clear();
        }
    }

    ss << "---------Merging shorts-------------------------" << std::endl;
    index = cShortsVector.size();
    ss << "Number of shorted connections found: " << index << std::endl;

    while(index > 1)
    {
        index--;
        sub_index = index;

        while(sub_index > 0)
        {
            sub_index--;

            if(CheckShortsConnection(cShortsVector[index], cShortsVector[sub_index]))
            {
                cShortsVector[sub_index] = MergeShorts(cShortsVector[index], cShortsVector[sub_index]);
                cShortsVector.erase(cShortsVector.begin() + index);
                break;
            }
        }
    }

    ss << "---------Outcome--------------------------------" << std::endl;

    for(auto someShort: cShortsVector)
    {
        for(auto cMemberChannel: someShort)
        {
            for(auto i: cMemberChannel) ss << i << ' ';
        }

        ss << std::endl;
    }

    LOG(INFO) << ss.str();
}

void HybridTester::SetBeBoardForShortsFinding(BeBoard* pBoard)
{
    if(pBoard->getBoardType() == BoardType::D19C) fBeBoardInterface->WriteBoardReg(pBoard, "fc7_daq_cnfg.fast_command_block.test_pulse.delay_after_test_pulse", 1);
    setFWTestPulse(true);

    // (potential, group, enable test pulse, hole mode)
    setSystemTestPulse(fTestPulseAmplitude, 0x00, true, fHoleMode);

    // edit G.A: in order to be compatible with CBC3 (9 bit trigger latency) the recommended method is this:
    LatencyVisitor cLatencyVisitor(fReadoutChipInterface, 0x01);
    this->accept(cLatencyVisitor);

    // Take the default VCth which should correspond to the pedestal and add 8 depending on the mode to exclude noise
    // ThresholdVisitor in read mode
    ThresholdVisitor cThresholdVisitor(fReadoutChipInterface);
    this->accept(cThresholdVisitor);
    uint16_t cVcth = cThresholdVisitor.getThreshold();

    int cVcthStep = (fHoleMode == 1) ? +10 : -10;
    LOG(INFO) << "VCth value from config file is: " << +cVcth << " ;  changing by " << cVcthStep << "  to " << +(cVcth + cVcthStep) << " supress noise hits for crude latency scan!";
    cVcth += cVcthStep;

    //  Set that VCth Value on all FEs
    cThresholdVisitor.setOption('w');
    cThresholdVisitor.setThreshold(cVcth);
    this->accept(cThresholdVisitor);
}

void HybridTester::SetTestGroup(BeBoard* pBoard, uint8_t pTestGroup)
{
    for(auto cOpticalGroup: *pBoard)
    {
        for(auto cHybrid: *cOpticalGroup)
        {
            for(auto cCbc: *cHybrid)
            {
                std::vector<std::pair<std::string, uint16_t>> cRegVec;
                uint16_t                                      cRegValue = this->to_reg(0, pTestGroup);

                cRegVec.push_back(std::make_pair("TestPulseDel&ChanGroup", cRegValue));

                this->fReadoutChipInterface->WriteChipMultReg(static_cast<ReadoutChip*>(cCbc), cRegVec);
            }
        }
    }
}

void HybridTester::FindShorts()
{
    std::stringstream                              ss;
    uint8_t                                        cTestPulseGroupId = 0;
    std::array<int, 5>                             cShortedChannelInfo;
    std::array<std::vector<std::array<int, 5>>, 8> cShortedGroupsArray;

    std::array<int, 2>              cGroundedChannel;
    std::vector<std::array<int, 2>> cGroundedChannelsList;

    ThresholdVisitor cReader(fReadoutChipInterface);
    accept(cReader);
    fHistTop->GetYaxis()->SetRangeUser(0, fTotalEvents);
    fHistBottom->GetYaxis()->SetRangeUser(0, fTotalEvents);

    for(auto pBoard: *fDetectorContainer)
    {
        BeBoard* theBoard = static_cast<BeBoard*>(pBoard);
        uint32_t cN       = 1;
        uint32_t cNthAcq  = 0;

        SetBeBoardForShortsFinding(theBoard);

        ss << "\nShorted channels searching procedure\nSides: Top - 0\tBottom - 1 (Channel numbering starts from 0)\n" << std::endl;
        ss << "      Side\t| Channel_ID\t| Group_ID\t| Shorted_With_Group_ID" << std::endl;

        for(cTestPulseGroupId = 0; cTestPulseGroupId < 8; cTestPulseGroupId++)
        {
            cN      = 1;
            cNthAcq = 0;

            this->SetTestGroup(theBoard, (uint8_t)cTestPulseGroupId);

            fBeBoardInterface->Start(theBoard);

            while(cN <= fTotalEvents)
            {
                // Run( theBoard, cNthAcq );
                ReadData(theBoard);
                // ReadNEvents ( theBoard, cNthAcq );
                const std::vector<Event*>& events = GetEvents();

                // Loop over Events from this Acquisition
                for(auto& cEvent: events)
                {
                    HistogramFiller cFiller(fHistBottom, fHistTop, cEvent);
                    theBoard->accept(cFiller);

                    if(cN % 100 == 0) UpdateHists();

                    cN++;
                }

                cNthAcq++;
            }

            fBeBoardInterface->Stop(theBoard);

            std::vector<std::array<int, 5>> cShortedChannelsGroup;

            for(uint16_t cChannelId = 1; cChannelId < fNCbc * 127 + 1; cChannelId++)
            {
                if(fHistTop->GetBinContent(cChannelId) > 0.5 * fTotalEvents && ((cChannelId - 1) % 127) % 8 != cTestPulseGroupId)
                {
                    cShortedChannelInfo = {0, (cChannelId - 1), ((cChannelId - 1) % 127) % 8, cTestPulseGroupId, 0};
                    cShortedChannelsGroup.push_back(cShortedChannelInfo);
                    ss << "\t0\t|\t" << cShortedChannelInfo[1] << "\t|\t" << cShortedChannelInfo[2] << "\t|\t" << cShortedChannelInfo[3] << std::endl;
                    fHistTopMerged->SetBinContent(cChannelId, fHistTop->GetBinContent(cChannelId));
                }

                if(fHistTop->GetBinContent(cChannelId) < 0.5 * fTotalEvents && ((cChannelId - 1) % 127) % 8 == cTestPulseGroupId)
                {
                    cGroundedChannel = {0, (cChannelId - 1)};
                    cGroundedChannelsList.push_back(cGroundedChannel);
                    ss << "\t0\t|\t" << (cChannelId - 1) << "\t|\t" << (cTestPulseGroupId + 0) << "\t|\tGND" << std::endl;
                }

                if(fHistBottom->GetBinContent(cChannelId) > 0.5 * fTotalEvents && ((cChannelId - 1) % 127) % 8 != cTestPulseGroupId)
                {
                    cShortedChannelInfo = {1, (cChannelId - 1), ((cChannelId - 1) % 127) % 8, cTestPulseGroupId, 0};
                    cShortedChannelsGroup.push_back(cShortedChannelInfo);
                    ss << "\t1\t|\t" << cShortedChannelInfo[1] << "\t|\t" << cShortedChannelInfo[2] << "\t|\t" << cShortedChannelInfo[3] << std::endl;
                    fHistBottomMerged->SetBinContent(cChannelId, fHistBottom->GetBinContent(cChannelId));
                }

                if(fHistBottom->GetBinContent(cChannelId) < 0.5 * fTotalEvents && ((cChannelId - 1) % 127) % 8 == cTestPulseGroupId)
                {
                    cGroundedChannel = {1, (cChannelId - 1)};
                    cGroundedChannelsList.push_back(cGroundedChannel);
                    ss << "\t1\t|\t" << (cChannelId - 1) << "\t|\t" << (cTestPulseGroupId + 0) << "\t|\tGND" << std::endl;
                }
            }

            cShortedGroupsArray[cTestPulseGroupId] = cShortedChannelsGroup;
            // if (cTestPulseGroupId == 2) return;
            fHistBottom->Reset();
            fHistTop->Reset();
            ss << "------------------------------------------------------------------------" << std::endl;
        }
    }

    LOG(INFO) << ss.str();
    fHistTopMerged->Scale(100 / double_t(fTotalEvents));
    fHistTopMerged->GetYaxis()->SetRangeUser(0, 100);
    fHistBottomMerged->Scale(100 / double_t(fTotalEvents));
    fHistBottomMerged->GetYaxis()->SetRangeUser(0, 100);
    ReconstructShorts(cShortedGroupsArray);
    UpdateHistsMerged();

    // LOG (INFO) << ss.str();
}

void HybridTester::Measure()
{
    LOG(INFO) << "Mesuring Efficiency per Strip ... ";
    LOG(INFO) << "Taking data with " << fTotalEvents << " Events!";

    ThresholdVisitor cReader(fReadoutChipInterface);
    accept(cReader);
    fHistTop->GetYaxis()->SetRangeUser(0, fTotalEvents);
    fHistBottom->GetYaxis()->SetRangeUser(0, fTotalEvents);

    for(auto pBoard: *fDetectorContainer)
    {
        BeBoard* theBoard = static_cast<BeBoard*>(pBoard);
        uint32_t cN       = 1;
        uint32_t cNthAcq  = 0;

        fBeBoardInterface->Start(theBoard);

        while(cN <= fTotalEvents)
        {
            // Run( theBoard, cNthAcq );
            ReadData(theBoard);
            // ReadNEvents ( theBoard, cNthAcq );
            const std::vector<Event*>& events = GetEvents();

            // Loop over Events from this Acquisition
            for(auto& cEvent: events)
            {
                HistogramFiller cFiller(fHistBottom, fHistTop, cEvent);
                theBoard->accept(cFiller);

                if(cN % 100 == 0) UpdateHists();

                cN++;
            }

            cNthAcq++;
        }

        fBeBoardInterface->Stop(theBoard);
    }

    for(uint8_t i = 1; i < (fNCbc * 254u) / 2u; i++)
    {
        double cOccupancyTop    = 100 * fHistTop->GetBinContent(i) / (double)(fTotalEvents);
        double cOccupancyBottom = 100 * fHistBottom->GetBinContent(i) / (double)(fTotalEvents);
        fHistOccupancyBottom->Fill(cOccupancyBottom);
        fHistOccupancyTop->Fill(cOccupancyTop);
    }

    fHistTop->Scale(100 / double_t(fTotalEvents));
    fHistTop->GetYaxis()->SetRangeUser(0, 100);
    fHistBottom->Scale(100 / double_t(fTotalEvents));
    fHistBottom->GetYaxis()->SetRangeUser(0, 100);
    UpdateHists();

    LOG(INFO) << "\t\tMean occupancy for the Top side: " << fHistOccupancyTop->GetMean() << " ± " << fHistOccupancyTop->GetRMS() << RESET;
    LOG(INFO) << "\t\tMean occupancy for the Botton side: " << fHistOccupancyBottom->GetMean() << " ± " << fHistOccupancyBottom->GetRMS() << RESET;
    ClassifyChannels();
}
void HybridTester::ClassifyChannels(double pNoiseLevel, double pDeadLevel)
{
    for(uint8_t i = 1; i < (fNCbc * 254u) / 2u; i++)
    {
        if(fHistBottom->GetBinContent(i) >= pNoiseLevel) fNoisyChannelsBottom.push_back(i);

        if(fHistTop->GetBinContent(i) >= pNoiseLevel) fNoisyChannelsTop.push_back(i);

        if(fHistBottom->GetBinContent(i) <= pDeadLevel) fDeadChannelsBottom.push_back(i);

        if(fHistTop->GetBinContent(i) <= pDeadLevel) fDeadChannelsTop.push_back(i);
    }
}
void HybridTester::DisplayNoisyChannels(std::ostream& os)
{
    std::string line;

    line = "# Noisy channels on Bottom Sensor : ";

    for(size_t i = 0; i < fNoisyChannelsBottom.size(); i++)
    {
        line += std::to_string(fNoisyChannelsBottom[i]);
        line += (i < fNoisyChannelsBottom.size() - 1) ? "," : "";
    }

    os << line << std::endl;

    line = "# Noisy channels on Top Sensor : ";

    for(size_t i = 0; i < fNoisyChannelsTop.size(); i++)
    {
        line += std::to_string(fNoisyChannelsTop[i]);
        line += (i < fNoisyChannelsTop.size() - 1) ? "," : "";
    }

    os << line << std::endl;
}
void HybridTester::DisplayDeadChannels(std::ostream& os)
{
    std::string line;

    line = "# Dead channels on Bottom Sensor : ";

    for(size_t i = 0; i < fDeadChannelsBottom.size(); i++)
    {
        line += std::to_string(fDeadChannelsBottom[i]);
        line += (i < fDeadChannelsBottom.size() - 1) ? "," : "";
    }

    os << line << std::endl;

    line = "# Dead channels on Top Sensor : ";

    for(size_t i = 0; i < fDeadChannelsTop.size(); i++)
    {
        line += std::to_string(fDeadChannelsTop[i]);
        line += (i < fDeadChannelsTop.size() - 1) ? "," : "";
    }

    os << line << std::endl;
}
void HybridTester::AntennaScan(uint8_t pDigiPotentiometer)
{
#ifdef __ANTENNA__
    LOG(INFO) << "Mesuring Efficiency per Strip ... ";
    LOG(INFO) << "Taking data with " << fTotalEvents << " Events!";

    ThresholdVisitor cReader(fReadoutChipInterface);
    accept(cReader);

    Antenna cAntenna;
    uint8_t cADCChipSlave = 4;
    sleep(0.1);
    // cAntenna.TurnOnAnalogSwitchChannel (from 1 to 4); //put the signal on a given antenna strip

    cAntenna.initializeAntenna();         // initialize USB communication
    cAntenna.ConfigureADC(cADCChipSlave); // initialize SPI communication for ADC
    cAntenna.SelectTriggerSource(trigSource);
    if(trigSource == 5)
    {
        cAntenna.ConfigureClockGenerator(3, 8); // initialize SPI communication for ADC
    }
    else if(trigSource == 7) {}
    else { LOG(INFO) << "ERROR, wrong trig source set " << int(trigSource); }

    cAntenna.ConfigureDigitalPotentiometer(2, pDigiPotentiometer); // configure bias for antenna pull-up

    fHistTop->GetYaxis()->SetRangeUser(0, fTotalEvents);
    fHistBottom->GetYaxis()->SetRangeUser(0, fTotalEvents);

    uint8_t analog_switch_cs = 0;
    cAntenna.ConfigureAnalogueSwitch(analog_switch_cs); // configure communication with analogue switch

    for(uint8_t channel_position = 1; channel_position < 5; channel_position++)
    {
        cAntenna.TurnOnAnalogSwitchChannel(channel_position);
        LOG(INFO) << "Turning analogue switch chanel " << +channel_position;

        if(channel_position == 9) break;

        for(auto pBoard: *fDetectorContainer)
        {
            BeBoard* theBoard = static_cast<BeBoard*>(pBoard);
            uint32_t cN       = 1;
            uint32_t cNthAcq  = 0;

            this->StartBoard(theBoard);

            while(cN <= fTotalEvents)
            {
                // Run( theBoard, cNthAcq );
                ReadData(theBoard);

                const std::vector<Event*>& events = GetEvents();

                // Loop over Events from this Acquisition
                for(auto& cEvent: events)
                {
                    HistogramFiller cFiller(fHistBottom, fHistTop, cEvent);
                    theBoard->accept(cFiller);

                    if(cN % 100 == 0) UpdateHists();

                    cN++;
                }

                cNthAcq++;
            }

            this->StopBoard(theBoard);

            /*Here the reconstruction of histograms happens*/
            for(uint16_t channel_id = 1; channel_id < fNCbc * 127 + 1; channel_id++)
            {
                if(fHistTopMerged->GetBinContent(channel_id) < fHistTop->GetBinContent(channel_id)) fHistTopMerged->SetBinContent(channel_id, fHistTop->GetBinContent(channel_id));

                if(fHistBottomMerged->GetBinContent(channel_id) < fHistBottom->GetBinContent(channel_id)) fHistBottomMerged->SetBinContent(channel_id, fHistBottom->GetBinContent(channel_id));
            }

            /*Here clearing histograms after each event*/
            fHistBottom->Reset();
            fHistTop->Reset();
        }
    }

    fHistTopMerged->Scale(100 / double_t(fTotalEvents));
    fHistTopMerged->GetYaxis()->SetRangeUser(0, 100);
    fHistBottomMerged->Scale(100 / double_t(fTotalEvents));
    fHistBottomMerged->GetYaxis()->SetRangeUser(0, 100);

    UpdateHistsMerged();

    // cAntenna.close();

#endif
}

void HybridTester::SaveTestingResults(std::string pHybridId)
{
    std::ifstream infile;
    std::string   line_buffer;
    std::string   content_buffer;
    std::string   date_string = currentDateTime();
    std::string   filename    = "Results/HybridTestingDatabase/Hybrid_ID" + pHybridId + "_on" + date_string + ".txt";
    std::ofstream myfile;
    myfile.open(filename.c_str());
    myfile << "Hybrid ID: " << pHybridId << std::endl;
    myfile << "Created on: " << date_string << std::endl << std::endl;
    myfile << " Hybrid Testing Report" << std::endl;
    myfile << "-----------------------" << std::endl << std::endl;
    myfile << " Write/Read Registers Test" << std::endl;
    myfile << "---------------------------" << std::endl;

    infile.open(fDirectoryName + "/registers_test.txt");

    while(getline(infile, line_buffer)) content_buffer += line_buffer + "\r\n"; // To get all the lines.

    if(content_buffer == "") myfile << "Test not performed!" << std::endl;

    infile.close();
    myfile << content_buffer << std::endl;
    content_buffer = "";
    myfile << " Channels Functioning Test" << std::endl;
    myfile << "---------------------------" << std::endl;
    infile.open(fDirectoryName + "/channels_test2.txt");

    while(getline(infile, line_buffer)) content_buffer += line_buffer + "\r\n"; // To get all the lines.

    if(content_buffer == "") myfile << "Test not performed!" << std::endl;

    infile.close();
    myfile << content_buffer << std::endl;
    myfile.close();
    LOG(INFO) << std::endl << "Summary testing report written to: " << std::endl << filename;
}
void HybridTester::writeObjects()
{
    fResultFile->cd();
    fHistTop->Write(fHistTop->GetName(), TObject::kOverwrite);
    fHistBottom->Write(fHistBottom->GetName(), TObject::kOverwrite);
    fDataCanvas->Write(fDataCanvas->GetName(), TObject::kOverwrite);

    fHistOccupancyTop->Write(fHistOccupancyTop->GetName(), TObject::kOverwrite);
    fHistOccupancyBottom->Write(fHistOccupancyBottom->GetName(), TObject::kOverwrite);
    fSummaryCanvas->Write(fSummaryCanvas->GetName(), TObject::kOverwrite);

    // fResultFile->Write();
    // fResultFile->Close();
    LOG(INFO) << BOLDBLUE << "Results of occupancy measured written to " << fDirectoryName + "/Summary.root" << RESET;

    std::string cPdfName = fDirectoryName + "/HybridTestResults.pdf";
    fDataCanvas->SaveAs(cPdfName.c_str());
    cPdfName = fDirectoryName + "/NoiseOccupancySummary.pdf";
    fSummaryCanvas->SaveAs(cPdfName.c_str());

    if(fThresholdScan)
    {
        cPdfName = fDirectoryName + "/ThresholdScanResults.pdf";
        fSCurveCanvas->SaveAs(cPdfName.c_str());
    }

    this->SaveResults();
    fResultFile->Flush();
}

#endif
