/*!
  \file                  RD53DACScan.cc
  \brief                 Implementaion of DACScan
  \author                Umberto MOLINATTI
  \version               1.0
  \date                  18/03/21
  Support:               email to umberto.molinatti@cern.ch
*/

#include "RD53DACScan.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_ITchipTesting;

void DACScan::run(std::string configFile)
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
                for(const auto cChip: *cHybrid)
                {
                    for(int input = 0; input < 4096; input += 1)
                    {
                        LOG(INFO) << BOLDBLUE << "i        = " << BOLDYELLOW << input << " " << RESET;
                        for(int variable = 0; variable < 1; variable++)
                        {
                            ChipRegMap& pRD53RegMap = static_cast<RD53*>(cChip)->getRegMap();
                            if(input > pow(2.0, pRD53RegMap[writeVar[variable]].fBitSize) - 1) continue;
                            if(input == 0)
                            {
                                VMUXvolt[variable] = new double[5000];
                                DACcode[variable]  = new double[5000];
                                DNLcode[variable]  = new double[5000];
                                INLcode[variable]  = new double[5000];
                            }
                            static_cast<RD53Interface*>(this->fReadoutChipInterface)->WriteChipReg(cChip, writeVar[variable], input);
                            DACcode[variable][input] = input;
                            // Change voltage being read
                            RD53ChipInterface->ReadChipMonitor(cChip, readVar[variable]);

                            VMUXvolt[variable][input] = dKeithley2410.getVoltage(); // dKeithley2410->getOutputVoltage();
                            LOG(INFO) << BOLDBLUE << "VMUXvolt[input]        = " << BOLDYELLOW << VMUXvolt[variable][input] << " " << RESET;

                            if(input > 1)
                            {
                                if(((DACcode[variable][input] > 0 && DACcode[variable][input - 1] == 0) || (DACcode[variable][input - 1] > 0 && DACcode[variable][input] == 0)) &&
                                   fitStart[variable] == 0)
                                {
                                    fitStart[variable] = VMUXvolt[variable][input - 1];
                                    // The last two with inverse structure don't have the correct starting point
                                }
                                if(((DACcode[variable][input] == 4095 && DACcode[variable][input - 1] < 4095) || (DACcode[variable][input] < 4095 && DACcode[variable][input - 1] == 4095)) &&
                                   fitEnd[variable] == 0)
                                    fitEnd[variable] = VMUXvolt[variable][input];
                                if(fitEnd[variable] == 0 && input == pow(2.0, pRD53RegMap[writeVar[variable]].fBitSize) - 1 - 1) fitEnd[variable] = VMUXvolt[variable][input];
                            }

                            if(input > 0) { DNLcode[variable][input] = DACcode[variable][input] - DACcode[variable][input - 1] - 1; }
                            else { DNLcode[variable][0] = 0; }
                            INLcode[variable][input] = DACcode[variable][input] - DACcode[variable][0] - input;
                        }
                    }
                }
    // #endif
}

void DACScan::draw(bool saveData)
{
#ifdef __USE_ROOT__
    histos->fillDAC(*fDetectorContainer, fitStart, fitEnd, VMUXvolt, DACcode, DNLcode, INLcode, writeVar);
#endif
}
