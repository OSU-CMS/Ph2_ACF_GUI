#include "Arduino.h"
//#include "PowerSupplyChannel.h"

/*!
************************************************
* PowerSupply constructor.
************************************************
*/
Arduino::Arduino(std::string model, const pugi::xml_node configuration) : fModel(model), fConfiguration(configuration) { ; }

/*!
************************************************
* PowerSupply distructor.
************************************************
*/
Arduino::~Arduino(void)
{
    // for(auto it: fChannelMap) delete it.second;
    // fChannelMap.clear();
}

/*!
************************************************
 * Get pointer to channel by id
 \param id Unique channel id
 \return Pointer to channel
************************************************
*/
/*
PowerSupplyChannel* PowerSupply::getChannel(const std::string& id)
{
    if(fChannelMap.find(id) == fChannelMap.end()) { throw std::out_of_range("No channel with id " + id + " has been configured!"); }
    return fChannelMap.find(id)->second;
}
*/

/*!
************************************************
************************************************
*/

/*!
************************************************
* Some power supplies might not need to be turned on, so providing a default
*that does nothing
************************************************
*/
bool Arduino::turnOn(void) { return true; }

/*!
************************************************
* Some power supplies might not need to be turned off, so providing a default
*that does nothing
************************************************
*/
bool Arduino::turnOff(void) { return true; }

/*!
************************************************
************************************************
*/

float Arduino::getParameterFloat(std::string parName) { return 0; }

std::string Arduino::getModel() const { return fModel; }

/*!
************************************************
************************************************
*/
std::string Arduino::getID(void) const { return fId; }

/*!
************************************************
************************************************
*/

// void PowerSupply::setPowerType(std::string val) { fPowerType = val; }

/*!
************************************************
************************************************
*/
// std::string PowerSupply::getPowerType(void) const { return fPowerType; }
