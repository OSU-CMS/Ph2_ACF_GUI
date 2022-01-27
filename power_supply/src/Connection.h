#ifndef CONNECTION_H
#define CONNECTION_H

#include <iostream>
#include <string>

/*!
************************************************
 \class Connection.
 \brief Class for possibile comunication
 connections.
************************************************
*/

class Connection
{
  public:
    Connection() {}
    virtual ~Connection() {}

    virtual bool        isOpen()                          = 0;
    virtual void        write(const std::string& command) = 0;
    virtual std::string read(const std::string& command)  = 0;
};

#endif /* CONNECTION_H */
