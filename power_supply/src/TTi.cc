/*!
 * \authors Mattia Lizzo <mattia.lizzo@cern.ch>, INFN-Firenze
 * \authors Antonio Cassese <antonio.cassese@cern.ch>, INFN-Firenze
 * \date Sep 2 2019
 */

#include "TTi.h"
#include "EthernetConnection.h"
#include "SerialConnection.h"
#include "TTiErrors.h"

#include <chrono>
#include <thread>

/*!
************************************************
 * Class constructor.
 \param configuration xml configuration node.
************************************************
*/
TTi::TTi(const pugi::xml_node configuration) : PowerSupply("TTi", configuration) { configure(); }

/*!
************************************************
* Class destructor.
************************************************
*/
TTi::~TTi()
{
    if(fConnection != nullptr) delete fConnection;
}

/*!
************************************************
* Prints info from IDN?
************************************************
*/
void TTi::printInfo()
{
    std::string idnString = fConnection->read("*IDN?");
    idnString.erase(std::remove(idnString.begin(), idnString.end(), '\n'), idnString.end());
    idnString.erase(std::remove(idnString.begin(), idnString.end(), '\r'), idnString.end());
    std::cout << "\t\t**\t" << idnString << "\t**" << std::endl;
}

/*!
************************************************
* Configures the parameters based on xml
* configuration file.
************************************************
*/
void TTi::configure()
{
    std::cout << "Configuring TTi with ";
    std::string connectionType = fConfiguration.attribute("Connection").as_string();
    int         timeout        = fConfiguration.attribute("Timeout").as_int();
    fSeriesName                = fConfiguration.attribute("Series").as_string();
    std::cout << connectionType << " connection ..." << std::endl;
    if(fSeriesName != "TSX" && fSeriesName != "PL" && fSeriesName != "MX")
    {
        std::stringstream error;
        error << "TTi configuration: the series named " << fSeriesName << " has no TTi code implemented, aborting...";
        throw std::runtime_error(error.str());
    }
    if(connectionType == "Ethernet")
    {
        std::string ipAddress = fConfiguration.attribute("IPAddress").as_string();
        int         port      = fConfiguration.attribute("Port").as_int();
        fConnection           = new SharedEthernetConnection(ipAddress, port, timeout * 1e3);
    }
    else if(connectionType == "Serial")
    {
        int         baudRate    = fConfiguration.attribute("BaudRate").as_int();
        std::string port        = fConfiguration.attribute("Port").as_string();
        bool        flowControl = fConfiguration.attribute("FlowControl").as_bool();
        bool        parity      = fConfiguration.attribute("Parity").as_bool();
        bool        removeEcho  = fConfiguration.attribute("RemoveEcho").as_bool();
        std::string terminator  = fConfiguration.attribute("Terminator").as_string();
        std::string suffix      = fConfiguration.attribute("Suffix").as_string();
        terminator              = PowerSupply::convertToLFCR(terminator);
        suffix                  = PowerSupply::convertToLFCR(suffix);
        fConnection             = new SerialConnection(port, baudRate, flowControl, parity, removeEcho, terminator, suffix, timeout);
    }
    else
    {
        std::stringstream error;
        error << "TTi configuration: no connection " << connectionType << " available for TTi code, aborting...";
        throw std::runtime_error(error.str());
    }
    if(!isOpen())
    {
        std::stringstream error;
        error << "TTi connection " << connectionType
              << " not open, channel(s) will not be configured, aborting "
                 "execution...";
        throw std::runtime_error(error.str());
    }
    printInfo();
    // std::cout << "**\t" << fConnection->read("*IDN?") << "\t**" << std::endl;
    for(pugi::xml_node channel = fConfiguration.child("Channel"); channel; channel = channel.next_sibling("Channel"))
    {
        std::string inUse = channel.attribute("InUse").value();
        if(inUse != "Yes" && inUse != "yes") continue;
        std::string id = channel.attribute("ID").value();
        PowerSupply::fChannelMap.emplace(id, new TTiChannel(fConnection, channel, fSeriesName));
    }
}

/*!
************************************************
* Resets the instrument as follows. The output
* is set to minimum voltage, minimum current,
* maximum OVP, meter damping off and output off.
* No other action taken.
************************************************
*/
void TTi::reset() { fConnection->write("*RST"); }

/*!
************************************************
 * Checks if the connection with the instrument
 * is open.
 \return True if the connection is open, false
 otherwise.
************************************************
*/
bool TTi::isOpen() { return fConnection->isOpen(); }

/*******************************************************************/
/***************************** CHANNEL *****************************/
/*******************************************************************/

