/*!
 *
 * \file OTinjectionDelayOptimization.h
 * \brief OTinjectionDelayOptimization class
 * \author Fabio Ravera
 * \date 15/03/24
 *
 */

#ifndef OTinjectionDelayOptimization_h__
#define OTinjectionDelayOptimization_h__

#include "tools/Tool.h"
#include <map>
#ifdef __USE_ROOT__
// Calibration is not running on the SoC: I need to instantiate the DQM histogrammer here
#include "DQMUtils/DQMHistogramOTinjectionDelayOptimization.h"
#endif

class OTinjectionDelayOptimization : public Tool
{
  public:
    OTinjectionDelayOptimization();
    ~OTinjectionDelayOptimization();

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
    void optimizeInjectionDelay();

    void prepareInjectionDelayScan2S();
    void prepareInjectionDelayScanPS();

    void setLatencyAndDelay2S(uint16_t totalInjectionDelay);
    void setLatencyAndDelayPS(uint16_t totalInjectionDelay);

    std::pair<uint16_t, uint8_t> calculateDACsFromTotalDelay(uint16_t totalDelay, bool is2Smodule) const;

    uint32_t       fNumberOfEvents{100};
    uint16_t       fMaximumDelay{150};
    uint16_t       fDelayStep{1};
    float          fCBCtestPulseValue{1.};
    float          fSSAtestPulseValue{0.5};
    float          fMPAtestPulseValue{0.5};
    float          fCBCnumberOfSigmaNoiseAwayFromPedestal{5.};
    float          fSSAnumberOfSigmaNoiseAwayFromPedestal{5.};
    float          fMPAnumberOfSigmaNoiseAwayFromPedestal{5.};
    const uint16_t fInitialLatency{200};
    const int      fNumberOfDelayInClockCycle2S{25};
    const int      fNumberOfDelayInClockCyclePS{12};

#ifdef __USE_ROOT__
    // Calibration is not running on the SoC: Histogrammer is handeld by the calibration itself
    DQMHistogramOTinjectionDelayOptimization fDQMHistogramOTinjectionDelayOptimization;
#endif
};

#endif
