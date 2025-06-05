/*!

        \file                   OTCMNoise.h
        \brief                 class for performing Common Mode noise studies
        \author                 Georg AUZINGER adapted by Lesya Horyn
        \version                1.0
        \date                   29/10/14 -- LH 2/17/22
        Support :               mail to : georg.auzinger@cern.ch || lesya.horyn@cern.ch

 */
#ifndef OTCMNoise_H__
#define OTCMNoise_H__

#include "tools/Tool.h"

#include "Utils/CommonVisitors.h"

// ROOT

#ifdef __USE_ROOT__
#include "DQMUtils/DQMHistogramOTCMNoise.h"
#endif

#include <math.h>

using namespace Ph2_System;

// typedef std::map<Ph2_HwDescription::Chip*, std::map<std::string, TObject*>> CbcHistogramMap;
// typedef std::map<Chip*, TCanvas*> CanvasMap;
// typedef std::map<Ph2_HwDescription::Hybrid*, std::map<std::string, TObject*>> HybridHistogramMap;

/*!
 * \class OTCMNoise
 * \brief Class to perform Common Mode noise studies
 */

class OTCMNoise : public Tool
{
  public:
    OTCMNoise();
    ~OTCMNoise();
    void Initialize();
    void SetThresholds(int manualVcth = 0, float nSigma = 0);
    void TakeData(float nSigma = 0);

    void writeObjects();
    void Running() override;
    void Stop() override;
    void ConfigureCalibration() override;
    void Pause() override;
    void Resume() override;
    void Reset();

    static std::string fCalibrationDescription;

  private:
    void parseSettings();

    uint32_t           fNevents;
    uint32_t           fVcth;
    bool               f2DHistograms;
    bool               f2DHistogramsLight;
    uint32_t           fManualVcth;
    std::vector<float> fListOfThresholds{0};

#ifdef __USE_ROOT__
    DQMHistogramOTCMNoise fDQMHistogramOTCMNoise;
#endif
};

#endif
