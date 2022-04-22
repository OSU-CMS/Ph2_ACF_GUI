#ifndef _TCPSubscribeClient_h_
#define _TCPSubscribeClient_h_

#include "../NetworkUtils/TCPClientBase.h"
#include "../NetworkUtils/TCPTransceiverSocket.h"
#include <string>

class TCPSubscribeClient
    : public TCPReceiverSocket
    , public TCPClientBase
{
  public:
    // TCPSubscribeClient();
    TCPSubscribeClient(const std::string& serverIP, int serverPort);
    virtual ~TCPSubscribeClient(void);
};

#endif
