#ifndef TPCalibration_h__
#define TPCalibration_h__

#include "tools/Tool.h"
#ifdef __USE_ROOT__

#ifdef __USE_ROOT__
#include "DQMUtils/DQMHistogramTPCalibration.h"
#endif

#include "TCanvas.h"
#include "TF1.h"
#include "TGraph.h"
#include "TH1F.h"

#include "PedeNoise.h"

using namespace Ph2_System;

class TPCalibration : public PedeNoise
{
  private: // attributes
    int fStartAmp;
    int fEndAmp;
    int fStepsize;
    int fTPCount;

  public: // methods
    TPCalibration();
    ~TPCalibration();

    void  Init(int pStartAmp, int pEndAmp, int pStepsize);
    void  RunCalibration();
    void  SaveResults();
    float ConvertAmpToElectrons(float pTPAmp, bool pOffset);

    void Running() override;
    void Stop() override;
    void ConfigureCalibration() override;
    void Pause() override;
    void Resume() override;

  private: // methods
    void FillHistograms(int pTPAmp);
    void FitCorrelations();

#ifdef __USE_ROOT__
    DQMHistogramTPCalibration fDQMHistogramTPCalibration;
#endif
};

#endif
#endif
