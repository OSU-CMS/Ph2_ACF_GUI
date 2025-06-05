/*!
  \file                  RD53DACScan.h
  \brief                 Implementaion of DACScan
  \author                Umberto MOLINATTI
  \version               1.0
  \date                  18/03/21
  Support:               email to umberto.molinatti@cern.ch
*/

#ifndef RD53DACScan_H
#define RD53DACScan_H

#include "ITchipTestingInterface.h"
#include "tools/Tool.h"

// #ifdef __POWERSUPPLY__
// #include "DeviceHandler.h"
// #include "PowerSupply.h"
// #include "PowerSupplyChannel.h"
// #include "Keithley.h"
// #endif

#ifdef __USE_ROOT__
#include "DQMUtils/ProductionITRD53A/RD53DACScanHistograms.h"
#include "TApplication.h"
#endif

#include <string>

// #########################
// # DACScan test suite #
// #########################
class DACScan : public Tool
{
  public:
    void run(std::string configFile);
    // void   draw(const DetectorDataContainer& DataContainer, double* fitStart, double* fitEnd, double** VMUXvolt, double** DACcode, double** DNLcode, double** INLcode, const char* writeVar);
    void draw(bool saveData = true);

#ifdef __USE_ROOT__
    DACScanHistograms* histos;
#endif

  private:
    double*  fitStart = new double[9]();
    double*  fitEnd   = new double[9]();
    double** VMUXvolt = new double*[9];
    double** DACcode  = new double*[9];
    double** DNLcode  = new double*[9];
    double** INLcode  = new double*[9];

    std::string readVar[9]  = {"CAL_HI", "CAL_MED", "REF_KRUM_LIN", "Vthreshold_LIN", "VTH_SYNC", "VBL_SYNC", "VREF_KRUM_SYNC", "VTH_HI_DIFF", "VTH_LO_DIFF"};
    std::string writeVar[9] = {"VCAL_HIGH", "VCAL_MED", "REF_KRUM_LIN", "Vthreshold_LIN", "VTH_SYNC", "VBL_SYNC", "VREF_KRUM_SYNC", "VTH1_DIFF", "VTH2_DIFF"};

    // const char* readVar[9] = {"CAL_HI","CAL_MED","REF_KRUM_LIN","Vthreshold_LIN","VTH_SYNC","VBL_SYNC","VREF_KRUM_SYNC","VTH_HI_DIFF","VTH_LO_DIFF"};
    // const char* writeVar[9] = {"VCAL_HIGH","VCAL_MED","REF_KRUM_LIN","Vthreshold_LIN","VTH_SYNC","VBL_SYNC","VREF_KRUM_SYNC","VTH1_DIFF","VTH2_DIFF"};

    // const char* readVar[28] = {"DACbandgap","CAL_MED","CAL_HI","TEMPSENS_1","RADSENS_1","TEMPSENS_2","RADSENS_2","TEMPSENS_4","RADSENS_4","RADSENS_3","TEMPSENS_3","VOUT_BG","VREF_VDAC",
    //	"REF_KRUM_LIN","Vthreshold_LIN","VTH_SYNC","VBL_SYNC","VREF_KRUM_SYNC","VTH_HI_DIFF","VTH_LO_DIFF","VIN_ana_ShuLDO","VOUT_ana_ShuLDO","VREF_ana_ShuLDO","VOFF_ana_ShuLDO","VIN_dig_ShuLDO",
    //	"VOUT_dig_ShuLDO","VREF_dig_ShuLDO","VOFF_dig_ShuLDO"};
    // const char* writeVar[28] = {"DAC bandgap","VCAL_MED","VCAL_HIGH","TEMPSENS_1","RADSENS_1","TEMPSENS_2","RADSENS_2","TEMPSENS_4","RADSENS_4","RADSENS_3","TEMPSENS_3","VOUT_BG","VREF_VDAC",
    //	"REF_KRUM_LIN","Vthreshold_LIN","VTH_SYNC","VBL_SYNC","VREF_KRUM_SYNC","VTH1_DIFF","VTH2_DIFF","VIN_ana_ShuLDO","VOUT_ana_ShuLDO","VREF_ana_ShuLDO","VOFF_ana_ShuLDO","VIN_dig_ShuLDO",
    //	"VOUT_dig_ShuLDO","VREF_dig_ShuLDO","VOFF_dig_ShuLDO"};
};

#endif
