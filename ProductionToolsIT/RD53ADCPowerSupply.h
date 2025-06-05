/*!
  \file                  RD53ADCPowerSupply.h
  \brief                 Implementaion of ADC Power Supply Test
  \author                Umberto MOLINATTI
  \version               1.0
  \date                  05/02/21
  Support:               email to umberto.molinatti@cern.ch
*/

#ifndef RD53ADCPowerSupply_H
#define RD53ADCPowerSupply_H

#include "ITchipTestingInterface.h"
#include "tools/Tool.h"

// #ifdef __POWERSUPPLY__
// #include "DeviceHandler.h"
// #include "PowerSupply.h"
// #include "PowerSupplyChannel.h"
// #include "Keithley.h"
// #endif

// ######################
// # ADCPowerSupply test suite #
// ######################
class ADCPowerSupply : public Tool
{
  public:
    void run(std::string configFile);
    void draw(bool saveData = true);

    // #ifdef __USE_ROOT__
    //     ADCPowerSupplyHistograms* histos;
    // #endif

  private:
    double* VMUXvolt = new double[5000];
    double  fitStart;
    double  fitEnd;
    double* ADCcode = new double[5000];
};

#endif
