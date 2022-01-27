#ifndef CAENERRORS_H
#define CAENERRORS_H

#include <string>

static std::string convertError(unsigned errorCode)
{
    switch(errorCode)
    {
    case 0x0: return "No error";
    case 0x1: return "Operating system error";
    case 0x2: return "Writing error";
    case 0x3: return "Reading error";
    case 0x4: return "Time out error";
    case 0x5: return "Command Front End application is down";
    case 0x6: return "Communication with system not yet connected by a Login command";
    case 0x7: return "Execute Command not yet implemented";
    case 0x8: return "Get Property not yet implemented";
    case 0x9: return "Set Property not yet implemented";
    case 0xa: return "Communication with RS232 not yet implemented";
    case 0xb: return "User memory not sufficient";
    case 0xc: return "Value out of range";
    case 0xd: return "Property not yet implemented";
    case 0xe: return "Property not found";
    case 0xf: return "Command not found";
    case 0x10: return "Not a Property";
    case 0x11: return "Not a reading Property";
    case 0x12: return "Not a writing Property";
    case 0x13: return "Not a Command";
    case 0x14: return "Configuration change";
    case 0x15: return "Parameter’s Property not found";
    case 0x16: return "Parameter not found";
    case 0x17: return "No data present";
    case 0x18: return "Device already open";
    case 0x19: return "Too Many devices opened";
    case 0x1A: return "Function Parameter not valid";
    case 0x1B: return "Function not available for the connected device";
    case 0x1C: return "SOCKET ERROR";
    case 0x1D: return "COMMUNICATION ERROR";
    case 0x1E: return "NOT YET IMPLEMENTED";
    case 0x1000 + 1: return "CONNECTED";
    case 0x1000 + 2: return "NOTCONNECTED";
    case 0x1000 + 3: return "OS";
    case 0x1000 + 4: return "LOG IN FAILED";
    case 0x1000 + 5: return "LOG OUT FAILED";
    case 0x1000 + 6: return "LINK NOT SUPPORTED";
    default: { return std::string("Error ") + std::to_string(errorCode) + " not recognized.";
    }
#endif
