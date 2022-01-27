/*!
 * \authors Stefan Maier <s.maier@kit.edu>, KIT-Karlsruhe
 * \authors Mattia Lizzo <mattia.lizzo@cern.ch>, INFN-Firenze
 * \authors Francesco Fiori <francesco.fiori@cern.ch>, INFN-Firenze
 * \authors Antonio Cassese <antonio.cassese@cern.ch>, INFN-Firenze
 * \date Sep 4 2020
 */

#ifndef Arduino_H
#define Arduino_H
#include "pugixml.hpp"
#include <string>
#include <unordered_map>

// class PowerSupplyChannel;
/*!
************************************************
 \class PowerSupply.
 \brief Abstract PowerSupply class for power
 supplies management.
************************************************
*/

class Arduino
{
  public:
    Arduino(std::string model, const pugi::xml_node configuration);
    virtual ~Arduino(void);
    // PowerSupplyChannel* getChannel(const std::string& id);

    // Virtual methods
    virtual void configure(void) = 0;
    virtual bool turnOn();  // Some power supplies might not need to be turned
                            // on, so providing a default that does nothing
    virtual bool turnOff(); // Some power supplies might not need to be turned
                            // off, so providing a default that does nothing

    virtual float getParameterFloat(std::string parName);

    // Get/set methods
    std::string getModel() const;
    std::string getID() const;
    // void        setPowerType(std::string val);
    // std::string getPowerType() const;

  protected:
    std::string          fId;
    const std::string    fModel;
    const pugi::xml_node fConfiguration;
    // std::string                                          fPowerType;
    // std::unordered_map<std::string, PowerSupplyChannel*> fChannelMap;
};

#endif
