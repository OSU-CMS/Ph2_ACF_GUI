/*!
 *
 * \file OTBitErrorRateTest.h
 * \brief OTBitErrorRateTest class
 * \author Fabio Ravera
 * \date 02/07/24
 *
 */

#ifndef OTBitErrorRateTest_h__
#define OTBitErrorRateTest_h__

#include "tools/OTalignBoardDataWord.h"
#include <map>
#ifdef __USE_ROOT__
// Calibration is not running on the SoC: I need to instantiate the DQM histogrammer here
#include "DQMUtils/DQMHistogramOTBitErrorRateTest.h"
#endif

namespace Ph2_HwDescription
{
class OpticalGroup;
class BeBoard;
} // namespace Ph2_HwDescription

class OpticalGroupDataContainer;
class BoardDataContainer;
class OTBitErrorRateTest : public OTalignBoardDataWord
{
  public:
    OTBitErrorRateTest();
    ~OTBitErrorRateTest();

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
    void bitErrorRateTestPerLine(Ph2_HwDescription::BeBoard* theBoard,
                                 BoardDataContainer*         theBertContainer,
                                 BoardDataContainer*         theFECContainer,
                                 BoardDataContainer*         thePhaseClockDelayContainer,
                                 float                       numberOfBits,
                                 uint8_t                     lineNumber);
    void bitErrorRateTest();
    void bitErrorRateTest(uint8_t line);

    float fNumberOfBits{1E10};

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: Histogrammer is handeld by the calibration itself
    DQMHistogramOTBitErrorRateTest fDQMHistogramOTBitErrorRateTest;
#endif
};

#endif
