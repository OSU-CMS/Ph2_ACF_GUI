#include "../NetworkUtils/TCPPacket.h"
#include <arpa/inet.h>
#include <iostream>

//========================================================================================================================
TCPPacket::TCPPacket() : fBuffer("") {}

//========================================================================================================================
TCPPacket::~TCPPacket(void) {}

//==============================================================================
std::string TCPPacket::encode(char const* message, std::size_t length, uint32_t packetNumber) { return encode(std::string(message, length), packetNumber); }

//========================================================================================================================
std::string TCPPacket::encode(const std::string& message, uint32_t packetNumber)
{
    uint32_t    length            = htonl(TCPPacket::headerLength + message.length() + sizeof(uint32_t));
    uint32_t    localPacketNumber = htonl(packetNumber);
    std::string buffer            = std::string(TCPPacket::headerLength + sizeof(uint32_t), ' ') + message; // THE HEADER LENGTH IS SET TO 4 = sizeof(uint32_t)
    buffer[0]                     = (length)&0xff;
    buffer[1]                     = (length >> 8) & 0xff;
    buffer[2]                     = (length >> 16) & 0xff;
    buffer[3]                     = (length >> 24) & 0xff;
    buffer[4]                     = (localPacketNumber)&0xff;
    buffer[5]                     = (localPacketNumber >> 8) & 0xff;
    buffer[6]                     = (localPacketNumber >> 16) & 0xff;
    buffer[7]                     = (localPacketNumber >> 24) & 0xff;

    /*
    std::cout << __PRETTY_FUNCTION__
              //<< std::hex
              << "Buffer length: " << buffer.size() << ", packetNumber: "
              << packetNumber
              //              << ", htonl length: " << length
              << ", ntohl length: " << length << ", buffer: "
              << buffer
              //<< std::dec
              << std::endl;
    */
    return buffer;
}

//========================================================================================================================
void TCPPacket::reset(void) { fBuffer.clear(); }

//==============================================================================
bool TCPPacket::isEmpty(void) { return !fBuffer.size(); }

//========================================================================================================================
std::tuple<bool, uint32_t> TCPPacket::decode(std::string& message)
{
    std::tuple<bool, uint32_t> output{false, 0};
    if(fBuffer.length() < headerLength) return output;
    uint32_t length       = ntohl(reinterpret_cast<uint32_t&>(fBuffer.at(0))); // THE HEADER IS FIXED TO SIZE 4 = SIZEOF(uint32_t)
    uint32_t packetNumber = ntohl(reinterpret_cast<uint32_t&>(fBuffer.at(4))); // THE HEADER IS FIXED TO SIZE 4 = SIZEOF(uint32_t)
    // std::cout << __PRETTY_FUNCTION__ << "Receiving-";
    // or (auto l = 0; l < fBuffer.length(); l++)
    // for (auto l = 0; l < 4; l++)
    //  std::cout << std::hex << (unsigned int)fBuffer[l] << std::dec << "-";
    // std::cout << std::endl;
    /*
    std::cout << __PRETTY_FUNCTION__
              //<< std::hex
              << "Buffer length: " << fBuffer.length() << ", packetNumber: "
              << packetNumber
              //              << ", htonl length: " << length
              << ", ntohl length: " << length << ", buffer: "
              << fBuffer
              //<< std::dec
              << std::endl;
    */
    if(fBuffer.length() == length)
    {
        message = fBuffer.substr(headerLength + sizeof(uint32_t));
        fBuffer.clear();
        return std::make_tuple(true, packetNumber);
    }
    else if(fBuffer.length() > length)
    {
        message = fBuffer.substr(headerLength + sizeof(uint32_t), length - headerLength - sizeof(uint32_t));
        std::cout << __PRETTY_FUNCTION__ << "Erasing: " << length
                  << " characters!"
                  //<< " Msg:-" << message << "-"
                  << std::endl;
        fBuffer.erase(0, length);
        return std::make_tuple(true, packetNumber);
    }
    else
    {
        std::cout << __PRETTY_FUNCTION__ << "Can't decode an incomplete message! Length is only: " << fBuffer.length() << std::endl;
        return output;
    }
}

std::ostream& operator<<(std::ostream& out, const TCPPacket& packet)
{
    // out << packet.fBuffer.substr(TCPPacket::headerLength);
    out << packet.fBuffer;

    return out; // return std::ostream so we can chain calls to operator<<
}