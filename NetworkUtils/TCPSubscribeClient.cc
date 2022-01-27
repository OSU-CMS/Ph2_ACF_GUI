#include "../NetworkUtils/TCPSubscribeClient.h"

//========================================================================================================================
TCPSubscribeClient::TCPSubscribeClient(const std::string& serverIP, int serverPort) : TCPClientBase(serverIP, serverPort) {}

//========================================================================================================================
TCPSubscribeClient::~TCPSubscribeClient(void) {}
