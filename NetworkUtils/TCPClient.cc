#include "../NetworkUtils/TCPClient.h"

#include <sstream>
#include <stdexcept>
//g++ -std=c++1y -fPIC -shared *.cc -o NetworkUtils.so -lm -lboost_regex
//========================================================================================================================
TCPClient::TCPClient(const std::string& serverIP, int serverPort) : TCPClientBase(serverIP, serverPort) {}

//========================================================================================================================
TCPClient::~TCPClient(void) {}

extern "C"
{
    TCPClient* TCPClient_new(const char* serverIP, int serverPort)
    {
        std::cout << "\t" <<std::string(serverIP) << "\t" << serverPort << std::endl;
        return new TCPClient(std::string(serverIP), serverPort);
    }

    const char* sendAndReceivePacket_new(TCPClient* tcp, const char* sendBuffer)
    {
        std::string ret;
        try
        {
            //String magic necessary to prevent a cut-off of the string data
            ret = tcp->sendAndReceivePacket(std::string(sendBuffer));
            std::stringstream stream;
            stream << ret.c_str();
            static std::string str;
            str = stream.str();
            return str.c_str();
        }
        catch (const std::runtime_error &e){
            return "-1";
        }
    }

    bool connect_new(TCPClient* tcp, int pRetry)
    {
        tcp->setReceiveTimeout(5,0);
        return tcp->connect(pRetry);
    }
    bool disconnect_new(TCPClient* tcp)
    {
        return tcp->disconnect();
    }
    

}