#include "Channel.h"
#ifdef __USE_ROOT__

#include "TMath.h"
#include <cmath>

Channel::Channel(uint8_t pBeId, uint8_t pHybridId, uint8_t pOpticalGroupId, uint8_t pCbcId, uint8_t pChannelId)
    : fBeId(pBeId), fHybridId(pHybridId), fOpticalGroupId(pOpticalGroupId), fCbcId(pCbcId), fChannelId(pChannelId)
{
}

Channel::~Channel() {}

double Channel::getPedestal() const
{
    if(fFitted)
    {
        if(fFit != nullptr)
            return fabs(fFit->GetParameter(0));
        else
            return -1;
    }
    else
    {
        if(fDerivative != nullptr)
            return fabs(fDerivative->GetMean());
        else
            return -1;
    }
}

double Channel::getNoise() const
{
    if(fFitted)
    {
        if(fFit != nullptr)
            return fabs(fFit->GetParameter(1));
        else
            return -1;
    }
    else
    {
        if(fDerivative != nullptr)
            return fabs(fDerivative->GetRMS());
        else
            return -1;
    }
}

void Channel::setOffset(uint8_t pOffset) { fOffset = pOffset; }

void Channel::initializePulse(TString pName)
{
    TObject* cObj = gROOT->FindObject(pName);

    if(cObj) delete cObj;

    fPulse = new TGraph();
    fPulse->SetName(pName);
    fPulse->SetMarkerStyle(3);
    fPulse->GetXaxis()->SetTitle("TestPulseDelay [ns]");
    fPulse->GetYaxis()->SetTitle("TestPulseAmplitue [VCth]");
    // fPulse->GetYaxis()->SetRangeUser( 0, 255 );
    // fPulse->GetHistogram()->SetMaximum( 255. );
    // fPulse->GetHistogram()->SetMinimum( 0. );
    fPulse->GetYaxis()->SetLimits(0, 255);
}
void Channel::initializeHist(uint16_t pValue, TString pParameter)
{
    TString histname;

    pParameter += Form("%d", pValue);
    histname = Form("Scurve_Be%d_Fe%d_Cbc%d_Channel%d", fBeId, fHybridId, fCbcId, fChannelId);
    histname += pParameter;

    fScurve = dynamic_cast<TH1F*>(gROOT->FindObject(histname));

    if(fScurve) delete fScurve;

    fScurve = new TH1F(histname, Form("Scurve_Be%d_Fe%d_Cbc%d_Channel%d", fBeId, fHybridId, fCbcId, fChannelId), 1024, -0.5, 1023.5);
    fScurve->GetXaxis()->SetTitle(pParameter);
    fScurve->GetYaxis()->SetTitle("Occupancy");

    fScurve->SetMarkerStyle(7);
    fScurve->SetMarkerSize(2);
}

void Channel::fillHist(uint16_t pVcth) { fScurve->Fill(float(pVcth)); }

