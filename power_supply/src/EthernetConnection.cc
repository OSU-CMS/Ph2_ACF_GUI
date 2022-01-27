/*!
 * \authors Mattia Lizzo <mattia.lizzo@cern.ch>, INFN-Firenze
 * \date Sep 2 2019
 */

#include "EthernetConnection.h"
#include <boost/bind.hpp>
#include <chrono>
#include <thread>

using namespace boost::asio;

std::unordered_map<std::string, std::weak_ptr<EthernetConnection>> SharedEthernetConnection::map;

EthernetConnection::EthernetConnection(const std::string& IPaddress, int port, const unsigned timeout) : fSocket(fIoService), fTimeout(timeout)
{
    std::cout << "Checking IP " << IPaddress << " and port " << port << std::endl;
    boost::system::error_code ec;
    fSocket.connect(ip::tcp::endpoint(ip::address::from_string(IPaddress), port), ec);
    fPortIsOpen = !ec;
    if(ec)
    {
        std::stringstream error;
        error << "EthernetConnection ERROR: Cannot connect to IP: " << IPaddress << " port: " << port;
        throw std::runtime_error(error.str());
    }
    // Put the socket into non-blocking mode.
    // ip::tcp::socket::non_blocking non_blocking_io(true);
    // fSocket.io_control(non_blocking_io);
    fSocket.non_blocking(true);
}

bool EthernetConnection::isOpen() { return fPortIsOpen; }

void EthernetConnection::write(const std::string& command) { boost::asio::write(fSocket, buffer(command + '\n')); }

std::string EthernetConnection::read(const std::string& command)
{
    streambuf sb;
    write(command);
    std::size_t n         = 0;
    unsigned    timer     = 0;
    unsigned    retryTime = 10;
    while(n == 0 && timer < fTimeout)
    {
        try
        {
            n = read_until(fSocket, sb, '\n');
        }
        catch(std::exception& e)
        {
            // std::cout << timer << std::endl;
            timer += retryTime;
            std::this_thread::sleep_for(std::chrono::milliseconds(retryTime));
        }
    }
    if(n == 0)
    {
        std::stringstream error;
        error << "Ethernet ERROR: Read  timeout occurred after sending command: " << command;
        throw std::runtime_error(error.str());
    }

    auto bufs = sb.data();
    return {buffers_begin(bufs), buffers_begin(bufs) + n};
}

std::string EthernetConnection::IPaddress() { return fSocket.remote_endpoint().address().to_string(); }

SharedEthernetConnection::SharedEthernetConnection(const std::string& IPaddress, int port, const unsigned timeout)
{
    auto it = map.find(IPaddress);
    if(it == map.end())
    {
        p_connection = std::shared_ptr<EthernetConnection>(new EthernetConnection(IPaddress, port, timeout), [](EthernetConnection* ptr) {
            map.erase(ptr->IPaddress());
            delete ptr;
        });
        map.insert({IPaddress, p_connection});
    }
    else
    {
        p_connection = it->second.lock();
        std::cout << "use_count: " << p_connection.use_count() << "\n";
    }
}

bool SharedEthernetConnection::isOpen() { return p_connection->isOpen(); }

void SharedEthernetConnection::write(const std::string& command) { p_connection->write(command); }

std::string SharedEthernetConnection::read(const std::string& command) { return p_connection->read(command); }
