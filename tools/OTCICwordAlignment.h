/*!
 *
 * \file OTCICwordAlignment.h
 * \brief OTCICwordAlignment class
 * \author Fabio Ravera
 * \date 07/02/24
 *
 */

#ifndef OTCICwordAlignment_h__
#define OTCICwordAlignment_h__

#include "tools/Tool.h"
#include <map>
#ifdef __USE_ROOT__
// Calibration is not running on the SoC: I need to instantiate the DQM histogrammer here
#include "DQMUtils/DQMHistogramOTCICwordAlignment.h"
#endif

class OTCICwordAlignment : public Tool
{
  public:
    OTCICwordAlignment();
    ~OTCICwordAlignment();

    void Initialise(void);

    // State machine
    void Running() override;
    void Stop() override;
    void ConfigureCalibration() override;
    void Pause() override;
    void Resume() override;
    void Reset();

    static std::string   fCalibrationDescription;
    std::vector<uint8_t> fFeEnableRegs = {0};
    uint32_t             fEnableMask;

  private:
    void WordAlignment(uint32_t pWait_us = 10);

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: Histogrammer is handeld by the calibration itself
    DQMHistogramOTCICwordAlignment fDQMHistogramOTCICwordAlignment;
#endif
};

#endif
