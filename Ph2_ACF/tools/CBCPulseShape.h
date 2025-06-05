/*!
 *
 * \file CBCPulseShape.h
 * \brief Calibration example -> use it as a template
 * \author Fabio Ravera
 * \date 25 / 07 / 19
 *
 * \Support : fabio.ravera@cern.ch
 *
 */

#ifndef CBCPulseShape_h__
#define CBCPulseShape_h__

#include "PedeNoise.h"
#include <map>
#ifdef __USE_ROOT__
// Calibration is not running on the SoC: I need to instantiate the DQM histogrammer here
#include "DQMUtils/CBCHistogramPulseShape.h"
#endif

class CBCPulseShape : public PedeNoise
{
  public:
    CBCPulseShape();
    ~CBCPulseShape();

    void Initialise(bool pAllChan = false, bool pDisableStubLogic = true) override;
    void runCBCPulseShape(void);
    void writeObjects(void);

    // State machine
    void Running() override;
    void Stop(void) override;

    static std::string fCalibrationDescription;

  private:
    uint16_t fInitialLatency{0};
    uint16_t fInitialDelay{0};
    uint16_t fFinalDelay{0};
    uint16_t fDelayStep{0};
    uint16_t fPulseAmplitude{0};
    int8_t   fChannelGroup{-1};
    bool     fPlotPulseShapeSCurves{false};

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: Histogrammer is handeld by the calibration itself
    CBCHistogramPulseShape fCBCHistogramPulseShape;
#endif
};

#endif
