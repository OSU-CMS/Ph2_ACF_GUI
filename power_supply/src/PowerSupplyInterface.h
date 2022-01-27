#ifndef _PowerSupplyInterface_h_
#define _PowerSupplyInterface_h_

#include "../NetworkUtils/TCPServer.h"
#include "DeviceHandler.h"
#include "PowerSupply.h"
#include "PowerSupplyChannel.h"
#include "Scope.h"
#include "ScopeAgilent.h"

#include <mutex>
#include <string>

class PowerSupplyInterface : public TCPServer
{
  public:
    PowerSupplyInterface(int serverPort, std::string configFileName);
    virtual ~PowerSupplyInterface(void);

    std::string interpretMessage(const std::string& buffer) override;

  private:
    std::string getVariableValue(std::string variable, std::string buffer)
    {
        size_t begin = buffer.find(variable) + variable.size() + 1;
        size_t end   = buffer.find(',', begin);
        if(end == std::string::npos) end = buffer.size();
        return buffer.substr(begin, end - begin);
    }
    std::string        currentRun_ = "0";
    bool               running_    = false;
    bool               paused_     = false;
    DeviceHandler      fHandler;
    pugi::xml_document fDocSettings;

    std::string getTimeStamp();

    std::mutex fMutex;
};

#endif
