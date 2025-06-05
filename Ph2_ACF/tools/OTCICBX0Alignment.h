/*!
 *
 * \file OTCICBX0Alignment.h
 * \brief OTCICBX0Alignment class
 * \author Irene Zoi
 * \date 22/02/24
 *
 */

#ifndef OTCICBX0Alignment_h__
#define OTCICBX0Alignment_h__

#include "Tool.h"
#include <map>
#ifdef __USE_ROOT__
// Calibration is not running on the SoC: I need to instantiate the DQM histogrammer here
#include "DQMUtils/DQMHistogramOTCICBX0Alignment.h"
#endif

class OTCICBX0Alignment : public Tool
{
  public:
    OTCICBX0Alignment();
    ~OTCICBX0Alignment();

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
    void BX0Alignment();
    void ScanRetimePixAndBX0Alignment();

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: Histogrammer is handeld by the calibration itself
    DQMHistogramOTCICBX0Alignment fDQMHistogramOTCICBX0Alignment;
#endif
};

#endif