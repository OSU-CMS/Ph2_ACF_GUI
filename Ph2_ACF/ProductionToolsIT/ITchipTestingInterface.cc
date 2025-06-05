/*!
  \file                  ITchipTestingInterface.cc
  \brief                 User interface to ITchipTesting specific functions
  \author                Dominik Koukola
  \version               1.0
  \date                  28/06/21
  Support:               email to dominik.koukola@cern.ch
*/

#include "ITchipTestingInterface.h"

using namespace Ph2_ITchipTesting;

// void init()
// {
//     pugi::xml_document docSettings;
//     std::string cPowerSupply = "TestKeithley";

//     DeviceHandler ps_deviceHandler;
//     ps_deviceHandler.readSettings(configFile, docSettings);

//     PowerSupply* ps = ps_deviceHandler.getPowerSupply(cPowerSupply);
//     // PowerSupplyChannel* dPowerSupply = ps->getChannel("Front");

//     KeithleyChannel* dKeithley2410 = static_cast<KeithleyChannel*>(ps->getChannel("Front"));
// }

ITpowerSupplyChannelInterface::ITpowerSupplyChannelInterface(TCPClient* thePowerSupplyClient, std::string powerSupplyName, std::string channelID)
{
    this->fPowerSupplyClient = thePowerSupplyClient;
    this->powerSupplyName    = powerSupplyName;
    this->channelID          = channelID;

    if(fPowerSupplyClient == nullptr) LOG(ERROR) << BOLDRED << "Error parsed power supply TCP client is a nullpointer!" << RESET;
}

bool ITpowerSupplyChannelInterface::setupKeithley2410ChannelSense(uint16_t mode, float senseCompliance, bool turnOn) // turnOn=true
{
    // if(fPowerSupplyClient == nullptr)

    turnOff();

    std::string msg;
    switch(mode)
    {
    case CURRENTSENSE:
        msg = "K2410:setupIsense,PowerSupplyId:" + powerSupplyName + ",ChannelId:" + channelID + "," + "CurrCompl:" + std::to_string(senseCompliance);
        fPowerSupplyClient->sendAndReceivePacket(msg);

        // dKeithley2410->setVoltageMode();
        // dKeithley2410->setVoltage(0.0);
        // dKeithley2410->setCurrentCompliance(currentCompliance);
        break;

    case VOLTAGESENSE:
        msg = "K2410:setupVsense,PowerSupplyId:" + powerSupplyName + ",ChannelId:" + channelID + "," + "VoltCompl:" + std::to_string(senseCompliance);
        fPowerSupplyClient->sendAndReceivePacket(msg);

        // dKeithley2410->setCurrentMode();
        // dKeithley2410->setCurrent(0.0);
        // dKeithley2410->setParameter("Isrc_range", (float)1e-6);
        // dKeithley2410->setVoltageCompliance(voltageCompliance);
        break;

    default: LOG(ERROR) << BOLDRED << "Error invalid sense mode!" << RESET;
    }

    if(turnOn) this->turnOn();

    return true;
}

bool ITpowerSupplyChannelInterface::setupKeithley2410ChannelSource(uint16_t mode, float sourceValue, float senseCompliance, bool turnOn) // turnOn=true
{
    turnOff();

    std::string msg;
    switch(mode)
    {
    case CURRENTSOURCE:
        msg = "K2410:setupIsource,PowerSupplyId:" + powerSupplyName + ",ChannelId:" + channelID + ",Current:" + std::to_string(sourceValue) + ",VoltCompl:" + std::to_string(senseCompliance);
        fPowerSupplyClient->sendAndReceivePacket(msg);

        // dKeithley2410->setCurrentMode();
        // dKeithley2410->setCurrent(sourceValue);
        // dKeithley2410->setVoltageCompliance(senseCompliance);
        break;

    case VOLTAGESOURCE:

        msg = "K2410:setupVsource,PowerSupplyId:" + powerSupplyName + ",ChannelId:" + channelID + ",Voltage:" + std::to_string(sourceValue) + ",CurrCompl:" + std::to_string(senseCompliance);
        fPowerSupplyClient->sendAndReceivePacket(msg);

        // dKeithley2410->setVoltageMode();
        // dKeithley2410->setVoltage(sourceValue);
        // dKeithley2410->setCurrentCompliance(senseCompliance);
        break;

    default: LOG(ERROR) << BOLDRED << "Error invalid source mode!" << RESET;
    }

    if(turnOn) this->turnOn();

    return true;
}

bool ITpowerSupplyChannelInterface::setVoltage(float voltage)
{
    std::string msg = "SetVoltage,PowerSupplyId:" + powerSupplyName + ",ChannelId:" + channelID + ",Voltage:" + std::to_string(voltage);
    fPowerSupplyClient->sendAndReceivePacket(msg);
    return true;
}

bool ITpowerSupplyChannelInterface::setVoltageK2410(float voltage)
{
    std::string msg = "K2410:SetVoltage,PowerSupplyId:" + powerSupplyName + ",ChannelId:" + channelID + ",Voltage:" + std::to_string(voltage);
    fPowerSupplyClient->sendAndReceivePacket(msg);
    return true;
}

float ITpowerSupplyChannelInterface::getVoltage()
{
    std::string msg = "GetVoltage,PowerSupplyId:" + powerSupplyName + ",ChannelId:" + channelID;
    return std::stof(fPowerSupplyClient->sendAndReceivePacket(msg));
}

void ITpowerSupplyChannelInterface::turnOn()
{
    std::string msg = "TurnOn,PowerSupplyId:" + powerSupplyName + ",ChannelId:" + channelID;
    fPowerSupplyClient->sendAndReceivePacket(msg);
}

void ITpowerSupplyChannelInterface::turnOff()
{
    // fPowerSupplyClient->sendAndReceivePacket("TurnOff,PowerSupplyId:MyRohdeSchwarz,ChannelId:LV_Module1");
    std::string msg = "TurnOff,PowerSupplyId:" + powerSupplyName + ",ChannelId:" + channelID;
    fPowerSupplyClient->sendAndReceivePacket(msg);
}
