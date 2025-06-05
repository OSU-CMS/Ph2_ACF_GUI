/*!
 *
 * \file OTCICtoLpGBTecv.h
 * \brief OTCICtoLpGBTecv class
 * \author Irene Zoi, built on Fabio Ravera's OTverifyBoardDataWord and inspired by Stefan Maier's ECVLinkAlignmentOT
 *
 * \date 23/05/24
 *
 */

#ifndef OTCICtoLpGBTecv_h__
#define OTCICtoLpGBTecv_h__

#include "tools/OTverifyBoardDataWord.h"
#include "tools/Tool.h"
#include <map>
#ifdef __USE_ROOT__
// Calibration is not running on the SoC: I need to instantiate the DQM histogrammer here
#include "DQMUtils/DQMHistogramOTCICtoLpGBTecv.h"
#endif

class OTCICtoLpGBTecv : public OTverifyBoardDataWord
{
  public:
    OTCICtoLpGBTecv();
    ~OTCICtoLpGBTecv();

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
    void                                     runECV();
    void                                     runECVPoint(uint8_t clockPolarity, uint8_t clockStrength, uint8_t cicStrength, uint8_t phase);
    std::vector<std::pair<uint8_t, uint8_t>> stubPatterns{
        std::make_pair(0xea, 0xaa), // default
        std::make_pair(0x75, 0x55), // shift 1 -> patterns
        std::make_pair(0xba, 0xaa), // shift 2 ->  or <- patterns
        std::make_pair(0xD5, 0x55)  // shift 1 <- patterns
    };

    std::vector<uint32_t> L1Patterns{
        0x0ffffffe, // default
        0x07ffffff, // shift 1 -> patterns
        0x03ffffff, // shift 2 -> patterns
        0x1ffffffc, // shift 1 <- patterns
        0x3ffffffe  // shift 2 <- patterns
    };

    std::vector<float> fListOfLpGBTPhase{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
    std::vector<float> fListOfCICStrength{1, 2, 3, 4, 5};
    std::vector<float> fListOfClockPolarity{0, 1};
    std::vector<float> fListOfClockStrength{1, 2, 3, 4, 5, 6, 7};

    DetectorDataContainer fTheBoardPatternMatcher;
    DetectorDataContainer fTheBoardNumberOfBytesInPattern;

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: Histogrammer is handeld by the calibration itself
    DQMHistogramOTCICtoLpGBTecv fDQMHistogramOTCICtoLpGBTecv;
#endif
};

#endif
