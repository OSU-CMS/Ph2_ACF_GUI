/*!
 * \authors Mattia Lizzo <mattia.lizzo@cern.ch>, INFN-Firenze
 * \authors Francesco Fiori <francesco.fiori@cern.ch>, INFN-Firenze
 * \authors Antonio Cassese <antonio.cassese@cern.ch>, INFN-Firenze
 * \date Sep 2 2019
 */

#ifndef Multimeter_H
#define Multimeter_H
#include "pugixml.hpp"
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

/*!
************************************************
 \class Multimeter.
 \brief Abstract Multimeter class for
 multimeter management.
************************************************
*/

class Multimeter
{
  public:
    Multimeter(std::string model, const pugi::xml_node configuration);
    virtual ~Multimeter(void);

    // Virtual methods
    virtual void configure(void) = 0;

    // Get/set methods
    std::string convertToLFCR(std::string);
    std::string getID(void) const;
    std::string getModel(void) const;

  private:
    pugi::xml_document fConfigurationDoc; // IT MUST BE CREATED BEFORE fConfiguration!!!

  protected:
    const std::string    fModel;
    const pugi::xml_node fConfiguration;
};

#endif