void Channel::fitHist(uint32_t pEventsperVcth, bool pHole, uint16_t pValue, TString pParameter, TFile* pResultfile)
{
    fFitted = true;
    TString fitname;

    fitname = Form("Fit_Be%d_Fe%d_Cbc%d_Channel%d%s%d", fBeId, fHybridId, fCbcId, fChannelId, pParameter.Data(), pValue);

    fFit = dynamic_cast<TF1*>(gROOT->FindObject(fitname));

    if(fFit) delete fFit;

    if(fScurve != nullptr && fFit == nullptr)
    {
        // Normalize first
        // fScurve->Sumw2();
        fScurve->Scale(1 / double_t(pEventsperVcth));

        // Get first non 0 and first 1
        double cFirstNon0(0);
        double cFirst1(0);

        // Not Hole Mode
        if(!pHole)
        {
            for(Int_t cBin = 1; cBin < fScurve->GetNbinsX() - 1; cBin++)
            {
                double cContent = fScurve->GetBinContent(cBin);

                if(!cFirstNon0)
                {
                    if(cContent) cFirstNon0 = fScurve->GetBinCenter(cBin);
                }
                else if(cContent > 0.85)
                {
                    cFirst1 = fScurve->GetBinCenter(cBin);
                    break;
                }
            }
        }
        // Hole mode
        else
        {
            for(Int_t cBin = fScurve->GetNbinsX() - 1; cBin > 1; cBin--)
            {
                double cContent = fScurve->GetBinContent(cBin);

                if(!cFirstNon0)
                {
                    if(cContent) cFirstNon0 = fScurve->GetBinCenter(cBin);
                }
                else if(cContent > 0.85)
                {
                    cFirst1 = fScurve->GetBinCenter(cBin);
                    break;
                }
            }
        }

        // Get rough midpoint & width
        double cMid   = (cFirst1 + cFirstNon0) * 0.5;
        double cWidth = (cFirst1 - cFirstNon0) * 0.5;

        if(!pHole)
            fFit = new TF1(fitname, MyErf, cFirstNon0 - 10, cFirst1 + 10, 2);
        else
            fFit = new TF1(fitname, MyErf, cFirst1 - 10, cFirstNon0 + 10, 2);

        fFit->SetParameter(0, cMid);
        fFit->SetParameter(1, sqrt(2) * cWidth);

        // Fit
        fScurve->Fit(fFit, "RNQ+");
        // fScurve->Fit ( fFit, "RQ+" );

        // Eventually add TFitResultPointer
        // create a Directory in the file for the current Offset and save the channel Data
        TString cDirName;
        cDirName         = pParameter + Form("%d", pValue);
        TDirectory* cDir = dynamic_cast<TDirectory*>(gROOT->FindObject(cDirName));

        if(!cDir) cDir = pResultfile->mkdir(cDirName);

        pResultfile->cd(cDirName);

        fScurve->SetDirectory(cDir);
        // fFit->SetDirectory( cDir );
        fScurve->Write(fScurve->GetName(), TObject::kOverwrite);
        fFit->Write(fFit->GetName(), TObject::kOverwrite);
        pResultfile->Flush();

        pResultfile->cd();
    }
    else
        LOG(INFO) << "Historgram Empty for Hybrid " << fHybridId << " Chip " << fCbcId << " Channel " << fChannelId;
}

