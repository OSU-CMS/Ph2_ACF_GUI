/*!

        \file                   OTQuickNoise.h
        \brief                  class for performing quick noise scan
        \author                 Jerome de favereau
        \version                1.0
        \date                   07/18/2023
        Support :               mail to : jerome.defavereau@uclouvain.be

 */
#ifndef OTQuickNoise_H__
#define OTQuickNoise_H__

#include "Utils/CommonVisitors.h"
#include "tools/Tool.h"
#include <math.h>

using namespace Ph2_System;
/*!
 * \class OTQuickNoise
 * \brief Class to perform quick noise scan
 */

class OTQuickNoise : public Tool
{
  public:
    OTQuickNoise();
    ~OTQuickNoise();
    void Initialize();
    void SetThresholds();
    void TakeData();

    void writeObjects();
    void Running() override;
    void Stop() override;
    void ConfigureCalibration() override;
    void Pause() override;
    void Resume() override;

  private:
    void parseSettings();

    uint32_t fNevents;
    uint32_t fVcth;
    uint32_t fManualVcth;
};

#endif
