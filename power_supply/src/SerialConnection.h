#ifndef SerialConnection_H
#define SerialConnection_H

#include "Connection.h"
#include "rs232.h"

class SerialConnection : public Connection
{
  private:
    RS232Conn fSerial;
    bool      fPortIsOpen;

  public:
    SerialConnection(const std::string& portName, int baudRate, bool flowControl, bool parity, bool removeEcho, const std::string& terminator, const std::string suffix, int timeout);
    SerialConnection(const std::string& portName, const std::string& terminator, bool removeEcho, const std::string suffix, int timeout);

    RS232Conn getSerialConnection();

    // Override methods
    bool        isOpen() override;
    void        write(const std::string& command) override;
    std::string read(const std::string& command) override;
};

#endif
