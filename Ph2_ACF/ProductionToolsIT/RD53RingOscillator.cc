/*!
  \file                  RD53RingOscillator.cc
  \brief                 Implementaion of RingOscillator
  \author                Umberto MOLINATTI
  \version               1.0
  \date                  16/02/21
  Support:               email to umberto.molinatti@cern.ch
*/

#include "RD53RingOscillator.h"
#include "HWDescription/RD53ACommands.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;

void RingOscillator::run()
{
    // #########################
    // # Mark enabled channels #
    // #########################
    auto RD53ChipInterface = static_cast<RD53InterfaceRing*>(this->fReadoutChipInterface);
    for(const auto cBoard: *fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    for(int gPulse = 0; gPulse < 11; gPulse++)
                    {
                        gloPulse[gPulse] = pow(2, gPulse);
                        for(int ringOsc = 0; ringOsc < 8; ringOsc++)
                        {
                            static_cast<RD53InterfaceRing*>(this->fReadoutChipInterface)->WriteChipReg(cChip, "RING_OSC_" + std::to_string(ringOsc), 0); // Reset Oscillator
                            static_cast<RD53InterfaceRing*>(this->fReadoutChipInterface)->WriteChipReg(cChip, "RING_OSC_ENABLE", 255);
                            static_cast<RD53InterfaceRing*>(this->fReadoutChipInterface)->WriteChipReg(cChip, "GlobalPulseConf", 0x2000);
                            static_cast<RD53InterfaceRing*>(this->fReadoutChipInterface)->sendCommand(cChip, RD53ACmd::GlobalPulse{cChip->getId(), (size_t)gPulse});

                            oscCounts[ringOsc][gPulse]    = RD53ChipInterface->ReadChipReg(static_cast<RD53*>(cChip), "RING_OSC_" + std::to_string(ringOsc)) - 4096;
                            oscFrequency[ringOsc][gPulse] = oscCounts[ringOsc][gPulse] / (gloPulse[gPulse] / 0.16);
                        }
                    }
                    for(int vTrim = 0; vTrim < 16; vTrim++)
                    {
                        // Trim voltages
                        RD53ChipInterface->WriteChipReg(cChip, "VOLTAGE_TRIM", vTrim + 512);
                        trimVoltage[vTrim] = RD53ChipInterface->ReadChipMonitor(cChip, "VOUT_dig_ShuLDO") * 2;
                        for(int ringOsc = 0; ringOsc < 8; ringOsc++)
                        {
                            // Set up oscillators
                            static_cast<RD53InterfaceRing*>(this->fReadoutChipInterface)->WriteChipReg(cChip, "RING_OSC_" + std::to_string(ringOsc), 0); // Reset Oscillator
                            static_cast<RD53InterfaceRing*>(this->fReadoutChipInterface)->WriteChipReg(cChip, "RING_OSC_ENABLE", 255);
                            static_cast<RD53InterfaceRing*>(this->fReadoutChipInterface)->WriteChipReg(cChip, "GlobalPulseConf", 0x2000);
                            static_cast<RD53InterfaceRing*>(this->fReadoutChipInterface)->sendCommand(cChip, RD53ACmd::GlobalPulse{cChip->getId(), 9});
                            trimOscCounts[ringOsc][vTrim]    = RD53ChipInterface->ReadChipReg(static_cast<RD53*>(cChip), "RING_OSC_" + std::to_string(ringOsc)) - 4096;
                            trimOscFrequency[ringOsc][vTrim] = trimOscCounts[ringOsc][vTrim] / (gloPulse[9] / 0.16);
                        }
                    }
                }
}

void RingOscillator::draw()
{
#ifdef __USE_ROOT__
    histos->fillRO(gloPulse, oscCounts, oscFrequency, trimOscCounts, trimOscFrequency, trimVoltage);
#endif
}
