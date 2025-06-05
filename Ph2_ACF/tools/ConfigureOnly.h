#ifndef __CONFIGURE_ONLY__
#define __CONFIGURE_ONLY__

#include "tools/Tool.h"

class ConfigureOnly : public Tool
{
  public:
    ConfigureOnly();
    ~ConfigureOnly();

    void Running() override;
    void Stop() override;
    void Pause() override;
    void Resume() override;

    static std::string fCalibrationDescription;
};

#endif