/*!
 *
 * \file PedeNoise.h
 * \brief Calibration class, calibration of the hardware
 * \author Georg AUZINGER
 * \date 12 / 11 / 15
 *
 * \Support : georg.auzinger@cern.ch
 *
 */

#ifndef PedeNoise_h__
#define PedeNoise_h__

#include "Utils/CommonVisitors.h"
#include "Utils/ContainerRecycleBin.h"
#include "Utils/Visitor.h"
#include "tools/Tool.h"
#ifdef __USE_ROOT__
#include "DQMUtils/DQMHistogramPedeNoise.h"
#endif

#include <map>

using namespace Ph2_System;

class DetectorContainer;
class Occupancy;

class PedeNoise : public Tool
{
  public:
    PedeNoise();
    ~PedeNoise();
    void clearDataMembers();

    virtual void Initialise(bool pAllChan = false, bool pDisableStubLogic = true);
    void         measureNoise(); // method based on the one below that actually analyzes the scurves and extracts the noise
    void         sweepSCurves(); // actual methods to measure SCurves
    void         Validate();
    void         writeObjects();

    void         Running() override;
    void         Stop() override;
    void         ConfigureCalibration() override;
    void         Pause() override;
    void         Resume() override;
    virtual void Reset();

    static std::string fCalibrationDescription;

  protected:
    void measureSCurves(uint16_t pStripStartValue = 0, uint16_t pPixelStartValue = 0);
    void findPedestal(bool forceAllChannels = false);
    void extractPedeNoise();
    void disableStubLogic();
    void cleanContainerVector();
    void initializeRecycleBin() { fRecycleBin.setDetectorContainer(fDetectorContainer); }

    uint8_t fPulseAmplitude{0};
    uint8_t fPulseAmplitudePix{0};

    float fPedeNoiseLimit{0.0};
    bool  fPedeNoiseMask{false};
    float fPedeNoiseUntrimmedLimit{0.0};
    bool  fPedeNoiseMaskUntrimmed{false};

    uint32_t fEventsPerPoint{0};
    uint32_t fMaxNevents{65535};
    int      fNEventsPerBurst{-1};
    bool     fUseFixRange{false};
    uint16_t fMinThreshold{0};
    uint16_t fMaxThreshold{1023};
    float    fLimit{0.005};
    float    fMeanStrips{0};
    float    fMeanPixels{0};
    uint32_t fNeventsForValidation{0};
    float    fMaskingThreshold{0};
    bool     fMaskNoisyChannels{0};
    uint16_t fPedeNoiseLatency{0};

    DetectorDataContainer*                     fThresholdAndNoiseContainer;
    std::map<uint16_t, DetectorDataContainer*> fSCurveStripOccupancyMap, fSCurvePixelOccupancyMap;

  private:
    // to hold the original register values
    DetectorDataContainer  fEventTypes;
    DetectorDataContainer* fStubLogicValue;
    DetectorDataContainer* fHIPCountValue;
    DetectorDataContainer  fBoardRegContainer;

    bool fWithCBC = true;
    bool fWithSSA = false;
    bool fWithMPA = false;

    // Settings
    bool fPlotSCurves{false};
    bool fFitSCurves{false};
    bool fDisableStubLogic{true};

    void producePedeNoisePlots();

    // for validation
    void setThresholdtoNSigma(BoardContainer* board, float pNSigma);
    void maskNoisyChannels(BoardDataContainer* board);
    // helpers for SCurve measurement

    ContainerRecycleBin<Occupancy> fRecycleBin;

#ifdef __USE_ROOT__
    DQMHistogramPedeNoise fDQMHistogramPedeNoise;
#endif
};

#endif
