/*!
 * \authors Mattia Lizzo <mattia.lizzo@cern.ch>, INFN-Firenze
 * \authors Antonio Cassese <antonio.cassese@cern.ch>, INFN-Firenze
 * \date Sep 2 2019
 */

#ifndef CAENelsFastPS_H
#define CAENelsFastPS_H
#include "CAENelsFastPSErrors.h"
#include "EthernetConnection.h"
#include "PowerSupply.h"
#include "PowerSupplyChannel.h"

/*!
************************************************
 \class CAENelsFastPS.
 \brief Class for the control of the CAENels
 Fast PS.
************************************************
*/

class CAENelsFastPS : public PowerSupply
{
  public:
    CAENelsFastPS(const pugi::xml_node configuration);
    virtual ~CAENelsFastPS();
    void configure();
    bool isOpen(); // Lorenzo removed override

  private:
    EthernetConnection fEthernet;
};

/*!
************************************************
 \class CAENelsFastPSChannel.
 \brief Class for the control of the CAENels
 Fast PS channels.
************************************************
*/

class CAENelsFastPSChannel : public PowerSupplyChannel
{
  public:
    CAENelsFastPSChannel(EthernetConnection* ethernet, const pugi::xml_node configuration);
    virtual ~CAENelsFastPSChannel();
    void reset(); // Removed override
    bool isOn() override;
    void turnOn() override;
    void turnOff() override;

  public:
    // Get/set methods
    void  setVoltage(float) override;
    void  setCurrent(float) override;
    void  setVoltageCompliance(float) override;
    void  setCurrentCompliance(float) override;
    void  setOverVoltageProtection(float) override;
    void  setOverCurrentProtection(float) override;
    void  setParameter(std::string parName, float value) override { ; }
    void  setParameter(std::string parName, bool value) override { ; }
    void  setParameter(std::string parName, int value) override { ; }
    float getSetVoltage() override;
    float getOutputVoltage() override;
    float getCurrent(void) override;
    float getVoltageCompliance(void) override;
    float getCurrentCompliance(void) override;
    float getOverVoltageProtection() override;
    float getOverCurrentProtection() override;
    float getParameterFloat(std::string parName) override { return 0; }
    int   getParameterInt(std::string parName) override { return 0; }
    bool  getParameterBool(std::string parName) override { return false; }

  private:
    // Methods
    bool               setVoltageMode(float, const std::string&);
    bool               setCurrentMode(float, const std::string&);
    bool               setVoltageSlewRate(float);
    bool               setCurrentSlewRate(float);
    bool               setPID(const std::vector<float>&, const std::string&);
    bool               setVoltagePID(const std::vector<float>&);
    bool               setCurrentPID(const std::vector<float>&);
    bool               setVoltageWithRamp(float, float);
    bool               setCurrentWithRamp(float, float);
    bool               setVoltageMode();
    bool               setCurrentMode();
    bool               setVoltageRange(float);
    bool               setCurrentRange(float);
    std::vector<float> getPID(const std::string&);
    std::vector<float> getVoltagePID();
    std::vector<float> getCurrentPID();
    void               elaborateError(const std::string&);
    bool               checkAns(const std::string&, const std::string);
    bool               extractValue(const std::string&, unsigned int, float&);
    bool               isInCurrentMode();
    bool               isInVoltageMode();
    bool               isVoltTripped();
    bool               isCurrentTripped();
    bool               statusRegisterOverPower(const std::string&);
    bool               statusRegisterExternalInterlock2(const std::string&);
    bool               statusRegisterExternalInterlock1(const std::string&);
    bool               statusRegisterExcessiveRipple(const std::string&);
    bool               statusRegisterRegulationFault(const std::string&);
    bool               statusRegisterEarthFuse(const std::string&);
    bool               statusRegisterEarthLeakage(const std::string&);
    bool               statusRegisterDCLinkFault(const std::string&);
    bool               statusRegisterOverTemperature(const std::string&);
    bool               statusRegisterCrowbarProtection(const std::string&);
    bool               statusRegisterInputOverCurrent(const std::string&);
    bool               statusRegisterWaveformExecution(const std::string&);
    bool               statusRegisterRamping(const std::string&);
    bool               statusRegisterNormalUpdateMode(const std::string&);
    bool               statusRegisterAnalogInputUpdateMode(const std::string&);
    bool               statusRegisterRegulationMode(const std::string&);
    bool               statusRegisterRemoteControlMode(const std::string&);
    bool               statusRegisterLocalControlMode(const std::string&);
    bool               statusRegisterFaultCondition(const std::string&);
    bool               statusRegisterModuleOn(const std::string&);
    const std::string  bitStatusRegister();

  private:
    // Variables
    std::string         fAns; // String storing answers from ethernet.
    EthernetConnection* fEthernet;
};

#endif
