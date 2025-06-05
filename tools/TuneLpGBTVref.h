#ifndef __TUNE_LPGBT_VREF__
#define __TUNE_LPGBT_VREF__

#include "tools/Tool.h"

class TuneLpGBTVref : public Tool
{
  public:
    TuneLpGBTVref();
    ~TuneLpGBTVref();

    void Initialise();
    void Running() override;
    void Stop() override;
    void Pause() override;
    void Resume() override;

    static std::string fCalibrationDescription;

  private:
    void reset();
    void tuneVref();
    bool fDoManualVrefTuning{false};
};

#endif