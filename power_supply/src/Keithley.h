/*!
 * \authors Monica Scaringella <monica.scaringella@fi.infn.it>, INFN-Firenze
 * \date July 24 2020
 */

#ifndef Keithley_H
#define Keithley_H

#include "PowerSupply.h"
#include "PowerSupplyChannel.h"

/*!
 ************************************************
 \class Keithley.
 \brief Class for the control of the Keithley
 power supply.
 ************************************************
 */

class Connection;

class Keithley : public PowerSupply
{
  private:
    Connection* fConnection;
    std::string fSeriesName;
    void        printInfo();

  public:
    Keithley(const pugi::xml_node);
    virtual ~Keithley();
    void configure();
    void reset(); // Added 16/06 and to be tested
    bool isOpen();
    std::string getInfo();
};

/*!
 ************************************************
 \class KeithleyChannel.
 \brief Class for the control of the Keithley
 power supply channels.
 ************************************************
 */

class KeithleyChannel : public PowerSupplyChannel
{
  private:
    unsigned short    fChannel;
    const std::string fChannelName;
    Connection*       fConnection;
    std::string       fSeriesName;
    std::string       fChannelCommand;
    float             fSetVoltage;

  private:
    // KeithleyChannel private methods
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
    KeithleyChannel(Connection*, const pugi::xml_node, std::string);
    virtual ~KeithleyChannel();

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
