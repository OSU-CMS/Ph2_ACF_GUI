#ifndef __PACKET_HEADER__
#define __PACKET_HEADER__

#include <arpa/inet.h>
#include <iostream>
#include <vector>

#define END_OF_TRANSMISSION_MESSAGE "DoneWithRun"

class PacketHeader
{
  public:
    PacketHeader();
    virtual ~PacketHeader();

    uint8_t getPacketHeaderSize();

    void     addPacketHeader(std::string& thePacket);
    uint32_t getPacketSize(std::string& thePacket);
    uint32_t getPacketSize(std::vector<char>& thePacket);

  private:
    void     setPacketSize(uint64_t packetSize);
    uint32_t getPacketSize();

    static const uint8_t SIZE = 4;
    char                 fPacketSize[SIZE];
};

#endif
