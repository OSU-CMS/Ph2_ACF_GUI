/*!
 * \authors Mattia Lizzo <mattia.lizzo@cern.ch>, INFN-Firenze
 * \authors Francesco Fiori <francesco.fiori@cern.ch>, INFN-Firenze
 * \authors Antonio Cassese <antonio.cassese@cern.ch>, INFN-Firenze
 * \authors Lorenzo Uplegger <uplegger@fnal.gov>, Fermilab
 * \date Sep 2 2019
 */

#ifndef PowerSupplyChannel_H
#define PowerSupplyChannel_H
#include "pugixml.hpp"

/*!
************************************************
 \class PowerSupplyChannel.
 \brief Abstract Channel class for channel management.
************************************************
*/

class PowerSupplyChannel
{
  public:
    PowerSupplyChannel(const pugi::xml_node configuration) : fConfiguration(configuration), fId(configuration.attribute("ID").value()) { ; }

    virtual ~PowerSupplyChannel() { ; }

    std::string getID() const { return fId; }

    //[[deprecated("Use getOutputVoltage instead")]]
    float getVoltage() { return getOutputVoltage(); }

    // Virtual methods
    virtual void turnOn(void)  = 0;
    virtual void turnOff(void) = 0;
    virtual bool isOn(void)    = 0;

    // Get/set methods
    virtual void  setVoltage(float voltage)                      = 0;
    virtual void  setCurrent(float current)                      = 0;
    virtual void  setVoltageCompliance(float voltage)            = 0;
    virtual void  setCurrentCompliance(float current)            = 0;
    virtual void  setOverVoltageProtection(float voltage)        = 0;
    virtual void  setOverCurrentProtection(float current)        = 0;
    virtual float getSetVoltage(void)                            = 0;
    virtual float getOutputVoltage(void)                         = 0;
    virtual float getCurrent(void)                               = 0;
    virtual float getVoltageCompliance(void)                     = 0;
    virtual float getCurrentCompliance(void)                     = 0;
    virtual float getOverVoltageProtection(void)                 = 0;
    virtual float getOverCurrentProtection(void)                 = 0;
    virtual float getParameterFloat(std::string parName)         = 0;
    virtual int   getParameterInt(std::string parName)           = 0;
    virtual bool  getParameterBool(std::string parName)          = 0;
    virtual void  setParameter(std::string parName, float value) = 0;
    virtual void  setParameter(std::string parName, bool value)  = 0;
    virtual void  setParameter(std::string parName, int value)   = 0;

    // Ramp up and down the voltage of the current channel using read and setVoltage
    // pTimeSpan is given in ms
    // Return false if the channel should not be ramped (channel == OFF)
    bool rampToVoltage(float pTargetVoltage = 0, uint32_t pSteps = 15, uint32_t pTimeSpan = 1000);

  protected:
    const pugi::xml_node fConfiguration;
    std::string          fId;
};

#endif
