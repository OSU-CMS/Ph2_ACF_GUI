/*!
 * \authors Monica Scaringella <monica.scaringella@fi.infn.it>, INFN-Firenze
 * \date July 24 2020
 */

#include "KeySight.h"
#include "EthernetConnection.h"
#include "SerialConnection.h"
#include <string.h>

/*!
************************************************
 * Class constructor.
 \param configuration xml configuration node.
************************************************
*/
KeySight::KeySight(const pugi::xml_node configuration) : PowerSupply("KeySight", configuration) { configure(); }

/*!
************************************************
* Class destructor.
************************************************
*/
KeySight::~KeySight()
{
    if(fConnection != nullptr) delete fConnection;
}

/*!
************************************************
* Prints info from IDN?
************************************************
*/
void KeySight::printInfo()
{
    std::string idnString = fConnection->read("*IDN?");
    idnString.erase(std::remove(idnString.begin(), idnString.end(), '\n'), idnString.end());
    idnString.erase(std::remove(idnString.begin(), idnString.end(), '\r'), idnString.end());
    std::cout << "\t\t**\t" << idnString << "\t**" << std::endl;
}

/*!
************************************************
* Get info from IDN?
************************************************
*/
std::string KeySight::getInfo()
{
    std::string idnString = fConnection->read("*IDN?");
    idnString.erase(std::remove(idnString.begin(), idnString.end(), '\n'), idnString.end());
    idnString.erase(std::remove(idnString.begin(), idnString.end(), '\r'), idnString.end());
    return idnString;
}

