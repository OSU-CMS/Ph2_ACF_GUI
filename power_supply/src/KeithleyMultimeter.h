/*!
 * \authors Antonio Cassese <antonio.cassese@cern.ch>, INFN-Firenze
 * \date dic 29 2020
 */

#ifndef KEITHLEYMULTIMETER_H
#define KEITHLEYMULTIMETER_H
#include "Multimeter.h"

/*!
 ************************************************
 \class KeithleyMultimeter.
 \brief Class for the control of the Keithley
 multimeter.
 ************************************************
 */

class Connection;

class KeithleyMultimeter : public Multimeter
{
  private:
    Connection* fConnection;
    // std::string fSeriesName;
    void printInfo();

  public:
    KeithleyMultimeter(const pugi::xml_node);
    virtual ~KeithleyMultimeter();
    void configure();

    void        reset();
    bool        isOpen();
    float       measureVoltage();
    float       measureCurrent();
    void        enableInternalScan();
    std::string scanChannels(int chanStart, int chanStop);
    bool        createScannerCardMap(pugi::xml_node*, std::map<int, std::string>&, bool verbose = false);
};

#endif
