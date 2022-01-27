#ifndef TTIPLERRORS_H
#define TTIPLERRORS_H

#include <string>

#define POWERON 0x80
#define NOTUSED 0x42
#define COMMANDERROR 0x20
#define EXECUTIONERROR 0x10
#define TIMEOUT 0x8
#define QUERYERROR 0x4
#define OPERATIONCOMPLETE 0x1

static std::string convertErrorTSX(unsigned errorCode)
{
    switch(errorCode)
    {
    case 0: return "No error";
    case 1: return "Checksum error in non-volatile ram at power on.";
    case 2: return "Output stage failed to respond - possibly a system fault.";
    case 3: return "Output stage trip has occurred - attempting to recover.";
    case 100: return "Maximum set voltage value exceeded.";
    case 101: return "Maximum set amps value exceeded.";
    case 102: return "Minimum set voltage exceeded.";
    case 103: return "Minimum set amps value exceeded.";
    case 104: return "Maximum delta voltage value exceeded.";
    case 105: return "Maximum delta amps value exceeded.";
    case 107: return "Minimum set OVP value exceeded.";
    case 108: return "Maximum set OVP value exceeded.";
    case 109: return "Minimum delta amps value exceeded.";
    case 110: return "Minimum delta voltage value exceeded.";
    case 114: return "Illegal bus address requested.";
    case 115: return "Illegal store number.";
    case 116: return "Recall empty store requested.";
    case 117: return "Stored data is corrupt.";
    case 118: return "Output stage has tripped (OVP or Temperature).";
    case 119: return "Value out of range.";
    default: return std::string("Error ") + std::to_string(errorCode) + " not recognized.";
    }
}

static std::string convertErrorPL(unsigned errorCode)
{
    switch(errorCode)
    {
    case 0: return "No error encountered.";
    case 1: return "Internal hardware error detected.";
    case 2: return "Internal hardware error detected.";
    case 3: return "Internal hardware error detected.";
    case 4: return "Internal hardware error detected.";
    case 5: return "Internal hardware error detected.";
    case 6: return "Internal hardware error detected.";
    case 7: return "Internal hardware error detected.";
    case 8: return "Internal hardware error detected.";
    case 9: return "Internal hardware error detected.";
    case 100:
        return "Range error. The numeric value sent is not allowed. This includes "
               "numbers that are too big or too "
               "small for the parameter being set and non-integers being sent "
               "where only integers are allowed.";
    case 101:
        return "A recall of set up data has been requested but the store specified "
               "contains corrupted data. This "
               "indicates either a hardware fault or a temporary data corruption, "
               "which can be corrected by writing "
               "data to the store again.";
    case 102:
        return "A rec  ll of set up data has been requested but the store "
               "specified does not contain any data.";
    case 103:
        return "Attempt to read or write a command on the second output when it is "
               "not available. Typically this will "
               "occur if attempting to program the second output on single channel "
               "instruments or on a two-channel "
               "instrument which is set to parallel mode.";
    case 104:
        return "Command not valid with output on. This is typically caused by "
               "using the 'IRANGE <n>' command without "
               "first turning the output off.";
    case 200:
        return "Read Only: An attempt has been made to change the settings of the "
               "instrument from an interface without "
               "write privileges, see the Interface Locking section.";
    default: return std::string("Error ") + std::to_string(errorCode) + " not recognized.";
    }
}

static std::string convertErrorMX(unsigned errorCode)
{
    switch(errorCode)
    {
    case 0: return "No error encountered.";
    case 100: return "Numeric Error: the parameter value sent was outside the permitted range for the command in the present circumstances.";
    case 102: return "Recall Error: a recall of set up data has been requested but the store specified does not contain any data.";
    case 103:
        return "Command Invalid: the command is recognised but is not valid in the current circumstances. Typical examples would by trying to change V2 directly while the outputs are in voltage tracking mode with V1 as the master.";
    case 104:
        return "Range Change Error: An operation requiring a range change was requested but could not be completed. Typically this occurs because >0.5V was still present on output 1 and/or output 2 terminals at the time the command was executed.";
    case 200: return "Access Denied: an attempt was made to change the instrument’s settings from an interface which is locked out of write privileges by a lock held by another interface.";
    default: return std::string("Error ") + std::to_string(errorCode) + " not recognized.";
    }
}
#endif
