#ifndef ScopeChannel_H
#define ScopeChannel_H
#include "pugixml.hpp"

/*!
************************************************
 \class ScopeChannel.
 \brief Abstract Channel class for channel management.
************************************************
*/

class ScopeChannel
{
  public:
    ScopeChannel(const pugi::xml_node configuration) : fConfiguration(configuration), fId(configuration.attribute("ID").value()) { ; }

    virtual ~ScopeChannel() { ; }

    std::string getID() const { return fId; }

  protected:
    const pugi::xml_node fConfiguration;
    std::string          fId;
};

#endif
