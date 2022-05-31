/*!
 * \authors Kai Wei <kai.wei@cern.ch>, OSU
 * \date April 15 2022
 */

#ifndef KeySight_H
#define KeySight_H

#include "PowerSupply.h"
#include "PowerSupplyChannel.h"

/*!
 ************************************************
 \class KeySight.
 \brief Class for the control of the KeySight
 power supply.
 ************************************************
 */

class Connection;

class KeySight : public PowerSupply
{
  private:
    Connection* fConnection;
    std::string fSeriesName;
    void        printInfo();

  public:
    KeySight(const pugi::xml_node);
    virtual ~KeySight();
    void configure();
    void reset(); // Added 16/06 and to be tested
    bool isOpen();
    void setRemote();
    std::string getInfo();
};

/*!
 ************************************************
 \class KeySightChannel.
 \brief Class for the control of the KeySight
 power supply channels.
 ************************************************
 */

class KeySightChannel : public PowerSupplyChannel
{
  private:
    unsigned short    fChannel;
    const std::string fChannelName;
    Connection*       fConnection;
    std::string       fSeriesName;
    std::string       fChannelCommand;
    float             fSetVoltage;

  private:
    // KeySightChannel private methods
    void        write(std::string);
    std::string read(std::string);
    void        setRearOutput();
    void        setFrontOutput();
    void        setAutorange(bool value);
    void        setDisplay(bool value);
    void        setLocal(bool value);
    void        setVoltageRange(float value);
    void        setCurrentRange(float value);

  public:
    KeySightChannel(Connection*, const pugi::xml_node, std::string);
    virtual ~KeySightChannel();

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
    float getSetVoltage() override;
    float getOutputVoltage(void) override;
    float getCurrent(void) override;
    float getVoltageCompliance(void) override;
    float getCurrentCompliance(void) override;
    float getOverVoltageProtection(void) override;
    float getOverCurrentProtection(void) override;
    float getParameterFloat(std::string parName) override;
    int   getParameterInt(std::string parName) override;
    bool  getParameterBool(std::string parName) override;
    void  setParameter(std::string parName, float value) override;
    void  setParameter(std::string parName, bool value) override;
    void  setParameter(std::string parName, int value) override;
    void  setVoltageMode(void);
    void  setCurrentMode(void);
};

#endif
