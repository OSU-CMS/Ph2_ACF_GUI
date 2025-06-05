/*!
  \file                  RD53ADCPowerSupply.cc
  \brief                 Implementaion of ADC Power Supply Test
  \author                Umberto MOLINATTI
  \version               1.0
  \date                  05/02/21
  Support:               email to umberto.molinatti@cern.ch
*/

#include "RD53ADCPowerSupply.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_ITchipTesting;

void ADCPowerSupply::run(std::string configFile)
{
    LOG(INFO) << BOLDRED << "@@@ Again Performing Power Supply Test @@@" << RESET;
    // #ifdef __POWERSUPPLY__

    //     LOG(INFO) << BOLDYELLOW << "@@@ Still Performing Power Supply Test @@@" << RESET;
    //     pugi::xml_document docSettings;
    //     std::string cPowerSupply = "TestKeithley";

    //     DeviceHandler ps_deviceHandler;
    //     ps_deviceHandler.readSettings(configFile, docSettings);

    //     PowerSupply* ps = ps_deviceHandler.getPowerSupply(cPowerSupply);
    //     // PowerSupplyChannel* dPowerSupply = ps->getChannel("Front");

    //     KeithleyChannel* dKeithley2410 = static_cast<KeithleyChannel*>(ps->getChannel("Front"));

    //     //Configure Keithley for voltage measurements
    //     dKeithley2410->turnOff();
    //     //dKeithley2410->setCurrentMode(); //For external voltage reading
    //     //dKeithley2410->setCurrent(0.0);
    //     //dKeithley2410->setVoltageCompliance(2.0);
    // 	dKeithley2410->setVoltageMode(); //For external voltage injection
    // 	dKeithley2410->setVoltage(0.0);
    // 	dKeithley2410->setCurrentCompliance((float)1e-3);
    // 	dKeithley2410->setVoltageCompliance(1.0);
    //     // dKeithley2410->setCurrentRange(1e-6); // is private..
    //     //dKeithley2410->setParameter("Isrc_range", (float)1e-6);

    //     dKeithley2410->turnOn();
    // #endif

    ITpowerSupplyChannelInterface dKeithley2410(fPowerSupplyClient, "TestKeithley", "Front");

    dKeithley2410.setupKeithley2410ChannelSource(VOLTAGESOURCE, 0.0, (float)1e-3);
    // ITchipTestingInterface::setupKeithley2410ChannelSense("TestKeithley", "Front", VOLTAGESENSE, 1.0);

    // LOG(INFO) << "\tV(meas):\t" << BOLDWHITE << dKeithley2410->getOutputVoltage() << RESET;
    // LOG(INFO) << "\tI(meas):\t" << BOLDWHITE << dKeithley2410->getCurrent() << RESET;

    // #########################
    // # Mark enabled channels #
    // #########################
    auto RD53ChipInterface = static_cast<RD53Interface*>(this->fReadoutChipInterface);
    fitStart               = 0;
    fitEnd                 = 0;
    for(const auto cBoard: *fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    // RD53ChipInterface->ReadChipMonitor(cChip,"CAL_HI"); //Sets which variable is selected by the VMUX
                    RD53ChipInterface->ReadChipMonitor(cChip, "IMUXoutput"); // Sets which variable is selected by the VMUX
                    // for(int input=0;input<1000;input ++){ //For just reading
                    double step = 0.0002;
                    for(double input = 0.; input <= 0.9; input += step)
                    {
                        LOG(INFO) << "i=" << BOLDWHITE << input << RESET;

                        // ChipRegMap& pRD53RegMap = static_cast<RD53*>(cChip)->getRegMap();
                        // static_cast<RD53Interface*>(this->fReadoutChipInterface)->WriteChipReg(cChip,"VCAL_HIGH",input);
                        // #ifdef __POWERSUPPLY__
                        // 							dKeithley2410->setVoltage(input);
                        // 							VMUXvolt[int(input/step)] = dKeithley2410->getOutputVoltage();
                        // #endif
                        dKeithley2410.setVoltageK2410(input);
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        VMUXvolt[int(input / step)] = dKeithley2410.getVoltage();

                        ADCcode[int(input / step)] = RD53ChipInterface->ReadChipADC(cChip, "IMUXoutput");
                        // RD53ChipInterface->convertVorI2ADC(cChip, RD53ChipInterface->ReadChipMonitor(cChip,"IMUXoutput"));

                        if(int(input / step) > 5)
                        {
                            if(ADCcode[int(input / step)] != ADCcode[int(input / step) - 5] && fitStart == 0) { fitStart = VMUXvolt[int(input / step) - 5]; }
                            // if(ADCcode[int(input/step)]==ADCcode[int(input/step)-5] && fitEnd==0 && input > 0.45){
                            if(ADCcode[int(input / step)] == 4095 && fitEnd == 0 && input > 0.45) { fitEnd = VMUXvolt[int(input / step) - 5]; }
                            // if(fitEnd==0 && input == pRD53RegMap["VCAL_HIGH"].fBitSize - 1)
                            if(fitEnd == 0 && input > 0.89)
                            {
                                // fitEnd = pRD53RegMap["VCAL_HIGH"].fBitSize - 1;
                                fitEnd = 0.9;
                            }
                        }
                        LOG(INFO) << "fitStart=" << BOLDWHITE << fitStart << RESET;
                        LOG(INFO) << "fitEnd=" << BOLDWHITE << fitEnd << RESET;
                    }
                }
    // #ifdef __POWERSUPPLY__
    // 	dKeithley2410->setVoltage(0.0);
    // #endif
    dKeithley2410.setVoltage(0.0);
}

void ADCPowerSupply::draw(bool saveData)
{
    // #ifdef __USE_ROOT__
    // histos->fillPS(*fDetectorContainer, fitStart, fitEnd, VMUXvolt, ADCcode);
    // #endif
}
