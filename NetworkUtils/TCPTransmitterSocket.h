#ifndef _TCPTransmitterSocket_h_
#define _TCPTransmitterSocket_h_

#include "../NetworkUtils/TCPSocket.h"
#include <iostream>
#include <string>
#include <vector>

// A class that can write to a socket
class TCPTransmitterSocket : public virtual TCPSocket
{
  public:
    TCPTransmitterSocket(int socketId = invalidSocketId);
    virtual ~TCPTransmitterSocket(void);
    // TCPTransmitterSocket(TCPTransmitterSocket const&)  = delete ;
    TCPTransmitterSocket(TCPTransmitterSocket&& theTCPTransmitterSocket) = default;

    void send(char const* buffer, std::size_t size, bool forceEmptyPacket = false);
    void send(const std::string& buffer);
    void send(const std::vector<char>& buffer);
    void send(const std::vector<uint16_t>& buffer);

    template <typename T>
    void send(const std::vector<T>& buffer)
    {
        send(reinterpret_cast<const char*>(&buffer.at(0)), buffer.size() * sizeof(T));
    }

    void sendPacket(char const* buffer, std::size_t size);
    void sendPacket(const std::string& buffer);
    void setSendTimeout(unsigned int timeoutSeconds, unsigned int timeoutMicroSeconds);

  private:
    uint32_t fPacketNumber{0};
};

#endif
