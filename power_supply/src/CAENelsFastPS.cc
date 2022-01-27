#include "CAENelsFastPS.h"

#include <bitset>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>

/*!
************************************************
 * Class constructor.
 \param IPaddress IP address of the PS for
 ethernet connection.
 \param TCP TCP port for ethernet connection.
************************************************
*/

CAENelsFastPS::CAENelsFastPS(const pugi::xml_node configuration)
    : PowerSupply("CAENelsFastPS", configuration), fEthernet(configuration.attribute("IPAddress").value(), atoi(configuration.attribute("Port").value()))
{
    std::stringstream error;
    error << "CAENelsFastPS initialization: the new version of the code has not "
             "be tested and probably have unexpected "
             "behavoiur. Use it at your own risk.";
    throw std::runtime_error(error.str());
}

CAENelsFastPS::~CAENelsFastPS() {}

void CAENelsFastPS::configure()
{
    for(pugi::xml_node channel = fConfiguration.child("Channel"); channel; channel = channel.next_sibling("Channel"))
    {
        std::string inUse = channel.attribute("InUse").value();
        if(inUse.compare("Yes") != 0 && inUse.compare("yes") != 0) continue;

        std::string id = channel.attribute("ID").value();
        PowerSupply::fChannelMap.emplace(id, new CAENelsFastPSChannel(&fEthernet, channel));
    }
}

/*!
************************************************
 \brief Checks if ethernet connection is open.
 \return Ethernet connection status.
************************************************
*/

bool CAENelsFastPS::isOpen() { return fEthernet.isOpen(); }

CAENelsFastPSChannel::CAENelsFastPSChannel(EthernetConnection* ethernet, const pugi::xml_node configuration) : PowerSupplyChannel(configuration), fEthernet(ethernet) { fAns.clear(); }

CAENelsFastPSChannel::~CAENelsFastPSChannel() {}

/*!
************************************************
 * Searches for error code description as in
 * manual.
 \param errStr String with the error code.
************************************************
*/

void CAENelsFastPSChannel::elaborateError(const std::string& errStr)
{
    int numErr = stoi(errStr.substr(5, errStr.size() - 2));
    std::cout << "Error code " << numErr << ":\t";
    --numErr;
    std::cout << errorList[numErr] << std::endl;
}

/*!
************************************************
 * Checks if str is a valid answer, an error
 * or something unknown.
 \param str String to be checked.
 \param strAns Starting string foreseen in the
 answer. Default is "AK". "#" is authomatically
 added at the beginning.
 \return False if error or unknown answer.
************************************************
*/

bool CAENelsFastPSChannel::checkAns(const std::string& str, const std::string strAns = "AK")
{
    if(str.rfind("#NAK", 0) == 0)
    {
        elaborateError(str);
        return false;
    }
    else if(str.rfind("#" + strAns, 0) == 0)
    {
        return true;
    }
    std::cout << "Unknown answer from PS:" << std::endl << fAns << std::endl;
    return false;
}

bool CAENelsFastPSChannel::extractValue(const std::string& strAns, unsigned int nSkip, float& result)
{
    if(strAns.length() <= nSkip) return false;
    result = std::stof(strAns.substr(nSkip));
    return true;
}

/*!
************************************************
 * Reset function.
 * After resetting, CAEN power supply can be
 * turned on again.
 * Turning it off after a reset will cause an
 * error.
 \return False if error or no message back is
 received.
************************************************
*/

void CAENelsFastPSChannel::reset()
{
    fAns.clear();
    fAns = fEthernet->read("MRESET\r");
    if(checkAns(fAns))
    {
        fAns.clear();
        fAns = fEthernet->read("UPMODE:NORMAL\r");
    }
    if(!checkAns(fAns)) throw std::runtime_error(fAns);
}

bool CAENelsFastPSChannel::isOn()
{
    std::cout << "Make sure that the channel is on" << std::endl;
    return true;
}

/*!
 ************************************************
 * Turns on the module output driver, enabling
 * the output current terminals and allowing
 * the power supply to regulate and feed
 * current or voltage to the connected load.
 * Authomatically sets output current (voltage
 * to 0 A (V).
 \brief Turns on power supply.
 \return True if PS is turned on.
************************************************
*/

void CAENelsFastPSChannel::turnOn()
{
    fAns.clear();
    fAns = fEthernet->read("MON\r");
    // return checkAns(fAns);
}

