#ifndef ScopeAgilent_H
#define ScopeAgilent_H

#include "Scope.h"
#include "ScopeChannel.h"
#include <memory>
#include <unordered_map>

/*!
 ************************************************
 \class ScopeAgilent.
 \brief Class for the control of the ScopeAgilent
 power supply.
 ************************************************
 */

class Connection;

class ScopeAgilent : public Scope
{
  private:
    std::shared_ptr<Connection> fConnection;
    std::string                 fSeriesName;
    std::string                 fStoreImagePath;

  public:
    ScopeAgilent(const pugi::xml_node);
    virtual ~ScopeAgilent();
    void configure();
    void reset();
    bool isOpen();
    void printInfo();
    void printConnection();
};

/*!
 ************************************************
 \class ScopeAgilentChannel.
 \brief Class for the control of the ScopeAgilent
 power supply channels.
 ************************************************
 */

class ScopeAgilentChannel : public ScopeChannel
{
  private:
    const unsigned short        fChannel;
    std::string                 fChannelName;
    std::shared_ptr<Connection> fConnection;
    std::string                 fSeriesName;
    std::vector<std::string>    fEOMObservables;
    std::string                 fStoreImagePath;

    // private:
    // ScopeAgilentChannel private methods

  public:
    ScopeAgilentChannel(std::shared_ptr<Connection>, const pugi::xml_node, std::string, std::string);
    virtual ~ScopeAgilentChannel();
    std::string aquireWaveForm();
    std::string acquireEOM(int nTrigs, int winStart, int winStop, std::string measurementName = "");
    void        setEOMeasurement(std::string observablesstring);
    void        printConnection();
};

#endif
