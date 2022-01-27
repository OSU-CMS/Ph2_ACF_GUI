#include "../NetworkUtils/TCPReceiverSocket.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

//========================================================================================================================
TCPReceiverSocket::TCPReceiverSocket(int socketId) : TCPSocket(socketId) {}

//========================================================================================================================
TCPReceiverSocket::~TCPReceiverSocket(void) {}

//========================================================================================================================
std::string TCPReceiverSocket::receivePacket(void)
{
    std::tuple<bool, uint32_t> statusAndPacketNumber{false, 0};
    TCPPacket                  thePacket;
    std::string                retVal = "";
    do
    {
        // std::cout << __PRETTY_FUNCTION__ << "Receiving..." << thePacket.isEmpty() << std::endl;
        thePacket += receive<std::string>();
        statusAndPacketNumber = thePacket.decode(retVal);
        // std::cout << __PRETTY_FUNCTION__ << "Received!" << thePacket.isEmpty() << std::endl;
    } while(!thePacket.isEmpty() && !std::get<0>(statusAndPacketNumber));
    if(fIsFirstPacket)
    {
        fIsFirstPacket = false;
        fPacketNumber  = std::get<1>(statusAndPacketNumber);
    }
    else
    {
        if(++fPacketNumber != std::get<1>(statusAndPacketNumber))
        {
            std::string errorString = "Packet number do not match!!! Expected " + std::to_string(fPacketNumber) + " but received " + std::to_string(std::get<1>(statusAndPacketNumber));
            throw std::runtime_error(errorString);
        }
    }
    return retVal;
}

//========================================================================================================================
std::size_t TCPReceiverSocket::receive(char* buffer, std::size_t bufferSize, int timeoutMicroSeconds)
{
    // std::cout << __PRETTY_FUNCTION__ << "Receiving Message for socket: " << getSocketId() << std::endl;
    if(getSocketId() == 0) { throw std::logic_error("Bad socket object (this object was moved)"); }
    std::size_t dataRead;
    // dataRead = ::recv(getSocketId(), buffer, bufferSize, MSG_DONTWAIT);
    dataRead = ::read(getSocketId(), buffer, bufferSize);
    if(dataRead == static_cast<std::size_t>(-1))
    {
        std::stringstream error;
        switch(errno)
        {
        case EBADF: error << "Socket file descriptor " << getSocketId() << " is not a valid file descriptor or is not open for reading...Errno: " << errno; break;
        case EFAULT: error << "Buffer is outside your accessible address space...Errno: " << errno; break;
        case ENXIO:
        {
            // Fatal error. Programming bug
            error << "Read critical error caused by a programming bug...Errno: " << errno;
            throw std::domain_error(error.str());
        }
        case EINTR:
            // TODO: Check for user interrupt flags.
            //       Beyond the scope of this project
            //       so continue normal operations.
            error << "The call was interrupted by a signal before any data was read...Errno: " << errno;
            break;
        case EAGAIN:
        {
            // recv is non blocking so this error is issued every time there are no messages to read
            // std::cout << __PRETTY_FUNCTION__ << "Couldn't read any data: " << dataRead << std::endl;
            // std::this_thread::sleep_for (std::chrono::seconds(1));
            return dataRead;
        }
        case ENOTCONN:
        {
            // Connection broken.
            // Return the data we have available and exit
            // as if the connection was closed correctly.
            return dataRead;
        }
        default: { error << "Read: returned -1...Errno: " << errno;
        }
        }
        throw std::runtime_error(error.str());
    }
    else if(dataRead == static_cast<std::size_t>(0))
    {
        std::cout << __PRETTY_FUNCTION__ << "Connection closed!" << std::endl;
        throw std::runtime_error("Connection closed");
    }
    // std::cout << __PRETTY_FUNCTION__ << "Message received with no errors for socket: " << getSocketId() << std::endl;
    return dataRead;
}

//========================================================================================================================
void TCPReceiverSocket::setReceiveTimeout(unsigned int timeoutSeconds, unsigned int timeoutMicroSeconds)
{
    struct timeval tv;
    tv.tv_sec  = timeoutSeconds;
    tv.tv_usec = timeoutMicroSeconds;
    setsockopt(getSocketId(), SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
}
