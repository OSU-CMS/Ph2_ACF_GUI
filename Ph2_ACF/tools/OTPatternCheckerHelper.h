/*!
 *
 * \file OTPatternCheckerHelper.h
 * \brief OTPatternCheckerHelper class
 * \author Fabio Ravera
 * \date 24/01/25
 *
 */

#ifndef OTPatternCheckerHelper_h__
#define OTPatternCheckerHelper_h__

#include "tools/OTalignBoardDataWord.h"
#include <map>
#ifdef __USE_ROOT__
// Calibration is not running on the SoC: I need to instantiate the DQM histogrammer here
#include "DQMUtils/DQMHistogramOTPatternCheckerHelper.h"
#endif

class BoardDataContainer;

class OTPatternCheckerHelper : public OTalignBoardDataWord
{
  public:
    OTPatternCheckerHelper();
    ~OTPatternCheckerHelper();

    void Initialise(void);

    // State machine
    void Running() override;
    void Stop() override;
    void ConfigureCalibration() override;
    void Pause() override;
    void Resume() override;
    void Reset();

    static std::string fCalibrationDescription;
    void
    patternCheckerTest(BoardDataContainer* theErrorBitContainer, uint8_t line, const std::vector<uint32_t>& pattern, const std::vector<uint32_t>& patternMask, float numberOfBits, bool runAlignment);
    void patternCheckerTest(BoardDataContainer* theErrorBitContainer,
                            uint8_t             line,
                            BoardDataContainer& thePatternAndMaskContainer,
                            float               numberOfBits,
                            bool                runAlignment,
                            BoardDataContainer* theAlignmentPatternAndMaskContainer = nullptr);
    void prepareCalibration();

  private:
    void patternCheckerTest();

    float fNumberOfBits;

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: Histogrammer is handeld by the calibration itself
    DQMHistogramOTPatternCheckerHelper fDQMHistogramOTPatternCheckerHelper;
#endif
};

#endif
