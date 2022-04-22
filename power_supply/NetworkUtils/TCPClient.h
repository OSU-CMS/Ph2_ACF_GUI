#ifndef _TCPClient_h_
#define _TCPClient_h_

#include "../NetworkUtils/TCPClientBase.h"
#include "../NetworkUtils/TCPTransceiverSocket.h"
#include <string>

class TCPClient
    : public TCPTransceiverSocket
    , public TCPClientBase
{
  public:
    TCPClient(const std::string& serverIP, int serverPort);
    virtual ~TCPClient(void);
};

#endif