/*!
************************************************
 * Turns off the module output driver,
 * disabling the output terminals.
 \brief Turns off power supply.
 \return True if PS is turned off.
************************************************
*/

void CAENelsFastPSChannel::turnOff()
{
    fAns.clear();
    fAns = fEthernet->read("MOFF\r");
    // return checkAns(fAns);
}

/*!
************************************************
 * Asks for status register, converts to binary
 * and return.
 \return Binary string of the status register.
************************************************
*/

const std::string CAENelsFastPSChannel::bitStatusRegister()
{
    fAns.clear();
    fAns = fEthernet->read("MST\r");
    std::string       finalStr;
    std::stringstream ss;
    unsigned          n;
    if(checkAns(fAns, "MST"))
    {
        ss << std::hex << fAns.substr(5, fAns.length() - 2);
        ss >> n;
        std::bitset<32> b(n);
        std::string     auxStr = b.to_string();
        for(unsigned int v = 0; v < auxStr.length(); ++v) finalStr.push_back(auxStr[auxStr.length() - 1 - v]);
    }
    return finalStr;
}

/*!
************************************************
 * Expands status register and checks if
 * Over Power condition has occured.
 \brief Over Power condition has occured.
 \param strStatusRegister Takes as input the
 status register string in bit as it comes from
 CAENelsFastPSChannel::bitStatusRegister().
 \return True if Over Power condition has
 occured.
************************************************
*/

bool CAENelsFastPSChannel::statusRegisterOverPower(const std::string& strStatusRegister)
{
    if(strStatusRegister[29] == '1') return true;
    return false;
}

/*!
************************************************
 * Expands status register and checks if
 * External Interlock #2 has tripped.
 \brief External Interlock #2 has tripped.
 \param strStatusRegister Takes as input the
 status register string in bit as it comes from
 CAENelsFastPSChannel::bitStatusRegister().
 \return True if External Interlock #2 has
 tripped.
************************************************
*/

bool CAENelsFastPSChannel::statusRegisterExternalInterlock2(const std::string& strStatusRegister)
{
    if(strStatusRegister[27] == '1') return true;
    return false;
}

/*!
************************************************
 * Expands status register and checks if
 * External Interlock #1 has tripped.
 \brief External Interlock #1 has tripped.
 \param strStatusRegister Takes as input the
 status register string in bit as it comes from
 CAENelsFastPSChannel::bitStatusRegister().
 \return True if External Interlock #1 has
 tripped.
************************************************
*/

bool CAENelsFastPSChannel::statusRegisterExternalInterlock1(const std::string& strStatusRegister)
{
    if(strStatusRegister[26] == '1') return true;
    return false;
}

/*!
************************************************
 * Expands status register and checks if
 * module is having excessive ripple.
 \brief Module is having excessive ripple.
 \param strStatusRegister Takes as input the
 status register string in bit as it comes from
 CAENelsFastPSChannel::bitStatusRegister().
 \return True if module is having excessive
 ripple.
************************************************
*/

bool CAENelsFastPSChannel::statusRegisterExcessiveRipple(const std::string& strStatusRegister)
{
    if(strStatusRegister[25] == '1') return true;
    return false;
}

/*!
************************************************
 * Expands status register and checks if
 * module has experienced a regulation fault.
 \brief Module has experienced a regulation
 fault.
 \param strStatusRegister Takes as input the
 status register string in bit as it comes from
 CAENelsFastPSChannel::bitStatusRegister().
 \return True if module has experienced a
 regulation fault.
************************************************
*/

bool CAENelsFastPSChannel::statusRegisterRegulationFault(const std::string& strStatusRegister)
{
    if(strStatusRegister[24] == '1') return true;
    return false;
}

/*!
************************************************
 * Expands status register and checks if
 * earth fuse is blown.
 \brief Earth fuse blown.
 \param strStatusRegister Takes as input the
 status register string in bit as it comes from
 CAENelsFastPSChannel::bitStatusRegister().
 \return True if module is blown.
************************************************
*/

bool CAENelsFastPSChannel::statusRegisterEarthFuse(const std::string& strStatusRegister)
{
    if(strStatusRegister[23] == '1') return true;
    return false;
}

