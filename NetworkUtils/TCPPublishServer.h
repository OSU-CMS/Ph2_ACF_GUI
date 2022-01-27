#ifndef _TCPPublishServer_h_
#define _TCPPublishServer_h_

#include "../NetworkUtils/TCPServerBase.h"

class TCPPublishServer : public TCPServerBase
{
  public:
    TCPPublishServer(unsigned int serverPort, unsigned int maxNumberOfClients = -1);
    virtual ~TCPPublishServer(void);

  protected:
    void acceptConnections() override;
};

#endif
