/*!
 * \authors Mattia Lizzo <mattia.lizzo@cern.ch>, INFN-Firenze
 * \authors Francesco Fiori <francesco.fiori@cern.ch>, INFN-Firenze
 * \authors Antonio Cassese <antonio.cassese@cern.ch>, INFN-Firenze
 * \date Sep 2 2019
 */

#ifndef PowerSupply_H
#define PowerSupply_H
#include "pugixml.hpp"
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

class PowerSupplyChannel;
/*!
************************************************
 \class PowerSupply.
 \brief Abstract PowerSupply class for power
 supplies management.
************************************************
*/

class PowerSupply
{
  public:
    PowerSupply(std::string model, const pugi::xml_node configuration);
    virtual ~PowerSupply(void);

    // Virtual methods
    virtual void configure(void) = 0;
    virtual bool turnOn();          // Some power supplies might not need to be turned
                                    // on, so providing a default that does nothing
    virtual bool turnOff();         // Some power supplies might not need to be turned
                                    // off, so providing a default that does nothing
    virtual void reset();           // Some power supplies might not need to be reset, so providing a default that does nothing
    void         turnOnChannels();  // This method will turn on ALL channels following the sequence on the configuration file
    void         turnOffChannels(); // This method will turn off ALL channels following the sequence on the configuration file
    void         setRemote();

    // Get/set methods
    PowerSupplyChannel*      getChannel(const std::string& id);
    std::string              convertToLFCR(std::string);
    std::vector<std::string> getChannelList(void) const;
    std::string              getID(void) const;
    std::string              getModel(void) const;
    std::string              getPowerType(void) const;
    virtual std::string      getInfo();

    void setPowerType(std::string val);

    // Iterator on channels
    std::unordered_map<std::string, PowerSupplyChannel*>::const_iterator cBegin() const { return fChannelMap.cbegin(); }
    std::unordered_map<std::string, PowerSupplyChannel*>::const_iterator cEnd() const { return fChannelMap.cend(); }

  private:
    pugi::xml_document fConfigurationDoc; // IT MUST BE CREATED BEFORE fConfiguration!!!

  protected:
    const std::string                                    fModel;
    const pugi::xml_node                                 fConfiguration;
    std::string                                          fPowerType;
    std::unordered_map<std::string, PowerSupplyChannel*> fChannelMap;
    std::vector<std::string>                             fChannelList;
};

#endif
