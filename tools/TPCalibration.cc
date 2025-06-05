#include "TPCalibration.h"
#ifdef __USE_ROOT__

TPCalibration::TPCalibration()
{
    LOG(INFO) << "Constructor of TPCalibration" << RESET;
    PedeNoise();
    fTPCount  = 0;
    fStartAmp = 165;
    fEndAmp   = 255;
    fStepsize = 10;
}

TPCalibration::~TPCalibration()
{
    LOG(INFO) << "Close output file" << RESET;
    fResultFile->Close();
}

void TPCalibration::Init(int pStartAmp, int pEndAmp, int pStepsize)
{
#ifdef __USE_ROOT__
    fDQMHistogramTPCalibration.book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif

    LOG(INFO) << "Initialize the test pulse calibration" << RESET;
    LOG(INFO) << "Measure for amplitudes from " << pStartAmp << " till " << pEndAmp << " in steps of " << pStepsize << RESET;
    fStartAmp = pStartAmp;
    fStepsize = pStepsize;
    fEndAmp   = pEndAmp;
    LOG(INFO) << "Initialise histograms" << RESET;
    for(auto cBoard: *fDetectorContainer)
    {
        int cBeId = cBoard->getId();
        LOG(INFO) << "BeBoard" << cBeId << RESET;
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                int cHybridId = cHybrid->getId();
                LOG(INFO) << "  FE" << cHybridId << RESET;
                for(auto cCbc: *cHybrid)
                {
                    int cCbcId = cCbc->getId();
                    LOG(INFO) << "  - CBC" << cCbcId << RESET;

                    for(int cChannel = 0; cChannel < NCHANNELS; cChannel++)
                    {
                        TGraph* cChanCorr = new TGraph((fEndAmp - fStartAmp) / fStepsize + 1);
                        cChanCorr->SetTitle(Form("Correlation Channel %d", cChannel));
                        cChanCorr->SetName(Form("CorrChannel%d", cChannel));
                        cChanCorr->GetXaxis()->SetTitle("pedestal (VCth)");
                        cChanCorr->GetYaxis()->SetTitle("test pulse amplitude (ADC)");
                        bookHistogram(cCbc, Form("CorrChan%d", cChannel), cChanCorr);
                    }

                    TH1F* cChannelGain = new TH1F(Form("GainFe%dCbc%d", cHybridId, cCbcId), Form("GainFe%dCbc%d", cHybridId, cCbcId), NCHANNELS, -0.5, NCHANNELS - 0.5);
                    cChannelGain->GetXaxis()->SetTitle("channel");
                    cChannelGain->GetYaxis()->SetTitle("gain (Amp)");
                    bookHistogram(cCbc, "ChannelGain", cChannelGain);
                    TH1F* cChannelElGain = new TH1F(Form("GainElectronsFe%dCbc%d", cHybridId, cCbcId), Form("GainElectronsFe%dCbc%d", cHybridId, cCbcId), NCHANNELS, -0.5, NCHANNELS - 0.5);
                    cChannelElGain->GetXaxis()->SetTitle("channel");
                    cChannelElGain->GetYaxis()->SetTitle("gain (electrons)");
                    bookHistogram(cCbc, "ChannelElectronsGain", cChannelElGain);
                }
            }
        }
    }
}

void TPCalibration::RunCalibration()
{
    LOG(INFO) << GREEN << "Start the test pulse calibration" << RESET;
    fTPCount = 0;
    for(int cTPAmp = fStartAmp; cTPAmp <= fEndAmp; cTPAmp += fStepsize)
    {
        LOG(INFO) << BLUE << "Measure pedestals for amplitude " << cTPAmp << RESET;
        fPulseAmplitude = cTPAmp;
        measureNoise();
        FillHistograms(cTPAmp);
        fTPCount++;
    }
    FitCorrelations();
}

void TPCalibration::FillHistograms(int pTPAmp)
{
    LOG(INFO) << "Fill the histogram with the measured pedestals" << RESET;

    for(auto cBoard: *fDetectorContainer)
    {
        int cBeId = cBoard->getId();
        LOG(INFO) << "BeBoard" << cBeId << RESET;
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                int cHybridId = cHybrid->getId();
                LOG(INFO) << "  FE" << cHybridId << RESET;
                for(auto cCbc: *cHybrid)
                {
                    int cCbcId = cCbc->getId();
                    LOG(INFO) << "  - CBC" << cCbcId << RESET;
                    TH1F* cPedestalHist = dynamic_cast<TH1F*>(getHist(cCbc, "Cbc_Strippedestal"));
                    for(int cChannel = 0; cChannel < NCHANNELS; cChannel++)
                    {
                        TGraph* cCorrGraph = dynamic_cast<TGraph*>(getHist(cCbc, Form("CorrChan%d", cChannel)));
                        int     bin        = cChannel + 1;
                        float   cPed       = cPedestalHist->GetBinContent(bin);
                        cCorrGraph->SetPoint(fTPCount, cPed, pTPAmp);
                        // Reset the histograms, so that it can be refilled for the next
                        // test pulse amplitude
                        cPedestalHist->SetBinContent(bin, 0);
                        cPedestalHist->SetBinError(bin, 0);
                    }
                }
            }
        }
    }
}

