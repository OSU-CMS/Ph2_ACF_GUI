/*!
 *
 * \file OTPScommonNoise.h
 * \brief OTPScommonNoise class
 * \author Irene Zoi
 * \date 15/08/24
 *
 */

#ifndef OTPScommonNoise_h__
#define OTPScommonNoise_h__

#include "tools/Tool.h"
#include <map>
#ifdef __USE_ROOT__
// Calibration is not running on the SoC: I need to instantiate the DQM histogrammer here
#include "DQMUtils/DQMHistogramOTPScommonNoise.h"
#endif

class OTPScommonNoise : public Tool
{
  public:
    OTPScommonNoise();
    ~OTPScommonNoise();

    void Initialise(void);

    // State machine
    void Running() override;
    void Stop() override;
    void ConfigureCalibration() override;
    void Pause() override;
    void Resume() override;
    void Reset();

    static std::string fCalibrationDescription;
    std::vector<float> fListOfSigma{0, 3};

  protected:
    uint32_t fNumberOfEvents{10000};
    void     SetThresholds(float numberOfSigma);
    void     TakeData(float numberOfSigma);

  private:
#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: Histogrammer is handeld by the calibration itself
    DQMHistogramOTPScommonNoise fDQMHistogramOTPScommonNoise;
#endif
};

#endif
