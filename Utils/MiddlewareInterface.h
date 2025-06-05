#ifndef _MiddlewareInterface_h_
#define _MiddlewareInterface_h_

#include "NetworkUtils/TCPClient.h"
#include <string>

class ConfigureInfo;
class StartInfo;

class MiddlewareInterface : public TCPClient
{
  public:
    MiddlewareInterface(std::string serverIP, int serverPort);
    virtual ~MiddlewareInterface(void);
    void        initialize(void);
    void        configure(const ConfigureInfo& theConfigureInfo);
    void        halt(void);
    void        pause(void);
    void        resume(void);
    void        start(const StartInfo& theStartInfo);
    void        stop(void);
    std::string status(void);

  protected:
    // std::string currentRun_ = "0";
    // bool        running_    = false;
    // bool        paused_     = false;

  private:
    std::string sendCommand(const std::string& command);
};

#endif