/*!
************************************************
 * Expands status register and checks if
 * earth current leakage fault.
 \brief Earth current leakage fault.
 \param strStatusRegister Takes as input the
 status register string in bit as it comes from
 CAENelsFastPSChannel::bitStatusRegister().
 \return True if current leakage fault.
************************************************
*/

bool CAENelsFastPSChannel::statusRegisterEarthLeakage(const std::string& strStatusRegister)
{
    if(strStatusRegister[22] == '1') return true;
    return false;
}

/*!
************************************************
 * Expands status register and checks if
 * DC-link is in under-voltage condition.
 \brief DC-Link in under-voltage condition.
 \param strStatusRegister Takes as input the
 status register string in bit as it comes from
 CAENelsFastPSChannel::bitStatusRegister().
 \return True if DC-Link is in under-voltage
 condition.
************************************************
*/

bool CAENelsFastPSChannel::statusRegisterDCLinkFault(const std::string& strStatusRegister)
{
    if(strStatusRegister[21] == '1') return true;
    return false;
}

/*!
************************************************
 * Expands status register and checks if
 * Over Temperature condition has occurred.
 \brief Over Temperature condition occurred.
 \param strStatusRegister Takes as input the
 status register string in bit as it comes from
 CAENelsFastPSChannel::bitStatusRegister().
 \return True if Over Temperature condition has
 occurred.
************************************************
*/

bool CAENelsFastPSChannel::statusRegisterOverTemperature(const std::string& strStatusRegister)
{
    if(strStatusRegister[20] == '1') return true;
    return false;
}

/*!
************************************************
 * Expands status register and checks if
 * crowbar protection intervened.
 \brief Corwbar protection intervention.
 \param strStatusRegister Takes as input the
 status register string in bit as it comes from
 CAENelsFastPSChannel::bitStatusRegister().
 \return True if crowbar protection intervened.
************************************************
*/

bool CAENelsFastPSChannel::statusRegisterCrowbarProtection(const std::string& strStatusRegister)
{
    if(strStatusRegister[18] == '1') return true;
    return false;
}

/*!
************************************************
 * Expands status register and checks if
 * input over current has occurred.
 \brief Input over current.
 \param strStatusRegister Takes as input the
 status register string in bit as it comes from
 CAENelsFastPSChannel::bitStatusRegister().
 \return True if input over current has
 occurred.
************************************************
*/

bool CAENelsFastPSChannel::statusRegisterInputOverCurrent(const std::string& strStatusRegister)
{
    if(strStatusRegister[17] == '1') return true;
    return false;
}

/*!
************************************************
 * Expands status register and checks if
 * waveform is in execution.
 \brief Waveform is in execution.
 \param strStatusRegister Takes as input the
 status register string in bit as it comes from
 CAENelsFastPSChannel::bitStatusRegister().
 \return True if waveform is in execution
************************************************
*/

bool CAENelsFastPSChannel::statusRegisterWaveformExecution(const std::string& strStatusRegister)
{
    if(strStatusRegister[13] == '1') return true;
    return false;
}

/*!
************************************************
 * Expands status register and checks if
 * module is ramping current or voltage.
 \brief Module ramping.
 \param strStatusRegister Takes as input the
 status register string in bit as it comes from
 CAENelsFastPSChannel::bitStatusRegister().
 \return True if module is ramping current or
 voltage.
************************************************
*/

bool CAENelsFastPSChannel::statusRegisterRamping(const std::string& strStatusRegister)
{
    if(strStatusRegister[12] == '1') return true;
    return false;
}

/*!
************************************************
 * Expands status register and checks if
 * the update mode is normal.
 \brief Update mode is normal.
 \param strStatusRegister Takes as input the
 status register string in bit as it comes from
 CAENelsFastPSChannel::bitStatusRegister().
 \return True if normal mode.
************************************************
*/

bool CAENelsFastPSChannel::statusRegisterNormalUpdateMode(const std::string& strStatusRegister)
{
    if(strStatusRegister.substr(6, 2) == "00") return true;
    return false;
}

/*!
************************************************
 * Expands status register and checks if
 * the update mode is from analog input.
 \brief Update mode from analog input.
 \param strStatusRegister Takes as input the
 status register string in bit as it comes from
 CAENelsFastPSChannel::bitStatusRegister().
 \return True if analog input.
************************************************
*/

bool CAENelsFastPSChannel::statusRegisterAnalogInputUpdateMode(const std::string& strStatusRegister)
{
    if(strStatusRegister.substr(6, 2) == "11") return true;
    return false;
}