/*!
************************************************
 \brief Class constructor.
 * Class constructor, also clears registers
 * EER, QER and ESR.
 \param configuration xml configuration node.
************************************************
*/
TTiChannel::TTiChannel(Connection* connection, const pugi::xml_node configuration, std::string seriesName)
    : PowerSupplyChannel(configuration), fChannel(atoi(configuration.attribute("Channel").value())), fConnection(connection), fSeriesName(seriesName)
{
    fChannelName = std::to_string(fChannel);
    std::cout << "Inizializing channel " << this->getID() << " number " << fChannel << std::endl;
    try
    {
        fIsOn = false;
        statusEER();
        statusQER();
        statusESR();
    }
    catch(...)
    {
    }
}

/*!
************************************************
* Class destructor.
************************************************
*/
TTiChannel::~TTiChannel() {}

/*!
************************************************
 * Sends write command to connection.
 \param command Command to be send.
************************************************
*/
void TTiChannel::write(std::string command, int timeout = 0)
{
    try
    {
        fConnection->write(command);
        std::this_thread::sleep_for(std::chrono::milliseconds(timeout)); // read voltage continuously
        statusESR();
    }
    catch(...)
    {
        std::cerr << "Error in command \"" << command << "\"" << std::endl;
        throw;
    }
}

/*!
************************************************
 * Sends read command to connection.
 \param command Command to be send.
************************************************
*/
std::string TTiChannel::read(std::string command)
{
    std::string answer = fConnection->read(command);
    return answer;
}

/*!
************************************************
 * Format commands depending on series.
 \param command Command to be formatted.
 \return Formatted command.
************************************************
*/
std::string TTiChannel::formatCommand(std::string command)
{
    std::string newCommand;
    if(fSeriesName == "TSX")
        newCommand = command;
    else if(fSeriesName == "PL" || fSeriesName == "MX")
        newCommand = command + fChannelName;
    else
    {
        std::stringstream error;
        error << "TTi " + command + ": command not implemented for " << fSeriesName << ", aborting ...";
        throw std::runtime_error(error.str());
    }
    return newCommand;
}

/*!
************************************************
* Set channel output on.
************************************************
*/
void TTiChannel::turnOn(void)
{
    std::string command = formatCommand("OP");
    write(command + " 1");
    fIsOn = true;
}

/*!
************************************************
* Set channel output off.
************************************************
*/
void TTiChannel::turnOff(void)
{
    std::string command = formatCommand("OP");
    write(command + " 0");
    fIsOn = false;
}

/*!
************************************************
 * Checks if the channel output is on or not.
 \return True if channel output is on, false
 otherwise
************************************************
*/
bool TTiChannel::isOn(void)
{
    if(fSeriesName == "TSX")
        return fIsOn;
    else if(fSeriesName == "PL" || fSeriesName == "MX")
    {
        std::string command = formatCommand("OP");
        std::string answer  = read(command + "?");
        return std::stoi(answer);
    }
    else
    {
        std::stringstream error;
        error << "TTi isOn: command not implemented for " << fSeriesName << ", aborting ...";
        throw std::runtime_error(error.str());
    }
}

/*!
************************************************
 * Set channel output voltage value in voltage.
 \param voltage Voltage value in volts.
************************************************
*/
void TTiChannel::setVoltage(float voltage)
{
    std::string command = formatCommand("V");
    if(!isOn())
    {
        write(command + " " + std::to_string(voltage));
        fSetVoltage = voltage;
        return;
    }
    try
    {
        write(command + "V " + std::to_string(voltage), 5000);
        fSetVoltage = voltage;
    }
    catch(...)
    {
        std::cerr << "Failed in setting " << voltage << "V" << std::endl << "Actual read " << getOutputVoltage() << "V." << std::endl;
        throw;
    }
}

/*!
************************************************
 * Set channel output current limit value in
 * amperes.
 \param current Current value in amperes.
************************************************
*/
void TTiChannel::setCurrent(float current) { setCurrentCompliance(current); }

/*!
************************************************
 * Set channel output voltage value in voltage.
 \param voltage Voltage value in volts.
************************************************
*/
void TTiChannel::setVoltageCompliance(float voltage) { setVoltage(voltage); }

/*!
************************************************
 * Set channel output current limit value in
 * amperes.
 \param current Current value in amperes.
************************************************
*/
void TTiChannel::setCurrentCompliance(float current)
{
    std::string command = formatCommand("I");
    write(command + " " + std::to_string(current));
}

/*!
************************************************
 * Set channel output over voltage protection
 * trip point in voltage.
 \param voltage Trip point in volts.
************************************************
*/
void TTiChannel::setOverVoltageProtection(float voltage)
{
    std::string command = formatCommand("OVP");
    write(command + " " + std::to_string(voltage));
}

