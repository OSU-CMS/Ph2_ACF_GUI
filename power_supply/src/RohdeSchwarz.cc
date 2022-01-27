#include "RohdeSchwarz.h"
#include "EthernetConnection.h"
#include "SerialConnection.h"

#include <iostream>
#include <string>

RohdeSchwarz::RohdeSchwarz(const pugi::xml_node configuration) : PowerSupply("RohdeSchwarz", configuration) { configure(); }
RohdeSchwarz::~RohdeSchwarz()
{
    if(fConnection != nullptr) delete fConnection;
}

void RohdeSchwarz::configure()
{
    std::cout << "Configuring RohdeSchwarz ..." << std::endl;
    std::string connectionType = fConfiguration.attribute("Connection").as_string();
    int         timeout        = fConfiguration.attribute("Timeout").as_int();
    std::cout << connectionType << " connection ..." << std::endl;

    if(std::string(fConfiguration.attribute("Connection").value()).compare("Serial") == 0)
    {
        std::string port        = fConfiguration.attribute("Port").as_string();
        int         baudRate    = fConfiguration.attribute("BaudRate").as_int();
        bool        flowControl = fConfiguration.attribute("FlowControl").as_bool();
        bool        parity      = fConfiguration.attribute("Parity").as_bool();
        bool        removeEcho  = fConfiguration.attribute("RemoveEcho").as_bool();
        std::string terminator  = fConfiguration.attribute("Terminator").as_string();
        std::string suffix      = fConfiguration.attribute("Suffix").as_string();
        terminator              = PowerSupply::convertToLFCR(terminator);
        suffix                  = PowerSupply::convertToLFCR(suffix);
        fConnection             = new SerialConnection(port, baudRate, flowControl, parity, removeEcho, terminator, suffix, timeout);
    }
    else if(std::string(fConfiguration.attribute("Connection").value()).compare("Ethernet") == 0)
    {
        fConnection = new SharedEthernetConnection(fConfiguration.attribute("IPAddress").value(), std::stoi(fConfiguration.attribute("Port").value()));
    }
    else
    {
        std::stringstream error;
        error << "RohdeSchwarz: Cannot implement connection type " << fConfiguration.attribute("Connection").value() << ".\n"
              << "Possible values are Serial or Ethernet";
        throw std::runtime_error(error.str());
    }

    for(pugi::xml_node channel = fConfiguration.child("Channel"); channel; channel = channel.next_sibling("Channel"))
    {
        std::string inUse = channel.attribute("InUse").value();
        if(inUse.compare("Yes") != 0 && inUse.compare("yes") != 0) continue;
        std::string id = channel.attribute("ID").value();
        std::cout << __PRETTY_FUNCTION__ << "Configuring channel: " << id << std::endl;
        PowerSupply::fChannelMap.emplace(id, new RohdeSchwarzChannel(fConnection, channel));
    }
}

RohdeSchwarzChannel::RohdeSchwarzChannel(Connection* connection, const pugi::xml_node configuration) : PowerSupplyChannel(configuration), fConnection(connection)
{
    fChannelCommand = std::string("INST:NSEL ") + fConfiguration.attribute("Channel").value();
}
RohdeSchwarzChannel::~RohdeSchwarzChannel() {}

/*!
************************************************
 * Sends write command to connection.
 \param command Command to be send.
************************************************
*/
void RohdeSchwarzChannel::write(std::string command)
{
    fConnection->write(fChannelCommand);
    fConnection->write(command);
}

/*!
************************************************
 * Sends read command to connection.
 \param command Command to be send.
************************************************
*/
std::string RohdeSchwarzChannel::read(std::string command)
{
    fConnection->write(fChannelCommand);
    std::string answer = fConnection->read(command);
    return answer;
}

void RohdeSchwarzChannel::turnOn()
{
    write("OUTP ON");
    // std::cout << "Turn on channel " << fConfiguration.attribute("Channel").value() << " output." << std::endl;
}

void RohdeSchwarzChannel::turnOff()
{
    write("OUTP OFF");
    // std::cout << "Turn off channel " << fConfiguration.attribute("Channel").value() << " output." << std::endl;
}

bool RohdeSchwarzChannel::isOn()
{
    std::string answer = read("OUTP?");
    int         result;
    sscanf(answer.c_str(), "%d", &result);
    return result;
}

void RohdeSchwarzChannel::setVoltage(float voltage) { write("VOLT " + std::to_string(voltage)); }

void RohdeSchwarzChannel::setCurrent(float current) { setCurrentCompliance(current); }

float RohdeSchwarzChannel::getOutputVoltage()
{
    std::string answer = read("MEAS:VOLT?");
    float       result;
    sscanf(answer.c_str(), "%f", &result);
    return result;
}

float RohdeSchwarzChannel::getSetVoltage() { return getVoltageCompliance(); }

float RohdeSchwarzChannel::getCurrent()
{
    std::string answer = read("MEAS:CURR?");
    float       result;
    sscanf(answer.c_str(), "%f", &result);
    return result;
}

void RohdeSchwarzChannel::setVoltageCompliance(float voltage) { setVoltage(voltage); }

void RohdeSchwarzChannel::setCurrentCompliance(float current) { write("CURR " + std::to_string(current)); }

float RohdeSchwarzChannel::getVoltageCompliance()
{
    std::string answer = read("VOLT?");
    float       result;
    sscanf(answer.c_str(), "%f", &result);
    return result;
}

float RohdeSchwarzChannel::getCurrentCompliance()
{
    std::string answer = read("CURR?");
    float       result;
    sscanf(answer.c_str(), "%f", &result);
    return result;
}

void RohdeSchwarzChannel::setOverVoltageProtection(float maxVoltage) { write("VOLT:PROT:LEV " + std::to_string(maxVoltage)); }

void RohdeSchwarzChannel::setOverCurrentProtection(float maxCurrent)
{
    std::stringstream error;
    error << "Rohde&Schwarz setOverCurrentProtection method not implemented, "
             "aborting...";
    throw std::runtime_error(error.str());
}

float RohdeSchwarzChannel::getOverVoltageProtection()
{
    std::string answer = read("VOLT:PROT?");
    float       result;
    sscanf(answer.c_str(), "%f", &result);
    return result;
}

float RohdeSchwarzChannel::getOverCurrentProtection()
{
    std::stringstream error;
    error << "Rohde&Schwarz getOverCurrentProtection method not implemented, "
             "aborting...";
    throw std::runtime_error(error.str());
}

void RohdeSchwarzChannel::setParameter(std::string parName, float value) { write(parName + " " + std::to_string(value)); }

void RohdeSchwarzChannel::setParameter(std::string parName, bool value) { write(parName + " " + std::to_string(value)); }

void RohdeSchwarzChannel::setParameter(std::string parName, int value) { write(parName + " " + std::to_string(value)); }

float RohdeSchwarzChannel::getParameterFloat(std::string parName)
{
    std::string answer = read(parName);
    float       result;
    sscanf(answer.c_str(), "%f", &result);
    return result;
}

int RohdeSchwarzChannel::getParameterInt(std::string parName)
{
    std::string answer = read(parName);
    int         result;
    sscanf(answer.c_str(), "%d", &result);
    return result;
}

bool RohdeSchwarzChannel::getParameterBool(std::string parName)
{
    std::string answer = read(parName);
    int         result;
    sscanf(answer.c_str(), "%d", &result);
    return result;
}