/*!
************************************************
 * Expands status register and checks output
 * regulation mode, i.e. conctant current or
 * constant voltage.
 \brief Output regulation mode.
 \param strStatusRegister Takes as input the
 status register string in bit as it comes from
 CAENelsFastPSChannel::bitStatusRegister().
 \return True if constant voltage regulation.
************************************************
*/

bool CAENelsFastPSChannel::statusRegisterRegulationMode(const std::string& strStatusRegister)
{
    if(strStatusRegister[5] == '1') return true;
    return false;
}

/*!
************************************************
 * Expands status register and checks if
 * control mode is remote.
 \brief Remote control mode.
 \param strStatusRegister Takes as input the
 status register string in bit as it comes from
 CAENelsFastPSChannel::bitStatusRegister().
 \return True if remote control mode.
************************************************
*/

bool CAENelsFastPSChannel::statusRegisterRemoteControlMode(const std::string& strStatusRegister)
{
    if(strStatusRegister.substr(2, 2) == "00") return true;
    return false;
}

/*!
************************************************
 * Expands status register and checks if
 * control mode is local.
 \brief Local control mode.
 \param strStatusRegister Takes as input the
 status register string in bit as it comes from
 CAENelsFastPSChannel::bitStatusRegister().
 \return True if local control mode.
************************************************
*/

bool CAENelsFastPSChannel::statusRegisterLocalControlMode(const std::string& strStatusRegister)
{
    if(strStatusRegister.substr(2, 2) == "01") return true;
    return false;
}

/*!
************************************************
 * Expands status register and checks if
 * the module has experienced a fault condition.
 \brief Fault condition occurred.
 \param strStatusRegister Takes as input the
 status register string in bit as it comes from
 CAENelsFastPSChannel::bitStatusRegister().
 \return True if any fault condition occurred.
************************************************
*/

bool CAENelsFastPSChannel::statusRegisterFaultCondition(const std::string& strStatusRegister)
{
    if(strStatusRegister[1] == '1') return true;
    return false;
}

/*!
************************************************
 * Expands status register and checks if
 * the module is enabled and correctly
 * regulating the output.
 \brief Module on.
 \param strStatusRegister Takes as input the
 status register string in bit as it comes from
 CAENelsFastPSChannel::bitStatusRegister().
 \return True if the module is on.
************************************************
*/

bool CAENelsFastPSChannel::statusRegisterModuleOn(const std::string& strStatusRegister)
{
    if(strStatusRegister[0] == '1') return true;
    return false;
}

/*!
************************************************
 \brief Set PS as voltage regulator.
 \return True if properly set.
************************************************
*/

bool CAENelsFastPSChannel::setVoltageMode()
{
    fAns.clear();
    fAns = fEthernet->read("LOOP:V\r");
    if(!checkAns(fAns)) { std::cout << "Couldn't set Voltage mode" << std::endl; }
    return checkAns(fAns);
}

/*!
************************************************
 \brief Set PS as current regulator.
 \return True if properly set.
************************************************
*/

bool CAENelsFastPSChannel::setCurrentMode()
{
    fAns.clear();
    fAns = fEthernet->read("LOOP:I\r");
    if(!checkAns(fAns)) std::cout << "Couldn't set Current mode" << std::endl;
    return checkAns(fAns);
}

/*!
************************************************
 \brief Don't use it: it prints error and
 return false.
 \return Always false.
************************************************
*/

bool CAENelsFastPSChannel::setVoltageRange(float voltage)
{
    std::cout << "setVoltageRange function is not available for CAENels Fast PS" << std::endl;
    return false;
}

/*!
************************************************
 \brief Don't use it: it prints error and
 return false.
 \return Always false.
************************************************
*/

bool CAENelsFastPSChannel::setCurrentRange(float current)
{
    std::cout << "setCurrentRange function is not available for CAENels Fast PS" << std::endl;
    return false;
}

/*!
************************************************
 * Check if modules is in voltage mode.
 \return Returns true if module is in voltage
 mode.
************************************************
*/

bool CAENelsFastPSChannel::isInVoltageMode()
{
    fAns.clear();
    fAns = fEthernet->read("LOOP:?\r");
    if(fAns == "#LOOP:V\r\n") return true;
    return false;
}

