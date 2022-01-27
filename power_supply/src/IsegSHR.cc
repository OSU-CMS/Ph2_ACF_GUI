/*!
 * \author Massimiliano Marchisone <mmarchis.@cern.ch>
 * \date Jul 30 2020
 */

#include "IsegSHR.h"
#include "EthernetConnection.h"
#include "SerialConnection.h"
#include <iostream>
using namespace ::std;
#include <string>
using namespace ::std;

/*!
************************************************
 * Class constructor.
 \param configuration xml configuration node.
************************************************
*/
IsegSHR::IsegSHR(const pugi::xml_node configuration) : PowerSupply("IsegSHR", configuration) { configure(); }

/*!
************************************************
* Class destructor.
************************************************
*/
IsegSHR::~IsegSHR()
{
    if(fConnection != nullptr) delete fConnection;
}

/*!
************************************************
* Configures the parameters based on xml
* configuration file.
************************************************
*/
void IsegSHR::configure()
{
    cout << "Configuring IsegSHR ..." << endl;
    string connectionType = fConfiguration.attribute("Connection").as_string();
    int    timeout        = fConfiguration.attribute("Timeout").as_int();
    cout << connectionType << " connection..." << endl;

    if(connectionType.compare("Ethernet") == 0)
    {
        string ipAddress = fConfiguration.attribute("IPAddress").as_string();
        int    port      = fConfiguration.attribute("Port").as_int();
        fConnection      = new SharedEthernetConnection(ipAddress, port);
    }
    else if(connectionType.compare("Serial") == 0)
    {
        int         baudRate    = fConfiguration.attribute("BaudRate").as_int();
        string      port        = fConfiguration.attribute("Port").as_string();
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
        error << "IsegSHR configuration: no connection " << connectionType << " available for Iseg SHR code, aborting...";
        throw std::runtime_error(error.str());
    }

    for(pugi::xml_node channel = fConfiguration.child("Channel"); channel; channel = channel.next_sibling("Channel"))
    {
        string inUse = channel.attribute("InUse").value();
        if(inUse.compare("Yes") != 0 && inUse.compare("yes") != 0) continue;

        string id = channel.attribute("ID").value();
        cout << "Configuring channel: " << id << endl;
        PowerSupply::fChannelMap.emplace(id, new IsegSHRChannel(fConnection, channel));
    }
}

/*******************************************************************/
/***************************** CHANNEL *****************************/
/*******************************************************************/

/*!
************************************************
 * Class constructor.
 \param configuration xml configuration node.
************************************************
*/

IsegSHRChannel::IsegSHRChannel(Connection* connection, const pugi::xml_node configuration)
    : PowerSupplyChannel(configuration), fChannel(atoi(configuration.attribute("Channel").value())), fConnection(connection)
{
    cout << "Inizializing channel number " << fChannel << endl;
}
// isegChannel::isegChannel(Connection* connection, const pugi::xml_node configuration)
//     : Channel(configuration), fConnection(connection)
// {
//   fChannelCommand = std::string("(@") + (fConfiguration.attribute("Channel").value()-1) + ")\n";
// }

/*!
************************************************
* Class destructor.
************************************************
*/
IsegSHRChannel::~IsegSHRChannel() {}

/*!
************************************************
 * Sends write command to connection.
 \param command Command to be send.
************************************************
*/
void IsegSHRChannel::write(string command)
{
    fConnection->write(command);
    usleep(1e5); /// delay in us to allow the power supply to process the command before moving to the next one
}

/*!
************************************************
 * Sends read command to connection.
 \param command Command to be send.
************************************************
*/
string IsegSHRChannel::read(string command)
{
    string answer = fConnection->read(command);
    answer        = fConnection->read(command); /// DO NOT REMOVE: it prevents to output \r\n after a setting command instead of the actual set value
    answer.erase(std::remove(answer.begin(), answer.end(), '\n'), answer.end());
    answer.erase(std::remove(answer.begin(), answer.end(), '\r'), answer.end());

    return answer;
}

/*!
************************************************
 * Get channel status word.
 \return channel status word.
************************************************
*/
unsigned int IsegSHRChannel::getChannelStatus()
{
    unsigned int status = stoul(read(":READ:CHAN:STAT? (@" + to_string(fChannel) + ")"));
    return status;
}

/*!
************************************************
* Set channel polarity.
  \param polarity Polarity to be set.
************************************************
*/
void IsegSHRChannel::setPolarity(string polarity)
{
    if(polarity != "n" and polarity != "p")
        cout << "Invalid command, enter p or n" << endl;
    else
    {
        if(isOn() == true)
        {
            std::stringstream error;
            error << "Power supply is on, switch it off first!";
            throw std::runtime_error(error.str());
        }
        else
        {
            write(":CONF:OUTP:POL " + polarity + ",(@" + to_string(fChannel) + ")");
            cout << "Polarity set to " << getPolarity() << endl;
        }
    }
}

/*!
************************************************
* Get channel polarity.
  \return P for positive, N for negative
************************************************
*/
string IsegSHRChannel::getPolarity()
{
    string polarity = read(":CONF:OUTP:POL? (@" + to_string(fChannel) + ")");
    return polarity;
}

