#include "ArduinoKIRA.h"
#include "EthernetConnection.h"
#include "SerialConnection.h"

#include <iostream>
#include <string>

ArduinoKIRA::ArduinoKIRA(const pugi::xml_node configuration) : Arduino("ArduinoKIRA", configuration) { configure(); }
ArduinoKIRA::~ArduinoKIRA()
{
    if(fConnection != nullptr) delete fConnection;
}

void ArduinoKIRA::configure()
{
    std::cout << "Configuring ArduinoKIRA ..." << std::endl;
    if(std::string(fConfiguration.attribute("Connection").value()).compare("Serial") == 0)
    {
        fConnection = new SerialConnection(fConfiguration.attribute("Port").value(), 9600, true, false, false, "\r\n", "\n", 5);
    }
    else if(std::string(fConfiguration.attribute("Connection").value()).compare("Ethernet") == 0)
    {
        throw std::runtime_error("Shouldn't go via ethernet");
        fConnection = new SharedEthernetConnection(fConfiguration.attribute("IPAddress").value(), std::stoi(fConfiguration.attribute("Port").value()));
    }
    else
    {
        std::stringstream error;
        error << "ArduinoKIRA: Cannot implement connection type " << fConfiguration.attribute("Connection").value() << ".\n"
              << "Possible values are Serial or Ethernet";
        throw std::runtime_error(error.str());
    }

    /*
    for(pugi::xml_node channel = fConfiguration.child("Channel"); channel; channel = channel.next_sibling("Channel"))
    {
        std::string inUse = channel.attribute("InUse").value();
        if(inUse.compare("Yes") != 0 && inUse.compare("yes") != 0) continue;
        std::string id = channel.attribute("ID").value();
        std::cout << __PRETTY_FUNCTION__ << "Configuring channel: " << id << std::endl;
        PowerSupply::fChannelMap.emplace(id, new RohdeSchwarzChannel(fConnection, channel));
    }
    */
}

float ArduinoKIRA::getParameterFloat(std::string parName)
{
    std::string answer = fConnection->read(parName);
    // std::string answer = fConnection->read(parName);
    float result;
    sscanf(answer.c_str(), "%f", &result);
    return result;
}

int ArduinoKIRA::getParameterInt(std::string parName)
{
    std::string answer = fConnection->read(parName);
    int         result;
    sscanf(answer.c_str(), "%d", &result);
    return result;
}

bool ArduinoKIRA::getParameterBool(std::string parName)
{
    std::string answer = fConnection->read(parName);
    int         result;
    sscanf(answer.c_str(), "%d", &result);
    return result;
}

void ArduinoKIRA::sendCommand(std::string command) { fConnection->write(command); }

void ArduinoKIRA::setParameter(std::string parName, float value) { fConnection->write(parName + " " + std::to_string(value)); }

void ArduinoKIRA::setParameter(std::string parName, bool value) { fConnection->write(parName + " " + std::to_string(value)); }

void ArduinoKIRA::setParameter(std::string parName, int value) { fConnection->write(parName + " " + std::to_string(value)); }
