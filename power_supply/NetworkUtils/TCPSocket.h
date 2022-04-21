#ifndef _TCPSocket_h_
#define _TCPSocket_h_

#include <sys/time.h>

class TCPSocket
{
  public:
    virtual ~TCPSocket();

    // Designed to be a base class not used used directly.
    TCPSocket(int socketId = invalidSocketId);

    // Moveable but not Copyable
    TCPSocket(TCPSocket&& move);

    TCPSocket& operator=(TCPSocket&& move);
    void       swap(TCPSocket& other);

    // Explicitly deleting copy constructor
    TCPSocket(TCPSocket const&) = delete;
    TCPSocket& operator=(TCPSocket const&) = delete;

    int  getSocketId(void) const { return fSocketId; }
    void open(void);
    void close(void);
    void sendClose(void);

  protected:
    static constexpr int invalidSocketId = -1;

  private:
    int fSocketId;
};

#endif
