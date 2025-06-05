#include "Utils/PacketHeader.h"

//========================================================================================================================
PacketHeader::PacketHeader()
{
    for(uint8_t i = 0; i < SIZE; ++i) fPacketSize[i] = 0u;
}

//========================================================================================================================
PacketHeader::~PacketHeader() {}

//========================================================================================================================
uint8_t PacketHeader::getPacketHeaderSize() { return SIZE; }

//========================================================================================================================
void PacketHeader::addPacketHeader(std::string& thePacket)
{
    uint64_t thePacketSize = thePacket.size() + SIZE;
    setPacketSize(thePacketSize);
    std::string packetString(&fPacketSize[0], SIZE);
    thePacket.insert(0, packetString);
}

//========================================================================================================================
uint32_t PacketHeader::getPacketSize(std::string& thePacket)
{
    for(uint8_t i = 0; i < SIZE; ++i) fPacketSize[i] = thePacket[i];
    return getPacketSize();
}

//========================================================================================================================
uint32_t PacketHeader::getPacketSize(std::vector<char>& thePacket)
{
    std::string theStringPacket(thePacket.begin(), thePacket.begin() + SIZE);
    return getPacketSize(theStringPacket);
}

//========================================================================================================================
void PacketHeader::setPacketSize(uint64_t packetSize)
{
    uint64_t maximumSize = 1 << (SIZE * 8 - 1);
    if(packetSize >= maximumSize)
    {
        std::string outputMessage =
            std::string(__PRETTY_FUNCTION__) + " ERROR: requested packet sizes = " + std::to_string(packetSize) + " is >= than " + std::to_string(maximumSize) + " are not allowed";
        throw std::runtime_error(outputMessage);
    }
    uint32_t localPacketSize = htonl(packetSize);
    for(uint8_t i = 0u; i < SIZE; ++i) fPacketSize[i] = ((localPacketSize >> (8u * i)) & 0xff);
}

//========================================================================================================================
uint32_t PacketHeader::getPacketSize()
{
    uint32_t localPacketSize = 0;
    for(uint8_t i = 0; i < SIZE; ++i) localPacketSize += ((fPacketSize[i] & 0xff) << (8u * i));
    return htonl(localPacketSize);
}