/*!
************************************************
 * Set channel output over current protection
 * trip point in amperes.
 \param current Trip point in amperes.
************************************************
*/
void TTiChannel::setOverCurrentProtection(float current)
{
    if(fSeriesName == "TSX")
    {
        std::stringstream error;
        error << "TTi setOverCurrentProtection command does not exist for " << fSeriesName << ", aborting ...";
        throw std::runtime_error(error.str());
    }
    else if(fSeriesName == "PL" || fSeriesName == "MX")
    {
        std::string command = formatCommand("OCP");
        write(command + " " + std::to_string(current));
    }
    else
    {
        std::stringstream error;
        error << "TTi setOverCurrentProtection: command not implemented for " << fSeriesName << ", aborting ...";
        throw std::runtime_error(error.str());
    }
}

/*!
************************************************
 * Returns the channel output readback voltage.
 \return Readback voltage in voltage.
************************************************
*/
float TTiChannel::getOutputVoltage(void)
{
    std::string command = formatCommand("V");
    std::string answer  = read(command + "O?");
    float       result;
    sscanf(answer.c_str(), "%f", &result);
    return result;
}

/*!
************************************************
 * Returns the channel output readback voltage.
 \return Readback voltage in voltage.
************************************************
*/
float TTiChannel::getSetVoltage(void) { return fSetVoltage; }

/*!
************************************************
 * Returns the channel output readback current.
 \return Readback current in amperes.
************************************************
*/
float TTiChannel::getCurrent(void) { return getCurrentCompliance(); }

/*!
************************************************
 * Returns the channel output readback voltage.
 \return Readback voltage in voltage.
************************************************
*/
float TTiChannel::getVoltageCompliance(void) { return getOutputVoltage(); }

/*!
************************************************
 * Returns the channel output readback current.
 \return Readback current in amperes.
************************************************
*/
float TTiChannel::getCurrentCompliance(void)
{
    std::string command = formatCommand("I");
    std::string answer  = read(command + "O?");
    float       result;
    sscanf(answer.c_str(), "%f", &result);
    return result;
}

/*!
************************************************
 * Returns the voltage trip setting for the
 * channel output in voltage.
 \return Voltage trip setting in voltage.
************************************************
*/
float TTiChannel::getOverVoltageProtection(void)
{
    std::string command = formatCommand("OVP");
    std::string answer  = read(command + "?");
    float       result;
    sscanf(answer.c_str(), "%*s %f", &result);
    return result;
}

/*!
************************************************
 * Returns the current trip setting for the
 * channel output in amperes.
 \return Current trip setting in amperes.
************************************************
*/
float TTiChannel::getOverCurrentProtection(void)
{
    if(fSeriesName == "TSX")
    {
        std::stringstream error;
        error << "TTi getOverCurrentProtection command does not exist for " << fSeriesName << ", aborting ...";
        throw std::runtime_error(error.str());
    }
    else if(fSeriesName == "PL" || fSeriesName == "MX")
    {
        std::string command = formatCommand("OCP");
        std::string answer  = read(command + "?");
        float       result;
        sscanf(answer.c_str(), "%*s %f", &result);
        return result;
    }
    else
    {
        std::stringstream error;
        error << "TTi getOverCurrentProtection: command not implemented for " << fSeriesName << ", aborting ...";
        throw std::runtime_error(error.str());
    }
}

/*!
************************************************
 * Returns the channel parameter setting. </br>
 * Here is a list of possible values, please
 * refer to specific model manual:
 * <ul>
 * <li> **V**	    - Channel output voltage set.
 * <li> **I**	    - Channel output current set.
 * <li> **DELTAV**  - Channel output voltage step size.
 * <li> **DELTAI**  - Channel output current step size.
 * </ul>
 \brief Returns the channel float parameter
 setting.
 \param parName Name of the float parameter
 to be read.
 \return Returns the parameter read.
************************************************
*/
float TTiChannel::getParameterFloat(std::string parName)
{
    std::string command = formatCommand(parName);
    std::string answer  = read(command + "?");
    float       result;
    sscanf(answer.c_str(), "%*s %f", &result); // Check
    return result;
}

/*!
************************************************
 * Returns the channel parameter setting. </br>
 * Please refer to specific model manual for
 * a complete list of possible values.
 \brief Returns the channel unsigned parameter
 setting.
 \param parName Name of the unsigned parameter
 to be read.
 \return Returns the parameter read.
************************************************
*/
int TTiChannel::getParameterInt(std::string parName)
{
    std::string command = formatCommand(parName);
    std::string answer  = read(command + "?");
    int         result;
    sscanf(answer.c_str(), "%d", &result); // Check
    return result;
}

