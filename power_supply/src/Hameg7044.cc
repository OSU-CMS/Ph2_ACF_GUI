#include "Hameg7044.h"
#include "EthernetConnection.h"
#include "SerialConnection.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>

#include <boost/algorithm/string.hpp>

Hameg7044::Hameg7044(const pugi::xml_node configuration) : PowerSupply("Hameg7044", configuration) { configure(); }
Hameg7044::~Hameg7044()
{
    if(fConnection != nullptr) delete fConnection;
}

void Hameg7044::configure()
{
    std::cout << "Configuring Hameg7044 ..." << std::endl;
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
        std::stringstream error;
        error << "Hameg7044 Cannot be read out via Ethernet!";
        throw std::runtime_error(error.str());
    }
    else
    {
        std::stringstream error;
        error << "Hameg7044: Cannot implement connection type " << fConfiguration.attribute("Connection").value() << ".\n"
              << "Possible values are Serial or Ethernet";
        throw std::runtime_error(error.str());
    }

    for(pugi::xml_node channel = fConfiguration.child("Channel"); channel; channel = channel.next_sibling("Channel"))
    {
        std::string inUse = channel.attribute("InUse").value();
        if(inUse.compare("Yes") != 0 && inUse.compare("yes") != 0) continue;
        std::string id = channel.attribute("ID").value();
        std::cout << __PRETTY_FUNCTION__ << "Configuring channel: " << id << std::endl;
        PowerSupply::fChannelMap.emplace(id, new Hameg7044Channel(fConnection, channel));
    }
}

Hameg7044Channel::Hameg7044Channel(Connection* connection, const pugi::xml_node configuration) : PowerSupplyChannel(configuration), fConnection(connection)
{
    fChannelCommand = std::string("SEL ") + fConfiguration.attribute("Channel").value();
}
Hameg7044Channel::~Hameg7044Channel() {}

/*!
************************************************
 * Sends write command to connection.
 \param command Command to be send.
************************************************
*/
void Hameg7044Channel::write(std::string command)
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
std::string Hameg7044Channel::read(std::string command)
{
    fConnection->write(fChannelCommand);
    std::string answer = fConnection->read(command);
    return answer;
}

std::vector<std::string> Hameg7044Channel::getStatus()
{
    std::vector<std::string> hamegStatus;
    std::string              readout;
    fConnection->write(fChannelCommand);
    do {
        readout = fConnection->read("READ");
        // std::cout << readout << std::endl;
        if(readout == "") { break; }
    } while(readout.length() != 86);

    // std::cout << readout << std::endl;

    // remove whitespace
    readout.erase(std::remove_if(readout.begin(), readout.end(), ::isspace), readout.end());
    // Split the readout in voltage, current and channel status
    std::vector<std::string> lines;
    boost::split(lines, readout, boost::is_any_of(";"));

    std::vector<std::string> voltages;
    boost::split(voltages, lines[0], boost::is_any_of("V"));

    std::vector<std::string> currents;
    boost::split(currents, lines[1], boost::is_any_of("A"));

    std::vector<std::string> channelStati;
    boost::split(channelStati, lines[2], boost::is_any_of("1234"));

    hamegStatus.insert(hamegStatus.end(), voltages.begin(), voltages.end() - 1);
    hamegStatus.insert(hamegStatus.end(), currents.begin(), currents.end() - 1);
    hamegStatus.insert(hamegStatus.end(), channelStati.begin(), channelStati.end() - 1);

    return hamegStatus;
}

void Hameg7044Channel::turnOn()
{
    write("ON");
    write("ENABLE OUTPUT");
    std::cout << "Turn on channel " << fConfiguration.attribute("Channel").value() << " output." << std::endl;
}

void Hameg7044Channel::turnOff()
{
    write("OFF");
    std::cout << "Turn off channel " << fConfiguration.attribute("Channel").value() << " output." << std::endl;
}

bool Hameg7044Channel::isOn()
{
    std::vector<std::string> values = getStatus();
    std::string              answer = values[std::stoi(fConfiguration.attribute("Channel").value()) + 7];

    return (answer.find("OFF") == std::string::npos);
}

void Hameg7044Channel::setVoltage(float voltage) { write("SET " + std::to_string(voltage) + " V"); }

void Hameg7044Channel::setCurrent(float current) { setCurrentCompliance(current); }

float Hameg7044Channel::getOutputVoltage()
{
    std::vector<std::string> values = getStatus();
    std::string              answer = values[std::stoi(fConfiguration.attribute("Channel").value()) - 1];

    float result;
    sscanf(answer.c_str(), "%f", &result);
    return result;
}

float Hameg7044Channel::getSetVoltage()
{
    std::stringstream error;
    error << "Hameg7044 getSetVoltage() not defined, aborting...";
    throw std::runtime_error(error.str());
}

float Hameg7044Channel::getCurrent()
{
    std::vector<std::string> values = getStatus();
    std::string              answer = values[std::stoi(fConfiguration.attribute("Channel").value()) + 3];

    float result;
    sscanf(answer.c_str(), "%f", &result);
    return result;
}

void Hameg7044Channel::setVoltageCompliance(float voltage) { setVoltage(voltage); }

void Hameg7044Channel::setCurrentCompliance(float current) { write("SET " + std::to_string(current) + " A"); }

float Hameg7044Channel::getVoltageCompliance() { return getOutputVoltage(); }

float Hameg7044Channel::getCurrentCompliance()
{
    std::stringstream error;
    error << "Hameg7044 getCurrentCompliance method not implemented, "
             "aborting...";
    throw std::runtime_error(error.str());
}

void Hameg7044Channel::setOverVoltageProtection(float maxVoltage)
{
    std::stringstream error;
    error << "Hameg7044 setOverVoltageProtection method not implemented, "
             "aborting...";
    throw std::runtime_error(error.str());
}

void Hameg7044Channel::setOverCurrentProtection(float maxCurrent)
{
    std::stringstream error;
    error << "Hameg7044 setOverCurrentProtection method not implemented, "
             "aborting...";
    throw std::runtime_error(error.str());
}

float Hameg7044Channel::getOverVoltageProtection()
{
    std::stringstream error;
    error << "Hameg7044 getOverVoltageProtection method not implemented, "
             "aborting...";
    throw std::runtime_error(error.str());
}

float Hameg7044Channel::getOverCurrentProtection()
{
    std::stringstream error;
    error << "Hameg7044 getOverCurrentProtection method not implemented, "
             "aborting...";
    throw std::runtime_error(error.str());
}

void Hameg7044Channel::setParameter(std::string parName, float value) { write(parName + " " + std::to_string(value)); }

void Hameg7044Channel::setParameter(std::string parName, bool value) { write(parName + " " + std::to_string(value)); }

void Hameg7044Channel::setParameter(std::string parName, int value) { write(parName + " " + std::to_string(value)); }

float Hameg7044Channel::getParameterFloat(std::string parName)
{
    std::string answer = read(parName);
    float       result;
    sscanf(answer.c_str(), "%f", &result);
    return result;
}

int Hameg7044Channel::getParameterInt(std::string parName)
{
    std::string answer = read(parName);
    int         result;
    sscanf(answer.c_str(), "%d", &result);
    return result;
}

bool Hameg7044Channel::getParameterBool(std::string parName)
{
    std::string answer = read(parName);
    int         result;
    sscanf(answer.c_str(), "%d", &result);
    return result;
}
