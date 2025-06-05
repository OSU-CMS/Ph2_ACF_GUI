/*!
 *
 * \file OTinjectionOccupancyScan.h
 * \brief OTinjectionOccupancyScan class
 * \author Fabio Ravera
 * \date 15/05/24
 *
 */

#ifndef OTinjectionOccupancyScan_h__
#define OTinjectionOccupancyScan_h__

#include "tools/OTMeasureOccupancy.h"
#include <map>

class OTinjectionOccupancyScan : public OTMeasureOccupancy
{
  public:
    OTinjectionOccupancyScan();
    ~OTinjectionOccupancyScan();

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
    void               scanInjection();
    std::vector<float> fListOfPulseValues{0, 0.25, 0.5, 1., 2.};
    uint32_t           fNumberOfEventsWithoutInjection{1000000};
    uint32_t           fNumberOfEventsWithInjection{1000};
};

#endif
