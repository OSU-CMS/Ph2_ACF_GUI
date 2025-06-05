#ifndef _MiddlewareController_h_
#define _MiddlewareController_h_

#include "NetworkUtils/TCPServer.h"
#include "System/SystemController.h"
#include "miniDAQ/MiddlewareMessageHandler.h"
#include "tools/Tool.h"

#include <string>

#define PORT_BASE 5000     // The server listening port base
#define DQM_PORT_BASE 6000 // The DQM server listening port base
class MiddlewareController : public TCPServer
{
  public:
    MiddlewareController(uint16_t portShift = 0);
    virtual ~MiddlewareController(void);

    // The MiddlewareController only has 1 client so send is more appropriate than broadcast
    std::string interpretMessage(const std::string& buffer) override;

  private:
    MiddlewareMessageHandler fMiddlewareMessageHandler;
};

#endif
