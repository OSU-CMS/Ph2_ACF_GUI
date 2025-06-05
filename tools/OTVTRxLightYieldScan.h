/*!
 *
 * \file OTVTRxLightYieldScan.h
 * \brief OTVTRxLightYieldScan class
 * \author Fabio Ravera
 * \date 12/09/24
 *
 */

#ifndef OTVTRxLightYieldScan_h__
#define OTVTRxLightYieldScan_h__

#include "tools/Tool.h"
#include <map>
#ifdef __USE_ROOT__
// Calibration is not running on the SoC: I need to instantiate the DQM histogrammer here
#include "DQMUtils/DQMHistogramOTVTRxLightYieldScan.h"
#endif

class OTVTRxLightYieldScan : public Tool
{
  public:
    OTVTRxLightYieldScan();
    ~OTVTRxLightYieldScan();

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
    void scanVTRxLightYield();

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: Histogrammer is handeld by the calibration itself
    DQMHistogramOTVTRxLightYieldScan fDQMHistogramOTVTRxLightYieldScan;
#endif
};

#endif
