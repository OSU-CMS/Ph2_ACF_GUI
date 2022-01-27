/*!
 * \author Massimiliano Marchisone <mmarchis.@cern.ch>
 * \date Jul 30 2020
 */

#ifndef ISEGSHR_H
#define ISEGSHR_H

#include "PowerSupply.h"
#include "PowerSupplyChannel.h"
#include <string>
using namespace ::std;

/*!
 ************************************************
 \class IsegSHR.
 \brief Class for the control of the ISEG SHR
 power supply.
 ************************************************
 */

class Connection;

class IsegSHR : public PowerSupply
{
  private:
    Connection* fConnection;

  public:
    IsegSHR(const pugi::xml_node configuration);
    virtual ~IsegSHR();
    void configure();
};

/*!
 ************************************************
 \class IsegSHRChannel.
 \brief Class for the control of the ISEH SHR
 power supply channels.
 ************************************************
 */

class IsegSHRChannel : public PowerSupplyChannel
{
  private:
    const unsigned short fChannel;
    Connection*          fConnection;

  private:
    void   write(string);
    string read(string);

  public:
    IsegSHRChannel(Connection*, const pugi::xml_node);
    virtual ~IsegSHRChannel();

    void reset();
    void turnOn(void) override;
    void turnOff(void) override;
    bool isOn(void) override;

    // Get/set methods
    void  setVoltage(float voltage) override;
    void  setCurrent(float current) override;
    void  setVoltageCompliance(float voltage) override;
    void  setCurrentCompliance(float current) override;
    void  setOverVoltageProtection(float voltage) override;
    void  setOverCurrentProtection(float current) override;
    float getOutputVoltage() override;
    float getCurrent() override;
    float getVoltageCompliance() override;
    float getCurrentCompliance() override;
    float getOverVoltageProtection() override;
    float getOverCurrentProtection() override;
    float getParameterFloat(std::string parName) override;
    int   getParameterInt(std::string parName) override;
    bool  getParameterBool(std::string parName) override;
    void  setParameter(std::string parName, float value) override;
    void  setParameter(std::string parName, bool value) override;
    void  setParameter(std::string parName, int value) override;

    // Other methods defined only for IsegSHR power supply
    float        getSetVoltage();
    float        getSetCurrent();
    string       getPolarity();
    void         setPolarity(string);
    float        getVoltageRampUpSpeed();
    void         setVoltageRampUpSpeed(float);
    float        getVoltageRampDownSpeed();
    void         setVoltageRampDownSpeed(float);
    string       getDeviceID();
    string       getDeviceTemperature();
    unsigned int getChannelStatus();
    bool         isRamping();
};

#endif
