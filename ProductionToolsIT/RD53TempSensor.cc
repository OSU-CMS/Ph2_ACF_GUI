/*!
  \file                  RD53TempSensor.cc
  \brief                 Implementaion of TempSensor
  \author                Umberto MOLINATTI
  \version               1.0
  \date                  22/03/21
  Support:               email to umberto.molinatti@cern.ch
*/

#include "RD53TempSensor.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_ITchipTesting;

void TempSensor::run(std::string configFile)
{
    // #ifdef __POWERSUPPLY__

    //     pugi::xml_document docSettings;
    //     std::string cPowerSupply = "TestKeithley";

    //     DeviceHandler ps_deviceHandler;
    //     ps_deviceHandler.readSettings(configFile, docSettings);

    //     PowerSupply* ps = ps_deviceHandler.getPowerSupply(cPowerSupply);
    //     // PowerSupplyChannel* dPowerSupply = ps->getChannel("Front");

    //     KeithleyChannel* dKeithley2410 = static_cast<KeithleyChannel*>(ps->getChannel("Front"));

    //     //Configure Keithley for voltage measurements
    //     dKeithley2410->turnOff();
    //     dKeithley2410->setCurrentMode();
    //     // dKeithley2410->setCurrentRange(1e-6); // is private..
    //     dKeithley2410->setParameter("Isrc_range", (float)1e-6);
    //     dKeithley2410->setCurrent(0.0);
    //     dKeithley2410->setVoltageCompliance(2.0);

    //     dKeithley2410->turnOn();

    ITpowerSupplyChannelInterface dKeithley2410(fPowerSupplyClient, "TestKeithley", "Front");

    dKeithley2410.setupKeithley2410ChannelSense(VOLTAGESENSE, 2.0);

    // #########################
    // # Mark enabled channels #
    // #########################
    auto RD53ChipInterface = static_cast<RD53Interface*>(this->fReadoutChipInterface);
    for(const auto cBoard: *fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
            {
                for(const auto cChip: *cHybrid)
                {
                    for(int cycle = 0; cycle < 5; cycle++)
                    {
                        time[cycle] = cycle;
                        if(cycle == 0)
                        {
                            for(int mode = 0; mode < 2; mode++)
                            {
                                if(mode == 0)
                                { // Low power mode
                                    RD53ChipInterface->WriteChipReg(cChip, "PA_IN_BIAS_LIN", 0);
                                    RD53ChipInterface->WriteChipReg(cChip, "FC_BIAS_LIN", 0);
                                    RD53ChipInterface->WriteChipReg(cChip, "KRUM_CURR_LIN", 0);
                                    RD53ChipInterface->WriteChipReg(cChip, "LDAC_LIN", 0);
                                    RD53ChipInterface->WriteChipReg(cChip, "COMP_LIN", 0);
                                    RD53ChipInterface->WriteChipReg(cChip, "REF_KRUM_LIN", 0);
                                    RD53ChipInterface->WriteChipReg(cChip, "Vthreshold_LIN", 0);
                                    RD53ChipInterface->WriteChipReg(cChip, "IBIASP1_SYNC", 0);
                                    RD53ChipInterface->WriteChipReg(cChip, "IBIASP2_SYNC", 0);
                                    RD53ChipInterface->WriteChipReg(cChip, "IBIAS_SF_SYNC", 0);
                                    RD53ChipInterface->WriteChipReg(cChip, "IBIAS_KRUM_SYNC", 0);
                                    RD53ChipInterface->WriteChipReg(cChip, "IBIAS_DISC_SYNC", 0);
                                    RD53ChipInterface->WriteChipReg(cChip, "ICTRL_SYNCT_SYNC", 0);
                                    RD53ChipInterface->WriteChipReg(cChip, "VBL_SYNC", 0);
                                    RD53ChipInterface->WriteChipReg(cChip, "VTH_SYNC", 0);
                                    RD53ChipInterface->WriteChipReg(cChip, "VREF_KRUM_SYNC", 0);
                                    RD53ChipInterface->WriteChipReg(cChip, "CONF_FE_SYNC", 0);
                                    RD53ChipInterface->WriteChipReg(cChip, "PRMP_DIFF", 0);
                                    RD53ChipInterface->WriteChipReg(cChip, "FOL_DIFF", 0);
                                    RD53ChipInterface->WriteChipReg(cChip, "PRECOMP_DIFF", 0);
                                    RD53ChipInterface->WriteChipReg(cChip, "COMP_DIFF", 0);
                                    RD53ChipInterface->WriteChipReg(cChip, "VFF_DIFF", 0);
                                    RD53ChipInterface->WriteChipReg(cChip, "VTH1_DIFF", 0);
                                    RD53ChipInterface->WriteChipReg(cChip, "VTH2_DIFF", 0);
                                    RD53ChipInterface->WriteChipReg(cChip, "LCC_DIFF", 0);
                                    RD53ChipInterface->WriteChipReg(cChip, "EN_CORE_COL_LIN_1", 0);
                                    RD53ChipInterface->WriteChipReg(cChip, "EN_CORE_COL_LIN_2", 0);
                                    usleep(30000000);
                                }
                                else if(mode == 1)
                                { // Standard power mode
                                    RD53ChipInterface->WriteChipReg(cChip, "PA_IN_BIAS_LIN", 350);
                                    RD53ChipInterface->WriteChipReg(cChip, "FC_BIAS_LIN", 20);
                                    RD53ChipInterface->WriteChipReg(cChip, "KRUM_CURR_LIN", 29);
                                    RD53ChipInterface->WriteChipReg(cChip, "LDAC_LIN", 130);
                                    RD53ChipInterface->WriteChipReg(cChip, "COMP_LIN", 110);
                                    RD53ChipInterface->WriteChipReg(cChip, "REF_KRUM_LIN", 300);
                                    RD53ChipInterface->WriteChipReg(cChip, "Vthreshold_LIN", 400);
                                    RD53ChipInterface->WriteChipReg(cChip, "IBIASP1_SYNC", 100);
                                    RD53ChipInterface->WriteChipReg(cChip, "IBIASP2_SYNC", 150);
                                    RD53ChipInterface->WriteChipReg(cChip, "IBIAS_SF_SYNC", 80);
                                    RD53ChipInterface->WriteChipReg(cChip, "IBIAS_KRUM_SYNC", 55);
                                    RD53ChipInterface->WriteChipReg(cChip, "IBIAS_DISC_SYNC", 280);
                                    RD53ChipInterface->WriteChipReg(cChip, "ICTRL_SYNCT_SYNC", 100);
                                    RD53ChipInterface->WriteChipReg(cChip, "VBL_SYNC", 360);
                                    RD53ChipInterface->WriteChipReg(cChip, "VTH_SYNC", 400);
                                    RD53ChipInterface->WriteChipReg(cChip, "VREF_KRUM_SYNC", 450);
                                    RD53ChipInterface->WriteChipReg(cChip, "CONF_FE_SYNC", 2);
                                    RD53ChipInterface->WriteChipReg(cChip, "PRMP_DIFF", 511);
                                    RD53ChipInterface->WriteChipReg(cChip, "FOL_DIFF", 542);
                                    RD53ChipInterface->WriteChipReg(cChip, "PRECOMP_DIFF", 512);
                                    RD53ChipInterface->WriteChipReg(cChip, "COMP_DIFF", 1023);
                                    RD53ChipInterface->WriteChipReg(cChip, "VFF_DIFF", 40);
                                    RD53ChipInterface->WriteChipReg(cChip, "VTH1_DIFF", 700);
                                    RD53ChipInterface->WriteChipReg(cChip, "VTH2_DIFF", 100);
                                    RD53ChipInterface->WriteChipReg(cChip, "LCC_DIFF", 20);
                                    RD53ChipInterface->WriteChipReg(cChip, "EN_CORE_COL_LIN_1", 65535);
                                    RD53ChipInterface->WriteChipReg(cChip, "EN_CORE_COL_LIN_2", 1);
                                    usleep(30000000);
                                }
                                for(int sensor = 0; sensor < 4; sensor++)
                                {
                                    RD53ChipInterface->ReadChipMonitor(cChip, "TEMPSENS_" + std::to_string(sensor + 1));

                                    calibNTCtemp[sensor][mode] = RD53ChipInterface->ReadHybridTemperature(cChip);

                                    valueHigh = 0;
                                    // Get high bias voltage
                                    for(int sensorDEM = 0; sensorDEM < 16; sensorDEM += 7)
                                    {
                                        sensorConfigData = bits::pack<1, 4, 1, 1, 4, 1>(true, sensorDEM, 0, true, sensorDEM, 0);
                                        RD53ChipInterface->WriteChipReg(cChip, "SENSOR_CONFIG_0", sensorConfigData);
                                        RD53ChipInterface->WriteChipReg(cChip, "SENSOR_CONFIG_1", sensorConfigData);
                                        valueHigh += dKeithley2410.getVoltage(); // dKeithley2410->getOutputVoltage();
                                    }
                                    valueHigh = valueHigh / 3;

                                    valueLow = 0;
                                    // Get low bias voltage
                                    for(int sensorDEM = 0; sensorDEM < 16; sensorDEM += 7)
                                    {
                                        sensorConfigData = bits::pack<1, 4, 1, 1, 4, 1>(true, sensorDEM, 1, true, sensorDEM, 1);
                                        RD53ChipInterface->WriteChipReg(cChip, "SENSOR_CONFIG_0", sensorConfigData);
                                        RD53ChipInterface->WriteChipReg(cChip, "SENSOR_CONFIG_1", sensorConfigData);
                                        valueLow += dKeithley2410.getVoltage(); // dKeithley2410->getOutputVoltage();
                                    }
                                    valueLow = valueLow / 3;

                                    calibDV[sensor][mode] = valueLow - valueHigh;
                                    if(mode == 1)
                                    {
                                        double NTCfactor         = calibNTCtemp[sensor][0] - power[0] * (calibNTCtemp[sensor][1] - calibNTCtemp[sensor][0]) / (power[1] - power[0]);
                                        idealityFactor[sensor]   = e / (kb * log(R)) * ((calibDV[sensor][0] * power[1] - power[0] * calibDV[sensor][1]) / ((power[1] - power[0]) * (NTCfactor + T0C)));
                                        calibSenstemp[sensor][0] = (e / (kb * log(R))) * (calibDV[sensor][0]) / idealityFactor[sensor] - T0C;
                                        calibSenstemp[sensor][1] = (e / (kb * log(R))) * (calibDV[sensor][1]) / idealityFactor[sensor] - T0C;
                                    }
                                }
                            }
                        }
                        for(int sensor = 0; sensor < 4; sensor++)
                        {
                            RD53ChipInterface->ReadChipMonitor(cChip, "TEMPSENS_" + std::to_string(sensor + 1));

                            valueHigh = 0;
                            // Get high bias voltage
                            for(int sensorDEM = 0; sensorDEM < 16; sensorDEM += 7)
                            {
                                sensorConfigData = bits::pack<1, 4, 1, 1, 4, 1>(true, sensorDEM, 0, true, sensorDEM, 0);
                                RD53ChipInterface->WriteChipReg(cChip, "SENSOR_CONFIG_0", sensorConfigData);
                                RD53ChipInterface->WriteChipReg(cChip, "SENSOR_CONFIG_1", sensorConfigData);
                                valueHigh += dKeithley2410.getVoltage(); // dKeithley2410->getOutputVoltage();
                            }
                            valueHigh = valueHigh / 3;

                            valueLow = 0;
                            // Get low bias voltage
                            for(int sensorDEM = 0; sensorDEM < 16; sensorDEM += 7)
                            {
                                sensorConfigData = bits::pack<1, 4, 1, 1, 4, 1>(true, sensorDEM, 1, true, sensorDEM, 1);
                                RD53ChipInterface->WriteChipReg(cChip, "SENSOR_CONFIG_0", sensorConfigData);
                                RD53ChipInterface->WriteChipReg(cChip, "SENSOR_CONFIG_1", sensorConfigData);
                                valueLow += dKeithley2410.getVoltage(); // dKeithley2410->getOutputVoltage();
                            }
                            valueLow = valueLow / 3;

                            temperature[4][cycle]      = RD53ChipInterface->ReadHybridTemperature(cChip);
                            temperature[sensor][cycle] = (e / (kb * log(R))) * (valueLow - valueHigh) / idealityFactor[sensor] - T0C;
                        }
                    }
                }
            }
    // #endif
}

void TempSensor::draw()
{
#ifdef __USE_ROOT__
    histos->fillTC(time, temperature, idealityFactor, calibNTCtemp, calibSenstemp, power);
#endif
}
