/*!
 * \authors Mattia Lizzo <mattia.lizzo@cern.ch>, INFN-Firenze
 * \date Sep 2 2019
 */

#include "SerialConnection.h"

SerialConnection::SerialConnection(const std::string& portName, int baudRate, bool flowControl, bool parity, bool removeEcho, const std::string& terminator, const std::string suffix, int timeout)
{
    fSerial.setPortName(portName);
    fSerial.setBaudRate(baudRate);
    fSerial.setFlowControl(flowControl);
    fSerial.setParity(parity);
    fSerial.setRemoveEcho(removeEcho);
    fSerial.setTerminator(terminator);
    fSerial.setReadSuffix(suffix);
    fSerial.setTimeout(timeout);
    fPortIsOpen = fSerial.openPort();
}

SerialConnection::SerialConnection(const std::string& portName, const std::string& terminator, bool removeEcho, const std::string suffix, int timeout)
{
    // For the USB TMC communication //
    fSerial.setPortName(portName);
    fSerial.setTerminator(terminator);
    fSerial.setReadSuffix(suffix);
    fSerial.setTimeout(timeout);
    fSerial.setUSBTMCMode(true);
    fSerial.setRemoveEcho(removeEcho);
    fPortIsOpen = fSerial.openPort();
}

bool SerialConnection::isOpen() { return fPortIsOpen; }

void SerialConnection::write(const std::string& command) { fSerial.writeData(command); }

std::string SerialConnection::read(const std::string& command)
{
    std::string answer;
    fSerial.writeReadBack(command, answer);
    return answer;
}

RS232Conn SerialConnection::getSerialConnection() { return fSerial; }
