/*!
 *
 * \file OTSSAtoSSAecv.h
 * \brief OTSSAtoSSAecv class
 * \author Fabio Ravera
 * \date 26/06/24
 *
 */

#ifndef OTSSAtoSSAecv_h__
#define OTSSAtoSSAecv_h__

#include "tools/OTSSAtoMPAecv.h"
#include <map>
#ifdef __USE_ROOT__
// Calibration is not running on the SoC: I need to instantiate the DQM histogrammer here
#include "DQMUtils/DQMHistogramOTSSAtoSSAecv.h"
#endif

class OTSSAtoSSAecv : public OTSSAtoMPAecv
{
  public:
    OTSSAtoSSAecv();
    ~OTSSAtoSSAecv();

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
    void                           runSSAtoSSAecvScan();
    std::vector<Cluster>           produceStripClusterList() override;
    std::vector<std::vector<Stub>> createPSstubList() override;
    void                           setStripOffsetParameters(Ph2_HwDescription::ReadoutChip* theSSA) override;
    std::vector<float>             fListOfSSAslvsCurrents{1, 4, 7};
    GenericDataArray<float, 2>&    getStorageForStubErrorRate(Ph2_HwDescription::Hybrid* theHybrid, uint8_t chipId, uint8_t line, size_t stubPatternCounter) override;
    void                           setSampleClockEdgeAndPhase(Ph2_HwDescription::BeBoard* theBoard, uint8_t clockEdge, int samplingPhaseOffset);
    void                           resetPatternMatchingEfficiencyContainer() override;

    uint8_t fStripClusterColRightToLeft = 1;
    uint8_t fStripClusterColLeftToRight = 118;

    uint8_t fPixelClusterColRightToLeft = 117;
    uint8_t fPixelClusterColLeftToRight = 2;

    int                   fMinimum320PhaseShift = -1;
    int                   fMaximum320PhaseShift = +1;
    DetectorDataContainer fOriginalPhaseContainer;

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: Histogrammer is handeld by the calibration itself
    DQMHistogramOTSSAtoSSAecv fDQMHistogramOTSSAtoSSAecv;
#endif
};

#endif
