#include <boost/algorithm/string.hpp>
#include <ctime>
#include <iostream>
#include <stdio.h>
#include <unistd.h>

#include "PowerSupplyInterface.h"

//========================================================================================================================
PowerSupplyInterface::PowerSupplyInterface(int serverPort, std::string configFileName) : TCPServer(serverPort, 10) { fHandler.readSettings(configFileName, fDocSettings); }

//========================================================================================================================
PowerSupplyInterface::~PowerSupplyInterface(void) { std::cout << __PRETTY_FUNCTION__ << " DESTRUCTOR" << std::endl; }

//========================================================================================================================
std::string PowerSupplyInterface::interpretMessage(const std::string& buffer)
{
    std::lock_guard<std::mutex> theGuard(fMutex);

    std::cout << __PRETTY_FUNCTION__ << " Message received from OTSDAQ: " << buffer << std::endl;

    if(buffer.find("Initialize") != std::string::npos) // Changing the status changes the mode in
                               // threadMain (BBC) function
    {
        return "InitializeDone";
    }
    else if(buffer.find("Start") != std::string::npos) // Changing the status changes the
                                            // mode in threadMain (BBC)
                                            // function
    {
        return "StartDone";
    }
    else if(buffer.find("Stop") != std::string::npos)
    {
        return "StopDone";
    }
    else if(buffer.find("Halt") != std::string::npos)
    {
        return "HaltDone";
    }
    else if(buffer.find("Pause") != std::string::npos)
    {
        return "PauseDone";
    }
    else if(buffer.find("Resume") != std::string::npos)
    {
        return "ResumeDone";
    }
    else if(buffer.find("Configure") != std::string::npos)
    {
        return "ConfigureDone";
    }
    //else if(buffer.substr(0, 18) == "GetDeviceConnected")
    else if(buffer.find("GetDeviceConnected") < 0)
    {
        std::string replayMessage;
        replayMessage += "TimeStamp:" + getTimeStamp();
        replayMessage += ",ChannelList:{";
        for(const auto& readoutChannel: fHandler.getReadoutList()) { replayMessage += readoutChannel + ","; }
        replayMessage.erase(replayMessage.size() - 1);
        replayMessage += "}";
        return replayMessage;
    }
    //else if(buffer.substr(0, 6) == "TurnOn")
    else if(buffer.find("TurnOn") != std::string::npos)
    {
        std::string powerSupplyId = getVariableValue("PowerSupplyId", buffer);
        std::string channelId     = getVariableValue("ChannelId", buffer);
        auto        channel       = fHandler.getPowerSupply(powerSupplyId)->getChannel(channelId);
        channel->turnOn();
        std::cout << "Power supply = " << powerSupplyId << " ChannelId = " << channelId << " is On : " << channel->isOn() << std::endl;
        return "TurnOnDone";
    }
    //else if(buffer.substr(0, 7) == "TurnOff")
    else if(buffer.find("TurnOff") != std::string::npos)
    {
        std::string powerSupplyId = getVariableValue("PowerSupplyId", buffer);
        std::string channelId     = getVariableValue("ChannelId", buffer);
        auto        channel       = fHandler.getPowerSupply(powerSupplyId)->getChannel(channelId);
        channel->turnOff();
        std::cout << "Power supply = " << powerSupplyId << " ChannelId = " << channelId << " is On : " << channel->isOn() << std::endl;
        return "TurnOffDone";
    }
    //else if(buffer.substr(0, 9) == "GetStatus")
    else if(buffer.find("GetStatus") != std::string::npos)
    {
        std::string replayMessage;
        replayMessage += "TimeStamp:" + getTimeStamp();
        for(const auto& readoutChannel: fHandler.getStatus()) { replayMessage += ("," + readoutChannel); }
        return replayMessage;
    }
    else if(buffer.find("GetInfo") != std::string::npos)
    {
        std::string replayMessage;
        std::string powerSupplyId = getVariableValue("PowerSupplyId", buffer);
        //std::string channelId     = getVariableValue("ChannelId", buffer);
        auto        powersupply   = fHandler.getPowerSupply(powerSupplyId);
        replayMessage += powersupply->getInfo();
        //std::cout << replayMessage << std::endl;
        return replayMessage;
    }
    else if(buffer.find("SetVoltage") != std::string::npos)
    {
        std::string powerSupplyId = getVariableValue("PowerSupplyId", buffer);
        std::string channelId     = getVariableValue("ChannelId", buffer);
        float       voltage       = std::stof(getVariableValue("Value", buffer));
        fHandler.getPowerSupply(powerSupplyId)->getChannel(channelId)->setVoltage(voltage);
        return "SetVoltageDone";
    }
    else if(buffer.find("setOverVoltageProtection") != std::string::npos)
    {
        std::string powerSupplyId = getVariableValue("PowerSupplyId", buffer);
        std::string channelId     = getVariableValue("ChannelId", buffer);
        float       voltage       = std::stof(getVariableValue("Value", buffer));
        fHandler.getPowerSupply(powerSupplyId)->getChannel(channelId)->setOverVoltageProtection(voltage);
        return "setOverVoltageProtection";
    }  
    else if(buffer.find("GetCurrent") != std::string::npos)
    {
        std::string powerSupplyId = getVariableValue("PowerSupplyId", buffer);
        std::string channelId     = getVariableValue("ChannelId", buffer);
        float       current       = fHandler.getPowerSupply(powerSupplyId)->getChannel(channelId)->getCurrent();
        std::cout << "interface current: " << current << std::endl;
        return std::to_string(current);
    }
    else if(buffer.find("GetOutputVoltage") != std::string::npos)
    {
        std::string powerSupplyId = getVariableValue("PowerSupplyId", buffer);
        std::string channelId     = getVariableValue("ChannelId", buffer);
        float       voltage       = fHandler.getPowerSupply(powerSupplyId)->getChannel(channelId)->getOutputVoltage();
        return std::to_string(voltage);
    }
    else if(buffer.find("SetCurrentCompliance") != std::string::npos)
    {
        std::string powerSupplyId = getVariableValue("PowerSupplyId", buffer);
        std::string channelId     = getVariableValue("ChannelId", buffer);
        float       currentComp   = std::stof(getVariableValue("Value", buffer));
        fHandler.getPowerSupply(powerSupplyId)->getChannel(channelId)->setCurrentCompliance(currentComp);
        return "SetCurrentComplianceDone";
    }
    //else if(buffer.substr(0, 6) == "Error:")
    else if(buffer.find("Error:") != std::string::npos)
    {
        if(buffer == "Error: Connection closed") std::cerr << __PRETTY_FUNCTION__ << buffer << ". Closing client server connection!" << std::endl;
        return "";
    }
    else if(buffer.find("Scope:") != std::string::npos)
    {
        std::vector<std::string> arguments;
        boost::split(arguments, buffer, boost::is_any_of(":"));
        std::string          channel      = arguments.at(1);
        std::string          command      = arguments.at(2);
        ScopeAgilent*        scope        = dynamic_cast<ScopeAgilent*>(fHandler.getScope("AgilentScope"));
        ScopeAgilentChannel* scopechannel = dynamic_cast<ScopeAgilentChannel*>(scope->getChannel(channel));
        if(command.find("setEOM=") != std::string::npos)
        {
            std::vector<std::string> subargs;
            boost::split(subargs, command, boost::is_any_of("="));
            scopechannel->setEOMeasurement(subargs.at(1));
            return "Observables for EOM set.";
        }
        if(command.find("acquireEOM=") != std::string::npos)
        {
            std::vector<std::string> subargs;
            boost::split(subargs, command, boost::is_any_of("="));
            return scopechannel->acquireEOM(1000, 20, 80, subargs.at(1));
        }
    }
    else
    {
        std::cerr << __PRETTY_FUNCTION__ << " Can't recognize message: " << buffer << ". Aborting..." << std::endl;
        // Modified: abort();
    }

    if(running_ || paused_) // We go through here after start and resume or
                            // pause: sending back current status
    {
        std::cout << "Getting time and status here" << std::endl;
    }

    return "Didn't understand the message!";
}

//========================================================================================================================
std::string PowerSupplyInterface::getTimeStamp()
{
    time_t     rawtime;
    struct tm* timeinfo;
    char       buffer[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    std::string str(buffer);
    return str;
}
