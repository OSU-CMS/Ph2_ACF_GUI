#include "Utils/MiddlewareInterface.h"
#include "MessageUtils/cpp/QueryMessage.pb.h"
#include "MessageUtils/cpp/ReplyMessage.pb.h"
#include "Utils/ConfigureInfo.h"
#include "Utils/StartInfo.h"
#include <iostream>

//========================================================================================================================
MiddlewareInterface::MiddlewareInterface(std::string serverIP, int serverPort) : TCPClient(serverIP, serverPort) {}

//========================================================================================================================
MiddlewareInterface::~MiddlewareInterface(void) { std::cout << __PRETTY_FUNCTION__ << "DESTRUCTOR!" << std::endl; }

//========================================================================================================================
std::string MiddlewareInterface::sendCommand(const std::string& command)
{
    try
    {
        return TCPClient::sendAndReceivePacket(command);
    }
    catch(const std::exception& e)
    {
        std::cout << __PRETTY_FUNCTION__ << "Error: " << e.what() << " Need to take some actions here!" << std::endl;
        return "";
    }
}

//========================================================================================================================
void MiddlewareInterface::initialize(void)
{
    if(!TCPClient::connect())
    {
        std::cout << __PRETTY_FUNCTION__ << "ERROR CAN'T CONNECT TO SERVER!" << std::endl;
        abort();
    }

    MessageUtils::QueryMessage theQuery;
    theQuery.mutable_query_type()->set_type(MessageUtils::QueryType::INITIALIZE);
    std::string theCommandString;
    theQuery.SerializeToString(&theCommandString);
    std::string readBuffer = sendCommand(theCommandString);
    std::cout << __PRETTY_FUNCTION__ << "DONE WITH Initialize-" << readBuffer << "-" << std::endl;
}

//========================================================================================================================
void MiddlewareInterface::configure(const ConfigureInfo& theConfigureInfo)
{
    std::string theCommandString = theConfigureInfo.createProtobufMessage();
    std::string readBuffer       = sendCommand(theCommandString);
    std::cout << __PRETTY_FUNCTION__ << "DONE WITH Configure-" << readBuffer << "-" << std::endl;
}

//========================================================================================================================
void MiddlewareInterface::halt(void)
{
    std::cout << __PRETTY_FUNCTION__ << "Sending Halt!" << std::endl;

    MessageUtils::QueryMessage theQuery;
    theQuery.mutable_query_type()->set_type(MessageUtils::QueryType::HALT);
    std::string theCommandString;
    theQuery.SerializeToString(&theCommandString);
    std::string readBuffer = sendCommand(theCommandString);

    std::cout << __PRETTY_FUNCTION__ << "DONE WITH Halt-" << readBuffer << "-" << std::endl;
}

//========================================================================================================================
void MiddlewareInterface::pause(void) {}

//========================================================================================================================
void MiddlewareInterface::resume(void) {}

//========================================================================================================================
void MiddlewareInterface::start(const StartInfo& theStartInfo)
{
    std::string theCommandString = theStartInfo.createProtobufMessage();
    std::string readBuffer       = sendCommand(theCommandString);
    std::cout << __PRETTY_FUNCTION__ << "DONE WITH Start-" << readBuffer << "-" << std::endl;
}

//========================================================================================================================
std::string MiddlewareInterface::status()
{
    MessageUtils::QueryMessage theQuery;
    theQuery.mutable_query_type()->set_type(MessageUtils::QueryType::STATUS);
    std::string theCommandString;
    theQuery.SerializeToString(&theCommandString);
    std::string readBuffer = sendCommand(theCommandString);

    MessageUtils::ReplyMessage theStatus;
    theStatus.ParseFromString(readBuffer);

    std::string status;
    switch(theStatus.reply_type().type())
    {
    case MessageUtils::ReplyType::RUNNING:
    {
        status = "Running";
        break;
    }
    case MessageUtils::ReplyType::SUCCESS:
    {
        status = "Done";
        break;
    }
    case MessageUtils::ReplyType::ERROR:
    {
        status = "Error";
        break;
    }

    default: status = "Unknown"; break;
    }

    std::cout << __PRETTY_FUNCTION__ << "Status: " << status << std::endl;
    std::cout << __PRETTY_FUNCTION__ << "Message: " << theStatus.message() << std::endl;
    return status;
}
//========================================================================================================================
void MiddlewareInterface::stop(void)
{
    std::cout << __PRETTY_FUNCTION__ << "Sending Stop!" << std::endl;
    MessageUtils::QueryMessage theQuery;
    theQuery.mutable_query_type()->set_type(MessageUtils::QueryType::STOP);
    std::string theCommandString;
    theQuery.SerializeToString(&theCommandString);
    std::string readBuffer = sendCommand(theCommandString);

    std::cout << __PRETTY_FUNCTION__ << "DONE WITH Stop-" << readBuffer << "-" << std::endl;
}
