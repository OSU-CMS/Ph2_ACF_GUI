#ifndef _ots_TCPServer_h_
#define _ots_TCPServer_h_

#include "../NetworkUtils/TCPServerBase.h"
#include <string>

// namespace ots
//{

class TCPTransceiverSocket;

class TCPServer : public TCPServerBase
{
  public:
    TCPServer(unsigned int serverPort, unsigned int maxNumberOfClients = -1);
    virtual ~TCPServer(void);

    virtual std::string interpretMessage(const std::string& buffer) = 0;
    void                setReceiveTimeout(unsigned int timeoutSeconds, unsigned int timeoutMicroseconds);
    void                setSendTimeout(unsigned int timeoutSeconds, unsigned int timeoutMicroseconds);

  private:
    void           acceptConnections(void) override;
    void           connectClient(TCPTransceiverSocket* clientSocket);
    struct timeval fReceiveTimeout;
    struct timeval fSendTimeout;
    bool           fInDestructor;
};

//}

#endif
