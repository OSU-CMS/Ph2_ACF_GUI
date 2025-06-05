/*!
 *
 * \file OTRegisterTester.h
 * \brief OTRegisterTester class
 * \author Irene Zoi
 * \date 01/07/24
 *
 */

#ifndef OTRegisterTester_h__
#define OTRegisterTester_h__

#include "tools/Tool.h"
#include <map>
#ifdef __USE_ROOT__
// Calibration is not running on the SoC: I need to instantiate the DQM histogrammer here
#include "DQMUtils/DQMHistogramOTRegisterTester.h"
#endif

class OTRegisterTester : public Tool
{
  public:
    OTRegisterTester();
    ~OTRegisterTester();

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
    DetectorDataContainer fPatternMatchingEfficiencyContainer;

  protected:
    size_t  fNumberOfIterations{1000};
    uint8_t fPattern{0xAA};
    void    TestRegisters();
    float   EfficiencyCalculator(Ph2_HwDescription::Chip* theChip, const std::vector<std::string>& theRegisters);
#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: Histogrammer is handeld by the calibration itself
    DQMHistogramOTRegisterTester fDQMHistogramOTRegisterTester;
#endif
};

#endif
