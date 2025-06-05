/*!
 *
 * \file CalibrationExample.h
 * \brief Calibration example -> use it as a template
 * \author Fabio Ravera
 * \date 25 / 07 / 19
 *
 * \Support : fabio.ravera@cern.ch
 *
 */

#ifndef CalibrationExample_h__
#define CalibrationExample_h__

#include "tools/Tool.h"
#include <map>
#ifdef __USE_ROOT__
// Calibration is not running on the SoC: I need to instantiate the DQM histogrammer here
#include "DQMUtils/DQMHistogramCalibrationExample.h"
#endif

using namespace Ph2_System;

class CalibrationExample : public Tool
{
  public:
    CalibrationExample();
    ~CalibrationExample();

    void Initialise(void);
    void runCalibrationExample(void);
    void writeObjects(void);

    // State machine
    void Running() override;
    void Stop(void) override;

    static std::string fCalibrationDescription;

  private:
    uint32_t fEventsPerPoint;

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: Histogrammer is handeld by the calibration itself
    DQMHistogramCalibrationExample fDQMHistogramCalibrationExample;
#endif
};

#endif