/*!
************************************************
* Set channel output on.
************************************************
*/
void IsegSHRChannel::turnOn()
{
    if(isOn() == false)
    {
        write(":VOLT ON,(@" + to_string(fChannel) + ")");
        if(isOn() == true)
            cout << "Voltage output turned on" << endl;
        else
            cout << "Voltage output is still off" << endl;
    }
    else
        cout << "HV already on" << endl;
}

/*!
************************************************
* Set channel output off.
************************************************
*/
void IsegSHRChannel::turnOff()
{
    if(isOn() == true)
    {
        write(":VOLT OFF,(@" + to_string(fChannel) + ")");
        if(isOn() == false)
            cout << "Voltage output turned off" << endl;
        else
            cout << "Voltage output is still on" << endl;
    }
    else
        cout << "HV already off" << endl;
}

/*!
************************************************
 * Checks if the channel output is on or not.
 \return True if channel output is on, false
 otherwise
************************************************
*/
bool IsegSHRChannel::isOn()
{
    string answer = read(":READ:VOLT:ON? (@" + to_string(fChannel) + ")");
    float  result;
    sscanf(answer.c_str(), "%f", &result);
    return result;
}

/*!
************************************************
 * Set channel output voltage value in voltage.
 \param voltage Voltage value in volts.
************************************************
*/
void IsegSHRChannel::setVoltage(float voltage)
{
    int polarity;
    if(getPolarity() == "p")
        polarity = 1;
    else
        polarity = -1;

    if(voltage * polarity < 0)
    {
        std::stringstream error;
        error << "Voltage and polarity have different signs! Change polarity first!";
        throw std::runtime_error(error.str());
    }
    else
    {
        write(":VOLT " + std::to_string(voltage) + ",(@" + to_string(fChannel) + ")");
        cout << "Set voltage to: " << getSetVoltage() << " V" << endl;
    }
}

/*!
************************************************
 * Set channel output current value in amperes.
\param current Current value in amperes.
************************************************
*/
void IsegSHRChannel::setCurrent(float current)
{
    if(current < 0 || current > getCurrentCompliance())
    {
        std::stringstream error;
        error << "Please choose a value between 0 and " << getCurrentCompliance() << " A" << endl;
        throw std::runtime_error(error.str());
    }
    else
    {
        write(":CURR " + std::to_string(current) + ",(@" + to_string(fChannel) + ")");
        cout << "Set current to: " << getSetCurrent() << " A" << endl;
    }
}

/*!
************************************************
 * Set channel output voltage limiti value in voltage.
 \param voltage Voltage value in volts.
************************************************
*/
void IsegSHRChannel::setVoltageCompliance(float voltage)
{
    std::stringstream error;
    error << "Only manual setting possible, aborting...";
    throw std::runtime_error(error.str());
}

/*!
************************************************
 * Set channel output current limit value in
 * amperes.
 \param current Current value in amperes.
************************************************
*/
void IsegSHRChannel::setCurrentCompliance(float current)
{
    std::stringstream error;
    error << "Only manual setting possible, aborting...";
    throw std::runtime_error(error.str());
}

/*!
************************************************
 * Set channel output over voltage protection
 * trip point in voltage.
 \param maxVoltage Trip point in volts.
************************************************
*/
void IsegSHRChannel::setOverVoltageProtection(float maxVoltage) { setVoltageCompliance(maxVoltage); }

/*!
************************************************
 * Set channel output over current protection
 * trip point in amperes.
 \param current Trip point in amperes.
************************************************
*/
void IsegSHRChannel::setOverCurrentProtection(float maxCurrent) { setCurrentCompliance(maxCurrent); }

/*!
************************************************
 * Returns the channel output readback voltage.
 \return Readback voltage in voltage.
************************************************
*/
float IsegSHRChannel::getOutputVoltage()
{
    string answer = read(":MEAS:VOLT? (@" + to_string(fChannel) + ")");
    float  result;
    sscanf(answer.c_str(), "%f", &result);
    return result;
}

/*!
************************************************
 * Returns the channel set voltage.
 \return Set voltage in voltage.
************************************************
*/
float IsegSHRChannel::getSetVoltage()
{
    string answer = read(":READ:VOLT? (@" + to_string(fChannel) + ")");
    float  result;
    sscanf(answer.c_str(), "%f", &result);
    return result;
}

/*!
************************************************
 * Returns the channel output readback current.
 \return Readback current in amperes.
************************************************
*/
float IsegSHRChannel::getCurrent()
{
    string answer = read(":MEAS:CURR? (@" + to_string(fChannel) + ")");
    float  result;
    sscanf(answer.c_str(), "%f", &result);
    return result;
}

/*!
************************************************
 * Returns the channel set current.
 \return Set current in ampere.
************************************************
*/
float IsegSHRChannel::getSetCurrent()
{
    string answer = read(":READ:CURR? (@" + to_string(fChannel) + ")");
    float  result;
    sscanf(answer.c_str(), "%f", &result);
    return result;
}

