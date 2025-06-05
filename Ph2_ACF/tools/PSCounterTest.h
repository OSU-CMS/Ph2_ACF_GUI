/*!
 *
 * \file PSCounterTest.h
 * \brief PSCounterTest class
 * \author Adam Kobert
 * \date 25/07/24
 *
 */

#ifndef PSCounterTest_h__
#define PSCounterTest_h__

#include "tools/Tool.h"
#include <map>
#ifdef __USE_ROOT__
// Calibration is not running on the SoC: I need to instantiate the DQM histogrammer here
#include "DQMUtils/DQMHistogramPSCounterTest.h"
#endif

namespace Ph2_HwDescription
{
class BeBoard;
}

class PSCounterTest : public Tool
{
  public:
    PSCounterTest();
    ~PSCounterTest();

    void Initialise(void);

    // State machine
    void Running() override;
    void Stop() override;
    void ConfigureCalibration() override;
    void RunFast(uint16_t stripThreshold, uint16_t pixelThreshold);
    void Pause() override;
    void Resume() override;
    void Reset();

    static std::string fCalibrationDescription;

  private:
    std::vector<EventType> fEventTypes;
    DetectorDataContainer* fStubLogicValue;
    DetectorDataContainer* fHIPCountValue;
    DetectorDataContainer  fBoardRegContainer;
    bool                   fWithCBC = false;
    bool                   fWithSSA = false;
    bool                   fWithMPA = false;

    // Settings
    bool fDisableStubLogic{true};

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: Histogrammer is handeld by the calibration itself
    DQMHistogramPSCounterTest fDQMHistogramPSCounterTest;
#endif
};

#endif
