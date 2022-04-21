#include "../NetworkUtils/TCPPublishServer.h"
#include "../NetworkUtils/TCPTransmitterSocket.h"

#include <iostream>

//========================================================================================================================
TCPPublishServer::TCPPublishServer(unsigned int serverPort, unsigned int maxNumberOfClients) : TCPServerBase(serverPort, maxNumberOfClients) {}

//========================================================================================================================
TCPPublishServer::~TCPPublishServer(void) {}

//========================================================================================================================
void TCPPublishServer::acceptConnections()
{
    while(true)
    {
        try
        {
            // if(fConnectedClients.size() < fMaxNumberOfClients)
            std::cout << __PRETTY_FUNCTION__ << "Wating for clients to connect" << std::endl;
            TCPTransmitterSocket* clientSocket = acceptClient<TCPTransmitterSocket>();
            std::cout << __PRETTY_FUNCTION__ << "Client connected on socket: " << clientSocket->getSocketId() << std::endl;
        }
        catch(int e)
        {
            std::cout << __PRETTY_FUNCTION__ << "SHUTTING DOWN SOCKET" << std::endl;
            std::cout << __PRETTY_FUNCTION__ << "SHUTTING DOWN SOCKET" << std::endl;
            std::cout << __PRETTY_FUNCTION__ << "SHUTTING DOWN SOCKET" << std::endl;

            if(e == E_SHUTDOWN) break;
        }
    }
    // fAcceptPromise.set_value(true);
}
