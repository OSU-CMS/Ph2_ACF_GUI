#ifndef CAEN_H
#define CAEN_H

#include "CAENHVWrapper.h"
#include "PowerSupply.h"
#include "PowerSupplyChannel.h"

/*!
************************************************
 \class CAEN.
 \brief Class for the control of the CAEN PS.
************************************************
*/

class CAEN : public PowerSupply
{
  public:
    CAEN(const pugi::xml_node configuration);
    virtual ~CAEN();
    void configure();
    void clearAlarm();

  private:
    int fSystemHandle;
};

class CAENChannel : public PowerSupplyChannel
{
  private:
    // Variables
    const int            fSystemHandle;
    const unsigned short fSlot;
    const unsigned short fChannel;

  public:
    CAENChannel(int systemHandle, const pugi::xml_node configuration);
    virtual ~CAENChannel();

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
    void  setParameter(std::string parName, float value) override;
    void  setParameter(std::string parName, bool value) override;
    void  setParameter(std::string parName, int value) override;
    float getParameterFloat(std::string parName) override;
    int   getParameterInt(std::string parName) override;
    bool  getParameterBool(std::string parName) override;

    unsigned getChannelStatus();
    unsigned getParameterUnsigned(std::string parName);
    bool     isChannelRampingUp();
    bool     isChannelRampingDown();

  private:
    void setName(std::string name);
    void checkAnswer(const CAENHVRESULT& answer); // Throws an exception is answer is not ok
};

#endif