/*!
************************************************
 * Check if modules is in current mode.
 \return Returns true if module is in current
 mode.
************************************************
*/

bool CAENelsFastPSChannel::isInCurrentMode()
{
    fAns.clear();
    fAns = fEthernet->read("LOOP:?\r");
    if(fAns == "#LOOP:I\r\n") return true;
    return false;
}

/*!
************************************************
 * Sets voltage if module is in voltage mode
 * both for direct and ramp mode.
 \param current Voltage in voltage units.
 \param strMode Choose between "direct" and
 "ramp".
 \return False if voltage is not set.
************************************************
*/

bool CAENelsFastPSChannel::setVoltageMode(float voltage, const std::string& strMode)
{
    std::string strCommand;
    if(strMode == "direct")
        strCommand = "MWV:";
    else if(strMode == "ramp")
        strCommand = "MWVR:";
    else
    {
        std::cout << "Wrong strMode string in setVoltage: " << strMode << ". Choose between direct and ramp." << std::endl;
        return false;
    }
    if(isInVoltageMode())
    {
        fAns.clear();
        fAns = fEthernet->read(strCommand + std::to_string(voltage) + "\r");
        if(checkAns(fAns)) return true;
    }
    return false;
}

/*!
************************************************
 * Sets current if module is in current mode
 * both for direct and ramp mode.
 \param current Current in Ampere units.
 \param strMode Choose between "direct" and
 "ramp".
 \return False if current is not set.
************************************************
*/

bool CAENelsFastPSChannel::setCurrentMode(float current, const std::string& strMode)
{
    std::string strCommand;
    if(strMode == "direct")
        strCommand = "MWI:";
    else if(strMode == "ramp")
        strCommand = "MWIR:";
    else
    {
        std::cout << "Wrong strMode string in setCurrent: " << strMode << ". Choose between direct and ramp." << std::endl;
        return false;
    }
    if(isInCurrentMode())
    {
        fAns.clear();
        fAns = fEthernet->read(strCommand + std::to_string(current) + "\r");
        if(checkAns(fAns)) return true;
    }
    return false;
}

/*!
************************************************
 * Sets voltage if module is in voltage mode
 * for direct mode.
 \param voltage Voltage in Volt units.
 \return False if voltage is not set.
************************************************
*/

void CAENelsFastPSChannel::setVoltage(float voltage) { setVoltageMode(voltage, "direct"); }

/*!
************************************************
 * Sets current if module is in current mode
 * for direct mode.
 \param current Current in Ampere units.
 \return False if current is not set.
************************************************
*/

void CAENelsFastPSChannel::setCurrent(float current) { setCurrentMode(current, "direct"); }

/*!
************************************************
 * Sets voltage if module is in voltage mode
 * for direct mode.
 \param voltage Voltage in Volt units.
 \return False if voltage is not set.
************************************************
*/

void CAENelsFastPSChannel::setVoltageCompliance(float voltage)
{
    std::cout << "Warning, this methods actually calls setVoltage, consider to "
                 "rewrite it."
              << std::endl;
    setVoltage(voltage);
}

/*!
************************************************
 * Sets current if module is in current mode
 * for direct mode.
 \param current Current in Ampere units.
 \return False if current is not set.
************************************************
*/

void CAENelsFastPSChannel::setCurrentCompliance(float current)
{
    std::cout << "Warning, this methods actually calls setCurrent, consider to "
                 "rewrite it."
              << std::endl;
    setCurrent(current);
}

/*!
************************************************
 * Sets voltage ramp if module is in voltage
 * mode.
 \param slewRate Slew rate in Volt/seconds.
 \return False if slew rate not set.
************************************************
*/

bool CAENelsFastPSChannel::setVoltageSlewRate(float slewRate)
{
    if(isInVoltageMode())
    {
        fAns.clear();
        fAns = fEthernet->read("MSRV:" + std::to_string(slewRate) + "\r");
        return checkAns(fAns);
    }
    return false;
}

/*!
************************************************
 * Sets current ramp if module is in current
 * mode.
 \param slewRate Slew rate in Ampere/seconds.
 \return False if slew rate not set.
************************************************
*/

bool CAENelsFastPSChannel::setCurrentSlewRate(float slewRate)
{
    if(isInCurrentMode())
    {
        fAns.clear();
        fAns = fEthernet->read("MSRI:" + std::to_string(slewRate) + "\r");
        return checkAns(fAns);
    }
    return false;
}

