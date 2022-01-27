/*!
 * \authors Mattia Lizzo <mattia.lizzo@cern.ch>, INFN-Firenze
 * \authors Francesco Fiori <francesco.fiori@cern.ch>, INFN-Firenze
 * \authors Antonio Cassese <antonio.cassese@cern.ch>, INFN-Firenze
 * \authors Lorenzo Uplegger <uplegger@fnal.gov>, Fermilab
 * \authors Stefan Maier <s.maier@kit.edu>, KIT-Karlsruhe
 * \date Sep 2 2019
 */

#ifndef DeviceHandler_H
#define DeviceHandler_H
#include "pugixml.hpp"
#include <string>
#include <unordered_map>
#include <vector>

/*!
************************************************
 \class DeviceHandler.
 \brief DeviceHandler class for power supplies management.
************************************************
*/
class PowerSupply;
class Arduino;
class Multimeter;
class Scope;

class DeviceHandler
{
  public:
    DeviceHandler();
    virtual ~DeviceHandler();

    void                     readSettings(const std::string& docPath, pugi::xml_document& doc, bool verbose = true);
    PowerSupply*             getPowerSupply(const std::string& id);
    Arduino*                 getArduino(const std::string& id);
    Multimeter*              getMultimeter(const std::string& id);
    Scope*                   getScope(const std::string& id);
    std::vector<std::string> getReadoutList();
    std::vector<std::string> getStatus();

  private:
    // Variables
    pugi::xml_node                                fDocumentRoot;
    std::unordered_map<std::string, PowerSupply*> fPowerSupplyMap;
    std::unordered_map<std::string, Arduino*>     fArduinoMap;
    std::unordered_map<std::string, Multimeter*>  fMultimeterMap;
    std::unordered_map<std::string, Scope*>       fScopeMap;

    // Test and xml handling methods
    static pugi::xml_parse_result loadXml(const std::string& docPath, pugi::xml_document& doc);
    static bool                   loadXmlErrorHandler(const std::string& docPath, const pugi::xml_parse_result& result);
};

#endif
