#ifndef EthernetConnection_H
#define EthernetConnection_H

#include <boost/asio.hpp>
//#include <boost/beast/core/tcp_stream.hpp>

#include <unordered_map>

#include "Connection.h"

class EthernetConnection : public Connection
{
  private:
    boost::asio::io_service      fIoService;
    boost::asio::ip::tcp::socket fSocket;
    bool                         fPortIsOpen;
    const unsigned               fTimeout;

  public:
    EthernetConnection(const std::string& IPaddress, int port, const unsigned timeout = 10000);
    std::string IPaddress();

    // Override
    bool        isOpen() override;
    void        write(const std::string& command) override;
    std::string read(const std::string& command) override;
};

class SharedEthernetConnection : public Connection
{
  private:
    static std::unordered_map<std::string, std::weak_ptr<EthernetConnection>> map;
    std::shared_ptr<EthernetConnection>                                       p_connection;

  public:
    SharedEthernetConnection(const std::string& IPaddress, int port, const unsigned timeout = 10000);

    // Override
    bool        isOpen() override;
    void        write(const std::string& command) override;
    std::string read(const std::string& command) override;
};

#endif