/*!
************************************************
 * Sets voltage if module is in voltage mode
 * for ramp mode.
 \param voltage Voltage in Volt units.
 \return False if voltage not set.
************************************************
*/

bool CAENelsFastPSChannel::setVoltageWithRamp(float voltage, float slewRate)
{
    if(isInVoltageMode())
    {
        if(setVoltageSlewRate(slewRate)) return setVoltageMode(voltage, "ramp");
    }
    return false;
}

/*!
************************************************
 * Sets current if module is in current mode
 * for ramp mode.
 \param current Current in Ampere units.
 \return False if current not set.
************************************************
*/

bool CAENelsFastPSChannel::setCurrentWithRamp(float current, float slewRate)
{
    if(isInCurrentMode())
    {
        if(setCurrentSlewRate(slewRate)) return setCurrentMode(current, "ramp");
    }
    return false;
}

/*!
************************************************
 * Returns readback value of the power supply
 * actual output voltage in Volt and with
 * 6-digit precision.
 \return Actual output voltage or -9999 if
 error occurs.
************************************************
*/

float CAENelsFastPSChannel::getOutputVoltage()
{
    fAns.clear();
    float result;
    fAns = fEthernet->read("MRV\r");
    if(!checkAns(fAns, "MRV") || !extractValue(fAns, 5, result)) result = -9999;
    return result;
}

float CAENelsFastPSChannel::getSetVoltage()
{
    std::stringstream error;
    error << "CAENelsFastPS getSetVoltage() not defined, aborting...";
    throw std::runtime_error(error.str());
}

/*!
************************************************
 * Returns readback value of the power supply
 * actual output current in Ampere and with
 * 6-digit precision.
 \return Actual output current or -9999 if
 error occurs.
************************************************
*/

float CAENelsFastPSChannel::getCurrent()
{
    fAns.clear();
    float result;
    fAns = fEthernet->read("MRI\r");
    if(!checkAns(fAns, "MRI") || !extractValue(fAns, 5, result)) result = -9999;
    return result;
}

/*!
************************************************
 * Returns readback value of the power supply
 * actual output voltage in Volt and with
 * 6-digit precision.
 \return Actual output voltage or -9999 if
 error occurs.
************************************************
*/

float CAENelsFastPSChannel::getVoltageCompliance()
{
    std::cout << "Warning, this methods actually calls getOutputVoltage, consider to "
                 "rewrite it."
              << std::endl;
    return getOutputVoltage();
}

/*!
************************************************
 * Returns readback value of the power supply
 * actual output current in Ampere and with
 * 6-digit precision.
 \return Actual output current or -9999 if
 error occurs.
************************************************
*/

float CAENelsFastPSChannel::getCurrentCompliance()
{
    std::cout << "Warning, this methods actually calls getCurrent, consider to "
                 "rewrite it."
              << std::endl;
    return getCurrent();
}

/*!
************************************************
 \brief Don't use it: it prints error and
 return false.
 \return Always false.
************************************************
*/

void CAENelsFastPSChannel::setOverVoltageProtection(float maxVoltage) { std::cout << "setVoltageLimit function is not available for CAENels Fast PS" << std::endl; }

/*!
************************************************
 \brief Don't use it: it prints error and
 return false.
 \return Always false.
************************************************
*/

void CAENelsFastPSChannel::setOverCurrentProtection(float maxCurrent) { std::cout << "setCurrentLimit function is not available for CAENels Fast PS" << std::endl; }

/*!
************************************************
 \brief Not useful: voltage limit is fixed.
 \return Construction voltage limit.
************************************************
*/

float CAENelsFastPSChannel::getOverVoltageProtection()
{
    float result = 20;
    std::cout << "CAENels Fast PS voltage limit is always set at " << result << " V" << std::endl;
    return result;
}

/*!
************************************************
 \brief Not useful: current limit is fixed.
 \return Construction current limit.
************************************************
*/

float CAENelsFastPSChannel::getOverCurrentProtection()
{
    float result = 10;
    std::cout << "CAENels Fast PS current limit is always set at " << result << " A" << std::endl;
    return result;
}

// TODO Control the two following. - Antonio Nov 28 2019 14:40

bool CAENelsFastPSChannel::isVoltTripped() { return (getOutputVoltage() > getOverVoltageProtection()) ? true : false; }

