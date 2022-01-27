
#ifndef Scope_H
#define Scope_H
#include "pugixml.hpp"
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

class ScopeChannel;
/*!
************************************************
 \class Scope.
 \brief Abstract Scope class for power
 supplies management.
************************************************
*/

class Scope
{
  public:
    Scope(std::string model, const pugi::xml_node configuration);
    virtual ~Scope(void);

    // Virtual methods
    virtual void configure(void) = 0;

    // Get/set methods
    ScopeChannel*            getChannel(const std::string& id);
    std::string              convertToLFCR(std::string);
    std::vector<std::string> getChannelList(void) const;
    std::string              getID(void) const;
    std::string              getModel(void) const;
    std::string              getPowerType(void) const;

    void setPowerType(std::string val);

    // Iterator on channels
    std::unordered_map<std::string, ScopeChannel*>::const_iterator cBegin() const { return fChannelMap.cbegin(); }
    std::unordered_map<std::string, ScopeChannel*>::const_iterator cEnd() const { return fChannelMap.cend(); }

  private:
    pugi::xml_document fConfigurationDoc; // IT MUST BE CREATED BEFORE fConfiguration!!!

  protected:
    const std::string                              fModel;
    const pugi::xml_node                           fConfiguration;
    std::unordered_map<std::string, ScopeChannel*> fChannelMap;
    std::vector<std::string>                       fChannelList;
};

#endif
