/*!
  \file                  RD53TempSensor.h
  \brief                 Implementaion of TempSensor
  \author                Umberto MOLINATTI
  \version               1.0
  \date                  22/03/21
  Support:               email to umberto.molinatti@cern.ch
*/

#ifndef RD53TempSensor_H
#define RD53TempSensor_H

#include "ITchipTestingInterface.h"
#include "tools/Tool.h"

// #ifdef __POWERSUPPLY__
// #include "DeviceHandler.h"
// #include "PowerSupply.h"
// #include "PowerSupplyChannel.h"
// #include "Keithley.h"
// #endif

#include <cmath>

#include "HWInterface/BeBoardFWInterface.h"
#include "HWInterface/RD53FWInterface.h"
#include "HWInterface/ReadoutChipInterface.h"

#ifdef __USE_ROOT__
#include "DQMUtils/ProductionITRD53A/RD53TempSensorHistograms.h"
#endif

// #########################
// # TempSensor test suite #
// #########################
class TempSensor : public Tool
{
  public:
    void run(std::string configFile);
    void draw();

#ifdef __USE_ROOT__
    TempSensorHistograms* histos;
#endif

  private:
    double      time[100];
    double      temperature[5][100] = {{0}};
    double      idealityFactor[4];
    const float T0C = 273.15;         // [Kelvin]
    const float kb  = 1.38064852e-23; // [J/K]
    const float e   = 1.6021766208e-19;
    const float R   = 15; // By circuit design
    double      valueLow;
    double      valueHigh;
    double      power[2] = {0.24, 0.72}; // In direct powering

    // Calibration variables
    double calibDV[4][2];
    double calibNTCtemp[4][2];
    double calibSenstemp[4][2];

    uint16_t sensorConfigData;
};

#endif
