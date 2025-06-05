/*!
 *
 * \file OTMeasureOccupancy.h
 * \brief OTMeasureOccupancy class
 * \author Fabio Ravera
 * \date 03/04/24
 *
 */

#ifndef OTMeasureOccupancy_h__
#define OTMeasureOccupancy_h__

#include "tools/Tool.h"
#include <map>
#ifdef __USE_ROOT__
// Calibration is not running on the SoC: I need to instantiate the DQM histogrammer here
#include "DQMUtils/DQMHistogramOTMeasureOccupancy.h"
#endif

class OTMeasureOccupancy : public Tool
{
  public:
    OTMeasureOccupancy();
    ~OTMeasureOccupancy();

    void Initialise(void);

    // State machine
    void               Running() override;
    void               Stop() override;
    void               ConfigureCalibration() override;
    void               Pause() override;
    void               Resume() override;
    void               Reset();
    void               prepareOccupancyMeasurementPS();
    float              fSSAtestPulseValue{1.};
    float              fMPAtestPulseValue{1.};
    static std::string fCalibrationDescription;

  protected:
    void measureChannelOccupancy(size_t iteration = 0);
    void prepareOccupancyMeasurement2S();
    // void prepareOccupancyMeasurementPS();
    void applyThresholdOffset();

    uint32_t fNumberOfEvents{10000};
    float    fCBCtestPulseValue{1.};
    bool     fForceChannelGroup{false};
    int      fThresholdOffset{0};

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: Histogrammer is handeld by the calibration itself
    DQMHistogramOTMeasureOccupancy* fDQMHistogramOTMeasureOccupancy{nullptr};
#endif
};

#endif
