/*!
 *
 * \file OTCicBypassTest.h
 * \brief OTCicBypassTest class
 * \author Fabio Ravera
 * \date 20/03/24
 *
 */

#ifndef OTCicBypassTest_h__
#define OTCicBypassTest_h__

#include "tools/Tool.h"
#include <map>
#ifdef __USE_ROOT__
// Calibration is not running on the SoC: I need to instantiate the DQM histogrammer here
#include "DQMUtils/DQMHistogramOTCicBypassTest.h"
#endif

namespace Ph2_HwInterface
{
class D19cFWInterface;
}

class OTCicBypassTest : public Tool
{
  public:
    OTCicBypassTest();
    ~OTCicBypassTest();

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
    void runCICbypassTest();
    void injectStubs2S(Ph2_HwDescription::ReadoutChip* theCBC);
    void injectStubsPS(Ph2_HwDescription::ReadoutChip* theMPA);

    size_t fNumberOfIterations{1};
    // For simplicity, make sure bendind code is always greater than half value (0x7)
    std::map<uint8_t, uint8_t> fBendingAndCode{{0, 0x5}, {2, 0xA}, {4, 0xF}};

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: Histogrammer is handeld by the calibration itself
    DQMHistogramOTCicBypassTest fDQMHistogramOTCicBypassTest;
#endif
};

#endif
