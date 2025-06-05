/*!

        \file                   ExtTriggerLatencyScan.h
        \brief                 class to do latency and threshold scans
        \author                 Stefan Maier
        \version                1.0
        \date                   25/07/24
        Support :               mail to : stefan.maier@cern.ch

 */

#ifndef EXTTRIGGERLATENCYSCAN_H__
#define EXTTRIGGERLATENCYSCAN_H__

#include "tools/LatencyScan.h"

namespace Ph2_HwDescription
{
class BeBoard;
}

/*!
 * \class ExtTriggerLatencyScan
 * \brief Class to perform latency and threshold scans using external triggers. Inherits from Latency scan, only differs in the initialisation process
 */
class Occupancy;

class ExtTriggerLatencyScan : public LatencyScan
{
  public:
    ExtTriggerLatencyScan();
    ~ExtTriggerLatencyScan();
    //
    static std::string fCalibrationDescription;

    void Running() override;
    void Stop() override;
    void ConfigureCalibration() override;
    void Pause() override;
    void Resume() override;

  protected:
  private:
    void InitializeExternalTriggers();
};

#endif
