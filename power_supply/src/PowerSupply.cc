#include "PowerSupply.h"
#include "PowerSupplyChannel.h"
#include <iostream>
#include <string.h>

/*!
************************************************
* PowerSupply constructor.
************************************************
*/
PowerSupply::PowerSupply(std::string model, const pugi::xml_node configuration) : fModel(model), fConfiguration(fConfigurationDoc.append_copy(configuration))
{
    for(pugi::xml_node channel = fConfiguration.child("Channel"); channel; channel = channel.next_sibling("Channel"))
    {
        std::string inUse = channel.attribute("InUse").value();
        if(inUse.compare("Yes") != 0 && inUse.compare("yes") != 0) continue;
        for(auto& id: fChannelList)
        {
            if(id == channel.attribute("ID").value()) throw std::runtime_error("Another channel with id " + id + " has been configured. The channel ids must be unique.");
        }
        fChannelList.emplace_back(channel.attribute("ID").value());
    }
}

/*!
************************************************
* PowerSupply distructor.
************************************************
*/
PowerSupply::~PowerSupply(void)
{
    for(auto it: fChannelMap) delete it.second;
    fChannelMap.clear();
}

/*!
************************************************
 * Get pointer to channel by id
 \param id Unique channel id
 \return Pointer to channel
************************************************
*/
PowerSupplyChannel* PowerSupply::getChannel(const std::string& id)
{
    if(fChannelMap.find(id) == fChannelMap.end()) { throw std::out_of_range("No channel with id " + id + " has been configured!"); }
    return fChannelMap.find(id)->second;
}

/*!
************************************************
* \brief XML string conversion.
*  The xml string is converted to the string
*  needed.
* \param strXML String to be converted.
* \return String properly converted.
************************************************
*/

std::string PowerSupply::convertToLFCR(std::string strXML)
{
    if(!strXML.compare("CR") || !strXML.compare("\\r"))
        return "\r";
    else if(!strXML.compare("LF") || !strXML.compare("\\n"))
        return "\n";
    else if(!strXML.compare("CRLF") || !strXML.compare("\\r\\n"))
        return "\r\n";
    else if(!strXML.compare("LFCR") || !strXML.compare("\\n\\r"))
        return "\n\r";
    else
    {
        std::stringstream error;
        error << "PowerSupply configuration: string " << strXML
              << " is not understood, please check keeping in mind that the only available options are: \"\\n\" \"\\r\" \"\\n\\r\" \"\\r\\n\" \"LF\" \"CR\" \"CRLF\" \"LFCR\", aborting...";
        throw std::runtime_error(error.str());
    }
}

/*!
************************************************
* Some power supplies might not need to be turned on, so providing a default
*that does nothing
************************************************
*/
bool PowerSupply::turnOn(void) { return true; }

/*!
************************************************
* Some power supplies might not need to be turned off, so providing a default
*that does nothing
************************************************
*/
bool PowerSupply::turnOff(void) { return true; }

/*!
************************************************
* Some power supplies might not need to be reset,
* so providing a default that does nothing
************************************************
*/
void PowerSupply::reset(void) { ; }

/*!
************************************************
************************************************
*/
void PowerSupply::turnOnChannels()
{
    for(auto& channel: fChannelMap) channel.second->turnOn();
}

/*
************************************************
************************************************
*/
void PowerSupply::turnOffChannels()
{
    for(auto& channel: fChannelMap) channel.second->turnOff();
}

/*!
************************************************
************************************************
*/
std::vector<std::string> PowerSupply::getChannelList(void) const { return fChannelList; }

/*!
************************************************
************************************************
*/
std::string PowerSupply::getModel() const { return fModel; }

/*!
************************************************
************************************************
*/
void PowerSupply::setPowerType(std::string val) { fPowerType = val; }

/*!
************************************************
************************************************
*/
std::string PowerSupply::getPowerType(void) const { return fPowerType; }

/*!
************************************************
************************************************
*/
std::string PowerSupply::getInfo(){ return "emmm";}