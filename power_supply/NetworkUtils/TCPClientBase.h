#ifndef _TCPClientBase_h_
#define _TCPClientBase_h_

#include "../NetworkUtils/TCPTransceiverSocket.h"
#include <string>

class TCPClientBase : public virtual TCPSocket
{
  public:
    // TCPClientBase();
    TCPClientBase(const std::string& serverIP, int serverPort);
    virtual ~TCPClientBase(void);

    bool connect(int retry = -1, unsigned int sleepMilliSeconds = 1000);
    bool disconnect(void);

  private:
    std::string fServerIP;
    int         fServerPort;
    bool        fConnected;

    void resolveServer(std::string& serverIP);
};

#endif