bool CAENelsFastPSChannel::isCurrentTripped() { return (getCurrent() > getOverCurrentProtection()) ? true : false; }

/*!
************************************************
 * Set PID for current or voltage control mode.
 \param k_value PID setting values in the order
 defined by setCurrentPID and setVoltagePID.
 Beware, they are different!
 \param strMode Choose between "current" or
 "voltage".
 \return Returns true if PIDs properly set.
************************************************
*/

bool CAENelsFastPSChannel::setPID(const std::vector<float>& k_value, const std::string& strMode)
{
    bool        modeFlag = false;
    std::string strRead;
    if(strMode == "current")
    {
        modeFlag = isInCurrentMode();
        strRead  = "MWG:4";
    }
    else if(strMode == "voltage")
    {
        modeFlag = isInVoltageMode();
        strRead  = "MWG:6";
    }
    else
    {
        std::cout << "Wrong strMode string in PID configuration: " << strMode << ". Choose between current and voltage." << std::endl;
        return false;
    }
    // Check if proper mode is set
    if(!modeFlag)
    {
        std::cout << "Couldn't set PID parameters: CAENels not in " << strMode << " mode" << std::endl;
        return false;
    }
    bool answer = true;
    // Configuring parameters
    for(int i = 0; i < 6; i++)
    {
        fAns.clear();
        fAns = fEthernet->read(strRead + std::to_string(i) + ":" + std::to_string(k_value[i]) + "\r");
        answer &= checkAns(fAns);
        answer &= checkAns(fEthernet->read("MSAVE\r"));
    }
    return answer;
}

/*!
************************************************
 * Set PID for voltage control mode.
 \param k_value PID setting values in the
 following order:\n
 0) Proportional current.\n
 1) Integral current.\n
 2) Derivative current.\n
 3) Proportional voltage.\n
 4) Integral voltage.\n
 5) Derivative voltage.\n
 \return Returns true if PIDs properly set.
************************************************
*/

bool CAENelsFastPSChannel::setVoltagePID(const std::vector<float>& k_value) { return setPID(k_value, "voltage"); }

/*!
************************************************
 * Set PID for current control mode.
 \param k_value PID setting values in the
 following order:\n
 0) Proportional voltage.\n
 1) Integral voltage.\n
 2) Derivative voltage.\n
 3) Proportional current.\n
 4) Integral current.\n
 5) Derivative current.\n
 \return Returns true if PIDs properly set.
************************************************
*/

bool CAENelsFastPSChannel::setCurrentPID(const std::vector<float>& k_value) { return setPID(k_value, "current"); }

/*!
************************************************
 * Returns PIDs read for current or voltage
 * control mode.
 \param strMode Choose between "current" or
 "voltage".
 \return Returns PIDs.
************************************************
*/

std::vector<float> CAENelsFastPSChannel::getPID(const std::string& strMode)
{
    std::string        strRead;
    std::vector<float> result(6, -9999);
    if(strMode == "current")
        strRead = "MRG:4";
    else if(strMode == "voltage")
        strRead = "MRG:6";
    else
    {
        std::cout << "Wrong strMode string: " << strMode << ". Choose between current and voltage." << std::endl;
        return result;
    }
    for(int i = 0; i < 6; i++)
    {
        fAns.clear();
        fAns = fEthernet->read(strRead + std::to_string(i) + "\r");
        if(checkAns(fAns, "MRG")) extractValue(fAns, 8, result[i]);
    }
    return result;
}

/*!
************************************************
 * Read PID for voltage control mode.
 \return Returns voltage control mode PIDs in
 the following order:\n
 0) Proportional current.\n
 1) Integral current.\n
 2) Derivative current.\n
 3) Proportional voltage.\n
 4) Integral voltage.\n
 5) Derivative voltage.\n
************************************************
*/

std::vector<float> CAENelsFastPSChannel::getVoltagePID() { return getPID("voltage"); }

/*!
************************************************
 * Read PID for voltage control mode.
 \return Returns voltage control mode PIDs in
 the following order:\n
 0) Proportional voltage.\n
 1) Integral voltage.\n
 2) Derivative voltage.\n
 3) Proportional current.\n
 4) Integral current.\n
 5) Derivative current.\n
************************************************
*/

std::vector<float> CAENelsFastPSChannel::getCurrentPID() { return getPID("current"); }