void TPCalibration::FitCorrelations()
{
    LOG(INFO) << BOLDBLUE << "Linear fit of the correlations" << RESET;
    for(auto cBoard: *fDetectorContainer)
    {
        int cBeId = cBoard->getId();
        LOG(INFO) << "BeBoard" << cBeId << RESET;
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                int cHybridId = cHybrid->getId();
                LOG(INFO) << "  FE" << cHybridId << RESET;
                std::vector<float> cGainVec;
                for(auto cCbc: *cHybrid)
                {
                    int   cCbcId         = cCbc->getId();
                    TH1F* cChannelGain   = dynamic_cast<TH1F*>(getHist(cCbc, "ChannelGain"));
                    TH1F* cChannelElGain = dynamic_cast<TH1F*>(getHist(cCbc, "ChannelElectronsGain"));
                    for(int cChannel = 0; cChannel < NCHANNELS; cChannel++)
                    {
                        TGraph* cCorrGraph = dynamic_cast<TGraph*>(getHist(cCbc, Form("CorrChan%d", cChannel)));
                        cCorrGraph->Fit("pol1", "Q");
                        float cGain    = cCorrGraph->GetFunction("pol1")->GetParameter(1);
                        float cGainErr = cCorrGraph->GetFunction("pol1")->GetParError(1);
                        cChannelGain->SetBinContent(cChannel + 1, cGain);
                        cChannelGain->SetBinError(cChannel + 1, cGainErr);
                        cChannelElGain->SetBinContent(cChannel + 1, ConvertAmpToElectrons(cGain, false));
                        cChannelElGain->SetBinError(cChannel + 1, ConvertAmpToElectrons(cGainErr, false));
                        cGainVec.push_back(cGain);
                    }

                    // Make a histogram of the Gains in Amplitudes and Electrons
                    TString cHistName = Form("GainHistFe%dCbc%d", cHybridId, cCbcId);
                    float   cBinWidth = 0.005;
                    float   cMin      = cChannelGain->GetMinimum() * 0.9;
                    float   cMax      = cChannelGain->GetMaximum() * 1.1;
                    TH1F*   cGainHist = new TH1F(cHistName, cHistName, (cMax - cMin) / cBinWidth, cMin, cMax);
                    cGainHist->GetXaxis()->SetTitle("gain (Amp/VCth)");

                    cHistName         = Form("GainElectronsHistFe%dCbc%d", cHybridId, cCbcId);
                    TH1F* cGainElHist = new TH1F(cHistName, cHistName, (cMax - cMin) / cBinWidth, ConvertAmpToElectrons(cMin, false), ConvertAmpToElectrons(cMax, false));
                    cGainElHist->GetXaxis()->SetTitle("gain (electrons/VCth)");

                    for(auto cGain: cGainVec)
                    {
                        cGainHist->Fill(cGain);
                        cGainElHist->Fill(ConvertAmpToElectrons(cGain, false));
                    }
                    LOG(INFO) << "  - CBC" << cCbcId << ": Gain = " << GREEN << cGainElHist->GetMean() << "+-" << cGainElHist->GetStdDev() << RESET;
                    bookHistogram(cCbc, "GainHist", cGainHist);
                    bookHistogram(cCbc, "GainElectronsHist", cGainElHist);
                }
            }
        }
    }
}

void TPCalibration::SaveResults()
{
    LOG(INFO) << BOLDGREEN << "Write results to file: " << fResultFile->GetName() << RESET;
    for(auto cBoard: *fDetectorContainer)
    {
        int     cBeId = cBoard->getId();
        TString cPath = Form("Be%d", cBeId);
        fResultFile->mkdir(cPath);
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                int cHybridId = cHybrid->getId();
                cPath         = Form("Be%d/Fe%d", cBeId, cHybridId);
                fResultFile->mkdir(cPath);
                for(auto cCbc: *cHybrid)
                {
                    int cCbcId = cCbc->getId();
                    cPath      = Form("Be%d/Fe%d/Cbc%d", cBeId, cHybridId, cCbcId);
                    fResultFile->mkdir(cPath);
                    fResultFile->cd(cPath);
                    TH1F* cChannelHist = dynamic_cast<TH1F*>(getHist(cCbc, "ChannelGain"));
                    cChannelHist->Write();
                    TH1F* cChannelElHist = dynamic_cast<TH1F*>(getHist(cCbc, "ChannelElectronsGain"));
                    cChannelElHist->Write();
                    TH1F* cGainHist = dynamic_cast<TH1F*>(getHist(cCbc, "GainHist"));
                    cGainHist->Write();
                    TH1F* cGainElHist = dynamic_cast<TH1F*>(getHist(cCbc, "GainElectronsHist"));
                    cGainElHist->Write();

                    cPath = Form("Be%d/Fe%d/Cbc%d/ChannelCorr", cBeId, cHybridId, cCbcId);
                    fResultFile->mkdir(cPath);
                    fResultFile->cd(cPath);
                    for(int cChannel = 0; cChannel < NCHANNELS; cChannel++)
                    {
                        TGraph* cChanCorr = dynamic_cast<TGraph*>(getHist(cCbc, Form("CorrChan%d", cChannel)));
                        cChanCorr->Write();
                    }
                }
            }
        }
    }
    fResultFile->Flush();
    LOG(INFO) << "Results saved!" << RESET;
}

float TPCalibration::ConvertAmpToElectrons(float pTPAmp, bool pOffset = true)
{
    // Test pulse step resolution (of 0.086fC = 537electrons)
    // taken from the CBC3 manual (should be the same for CBC2)
    // 255 equals no test pulse with decreasing values the injected charge
    // increases
    if(!pOffset) { return pTPAmp * 537.; }
    return -(pTPAmp - 255) * 537.;
}

// State machine control functions

void TPCalibration::ConfigureCalibration() {}

void TPCalibration::Running() {}

void TPCalibration::Stop() {}

void TPCalibration::Pause() {}

void TPCalibration::Resume() {}

#endif
