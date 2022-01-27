#include "ScopeAgilent.h"
#include "Scope.h"
#include "SerialConnection.h"
#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <string.h>

/*!
************************************************
 * Class constructor.
 \param configuration xml configuration node.
************************************************
*/
ScopeAgilent::ScopeAgilent(const pugi::xml_node configuration) : Scope("ScopeAgilent", configuration) { configure(); }

/*!
************************************************
* Class destructor.
************************************************
*/
ScopeAgilent::~ScopeAgilent() {}

/*!
************************************************
* Prints info from IDN?
************************************************
*/
void ScopeAgilent::printInfo()
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
void ScopeAgilent::configure()
{
    std::cout << "Configuring ScopeAgilent with ";
    std::string connectionType = fConfiguration.attribute("Connection").as_string();
    int         timeout        = fConfiguration.attribute("Timeout").as_int();
    fSeriesName                = fConfiguration.attribute("Series").as_string();
    fStoreImagePath            = fConfiguration.attribute("storeImagePath").as_string();
    std::cout << connectionType << " connection ..." << std::endl;
    if(connectionType == "USBTMC")
    {
        std::string port       = fConfiguration.attribute("Port").as_string();
        std::string terminator = fConfiguration.attribute("Terminator").as_string();
        std::string suffix     = fConfiguration.attribute("Suffix").as_string();
        bool        removeEcho = fConfiguration.attribute("RemoveEcho").as_bool();
        terminator             = Scope::convertToLFCR(terminator);
        suffix                 = Scope::convertToLFCR(suffix);
        fConnection            = std::make_shared<SerialConnection>(port, terminator, removeEcho, suffix, timeout);
    }
    else
    {
        std::stringstream error;
        error << "ScopeAgilent configuration: no connection " << connectionType << " available for ScopeAgilent code, aborting...";
        throw std::runtime_error(error.str());
    }
    if(!isOpen())
    {
        std::stringstream error;
        error << "ScopeAgilent connection " << connectionType
              << " not open, channel(s) will not be configured, aborting "
                 "execution...";
        throw std::runtime_error(error.str());
    }
    // printInfo();
    std::cout << "**\t"
              << "Open connection " << fConnection << " with device -> " << fConnection->read("*IDN?") << "\t**" << std::endl;
    if(fStoreImagePath != "") { fConnection->write(":DISK:MDIRECTORY \"" + fStoreImagePath + "\""); }
    for(pugi::xml_node channel = fConfiguration.child("Channel"); channel; channel = channel.next_sibling("Channel"))
    {
        std::string inUse = channel.attribute("InUse").value();
        if(inUse != "Yes" && inUse != "yes") continue;
        std::string id = channel.attribute("ID").value();
        auto        ch = new ScopeAgilentChannel(fConnection, channel, fSeriesName, fStoreImagePath);
        Scope::fChannelMap.emplace(id, ch);
    }
    auto main_channel = dynamic_cast<ScopeAgilentChannel*>(this->getChannel("main"));
    main_channel->printConnection();
}

/*!
************************************************
 * Checks if the connection with the instrument
 * is open.
 \return True if the connection is open, false
 otherwise.
************************************************
*/
bool ScopeAgilent::isOpen() { return fConnection->isOpen(); }

/*!
************************************************
* Prints through stdio the pointer to fConnection
************************************************
*/

void ScopeAgilent::printConnection() { std::cout << fConnection << std::endl; }

/*!
************************************************
 * Class constructor of ScopeAgilentChannel
 \param connection shared_ptr to the Connection of the oscilloscope
 \param configuration the configuration file
 \param seriesName series name
 \param imagepath path in the oscilloscope memory to store screenshots. Nothing is stored if imagepath=""
************************************************
*/

ScopeAgilentChannel::ScopeAgilentChannel(std::shared_ptr<Connection> connection, const pugi::xml_node configuration, std::string seriesName, std::string imagepath)
    : ScopeChannel(configuration), fChannel(atoi(configuration.attribute("Channel").value())), fConnection(connection), fSeriesName(seriesName), fStoreImagePath(imagepath)
{
    fChannelName = std::to_string(fChannel);
    std::cout << "Inizializing channel " << this->getID() << " number " << fChannel << std::endl;
    try
    {
        // statusEER();
        // statusQER();
        // statusESR();
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
ScopeAgilentChannel::~ScopeAgilentChannel() {}

/*!
************************************************
* Acquires the waveform in the channel and returs it.
  Oscilloscope settings are hardcoded for the moment.
 \return Measured points of the waveform, separated by commas
************************************************
*/

std::string ScopeAgilentChannel::aquireWaveForm()
{
    fConnection->write(":SYSTEM:HEADER OFF");
    fConnection->write(":ACQUIRE:MODE RTIME");
    fConnection->write(":ACQUIRE:COMPLETE 100");
    fConnection->write(":WAVEFORM:SOURCE CHANNEL" + fChannelName);
    fConnection->write(":WAVEFORM:FORMAT ASCII");
    fConnection->write(":ACQUIRE:COUNT 8");
    fConnection->write(":ACQUIRE:POINTS 500");
    fConnection->write(":DIGITIZE CHANNEL" + fChannelName);
    return fConnection->read(":WAVEFORM:DATA?");
}

/*!
************************************************
* Sets the parameters for measurements of an eye opening diagram
 \param observables set of quantities to be measured
************************************************
*/

void ScopeAgilentChannel::setEOMeasurement(std::string observablesstring)
{
    std::vector<std::string> observables;
    boost::split(observables, observablesstring, boost::is_any_of("|"));

    if(observables.size() > 5)
    {
        std::stringstream error;
        std::cout << "Trying to set more than 5 observables in a EO measurement in an agilent scope. The scope does not support that." << std::endl;
        for(auto& obs: observables) std::cout << obs << std::endl;
    }

    fEOMObservables = observables;
}

/*!
************************************************
* Performs the measurement of the eye diagram. setEOMeasurement must have been called first to set fEOMObservables.
 \param nTrigs number of wave forms to be acquired for the measurement
 \param winStart starting point of the window to make an eye pattern (in percentage in horizontal)
 \param winStop stopping  point of the window to make an eye pattern (in percentage in horizontal)
 \param measurementName name to be given to the screenshot of the measurement
 \return
************************************************
*/

std::string ScopeAgilentChannel::acquireEOM(int nTrigs, int winStart, int winStop, std::string measurementName)
{
    std::cout << "[acquireEOM] Check1" << std::endl;
    fConnection->write(":MEASure:CLEar");
    fConnection->write(":MEASure:CGRade:EWINdow " + std::to_string(winStart) + "," + std::to_string(winStop));
    fConnection->write(":CDISplay");
    fConnection->write(":DISPlay:CGRade ON");

    fConnection->write(":TRIGger:MODE EDGE");
    fConnection->write(":MEASure:SOURce CHAN " + fChannelName);

    // up to five
    for(auto& obs: fEOMObservables) { fConnection->write(":MEASure:CGRade:" + obs); }

    for(int i = 0; i < nTrigs; ++i) { fConnection->write(":DIGitize"); }

    fConnection->write(":MEASure:SENDvalid 1");
    std::string results = fConnection->read(":MEASure:RESults?");

    if(fStoreImagePath != "") { fConnection->write(":DISK:SAVE:IMAGe \"" + fStoreImagePath + "\\" + measurementName + "\",PNG"); }

    return results;
}

void ScopeAgilentChannel::printConnection() { std::cout << fConnection << std::endl; }
