/*!
  \file                  ITchipTestingInterface.cc
  \brief                 User interface to ITchipTesting specific functions
  \author                Dominik Koukola
  \version               1.0
  \date                  28/06/21
  Support:               email to dominik.koukola@cern.ch
*/

#ifndef ITchipTestingInterface_H
#define ITchipTestingInterface_H

#include "NetworkUtils/TCPClient.h"
#include "Utils/ConsoleColor.h"
#include "Utils/easylogging++.h"

#include <string>

namespace Ph2_ITchipTesting
{
#define CURRENTSENSE 0
#define CURRENTSOURCE 1
#define VOLTAGESENSE 2
#define VOLTAGESOURCE 3

class ITpowerSupplyChannelInterface
{
  private:
    TCPClient*  fPowerSupplyClient;
    std::string powerSupplyName;
    std::string channelID;

  public:
    ITpowerSupplyChannelInterface(TCPClient* thePowerSupplyClient, std::string powerSupplyName, std::string channelID);

    void setPowerSupplyClient(TCPClient* thePowerSupplyClient) { fPowerSupplyClient = thePowerSupplyClient; };

    bool setupKeithley2410ChannelSense(uint16_t mode, float senseCompliance, bool turnOn = true);
    bool setupKeithley2410ChannelSource(uint16_t mode, float sourceValue, float senseCompliance, bool turnOn = true);

    bool setVoltageK2410(float voltage);

    bool  setVoltage(float voltage);
    float getVoltage();

    void turnOff();
    void turnOn();
};

} // namespace Ph2_ITchipTesting

#endif
