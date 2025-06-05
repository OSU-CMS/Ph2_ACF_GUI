/*!
 *
 * \file OTalignLpGBTinputs.h
 * \brief OTalignLpGBTinputs class
 * \author Fabio Ravera
 * \date 11/01/24
 *
 */

#ifndef OTalignLpGBTinputs_h__
#define OTalignLpGBTinputs_h__

#include "tools/Tool.h"
#include <map>
#ifdef __USE_ROOT__
// Calibration is not running on the SoC: I need to instantiate the DQM histogrammer here
#include "DQMUtils/DQMHistogramOTalignLpGBTinputs.h"
#endif

class OTalignLpGBTinputs : public Tool
{
  public:
    OTalignLpGBTinputs();
    ~OTalignLpGBTinputs();

    void Initialise(void);

    // State machine
    void Running() override;
    void Stop() override;
    void ConfigureCalibration() override;
    void Pause() override;
    void Resume() override;
    void Reset();

    static std::string fCalibrationDescription;

  private:
    void   AlignLpGBTInputs();
    size_t fNumberOfAlignmentIterations{100};
    float  fMinAlignmentSuccessRate{0.99};

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: Histogrammer is handeld by the calibration itself
    DQMHistogramOTalignLpGBTinputs fDQMHistogramOTalignLpGBTinputs;
#endif
};

#endif
