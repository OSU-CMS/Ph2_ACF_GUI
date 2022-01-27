#include <ctime>
#include <iostream>
#include <stdio.h>
#include <unistd.h>

#include "ITPowerSupplyInterface.h"

//========================================================================================================================
ITPowerSupplyInterface::ITPowerSupplyInterface(int serverPort, std::string configFileName) : TCPServer(serverPort, 10) { fHandler.readSettings(configFileName, fDocSettings); }

//========================================================================================================================
ITPowerSupplyInterface::~ITPowerSupplyInterface(void) { std::cout << __PRETTY_FUNCTION__ << " DESTRUCTOR" << std::endl; }

//========================================================================================================================
std::string ITPowerSupplyInterface::interpretMessage(const std::string& buffer)
{
    std::lock_guard<std::mutex> theGuard(fMutex);

    std::cout << __PRETTY_FUNCTION__ << " Message received from Ph2ACF: " << buffer << std::endl;

    if(buffer == "Initialize") // Changing the status changes the mode in
                               // threadMain (BBC) function
    { return "InitializeDone"; }
    else if(buffer.substr(0, 5) == "Start") // Changing the status changes the
                                            // mode in threadMain (BBC)
                                            // function
    {
        return "StartDone";
    }
    else if(buffer.substr(0, 4) == "Stop")
    {
        return "StopDone";
    }
    else if(buffer.substr(0, 4) == "Halt")
    {
        return "HaltDone";
    }
    else if(buffer == "Pause")
    {
        return "PauseDone";
    }
    else if(buffer == "Resume")
    {
        return "ResumeDone";
    }
    else if(buffer.substr(0, 9) == "Configure")
    {
        return "ConfigureDone";
    }
    else if(buffer.substr(0, 5) == "K2410")
    {
        std::cout << "Special handling of K2410" << std::endl;
        std::string command = getVariableValue("K2410", buffer);

        std::string powerSupplyId = getVariableValue("PowerSupplyId", buffer);
        std::string channelId     = getVariableValue("ChannelId", buffer);
        auto        channelK2410  = static_cast<KeithleyChannel*>(fHandler.getPowerSupply(powerSupplyId)->getChannel(channelId));

        if(command == "setupIsense")
        {        
            float currentCompliance = std::stof(getVariableValue("CurrCompl", buffer));
            
            channelK2410->setVoltageMode();
            channelK2410->setVoltage(0.0);
            channelK2410->setCurrentCompliance(currentCompliance);
        }
        else if(command == "setupVsense")
        {
            float voltageCompliance = std::stof(getVariableValue("VoltCompl", buffer));

            channelK2410->setCurrentMode();
            channelK2410->setCurrent(0.0);
            channelK2410->setParameter("Isrc_range", (float)1e-6);
            channelK2410->setVoltageCompliance(voltageCompliance);
        }
        else if(command == "setupIsource")
        {
            float current           = std::stof(getVariableValue("Current", buffer));
            float voltageCompliance = std::stof(getVariableValue("VoltCompl", buffer));

            channelK2410->setCurrentMode();
            channelK2410->setCurrent(current);
            channelK2410->setVoltageCompliance(voltageCompliance);
        }
        else if(command == "setupVsource")
        {
            float voltage           = std::stof(getVariableValue("Voltage", buffer));
            float currentCompliance = std::stof(getVariableValue("CurrCompl", buffer));

            channelK2410->setVoltageMode();
            channelK2410->setVoltage(voltage);
            channelK2410->setCurrentCompliance(currentCompliance);
        }
        else if(command == "SetVoltage")
        {
            std::string powerSupplyId = getVariableValue("PowerSupplyId", buffer);
            std::string channelId     = getVariableValue("ChannelId", buffer);
            float voltage             = std::stof(getVariableValue(",Voltage", buffer));   // "," important otherwise will find "SetVoltage" as variable

            std::cout << "Setting voltage to " << std::to_string(voltage) << " V for Power supply = " << powerSupplyId 
                    << " ChannelId = " << channelId << std::endl;

            channelK2410->setParameter("Vsrc_range", getVoltageRange(voltage));
            channelK2410->setVoltage(voltage);
        }
        else
        {
            std::cerr << __PRETTY_FUNCTION__ << " Couldn't recognige K2410 command: " << buffer << ". Aborting..." << std::endl;
            return "Didn't understand the K2410 command!";
        }

        return "K2410 done";
    }
    else if(buffer.substr(0, 18) == "GetDeviceConnected")
    {
        std::string replayMessage;
        replayMessage += "TimeStamp:" + getTimeStamp();
        replayMessage += ",ChannelList:{";
        for(const auto& readoutChannel: fHandler.getReadoutList()) { replayMessage += readoutChannel + ","; }
        replayMessage.erase(replayMessage.size() - 1);
        replayMessage += "}";
        return replayMessage;
    }
    else if(buffer.substr(0, 6) == "TurnOn")
    {
        std::string powerSupplyId = getVariableValue("PowerSupplyId", buffer);
        std::string channelId     = getVariableValue("ChannelId", buffer);
        auto        channel       = fHandler.getPowerSupply(powerSupplyId)->getChannel(channelId);
        channel->turnOn();
        std::cout << "Power supply = " << powerSupplyId << " ChannelId = " << channelId << " is On : " << channel->isOn() << std::endl;
        return "TurnOnDone";
    }
    else if(buffer.substr(0, 7) == "TurnOff")
    {
        std::string powerSupplyId = getVariableValue("PowerSupplyId", buffer);
        std::string channelId     = getVariableValue("ChannelId", buffer);
        auto        channel       = fHandler.getPowerSupply(powerSupplyId)->getChannel(channelId);
        channel->turnOff();
        std::cout << "Power supply = " << powerSupplyId << " ChannelId = " << channelId << " is On : " << channel->isOn() << std::endl;
        return "TurnOffDone";
    }
    else if(buffer.substr(0, 9) == "GetStatus")
    {
        std::string replayMessage;
        replayMessage += "TimeStamp:" + getTimeStamp();
        for(const auto& readoutChannel: fHandler.getStatus()) { replayMessage += ("," + readoutChannel); }
        return replayMessage;
    }
    else if(buffer.substr(0, 10) == "SetVoltage")
    {
        std::string powerSupplyId = getVariableValue("PowerSupplyId", buffer);
        std::string channelId     = getVariableValue("ChannelId", buffer);
        float voltage             = std::stof(getVariableValue(",Voltage", buffer));   // "," important otherwise will find "SetVoltage" as variable

        std::cout << "Setting voltage to " << std::to_string(voltage) << " V for Power supply = " << powerSupplyId 
                  << " ChannelId = " << channelId << std::endl;

        fHandler.getPowerSupply(powerSupplyId)->getChannel(channelId)->setVoltage(voltage);

        return "SetVoltageDone";
    }
    else if(buffer.substr(0, 10) == "GetVoltage")
    {
        std::string powerSupplyId = getVariableValue("PowerSupplyId", buffer);
        std::string channelId     = getVariableValue("ChannelId", buffer);

        float voltage = fHandler.getPowerSupply(powerSupplyId)->getChannel(channelId)->getOutputVoltage();

        std::cout << "Got voltage = " << std::to_string(voltage) << " V from Power supply = " << powerSupplyId 
                  << " ChannelId = " << channelId << std::endl;
        return std::to_string(voltage);
    }
    else if(buffer.substr(0, 10) == "SetCurrent")
    {
        std::string powerSupplyId = getVariableValue("PowerSupplyId", buffer);
        std::string channelId     = getVariableValue("ChannelId", buffer);
        float current             = std::stof(getVariableValue(",Current", buffer));   // "," important otherwise will find "SetVoltage" as variable

        std::cout << "Setting current to " << std::to_string(current) << " A for Power supply = " << powerSupplyId 
                  << " ChannelId = " << channelId << std::endl;

        fHandler.getPowerSupply(powerSupplyId)->getChannel(channelId)->setCurrent(current);

        return "SetCurrentDone";
    }
    else if(buffer.substr(0, 10) == "GetCurrent")
    {
        std::string powerSupplyId = getVariableValue("PowerSupplyId", buffer);
        std::string channelId     = getVariableValue("ChannelId", buffer);

        float current = fHandler.getPowerSupply(powerSupplyId)->getChannel(channelId)->getCurrent();

        std::cout << "Got current = " << std::to_string(current) << " A from Power supply = " << powerSupplyId 
                  << " ChannelId = " << channelId << std::endl;
        return std::to_string(current);
    }
    else if(buffer.substr(0, 6) == "Error:")
    {
        if(buffer == "Error: Connection closed") std::cerr << __PRETTY_FUNCTION__ << buffer << ". Closing client server connection!" << std::endl;
        return "";
    }
    else
    {
        std::cerr << __PRETTY_FUNCTION__ << " Can't recognige message: " << buffer << ". Aborting..." << std::endl;
        abort();
    }

    if(running_ || paused_) // We go through here after start and resume or
                            // pause: sending back current status
    { std::cout << "Getting time and status here" << std::endl; }

    return "Didn't understand the message!";
}

float ITPowerSupplyInterface::getVoltageRange(float voltage)
{
    if(voltage < (float)0.2)
        return 0.2;
    else if(voltage < 2) 
        return 2;
    else if(voltage < 20) 
        return 20;
    else 
        return 1000;
}

//========================================================================================================================
std::string ITPowerSupplyInterface::getTimeStamp()
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
