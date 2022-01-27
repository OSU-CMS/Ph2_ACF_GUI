#include "../NetworkUtils/TCPSocket.h"
#include <iostream>
#include <stdexcept>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

//========================================================================================================================
TCPSocket::TCPSocket(int socketId) : fSocketId(socketId) { open(); }

//========================================================================================================================
TCPSocket::~TCPSocket()
{
    try
    {
        close();
        std::cout << __PRETTY_FUNCTION__ << "Clean socket close!" << std::endl;
    }
    catch(...)
    {
        // We should log this
        // TODO: LOGGING CODE HERE

        // If the user really want to catch close errors
        // they should call close() manually and handle
        // any generated exceptions. By using the
        // destructor they are indicating that failures is
        // an OK condition.
    }
}

//========================================================================================================================
void TCPSocket::open()
{
    if(fSocketId == invalidSocketId && (fSocketId = ::socket(PF_INET, SOCK_STREAM, 0)) == invalidSocketId) throw std::runtime_error(std::string("Bad socket: ") + strerror(errno));
}

//========================================================================================================================
void TCPSocket::close()
{
    if(fSocketId == invalidSocketId) { throw std::logic_error("Bad socket object (this object was moved)"); }
    int state = ::close(fSocketId);
    // std::cout << __PRETTY_FUNCTION__ << "Socket id: " << getSocketId() << " close state: " << state << " errno: " <<
    // errno << std::endl;
    if(state == 0) // 0 means socket closed correctly
        fSocketId = invalidSocketId;
    else
    {
        switch(errno)
        {
        case EBADF: throw std::domain_error(std::string("Close: EBADF: ") + std::to_string(fSocketId) + " " + strerror(errno));
        case EIO: throw std::runtime_error(std::string("Close: EIO: ") + std::to_string(fSocketId) + " " + strerror(errno));
        case EINTR:
        {
            // TODO: Check for user interrupt flags.
            //       Beyond the scope of this project
            //       so continue normal operations.
            return;
        }
        default: throw std::runtime_error(std::string("Close: ???: ") + std::to_string(fSocketId) + " " + strerror(errno));
        }
    }
}

//========================================================================================================================
void TCPSocket::swap(TCPSocket& other)
{
    using std::swap;
    swap(fSocketId, other.fSocketId);
}

//========================================================================================================================
TCPSocket::TCPSocket(TCPSocket&& move) : fSocketId(invalidSocketId) { move.swap(*this); }

//========================================================================================================================
TCPSocket& TCPSocket::operator=(TCPSocket&& move)
{
    move.swap(*this);
    return *this;
}

//========================================================================================================================
void TCPSocket::sendClose()
{
    if(::shutdown(getSocketId(), SHUT_WR) != 0) { throw std::domain_error(std::string("Shutdown: critical error: ") + strerror(errno)); }
}