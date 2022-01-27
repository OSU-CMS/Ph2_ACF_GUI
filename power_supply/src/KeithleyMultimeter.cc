#include "KeithleyMultimeter.h"
#include "EthernetConnection.h"
#include "SerialConnection.h"

#include <cstdlib>
#include <iostream>
#include <string>

/*!
************************************************
 * Class constructor.
 \param configuration xml configuration node.
************************************************
*/

KeithleyMultimeter::KeithleyMultimeter(const pugi::xml_node configuration) : Multimeter("KeithleyMultimeter", configuration) { configure(); }

/*!
************************************************
* Class destructor.
************************************************
*/
KeithleyMultimeter::~KeithleyMultimeter(void)
{
    if(fConnection != nullptr) delete fConnection;
}

/*!
************************************************
* Prints info from IDN?
************************************************
*/
void KeithleyMultimeter::printInfo()
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
void KeithleyMultimeter::configure()
{
    std::cout << "Configuring KeithleyMultimeter with ";
    std::string connectionType = fConfiguration.attribute("Connection").as_string();
    int         timeout        = fConfiguration.attribute("Timeout").as_int();
    // fSeriesName                = fConfiguration.attribute("Series").as_string();
    std::cout << connectionType << " connection ..." << std::endl;

    if(connectionType == "Ethernet")
    {
        std::string ipAddress = fConfiguration.attribute("IPAddress").as_string();
        int         port      = fConfiguration.attribute("Port").as_int();
        fConnection           = new SharedEthernetConnection(ipAddress, port, timeout * 1e3);
    }
    else if(!connectionType.compare("Serial"))
    {
        std::string port        = fConfiguration.attribute("Port").as_string();
        int         baudRate    = fConfiguration.attribute("BaudRate").as_int();
        bool        flowControl = fConfiguration.attribute("FlowControl").as_bool();
        bool        parity      = fConfiguration.attribute("Parity").as_bool();
        bool        removeEcho  = fConfiguration.attribute("RemoveEcho").as_bool();
        std::string terminator  = fConfiguration.attribute("Terminator").as_string();
        std::string suffix      = fConfiguration.attribute("Suffix").as_string();
        terminator              = Multimeter::convertToLFCR(terminator);
        suffix                  = Multimeter::convertToLFCR(suffix);
        fConnection             = new SerialConnection(port, baudRate, flowControl, parity, removeEcho, terminator, suffix, timeout);
    }
    else
    {
        std::stringstream error;
        error << "Keithley configuration: no connection " << connectionType << " available for Keithley code, aborting...";
        throw std::runtime_error(error.str());
    }
    if(!isOpen())
    {
        std::stringstream error;
        error << "Keithley connection " << connectionType << " not open, channel(s) will not be configured, aborting execution...";
        throw std::runtime_error(error.str());
    }
    printInfo();
}
/*!
************************************************
* Send reset command.
************************************************
*/
void KeithleyMultimeter::reset() { fConnection->write("*RST"); }

/*!
************************************************
 * Checks if the connection with the instrument
 * is open.
 \return True if the connection is open, false
 otherwise.
************************************************
*/
bool KeithleyMultimeter::isOpen() { return fConnection->isOpen(); }

/*!
************************************************
 * Returns the readback voltage.
 \return Readback voltage in volts.
************************************************
*/
float KeithleyMultimeter::measureVoltage()
{
    std::string answer = fConnection->read(":READ?");
    float       result;
    sscanf(answer.c_str(), "%f , %*f", &result);
    return result;
}

/*!
************************************************
 * Returns the readback current.
 \return Readback current in amperes.
************************************************
*/
float KeithleyMultimeter::measureCurrent()
{
    std::string answer = fConnection->read(":READ?");
    float       result;
    sscanf(answer.c_str(), "%*f , %f", &result);
    return result;
}

/*!
************************************************
* Enables internal scan.
************************************************
*/
void KeithleyMultimeter::enableInternalScan() { fConnection->write(":ROUT:OPEN:ALL"); }

/*!
************************************************
 * Return read value from channels in the
 * range.
 \param chanStart First channel of the range.
 \param chanStop Last channel of the range.
 \return Return string with read values.
************************************************
*/
std::string KeithleyMultimeter::scanChannels(int chanStart, int chanStop)
{
    fConnection->write(":FORM:ELEM READ");
    fConnection->write(":TRAC:CLE");
    fConnection->write(":ROUT:SCAN (@" + std::to_string(chanStart) + ":" + std::to_string(chanStop) + ")");
    fConnection->write(":ROUT:SCAN:LSEL INT");
    fConnection->write("TRAC:POIN " + std::to_string(chanStop - chanStart + 1)); // select buffer size
    fConnection->write("*WAI");
    fConnection->write("TRACE:FEED SENSE1"); // put raw reading in buffer
    fConnection->write("*WAI");
    fConnection->write("TRACE:FEED:CONT NEXT"); // storage process starts, fills buffer, then stops
    fConnection->write("*WAI");
    for(int i = 0; i < (chanStop - chanStart + 1); i++)
    {
        fConnection->write("INIT");
        fConnection->write("*WAI");
    }
    return fConnection->read(":TRACE:DATA?");
}

/*!
************************************************
 * Creates scanner card map between channel
 * number and "names".
 \param configNode xml node with informations.
 \param channelMap std::map for map creation
 storage.
 \param verbose Verbosity.
 \return Returns false (method not
 implemented).
************************************************
*/

bool KeithleyMultimeter::createScannerCardMap(pugi::xml_node* configNode, std::map<int, std::string>& channelMap, bool verbose)
{
    if(verbose) std::cout << "Creating map for Keithley multimeter" << std::endl;
    std::ostringstream  searchChannelStr;
    pugi::xml_attribute attr;
    int                 vChannel;
    for(pugi::xml_attribute attr = configNode->first_attribute(); attr; attr = attr.next_attribute())
    {
        if(std::sscanf(attr.name(), "Channel_%d", &vChannel) != 1) continue;
        if(verbose) std::cout << "Channel map emplace vChannel: " << vChannel << ", attribute: " << attr.value() << std::endl;
        channelMap.emplace(vChannel, attr.value());
    }
    return true;
}
