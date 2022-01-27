/*!
 * \authors Mattia Lizzo <mattia.lizzo@cern.ch>, INFN-Firenze
 * \authors Antonio Cassese <antonio.cassese@cern.ch>, INFN-Firenze
 * \date Sep 2 2019
 */

#include "CAEN.h"

#include <bitset>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
using namespace std;
#include <sstream>
#include <stdexcept>

/*!
************************************************
 * Class constructor.
 \param IPaddress IP address of the PS for
 ethernet connection.
 \param port Connection port.
************************************************
*/

CAEN::CAEN(const pugi::xml_node configuration) : PowerSupply("CAEN", configuration), fSystemHandle(-1) { configure(); }

CAEN::~CAEN()
{
    CAENHVRESULT answer = CAENHV_DeinitSystem(fSystemHandle);
    if(answer == CAENHV_OK)
        std::cout << "CAENHV_DeinitSystem: Connection closed (num. " << answer << ")" << std::endl;
    else
        std::cout << "CAENHV_DeinitSystem: " << CAENHV_GetError(fSystemHandle) << " (num. " << answer << ")" << std::endl;
}

void CAEN::configure()
{
    std::cout << "Configuring CAEN ..." << std::endl;
    char ipAddress[30];
    strcpy(ipAddress, fConfiguration.attribute("IPAddress").value());
    int link    = LINKTYPE_TCPIP; // Go in the configuration?
    int sysType = 3;              // Go in the configuration?

    CAENHVRESULT answer = CAENHV_InitSystem((CAENHV_SYSTEM_TYPE_t)sysType, link, ipAddress, fConfiguration.attribute("UserName").value(), fConfiguration.attribute("Password").value(), &fSystemHandle);
    if(answer != CAENHV_OK)
    {
        std::stringstream error;
        error << "CAENHV_InitSystem: " << CAENHV_GetError(fSystemHandle) << " (num. " << answer << ")";
        throw std::runtime_error(error.str());
    }
    for(pugi::xml_node channel = fConfiguration.child("Channel"); channel; channel = channel.next_sibling("Channel"))
    {
        std::string inUse = channel.attribute("InUse").value();
        if(inUse.compare("Yes") != 0 && inUse.compare("yes") != 0) continue;

        std::string id = channel.attribute("ID").value();
        PowerSupply::fChannelMap.emplace(id, new CAENChannel(fSystemHandle, channel));
    }
}

/*!
************************************************
 * Clear alarms preventing to switch on channels.
 \brief Clear alarms preventing to switch on channels.
************************************************
*/
void CAEN::clearAlarm() { CAENHV_ExecComm(fSystemHandle, "ClearAlarm"); }

CAENChannel::CAENChannel(int systemHandle, const pugi::xml_node configuration)
    : PowerSupplyChannel(configuration), fSystemHandle(systemHandle), fSlot(atoi(configuration.attribute("Slot").value())), fChannel(atoi(configuration.attribute("Channel").value()))
{
    setName(fId);
    if(!fId.find("HV")) // configurations valid only for HV power supply
    {
        setParameter("RUp", std::stof(configuration.attribute("RUp").value()));
        setParameter("RDWn", std::stof(configuration.attribute("RDwn").value()));
    }
    if(!fId.find("LV")) // configurations valid only for LV power supply
    {
        setParameter("RUpTime", std::stof(configuration.attribute("RUpTime").value()));
        setParameter("RDwTime", std::stof(configuration.attribute("RDwTime").value()));
        setParameter("UNVThr", std::stof(configuration.attribute("UNVThr").value()));
        setParameter("OVVThr", std::stof(configuration.attribute("OVVThr").value()));
    }
    setParameter("Trip", std::stof(configuration.attribute("Trip").value()));
    setParameter("I0Set", std::stof(configuration.attribute("I0Set").value()));
    setParameter("V0Set", std::stof(configuration.attribute("V0Set").value()));
}

CAENChannel::~CAENChannel() {}

/*!
 ************************************************
 * Turns on the channel output driver, enabling
 * the output current terminals and allowing
 * the power supply to regulate and feed
 * current or voltage to the connected load.
 * Authomatically sets output current (voltage
 * to 0 A (V).
 \brief Turns on the channel.
************************************************
*/
void CAENChannel::turnOn(void) { setParameter("Pw", true); }

/*!
************************************************
 * Turns off the channel output driver,
 * disabling the output terminals.
 \brief Turns off the channel.
************************************************
*/
void CAENChannel::turnOff(void) { setParameter("Pw", false); }

/*!
************************************************
 * Checks if the channel is on or off.
 \brief Checks if the channel is on or off.
 \return true if channel is on, false if channel is off
************************************************
*/
bool CAENChannel::isOn(void) { return getParameterBool("Pw"); }

/*!
************************************************
 * Set the channel voltage.
 \brief Set the channel volatge.
************************************************
*/
void CAENChannel::setVoltage(float voltage) { setParameter("V0Set", voltage); }

/*!
************************************************
 * Set the channel current compliance.
 \brief Set the channel current compliance.
************************************************
*/
void CAENChannel::setCurrent(float current) { setParameter("I0Set", current); }

/*!
************************************************
 * Set the channel voltage.
 \brief Set the channel voltage.
************************************************
*/
void CAENChannel::setVoltageCompliance(float voltage) { setVoltage(voltage); }

/*!
************************************************
 * Set the channel current compliance.
 \brief Set the channel current compliance.
************************************************
*/
void CAENChannel::setCurrentCompliance(float current) { setCurrent(current); }

/*!
************************************************
 * Set the channel over voltage protection
 * limit.
 \brief Set the channel over voltage protection
limit.
************************************************
*/
void CAENChannel::setOverVoltageProtection(float voltage) { setParameter("OVVThr", voltage); }