void Channel::differentiateHist(uint32_t pEventsperVcth, bool pHole, uint16_t pValue, TString pParameter, TFile* pResultfile)
{
    // TODO
    fFitted = false;
    TString graphname;

    graphname = Form("fDerivative_Be%d_Fe%d_Cbc%d_Channel%d%s%d", fBeId, fHybridId, fCbcId, fChannelId, pParameter.Data(), pValue);

    fDerivative = dynamic_cast<TH1F*>(gROOT->FindObject(graphname));

    if(fDerivative) delete fDerivative;

    fDerivative = new TH1F(graphname, Form("Derivative_Scurve_Be%d_Fe%d_Cbc%d_Channel%d", fBeId, fHybridId, fCbcId, fChannelId), 1024, 0, 1024);
    fDerivative->GetXaxis()->SetTitle(pParameter);
    fDerivative->GetYaxis()->SetTitle("Slope");

    if(fScurve != nullptr && fFit != nullptr)
    {
        // Normalize first
        // fScurve->Sumw2();
        fScurve->Scale(1 / double_t(pEventsperVcth));

        // Histogram of Differences

        // double_t cPrev = fScurve->GetBinContent( fScurve->GetBin( -0.5 ) );
        double_t cDiff;
        double_t cCurrent;
        double_t cPrev;
        bool     cActive; // indicates existence of data points
        int      cDiffCounter = 0;

        double cBin = 0;

        if(pHole)
        {
            cPrev   = fScurve->GetBinContent(fScurve->FindBin(0));
            cActive = false;

            for(cBin = fScurve->FindBin(0); cBin <= fScurve->FindBin(1023); cBin++)
            {
                cCurrent = fScurve->GetBinContent(cBin);
                cDiff    = cPrev - cCurrent;

                if(cPrev > 0.75) cActive = true; // sampling begins

                int iBinDerivative = fDerivative->FindBin((fScurve->GetBinCenter(cBin + 1) + fScurve->GetBinCenter(cBin)) / 2);

                if(cActive) fDerivative->SetBinContent(iBinDerivative, cDiff);

                // if ( cActive ) fDerivative->SetBinContent ( cBin + 1,  cDiff  );

                if(cActive && cDiff == 0 && cCurrent == 0) cDiffCounter++;

                if(cDiffCounter == 8) break;

                cPrev = cCurrent;
            }
        }
        else
        {
            cPrev   = fScurve->GetBinContent(fScurve->FindBin(1023));
            cActive = false;

            for(cBin = fScurve->FindBin(1023); cBin >= fScurve->FindBin(0); cBin--)
            {
                cCurrent = fScurve->GetBinContent(cBin);
                cDiff    = cPrev - cCurrent;

                if(cPrev > 0.75) cActive = true; // sampling begins

                int iBinDerivative = fDerivative->FindBin((fScurve->GetBinCenter(cBin - 1) + fScurve->GetBinCenter(cBin)) / 2.0);

                if(cActive) fDerivative->SetBinContent(iBinDerivative, cDiff);

                // original
                // if ( cActive ) fDerivative->SetBinContent ( fDerivative->FindBin ( cBin - 0.5 ),   cDiff  );
                // if ( cActive ) fDerivative->SetBinContent ( cBin - 1,   cDiff  );

                if(cActive && cDiff == 0 && cCurrent == 0) cDiffCounter++;

                if(cDiffCounter == 8) break;

                cPrev = cCurrent;
            }
        }

        // Eventually add TDerivativeResultPointer
        // create a Directory in the file for the current Offset and save the channel Data
        TString cDirName;
        cDirName         = pParameter + Form("%d", pValue);
        TDirectory* cDir = dynamic_cast<TDirectory*>(gROOT->FindObject(cDirName));

        if(!cDir) cDir = pResultfile->mkdir(cDirName);

        pResultfile->cd(cDirName);

        fScurve->SetDirectory(cDir);
        fDerivative->SetDirectory(cDir);
        fScurve->Write(fScurve->GetName(), TObject::kOverwrite);
        fDerivative->Write(fDerivative->GetName(), TObject::kOverwrite);
        pResultfile->Flush();

        pResultfile->cd();
    }
    else
        LOG(INFO) << "Historgram Empty for Hybrid " << fHybridId << " Chip " << fCbcId << " Channel " << fChannelId;
}

void Channel::resetHist() {}

TestGroup::TestGroup(uint8_t pBeId, uint8_t pHybridId, uint8_t pCbcId, uint8_t pGroupId) : fBeId(pBeId), fHybridId(pHybridId), fCbcId(pCbcId), fGroupId(pGroupId) {}

TestGroupGraph::TestGroupGraph() { fVplusVcthGraph = nullptr; }

TestGroupGraph::TestGroupGraph(uint8_t pBeId, uint8_t pHybridId, uint8_t pCbcId, uint8_t pGroupId)
{
    TString graphname = Form("VplusVcthGraph_Fe%d_Cbc%d_Group%d", pHybridId, pCbcId, pGroupId);
    fVplusVcthGraph   = dynamic_cast<TGraphErrors*>(gROOT->FindObject(graphname));

    if(fVplusVcthGraph) delete fVplusVcthGraph;

    fVplusVcthGraph = new TGraphErrors();
    fVplusVcthGraph->SetName(graphname);
}

void TestGroupGraph::FillVplusVcthGraph(uint8_t& pVplus, double pPedestal, double pNoise)
{
    if(fVplusVcthGraph != nullptr)
    {
        fVplusVcthGraph->SetPoint(fVplusVcthGraph->GetN(), pPedestal, pVplus);
        fVplusVcthGraph->SetPointError(fVplusVcthGraph->GetN() - 1, pNoise, 0);
    }
}

#endif