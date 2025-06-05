/*!
 *
 * \file OTLpGBTEyeOpeningTest.h
 * \brief OTLpGBTEyeOpeningTest class
 * \author Fabio Ravera
 * \date 24/09/24
 *
 */

#ifndef OTLpGBTEyeOpeningTest_h__
#define OTLpGBTEyeOpeningTest_h__

#include "tools/Tool.h"
#include <map>
#ifdef __USE_ROOT__
// Calibration is not running on the SoC: I need to instantiate the DQM histogrammer here
#include "DQMUtils/DQMHistogramOTLpGBTEyeOpeningTest.h"
#endif

class OTLpGBTEyeOpeningTest : public Tool
{
  public:
    OTLpGBTEyeOpeningTest();
    ~OTLpGBTEyeOpeningTest();

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
    void               runEyeOpeningTest();
    std::vector<float> fPowerList;

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: Histogrammer is handeld by the calibration itself
    DQMHistogramOTLpGBTEyeOpeningTest fDQMHistogramOTLpGBTEyeOpeningTest;
#endif
};

#endif