/*!
************************************************
 * Returns the channel output readback voltage limit.
 \return Readback voltage in voltage.
************************************************
*/
float IsegSHRChannel::getVoltageCompliance()
{
    string answer = read(":READ:VOLT:LIM? (@" + to_string(fChannel) + ")");
    float  result;
    sscanf(answer.c_str(), "%f", &result);
    return result;
}

/*!
************************************************
 * Returns the channel output readback current limit.
 \return Readback current in amperes.
************************************************
*/
float IsegSHRChannel::getCurrentCompliance()
{
    string answer = read(":READ:CURR:LIM? (@" + to_string(fChannel) + ")");
    float  result;
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
float IsegSHRChannel::getOverVoltageProtection()
{
    float result = getVoltageCompliance();
    return result;
}
/*!
************************************************
 * Returns the current trip setting for the
 * channel output in amperes.
 \return Current trip setting in amperes.
************************************************
*/
float IsegSHRChannel::getOverCurrentProtection()
{
    float result = getCurrentCompliance();
    return result;
}

/*!
************************************************
* Reset the device to save values: Set HV to 0V,
* current to default and turn HV off with ramp for all channels.
************************************************
*/
void IsegSHRChannel::reset()
{
    write("*RST");
    cout << "Sent reset" << endl;
}

/*!
************************************************
* Get module ID
\return module ID
************************************************
*/
string IsegSHRChannel::getDeviceID()
{
    string answer = read("*IDN?");
    return answer;
}

/*!
************************************************
* Indicates if the channel is ramping
\return ramping status
************************************************
*/
bool IsegSHRChannel::isRamping()
{
    unsigned int status = getChannelStatus();
    status &= 16; /// Bit 4 indicates ramp status
    bool answer;
    if(status == 16)
        answer = true;
    else
        answer = false;

    return answer;
}

/*!
************************************************
 * Set voltage ramp up speed (V/s).
 \param speed ramp up speed (V/s).
************************************************
*/
void IsegSHRChannel::setVoltageRampUpSpeed(float speed)
{
    if(speed < 0.01 || speed > 400)
    {
        std::stringstream error;
        error << "Speed exceeds ramp up speed bounds. Please choose a value between 0.01 and 400 V/s";
        throw std::runtime_error(error.str());
    }
    else
    {
        write(":CONF:RAMP:VOLT:UP " + std::to_string(speed) + ", (@" + to_string(fChannel) + ")");
    }
}

/*!
************************************************
 * Set voltage ramp down speed (V/s).
 \param speed ramp down speed (V/s).
************************************************
*/
void IsegSHRChannel::setVoltageRampDownSpeed(float speed)
{
    if(speed < 0.01 || speed > 400)
    {
        std::stringstream error;
        error << "Speed exceeds ramp down speed bounds. Please choose a value between 0.01 and 400 V/s";
        throw std::runtime_error(error.str());
    }
    else
    {
        write(":CONF:RAMP:VOLT:DO " + std::to_string(speed) + ", (@" + to_string(fChannel) + ")");
    }
}

/*!
************************************************
 * Get voltage ramp up speed (V/s).
 \return ramp up speed (V/s).
************************************************
*/
float IsegSHRChannel::getVoltageRampUpSpeed()
{
    string answer = read(":CONF:RAMP:VOLT:UP? (@" + to_string(fChannel) + ")");
    float  result;
    sscanf(answer.c_str(), "%f", &result);
    return result;
}

/*!
************************************************
 * Get voltage ramp down speed (V/s).
 \return ramp down speed (V/s).
************************************************
*/
float IsegSHRChannel::getVoltageRampDownSpeed()
{
    string answer = read(":CONF:RAMP:VOLT:DO? (@" + to_string(fChannel) + ")");
    float  result;
    sscanf(answer.c_str(), "%f", &result);
    return result;
}

/*!
************************************************
 * Get device temperature (C).
 \return device temperature (C).
************************************************
*/
string IsegSHRChannel::getDeviceTemperature()
{
    string answer = read(":READ:MOD:TEMP?");
    return answer;
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
float IsegSHRChannel::getParameterFloat(std::string parName)
{
    string answer = read(parName + "?");
    float  result;
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
int IsegSHRChannel::getParameterInt(std::string parName)
{
    string answer = read(parName + "?");
    int    result;
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
bool IsegSHRChannel::getParameterBool(std::string parName)
{
    std::stringstream error;
    error << "Sorry, there are no bool parameter for IsegSHR power supply to be read, skipping the command ...";
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
void IsegSHRChannel::setParameter(string parName, float value) { write(parName + " " + to_string(value)); }

/*!
************************************************
 * This method is doing nothing but raising
 * exeption.
 \param parName Name of the parameter to be
 set.
 \param value Value to set.
************************************************
*/
void IsegSHRChannel::setParameter(string parName, bool value)
{
    std::stringstream error;
    error << "Sorry, there are no bool parameter for IsegSHR power supply to be set, skipping the command ...";
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
void IsegSHRChannel::setParameter(std::string parName, int value)
{
    std::stringstream error;
    error << "Sorry, there are no integer parameter for IsegSHR power supply to be set, skipping the command ...";
    throw std::runtime_error(error.str());
}
