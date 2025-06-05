/*!
 *
 * \file OTSSAtoMPAecv.h
 * \brief OTSSAtoMPAecv class
 * \author Fabio Ravera
 * \date 11/06/24
 *
 */

#ifndef OTSSAtoMPAecv_h__
#define OTSSAtoMPAecv_h__

#include "tools/OTverifyMPASSAdataWord.h"
#include <map>
#ifdef __USE_ROOT__
// Calibration is not running on the SoC: I need to instantiate the DQM histogrammer here
#include "DQMUtils/DQMHistogramOTSSAtoMPAecv.h"
#endif

namespace Ph2_HwDescription
{
class BeBoard;
class ReadoutChip;
} // namespace Ph2_HwDescription
class OTSSAtoMPAecv : public OTverifyMPASSAdataWord
{
  public:
    OTSSAtoMPAecv();
    ~OTSSAtoMPAecv();

    void Initialise(void);

    // State machine
    void Running() override;
    void Stop() override;
    void ConfigureCalibration() override;
    void Pause() override;
    void Resume() override;
    void Reset();

    static std::string fCalibrationDescription;

  protected:
    virtual void resetPatternMatchingEfficiencyContainer();

  private:
    void               runSSAtoMPAecvScan();
    void               runSSAtoMPAecvScan(uint8_t slvsCurrent);
    void               setSampleClockEdgeAndPhase(Ph2_HwDescription::BeBoard* theBoard, uint8_t clockEdge, int samplingPhaseOffset);
    std::vector<float> fListOfSSAslvsCurrents{1, 4, 7};

    int                   fMinimum320PhaseShift = -1;
    int                   fMaximum320PhaseShift = +1;
    DetectorDataContainer fOriginalL1PhaseContainer;
    DetectorDataContainer fOriginalStubPhaseContainer;

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: Histogrammer is handeld by the calibration itself
    DQMHistogramOTSSAtoMPAecv fDQMHistogramOTSSAtoMPAecv;
#endif
};

#endif
