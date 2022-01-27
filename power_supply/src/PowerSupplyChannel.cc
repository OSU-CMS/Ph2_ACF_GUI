#include "PowerSupplyChannel.h"
#include <iostream>
#include <unistd.h>

bool PowerSupplyChannel::rampToVoltage(float pTargetVoltage, uint32_t pSteps, uint32_t pTimeSpan)
{
    // std::cout <<"Ramp to target voltage: " << pTargetVoltage <<"\tSteps: " << pSteps <<"\tTiemspan (ms): " << pTimeSpan <<std::endl;

    float setVoltage = this->getOutputVoltage();
    // std::cout <<"Current voltage:" << setVoltage <<std::endl;

    // calculatae the stepsize, invert sign for correct direction
    float voltageStep = -((setVoltage - pTargetVoltage) / pSteps);
    // std::cout <<"Voltagestep: " << voltageStep <<std::endl;

    float pause = (int)pTimeSpan / pSteps;
    // std::cout <<"Pause: " << pause <<std::endl;

    bool channelIsOn = isOn();

    if(channelIsOn)
    {
        if(abs(setVoltage - pTargetVoltage) >= 50)
        {
            for(uint32_t step = 0; step < pSteps; step++)
            {
                float newVoltage = setVoltage + voltageStep;
                std::cout << "Step: " << step << "\tNew Voltage: " << newVoltage << std::endl;

                // Set new voltage on the power supply channel
                this->setVoltage(newVoltage);

                // and remember new value, could be replaced by a read function
                setVoltage = newVoltage;

                // wait - pause in ms
                usleep(1E3 * pause);
            }
            return true;
        }
        else
        {
            std::cout << " Dont ramp, just set" << std::endl;
            this->setVoltage(pTargetVoltage);
            return true;
        }
    }
    else
    {
        if(abs(pTargetVoltage) > 0)
        {
            std::cout << "Channel " << this->getID() << " is not on, cannot ramp..." << std::endl;
            return false;
        }
        else // Allow to set the voltage to 0 although the channel is not enabled, avoids having unwanted voltages on channel when turning it on
        {
            std::cout << " Set to 0V" << std::endl;
            this->setVoltage(pTargetVoltage);
            return true;
        }
    }
}
