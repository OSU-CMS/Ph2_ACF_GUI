/*!
 *
 * \file OTCICphaseAlignment.h
 * \brief OTCICphaseAlignment class
 * \author Fabio Ravera
 * \date 07/02/24
 *
 */

#ifndef OTCICphaseAlignment_h__
#define OTCICphaseAlignment_h__

#include "tools/Tool.h"
#include <map>
#ifdef __USE_ROOT__
// Calibration is not running on the SoC: I need to instantiate the DQM histogrammer here
#include "DQMUtils/DQMHistogramOTCICphaseAlignment.h"
#endif

namespace Ph2_HwDescription
{
class BeBoard;
}

class OTCICphaseAlignment : public Tool
{
  public:
    OTCICphaseAlignment();
    ~OTCICphaseAlignment();

    virtual void Initialise(void);

    // State machine
    void Running() override;
    void Stop() override;
    void ConfigureCalibration() override;
    void Pause() override;
    void Resume() override;
    void Reset();

    static std::string fCalibrationDescription;

  private:
    void phaseAlignment();
    void AlignAllCICinputsPS(Ph2_HwDescription::BeBoard* theBoard,
                             BoardDataContainer*         thePhaseHistogramBoardDataContainer,
                             BoardDataContainer*         theLockingEfficiencyBoardDataContainer,
                             BoardDataContainer*         theBestPhaseBoardDataContainer);
    void AlignAllCICinputs2S(Ph2_HwDescription::BeBoard* theBoard,
                             BoardDataContainer*         thePhaseHistogramBoardDataContainer,
                             BoardDataContainer*         theLockingEfficiencyBoardDataContainer,
                             BoardDataContainer*         theBestPhaseBoardDataContainer);

    size_t fNumberOfAlignmentIterations{100};
    float  fMinLockingSuccessRate{1.};

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: Histogrammer is handeld by the calibration itself
    DQMHistogramOTCICphaseAlignment fDQMHistogramOTCICphaseAlignment;
#endif
};

#endif
