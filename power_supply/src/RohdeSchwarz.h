/*!
 * \author Stefan Maier <s.maier@kit.edu>, ETP-Karlsruhe
 * \date Apr 30 2020
 */
#ifndef ROHDESCHWARZ_H
#define ROHDESCHWARZ_H

#include "PowerSupply.h"
#include "PowerSupplyChannel.h"

class Connection;

class RohdeSchwarz : public PowerSupply
{
  public:
    RohdeSchwarz(const pugi::xml_node configuration);
    virtual ~RohdeSchwarz();
    void configure();
    // bool turnOn();
    // bool turnOff();

  private:
    Connection* fConnection;
};

class RohdeSchwarzChannel : public PowerSupplyChannel
{
  public:
    RohdeSchwarzChannel(Connection* connection, const pugi::xml_node configuration);
    virtual ~RohdeSchwarzChannel();

    // bool  reset() override;
    // bool  isOpen() override;
    void turnOn(void) override;
    void turnOff(void) override;
    bool isOn(void) override;
    // bool  setVoltageMode() override;
    // bool  setCurrentMode() override;
    // bool  setVoltageRange(float voltage) override;
    // bool  setCurrentRange(float current) override;
    void setVoltage(float voltage) override;
    void setCurrent(float current) override;
    void setVoltageCompliance(float voltage) override;
    void setCurrentCompliance(float current) override;
    void setOverVoltageProtection(float voltage) override;
    void setOverCurrentProtection(float current) override;

    float getSetVoltage() override;
    float getOutputVoltage() override;
    float getCurrent() override;
    float getVoltageCompliance() override;
    float getCurrentCompliance() override;
    float getOverVoltageProtection() override;
    float getOverCurrentProtection() override;
    void  setParameter(std::string parName, float value) override;
    void  setParameter(std::string parName, bool value) override;
    void  setParameter(std::string parName, int value) override;
    float getParameterFloat(std::string parName) override;
    int   getParameterInt(std::string parName) override;
    bool  getParameterBool(std::string parName) override;

  private:
    Connection* fConnection;
    std::string fChannelCommand;

    void        write(std::string);
    std::string read(std::string);
};

#endif
