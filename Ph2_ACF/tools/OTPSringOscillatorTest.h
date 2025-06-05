/*!
 *
 * \file OTPSringOscillatorTest.h
 * \brief OTPSringOscillatorTest class
 * \author Fabio Ravera
 * \date 26/07/24
 *
 */

#ifndef OTPSringOscillatorTest_h__
#define OTPSringOscillatorTest_h__

#include "tools/Tool.h"
#include <map>
#ifdef __USE_ROOT__
// Calibration is not running on the SoC: I need to instantiate the DQM histogrammer here
#include "DQMUtils/DQMHistogramOTPSringOscillatorTest.h"
#endif

class OTPSringOscillatorTest : public Tool
{
  public:
    OTPSringOscillatorTest();
    ~OTPSringOscillatorTest();

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
    void    runRingOscillatorTest();
    void    runMPAringOscillatorTest();
    void    runSSAringOscillatorTest();
    uint8_t fNumberOfClockCycles = 100;

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: Histogrammer is handeld by the calibration itself
    DQMHistogramOTPSringOscillatorTest fDQMHistogramOTPSringOscillatorTest;
#endif
};

#endif
