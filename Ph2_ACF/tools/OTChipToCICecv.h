/*!
 *
 * \file OTChipToCICecv.h
 * \brief OTChipToCICecv class
 * \author Kuldeep Pal
 * \date 28/05/24
 *
 */

#ifndef OTChipToCICecv_h__
#define OTChipToCICecv_h__

#include "tools/OTalignLpGBTinputsForBypass.h"
#include <map>
#ifdef __USE_ROOT__
// Calibration is not running on the SoC: I need to instantiate the DQM histogrammer here
#include "DQMUtils/DQMHistogramOTChipToCICecv.h"
#endif

namespace Ph2_HwDescription
{
class BeBoard;
}

class OTChipToCICecv : public OTalignLpGBTinputsForBypass
{
  public:
    OTChipToCICecv();
    ~OTChipToCICecv();

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
    void               runOTChiptoCICecv();
    std::vector<float> fListOfCBCslvsCurrents{0, 8, 14};
    std::vector<float> fListOfMPAslvsCurrents{1, 4, 7};

    void setCICPhase(Ph2_HwDescription::BeBoard* theBoard, uint8_t phase, uint8_t phyPort);
    void setSlvsChipCurrent(Ph2_HwDescription::BeBoard* theBoard, uint8_t slvsCurrent);

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: Histogrammer is handeld by the calibration itself
    DQMHistogramOTChipToCICecv fDQMHistogramOTChipToCICecv;
#endif
};

#endif
