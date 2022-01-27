/*!
 * \authors Mattia Lizzo <mattia.lizzo@cern.ch>, INFN-Firenze
 * \authors Antonio Cassese <antonio.cassese@cern.ch>, INFN-Firenze
 * \date Sep 2 2019
 */

#ifndef TTi_H
#define TTi_H

#include "PowerSupply.h"
#include "PowerSupplyChannel.h"

/*!
 ************************************************
 \class TTi.
 \brief Class for the control of the TTi
 power supply.
 ************************************************
 */

class Connection;

class TTi : public PowerSupply
{
  private:
    Connection* fConnection;
    std::string fSeriesName;
    void        printInfo();

  public:
    TTi(const pugi::xml_node);
    virtual ~TTi();
    void configure();
    void reset();
    bool isOpen();
};

/*!
 ************************************************
 \class TTiChannel.
 \brief Class for the control of the TTi
 power supply channels.
 ************************************************
 */

class TTiChannel : public PowerSupplyChannel
{
  private:
    const unsigned short fChannel;
    std::string          fChannelName;
    Connection*          fConnection;
    std::string          fSeriesName;
    bool                 fIsOn;
    float                fSetVoltage;

  private:
    // TTiChannel private methods
    void        statusEER();
    void        statusQER();
    void        statusESR();
    void        write(std::string, int);
    std::string read(std::string);
    std::string formatCommand(std::string);

  public:
    TTiChannel(Connection*, const pugi::xml_node, std::string);
    virtual ~TTiChannel();

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
    float getOutputVoltage() override;
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
};

#endif
