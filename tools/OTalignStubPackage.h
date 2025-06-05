/*!
 *
 * \file OTalignStubPackage.h
 * \brief OTalignStubPackage class
 * \author Fabio Ravera
 * \date 26/01/24
 *
 */

#ifndef OTalignStubPackage_h__
#define OTalignStubPackage_h__

#include "tools/Tool.h"
#include <map>
#ifdef __USE_ROOT__
// Calibration is not running on the SoC: I need to instantiate the DQM histogrammer here
#include "DQMUtils/DQMHistogramOTalignStubPackage.h"
#endif

namespace Ph2_HwDescription
{
class BeBoard;
}

class OTalignStubPackage : public Tool
{
  public:
    OTalignStubPackage();
    ~OTalignStubPackage();

    void Initialise(void);

    // State machine
    void Running() override;
    void Stop() override;
    void ConfigureCalibration() override;
    void Pause() override;
    void Resume() override;
    void Reset();

    static std::string fCalibrationDescription;

    bool AlignStubPackage();

  private:
    bool fIsKickoff{false};
#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: Histogrammer is handeld by the calibration itself
    DQMHistogramOTalignStubPackage fDQMHistogramOTalignStubPackage;
#endif
};

#endif