/*!
************************************************
* Configures the parameters based on xml
* configuration file.
************************************************
*/
void KeySight::configure()
{
    std::cout << "Configuring KeySight with ";
    std::string connectionType = fConfiguration.attribute("Connection").as_string();
    int         timeout        = fConfiguration.attribute("Timeout").as_int();
    fSeriesName                = fConfiguration.attribute("Series").as_string();
    std::cout << connectionType << " connection ..." << std::endl;

    if(!connectionType.compare("Serial"))
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
    else
    {
        std::stringstream error;
        error << "KeySight configuration: no connection " << connectionType << " available for KeySight code, aborting...";
        throw std::runtime_error(error.str());
    }
    if(!isOpen())
    {
        std::stringstream error;
        error << "KeySight connection " << connectionType << " not open, channel(s) will not be configured, aborting execution...";
        throw std::runtime_error(error.str());
    }
    printInfo();
    for(pugi::xml_node channel = fConfiguration.child("Channel"); channel; channel = channel.next_sibling("Channel"))
    {
        std::string inUse = channel.attribute("InUse").value();
        if(inUse.compare("Yes") != 0 && inUse.compare("yes") != 0) continue;
        std::string id = channel.attribute("ID").value();
        PowerSupply::fChannelMap.emplace(id, new KeySightChannel(fConnection, channel, fSeriesName));
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
void KeySight::reset() { fConnection->write("*RST"); }

/*!
************************************************
 * Checks if the connection with the instrument
 * is open.
 \return True if the connection is open, false
 otherwise.
************************************************
*/
bool KeySight::isOpen() { return fConnection->isOpen(); }

/*******************************************************************/
/***************************** CHANNEL *****************************/
/*******************************************************************/

/*!
************************************************
 * Class constructor.
 \param configuration xml configuration node.
************************************************
*/
KeySightChannel::KeySightChannel(Connection* connection, const pugi::xml_node configuration, std::string seriesName)
    : PowerSupplyChannel(configuration), fChannelName(configuration.attribute("Channel").value()), fConnection(connection), fSeriesName(seriesName)
{
    fChannel = (fChannelName == "FRON") ? 0 : 1;
    std::cout << "Inizializing channel " << this->getID() << " named " << fChannelName << " number " << fChannel << std::endl;
    fChannelCommand = std::string(":ROUT:TERM ") + fConfiguration.attribute("Channel").value();
}

/*!
************************************************
* Class destructor.
************************************************
*/
KeySightChannel::~KeySightChannel() {}

/*!
************************************************
 * Sends write command to connection.
 \param command Command to be send.
************************************************
*/
void KeySightChannel::write(std::string command)
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
std::string KeySightChannel::read(std::string command)
{
    fConnection->write(fChannelCommand);
    std::string const answer = fConnection->read(command);
    return answer;
}

/*!
************************************************
* Set channel output on.
************************************************
*/
void KeySightChannel::turnOn(void)
{
    write(":OUTPUT ON");
    std::cout << "Turn on channel " << fConfiguration.attribute("Channel").value() << " output." << std::endl;
}

/*!
************************************************
* Set channel output off.
************************************************
*/
void KeySightChannel::turnOff(void)
{
    write(":OUTPUT OFF");
    std::cout << "Turn off channel " << fConfiguration.attribute("Channel").value() << " output." << std::endl;
}

/*!
************************************************
 * Checks if the channel output is on or not.
 \return True if channel output is on, false
 otherwise
************************************************
*/
bool KeySightChannel::isOn(void)
{
    /*
    std::string answer = read("output?");
    if(answer.compare(0, 1, "0") == 0)
        return false;
    else if(answer.compare(0, 1, "1") == 0)
        return true;
    */

    // Only the selected channel can be activated. If not selected it is not on by default
    std::string currentChannel = fConnection->read(":ROUT:TERM?");        // Get selected channel
    currentChannel.erase(4, 1);                                           // Somehow the power supply returns FRON_ instead of FRONT
    std::string thisChannel(fConfiguration.attribute("Channel").value()); // Check the name of this channel
    thisChannel.erase(4, 1);
    int result;
    if(currentChannel == thisChannel) // Compare and check if the output is enabled
    {
        std::string const answer = fConnection->read("OUTP?");
        sscanf(answer.c_str(), "%d", &result);
    }
    else
    {
        result = false;
    }
    return result;
}

/*!
************************************************
 * Set channel source voltage value in volts.
 \param voltage Voltage value in volts.
************************************************
*/

void KeySightChannel::setVoltage(float voltage)
{
    std::string command;
    command.reserve(50);
    std::sprintf(&command[0], ":SOURCE:VOLTAGE:LEV:IMM %e", voltage);
    write(command.c_str());
    fSetVoltage = voltage;
}

/*!
************************************************
 * Set channel source current value in amperes
 \param current Current value in amperes.
************************************************
*/
void KeySightChannel::setCurrent(float current)
{
    std::string command;
    command.reserve(50);
    std::sprintf(&command[0], ":SOURCE:CURRENT:LEV:IMM %e", current);
    write(command.c_str());
}

/*!
************************************************
 * Set channel voltage compliance when the
 * instrument is in current source mode
 \param voltage Voltage compliance value in volts
************************************************
*/
void KeySightChannel::setVoltageCompliance(float voltage)
{
    std::string command;
    command.reserve(50);
    std::sprintf(&command[0], ":SOURCE:VOLT:PROTECTION:LEV %e", voltage);
    write(command.c_str());
}

/*!
************************************************
 * Set channel current compliance when the
 * instrument is in voltage source mode
 \param current Current compliance value in amperes
************************************************
*/
void KeySightChannel::setCurrentCompliance(float current)
{
    std::string command;
    command.reserve(50);
    std::sprintf(&command[0], ":SOURCE:CURR:PROTECTION:LEV %e", current);
    write(command.c_str());
}

/*!
************************************************
 * This method is doing nothing but raising
 * exception.
 \param voltage
************************************************
*/
void KeySightChannel::setOverVoltageProtection(float voltage)
{
    std::stringstream error;
    error << "setOverVoltageProtection: command not implemented for KeySight  aborting ...";
    throw std::runtime_error(error.str());
}

/*!
************************************************
 * This method is doing nothing but raising
 * exception.
 \param current
************************************************
*/
void KeySightChannel::setOverCurrentProtection(float current)
{
    std::stringstream error;
    error << "setOverCurrentProtection: command not implemented for KeySight  aborting ...";
    throw std::runtime_error(error.str());
}

/*!
************************************************
 * Returns the channel output readback voltage.
 \return Readback voltage in volts.
************************************************
*/
float KeySightChannel::getOutputVoltage(void)
{
    float result = 0;
    if(isOn())
    {
        // Somehow the current should be read first before the voltage is measured, otherwise, the voltage measurement is "old"
        //fConnection->write("form:elem curr");
        //fConnection->read(":meas:curr?");

        //fConnection->write(":form:elem volt");
        // usleep(100);
         fConnection->write("MEASURE:VOLTAGE?");
        std::string const answer =  fConnection->read("MEASURE:VOLTAGE?");
        sscanf(answer.c_str(), "%f", &result);
    }
    return result;
}

/*!
************************************************
 * Returns the channel output voltage set.
 \return Voltage set in volts.
************************************************
*/
float KeySightChannel::getSetVoltage(void)
{
    // answer = read(":source:voltage?");
    // std::cout << "fSetVoltage: " << fSetVoltage << std::endl;
    float             result;
    std::string const answer = fConnection->read(":SOURCE:VOLTAGE?");
    sscanf(answer.c_str(), "%f", &result);
    return result;
}

/*!
************************************************
 * Returns the channel output readback current.
 \return Readback current in amperes.
************************************************
*/
float KeySightChannel::getCurrent(void)
{
    float result = 0;
    if(isOn())
    {
        //fConnection->write("form:elem curr");
        // usleep(100);
        std::string const answer = fConnection->read(":SOURCE:CURRENT?");
        // answer             = fConnection->read(":meas:curr?");
        sscanf(answer.c_str(), "%f", &result);
    }
    return result;
}

/*!
************************************************
 * Returns the voltage compliance when
 * instrument is set in current source mode
 \return Voltage compliance in volts.
************************************************
*/
float KeySightChannel::getVoltageCompliance(void)
{
    float             result;
    std::string const answer = fConnection->read("sense:voltage:protection?");
    sscanf(answer.c_str(), "%f", &result);
    return result;
}

/*!
************************************************
 * Returns the current compliance when
 * instrument is set in voltage source mode
 \return Current compliance in amperes.
************************************************
*/
float KeySightChannel::getCurrentCompliance(void)
{
    float             result;
    std::string const answer = fConnection->read("sense:current:protection?");
    sscanf(answer.c_str(), "%f", &result);
    return result;
}

/*!
************************************************
* This method is doing nothing but raising
* exception.
************************************************
*/
float KeySightChannel::getOverVoltageProtection(void)
{
    std::stringstream error;
    error << "getOverVoltageProtection: command not implemented for KeySight  aborting ...";
    throw std::runtime_error(error.str());
    return -1;
}

/*!
************************************************
* This method is doing nothing but raising
* exception.
************************************************
*/
float KeySightChannel::getOverCurrentProtection(void)
{
    std::stringstream error;
    error << "getOverCurrentProtection: command not implemented for KeySight  aborting ...";
    throw std::runtime_error(error.str());
    return -1;
}

/*!
************************************************
 * This method is doing nothing but raising
 * exception.
 \param parName Name of the parameter to be
 read.
************************************************
*/

float KeySightChannel::getParameterFloat(std::string parName)
{
    std::stringstream error;
    error << "Sorry, there are no float parameter for KeySight  power supply to be read, skipping the command ...";
    throw std::runtime_error(error.str());
}

/*!
************************************************
 * This method is doing nothing but raising
 * exception.
 \param parName Name of the parameter to be
 read.
************************************************
*/
int KeySightChannel::getParameterInt(std::string parName)
{
    std::stringstream error;
    error << "Sorry, there are no int parameter for KeySight  power supply to be read, skipping the command ...";
    throw std::runtime_error(error.str());
}

/*!
************************************************
 * This method is doing nothing but raising
 * exception.
 \param parName Name of the parameter to be
 read.
************************************************
*/
bool KeySightChannel::getParameterBool(std::string parName)
{
    std::stringstream error;
    error << "Sorry, there are no bool parameter for KeySight  power supply to be read, skipping the command ...";
    throw std::runtime_error(error.str());
}

/*!
************************************************
 * Sets Source Voltage/Current range or sends
 * a generic command with float value when
 * preceded by "command arg "or a command
 * with no argument when precede by "command "
 * </br>
 * Please refer to specific model manual for
 * a complete list of possible commands and
 * values.
 \brief Sets source range or sends generic
 command
 \param parName Vsrc_range/Isrc_range for V/I
 source range, command for generic command
 \param value Floating value to set.
************************************************
*/

void KeySightChannel::setParameter(std::string parName, float value)
{
    std::stringstream error;
    std::string       command;
    command.reserve(50);
    // std::cout << "parName: " << parName << " value: " << value << std::endl;

    if(parName == "Vsrc_range")
        setVoltageRange(value);
    else if(parName == "Isrc_range")
        setCurrentRange(value);
    else if(parName.compare(0, 11, "command_arg") == 0)
    {
        std::string const commandTmp = parName.substr(12, parName.length());
        std::sprintf(&command[0], "%s %f", commandTmp.c_str(), value);
        // std::cout << "Sending command: " << command.c_str() << std::endl;
        write(command.c_str());
    }
    else if(parName.compare(0, 7, "command") == 0) // This should not be possible...
    {
        command = parName.substr(8, parName.length());
        // std::cout << "Sending command: " << command.c_str() << std::endl;
        write(command.c_str());
    }
    else
    {
        error << parName << " invalid parameter, skipping the command ...";
        throw std::runtime_error(error.str());
    }
}

/*!
************************************************
 * Sets autorange (on/off) for the measuring
 * function.
 \param parName Name of the parameter to be
 set.
 \param value Value to set.
************************************************
*/
void KeySightChannel::setParameter(std::string parName, bool value)
{
    if(parName == "autorange")
        setAutorange(value);
    else if(parName == "display")
        setDisplay(value);
    else if(parName == "local")
        setLocal(value);
    else
    {
        std::stringstream error;
        error << parName << " invalid parameter, skipping the command ...";
        throw std::runtime_error(error.str());
    }
}

/*!
************************************************
 * This method is doing nothing but raising
 * exception.
 \param parName Name of the parameter to be
 set.
 \param value Value to set.
************************************************
*/
void KeySightChannel::setParameter(std::string parName, int value)
{
    std::stringstream error;
    error << "Sorry, there are no unsigned parameter for KeySight power supply to be set, skipping the command ...";
    throw std::runtime_error(error.str());
}

/*!
************************************************
* Sets rear as active panel. This method is
* deprecated and will be removed.
************************************************
*/
void KeySightChannel::setRearOutput() { write(":rout:term rear"); }

/*!
************************************************
* Sets front as active panel. This method is
* deprecated and will be removed.
************************************************
*/
void KeySightChannel::setFrontOutput() { write(":rout:term front"); }

/*!
************************************************
* Sets autorange.
* \param True for autorange on, false
* otherwise.
************************************************
*/
void KeySightChannel::setAutorange(bool value)
{
    // First check which is the measuring function
    std::string answer;
    answer.reserve(100);

    answer = read(":sense:function?");

    int result_curr;
    int result_volt;

    result_curr = answer.compare("\"CURR:DC\"");
    result_volt = answer.compare("\"VOLT:DC\"");

    if(value)
    {
        if(result_curr == 0)
        {
            std::cout << "Setting current autorange on" << std::endl;
            write(":sense:current:range:auto on");
        }
        else if(result_volt == 0)
        {
            std::cout << "Setting voltage autorange on" << std::endl;
            write(":sense:voltage:range:auto on");
        }
    }
    else
    {
        if(result_curr == 0)
        {
            std::cout << "Setting current autorange off" << std::endl;
            write(":sense:current:range:auto off");
        }
        else if(result_volt == 0)
        {
            std::cout << "Setting voltage autorange off" << std::endl;
            write(":sense:voltage:range:auto off");
        }
    }
}

/*!
************************************************
* Switches display on and off.
* \param True for display on, false otherwise.
************************************************
*/
void KeySightChannel::setDisplay(bool value)
{
    std::string const onOffString = value ? "ON" : "OFF";
    std::string const command     = ":DISPlay:ENABle " + onOffString;
    // std::cout << "Sending command: " << command << std::endl;
    write(command);
}

/*!
************************************************
* Sets local control on and off.
* \param True for local on false otherwise.
************************************************
*/
void KeySightChannel::setLocal(bool value)
{
    if(!value) return;
    std::string const command = ":SYSTem:LOCal";
    write(command);
}

/*!
************************************************
* Sets voltage range.
* \param Voltage range, possible values are:
* 0.2V, 2V, 20V, 1000V
************************************************
*/
void KeySightChannel::setVoltageRange(float value)
{
    if(value == (float)0.2 ||  value == 2 || value == 20 || value == 1000)
    {
        std::cout << "Setting V source range to " << value << "V" << std::endl;
        std::string command;
        command.reserve(50);
        std::sprintf(&command[0], ":source:voltage:range %e", value);
        write(command.c_str());
    }
    else
    {
        std::stringstream error;
        error << "invalid value for Voltage source range, valid values are 0.2V, 2V, 20V, 1000V, skipping the command ...";
        throw std::runtime_error(error.str());
    }
}

/*!
************************************************
* Sets current range.
* \param Current range, possible values are:
* 1e-6A, 1e-5A, 1e-4A, 1e-3, 2e-3A, 0.1A, 1A
************************************************
*/
void KeySightChannel::setCurrentRange(float value)
{
    if(value == (float)1e-6 || value == (float)1e-5 || value == (float)1e-4 || value == (float)1e-3 || value == (float)2e-3 || value == (float)0.1 || value == (float)1)
    {
        std::cout << "Setting I source range to " << value << std::endl;
        std::string command;
        command.reserve(50);
        std::sprintf(&command[0], ":source:current:range %e", value);
        write(command.c_str());
    }
    else
    {
        std::stringstream error;
        error << "invalid value for Current source range, valid values are 1uA,10uA,100uA,1mA,20mA,100mA,1A skipping the command ...";
        throw std::runtime_error(error.str());
    }
}

/*!
************************************************
 * Set channel in voltage mode
************************************************
*/
void KeySightChannel::setVoltageMode(void) 
{
    std::string command;
    command.reserve(50);
    std::sprintf(&command[0], ":SOUR:FUNC VOLT");
    write(command.c_str());
}

/*!
************************************************
 * Set channel in current mode
************************************************
*/
void KeySightChannel::setCurrentMode(void) 
{
    std::string command;
    command.reserve(50);
    std::sprintf(&command[0], ":SOUR:FUNC CURR");
    write(command.c_str());
}