/*!
************************************************
 * Set the channel over current protection
 * limit.
 \brief Set the channel over current protection
limit.
************************************************
*/
void CAENChannel::setOverCurrentProtection(float current)
{
    std::stringstream error;
    error << "CAEN setOverCurrentProtection not defined, aborting...";
    throw std::runtime_error(error.str());
}

/*!
************************************************
 * Get the channel output voltage.
 \brief Get the channel output voltage.
 \return The channel voltage value
************************************************
*/
float CAENChannel::getOutputVoltage(void) { return getParameterFloat("VMon"); }

/*!
************************************************
 * Get the channel set voltage.
 \brief Get the channel set voltage.
 \return The channel voltage value
************************************************
*/
float CAENChannel::getSetVoltage(void) { return getParameterFloat("V0Set"); }

/*!
************************************************
 * Get the channel current.
 \brief Get the channel current.
 \return The channel current value.
************************************************
*/
float CAENChannel::getCurrent(void) { return getParameterFloat("IMon"); }

/*!
************************************************
 * Get the channel voltage.
 \brief Get the channel voltage.
************************************************
*/
float CAENChannel::getVoltageCompliance(void) { return getParameterFloat("V0Set"); }

/*!
************************************************
 * Get the channel current compliance.
 \brief Get the channel current compliance.
************************************************
*/
float CAENChannel::getCurrentCompliance(void) { return getParameterFloat("I0Set"); }

/*!
************************************************
 * Get the channel over voltage protection
 * limit.
 \brief Get the channel over voltage protection
 limit.
 \return The channel over voltage protection
 limit value.
************************************************
*/
float CAENChannel::getOverVoltageProtection(void) { return getParameterFloat("OVVThr"); }

/*!
************************************************
 * Get the channel over current protection
 * limit.
 \brief Get the channel over current protection
 limit.
 \return The channel over current protection
 limit value.
************************************************
*/
float CAENChannel::getOverCurrentProtection(void)
{
    std::stringstream error;
    error << "CAEN getOverCurrentProtection not defined, aborting...";
    throw std::runtime_error(error.str());
}

/*!
************************************************
 * Get the channel status according to manual HV_Wrapper.
 \brief Get the channel status.
 \return Channel status.
************************************************
*/
unsigned CAENChannel::getChannelStatus() { return getParameterUnsigned("Status"); }

/*!
************************************************
 * Tells whether the channel is ramping up.
 \brief Get ramp up status.
 \return Ramp up status.
************************************************
*/
bool CAENChannel::isChannelRampingUp() { return getChannelStatus() >> 1; }

/*!
************************************************
 * Tells whether the channel is ramping down.
 \brief Get ramp down status.
 \return Ramp down status.
************************************************
*/
bool CAENChannel::isChannelRampingDown() { return getChannelStatus() >> 2; }

float CAENChannel::getParameterFloat(std::string parName)
{
    float value = -9999;
    checkAnswer(CAENHV_GetChParam(fSystemHandle, fSlot, parName.c_str(), 1, &fChannel, &value));
    return value;
}

unsigned CAENChannel::getParameterUnsigned(std::string parName)
{
    unsigned value = -9999;
    checkAnswer(CAENHV_GetChParam(fSystemHandle, fSlot, parName.c_str(), 1, &fChannel, &value));
    return value;
}

int CAENChannel::getParameterInt(std::string parName)
{
    int value = -9999;
    checkAnswer(CAENHV_GetChParam(fSystemHandle, fSlot, parName.c_str(), 1, &fChannel, &value));
    return value;
}

bool CAENChannel::getParameterBool(std::string parName)
{
    bool value = false;
    checkAnswer(CAENHV_GetChParam(fSystemHandle, fSlot, parName.c_str(), 1, &fChannel, &value));
    return value;
}

void CAENChannel::setParameter(std::string parName, float value) { checkAnswer(CAENHV_SetChParam(fSystemHandle, fSlot, parName.c_str(), 1, &fChannel, &value)); }

void CAENChannel::setParameter(std::string parName, bool value) { setParameter(parName, (int)value); }

void CAENChannel::setParameter(std::string parName, int value) { checkAnswer(CAENHV_SetChParam(fSystemHandle, fSlot, parName.c_str(), 1, &fChannel, &value)); }

void CAENChannel::setName(std::string name)
{
    char value[12];
    checkAnswer(CAENHV_GetChName(fSystemHandle, fSlot, 1, &fChannel, &value));
    if(name.compare(value) != 0) checkAnswer(CAENHV_SetChName(fSystemHandle, fSlot, 1, &fChannel, name.c_str()));
}

/*!
************************************************
 * Checks if answer is a valid answer, an error
 * or something unknown.
 * Throws an exception if the answer is an error.
 \param const CAENHVRESULT& answer to be checked.
************************************************
*/
void CAENChannel::checkAnswer(const CAENHVRESULT& answer)
{
    if(answer != CAENHV_OK)
    {
        std::stringstream error;
        std::string       errorLevel  = "ERROR";
        std::string       errorSource = "CHANNEL";
        CAENHVRESULT      errorNumber = answer;
        if(answer < 0)
        {
            errorSource = "SUPPLY";
            errorNumber *= -1;
        }
        if(answer == 0x14) { errorLevel = "WARNING"; }
        error << "[" << __LINE__ << "] " << __PRETTY_FUNCTION__ << " CAEN " << errorSource << " " << errorLevel << ": " << CAENHV_GetError(fSystemHandle) << " (num. " << std::hex << "0x"
              << errorNumber << std::dec << ")";
        if(errorLevel.compare("ERROR") == 0) { throw std::runtime_error(error.str()); }
        else
        {
            std::cout << error.str() << std::endl;
        }
    }
}