/*!
************************************************
 * This method is doing nothing but raising
 * exeption.
 \param parName Name of the parameter to be
 read.
************************************************
*/
bool TTiChannel::getParameterBool(std::string parName)
{
    std::stringstream error;
    error << "Sorry, there are no bool parameter for TTi power supply to be "
             "read, skipping the command ...";
    throw std::runtime_error(error.str());
}

/*!
************************************************
 * Set channel parameter setting. </br>
 * Please refer to specific model manual for
 * a complete list of possible settings and
 * values.
 \brief Set a floating parameter.
 \param parName Name of the floating parameter
 to be set.
 \param value Floating value to set.
************************************************
*/
void TTiChannel::setParameter(std::string parName, float value)
{
    std::string command = formatCommand(parName);
    write(command + " " + std::to_string(value));
}

/*!
************************************************
 * This method is doing nothing but raising
 * exeption.
 \param parName Name of the parameter to be
 set.
 \param value Value to set.
************************************************
*/
void TTiChannel::setParameter(std::string parName, bool value)
{
    std::stringstream error;
    error << "Sorry, there are no bool parameter for TTi power supply to be set, "
             "skipping the command ...";
    throw std::runtime_error(error.str());
}

/*!
************************************************
 * This method is doing nothing but raising
 * exeption.
 \param parName Name of the parameter to be
 set.
 \param value Value to set.
************************************************
*/
void TTiChannel::setParameter(std::string parName, int value)
{
    std::stringstream error;
    error << "Sorry, there are no integer parameter for TTi power supply to be "
             "set, skipping the command ...";
    throw std::runtime_error(error.str());
}

/*!
************************************************
* Checks the value in the Execution Error
* Register, if different from zero throws an
* exeption. The register is then
* cleared.
************************************************
*/
void TTiChannel::statusEER()
{
    std::string answer = fConnection->read("EER?");
    if(!std::stoi(answer)) return;

    std::string instrumentError;
    if(fSeriesName == "TSX")
        instrumentError = convertErrorTSX(std::stoi(answer));
    else if(fSeriesName == "PL")
        instrumentError = convertErrorPL(std::stoi(answer));
    else if(fSeriesName == "MX")
        instrumentError = convertErrorMX(std::stoi(answer));
    else
        instrumentError = "Instrument error not defined for " + fSeriesName + ".";
    std::stringstream error;
    error << "TTi EER (Execution Error Register) value: " << answer << "Corresponding to : " << instrumentError << " Check instrument manuals for more details. Aborting ...";
    throw std::runtime_error(error.str());
}

/*!
************************************************
* Checks the value in the Query Error
* Register, if different from zero throws an
* exeption. The register is then
* cleared.
************************************************
*/
void TTiChannel::statusQER()
{
    std::string answer = fConnection->read("QER?");
    if(!std::stoi(answer)) return;

    std::stringstream error;
    error << "TTi QER (Query Error Register) value: " << answer << ", check instrument manuals for more details. Aborting ...";
    throw std::runtime_error(error.str());
}

/*!
************************************************
* Checks the value in the Standard Event Status
* Register. The register is then cleared.
************************************************
*/
void TTiChannel::statusESR()
{
    std::string  answer    = fConnection->read("*ESR?");
    unsigned int errorCode = stoi(answer);
    if(errorCode & POWERON) std::cout << "Power on" << std::endl;
    if(errorCode & NOTUSED) std::cout << "Bit not used" << std::endl;
    if(errorCode & COMMANDERROR)
    {
        std::stringstream error;
        error << "TTi ESR (Standard Event Status Register) value: " << errorCode << ", Command Error, bad syntax. Aborting ...";
        throw std::runtime_error(error.str());
    }
    if(errorCode & EXECUTIONERROR)
    {
        std::cout << "TTi ESR (Standard Event Status Register) value: " << errorCode << ", Execution Error." << std::endl;
        statusEER();
    }
    if(errorCode & TIMEOUT)
    {
        std::stringstream error;
        error << "TTi ESR (Standard Event Status Register) value: " << errorCode << ", Operation Time-Out Error.";
        throw std::runtime_error(error.str());
    }
    if(errorCode & QUERYERROR)
    {
        std::cout << "TTi ESR (Standard Event Status Register) value: " << errorCode << ", Query Error." << std::endl;
        statusQER();
    }
    if(errorCode & OPERATIONCOMPLETE) std::cout << "Operation complete" << std::endl;
}
