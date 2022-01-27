/*!
 * \author Stefan Maier <s.maier@kit.edu>, ETP-Karlsruhe
 * \date Apr 30 2020
 */
#ifndef ARDUINOKIRA_H
#define ARDUINOKIRA_H

#include "Arduino.h"
//#include "PowerSupplyChannel.h"

class Connection;

class ArduinoKIRA : public Arduino
{
  public:
    ArduinoKIRA(const pugi::xml_node configuration);
    virtual ~ArduinoKIRA();
    void  configure();
    float getParameterFloat(std::string parName);
    int   getParameterInt(std::string parName);
    bool  getParameterBool(std::string parName);

    void sendCommand(std::string command);
    void setParameter(std::string parName, float value);
    void setParameter(std::string parName, int value);
    void setParameter(std::string parName, bool value);
    // bool turnOn();
    // bool turnOff();

  private:
    Connection* fConnection;
};

#endif
