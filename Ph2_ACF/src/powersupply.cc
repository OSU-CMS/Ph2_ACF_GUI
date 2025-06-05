#include "System/SystemController.h"
#include "Utils/CommonVisitors.h"
#include "Utils/Timer.h"
#include "Utils/argvparser.h"
#include "Utils/gui_logger.h"
#include "tools/Channel.h"
#ifdef __POWERSUPPLY__
// Libraries
#include "DeviceHandler.h"
#include "PowerSupply.h"
#include "PowerSupplyChannel.h"
#endif

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;
using namespace CommandLineProcessing;
INITIALIZE_EASYLOGGINGPP

int main(int argc, char** argv)
{
#ifdef __POWERSUPPLY__
    // configure the logger
    el::Configurations conf(std::string(std::getenv("PH2ACF_BASE_DIR")) + "/settings/logger.conf");
    el::Loggers::reconfigureAllLoggers(conf);

    el::Helpers::installLogDispatchCallback<gui::LogDispatcher>("GUILogDispatcher");

    ArgvParser cmd;

    // init
    cmd.setIntroductoryDescription("CMS Ph2_ACF  system test application");
    // error codes
    cmd.addErrorCode(0, "Success");
    cmd.addErrorCode(1, "Error");
    // options
    cmd.setHelpOption("h", "help", "Print this help page");

    cmd.defineOption("name", "Name of the power supply as described in the HW file", ArgvParser::OptionRequiresValue);
    cmd.defineOptionAlternative("name", "n");

    cmd.defineOption("channel", "Name of the channel as described in the HW file to set configurations", ArgvParser::OptionRequiresValue);
    cmd.defineOptionAlternative("channel", "c");

    cmd.defineOption("voltage", "Voltage to be set at given channel", ArgvParser::OptionRequiresValue);
    cmd.defineOptionAlternative("voltage", "v");

    cmd.defineOption("i_max", "Maximum allowed current to be set at given channel", ArgvParser::OptionRequiresValue);
    cmd.defineOptionAlternative("i_max", "seti_max");

    cmd.defineOption("v_max", "Voltage protection setting for the given channel", ArgvParser::OptionRequiresValue);
    cmd.defineOptionAlternative("v_max", "setv_max");

    cmd.defineOption("off", "Turn off channel of power supply, if no channel is given, turn off all channels");
    cmd.defineOptionAlternative("off", "turnoff");

    cmd.defineOption("on", "Turn on channel of power supply, requires a channel");
    cmd.defineOptionAlternative("on", "turnon");

    cmd.defineOption("file", "Hw Description File . Default value: settings/D19CDescription_Cic2.xml", ArgvParser::OptionRequiresValue);
    cmd.defineOptionAlternative("file", "f");

    cmd.defineOption("gui", "Named pipe for GUI communication", ArgvParser::OptionRequiresValue /*| ArgvParser::OptionRequired*/);
    cmd.defineOptionAlternative("gui", "g");

    cmd.defineOption("measure", "Measure current ");
    cmd.defineOptionAlternative("measure", "m");

    cmd.defineOption("report", "Report Status");
    cmd.defineOptionAlternative("report", "r");

    int result = cmd.parse(argc, argv);

    if(result != ArgvParser::NoParserError)
    {
        LOG(INFO) << cmd.parseErrorDescription(result);
        exit(1);
    }
    // now query the parsing results
    std::string cHWFile      = (cmd.foundOption("file")) ? cmd.optionValue("file") : "settings/D19CDescription_Cic2_PS.xml";
    std::string cPowerSupply = (cmd.foundOption("name")) ? cmd.optionValue("name") : "";
    std::string cChannel     = (cmd.foundOption("channel")) ? cmd.optionValue("channel") : "";
    double      cVoltsLimit  = (cmd.foundOption("v_max")) ? std::stod(cmd.optionValue("v_max").c_str()) : 10.5;
    double      cAmpsLimit   = (cmd.foundOption("i_max")) ? std::stod(cmd.optionValue("i_max").c_str()) : 1.3;
    double      cVolts       = (cmd.foundOption("v")) ? std::stod(cmd.optionValue("v").c_str()) : 0;

    bool cTurnOff = cmd.foundOption("off");
    bool cTurnOn  = cmd.foundOption("on");
    // Avoid undefined state
    if(cTurnOn && cTurnOff)
    {
        cTurnOff = true;
        cTurnOn  = false;
    }

    // Check if there is a gui involved, if not dump information in a dummy pipe
    std::string guiPipe = (cmd.foundOption("gui")) ? cmd.optionValue("gui") : "/tmp/guiDummyPipe";
    gui::init(guiPipe.c_str());

    std::string docPath = cHWFile;
    LOG(INFO) << "Init PS with " << docPath;

    pugi::xml_document docSettings;

    DeviceHandler theHandler;
    theHandler.readSettings(docPath, docSettings);

    try
    {
        theHandler.getPowerSupply(cPowerSupply);
    }
    catch(const std::out_of_range& oor)
    {
        std::cerr << "Out of Range error: " << oor.what() << '\n';
        exit(0);
    }

    // Get all channels of the powersupply
    std::vector<std::pair<std::string, bool>> channelNames;
    pugi::xml_document                        doc;
    if(!doc.load_file(cHWFile.c_str())) return -1;
    pugi::xml_node devices = doc.child("Devices");
    for(pugi::xml_node ps = devices.first_child(); ps; ps = ps.next_sibling())
    {
        std::string s(ps.attribute("ID").value());
        if(s == cPowerSupply)
        {
            for(pugi::xml_node channel = ps.child("Channel"); channel; channel = channel.next_sibling("Channel"))
            {
                std::string name(channel.attribute("ID").value());
                std::string use(channel.attribute("InUse").value());

                channelNames.push_back(std::make_pair(name, use == "Yes"));
            }
        }
    }
    if(cmd.foundOption("measure"))
    {
        LOG(INFO) << BOLDBLUE << "Measuring current consumption on all channels.." << RESET;
        for(auto channelName: channelNames)
        {
            std::string current = std::to_string(theHandler.getPowerSupply(cPowerSupply)->getChannel(channelName.first)->getCurrent());
            std::string voltage = std::to_string(theHandler.getPowerSupply(cPowerSupply)->getChannel(channelName.first)->getVoltage());
            LOG(INFO) << "\tV(meas) [Ch#" << channelName.first << "] :\t" << BOLDWHITE << voltage << "\tI(meas) [Ch#" << channelName.first << "] :\t" << BOLDWHITE << current << RESET;
        }
    }

    if(cmd.foundOption("channel"))
    {
        theHandler.getPowerSupply(cPowerSupply)->getChannel(cChannel);
        if(cTurnOff) // No channel given but turn off called -> Turn off power supply master output
        {
            LOG(INFO) << "Turn off output on channel " << cChannel << " on power supply " << cPowerSupply;
            theHandler.getPowerSupply(cPowerSupply)->getChannel(cChannel)->turnOff();
        }
        if(cTurnOn)
        {
            LOG(INFO) << "Turn on output on channel " << cChannel << " on power supply " << cPowerSupply;
            theHandler.getPowerSupply(cPowerSupply)->getChannel(cChannel)->turnOn();
        }
        if(cmd.foundOption("v_max")) { theHandler.getPowerSupply(cPowerSupply)->getChannel(cChannel)->setOverVoltageProtection(cVoltsLimit); }
        if(cmd.foundOption("i_max")) { theHandler.getPowerSupply(cPowerSupply)->getChannel(cChannel)->setCurrentCompliance(cAmpsLimit); }
        if(cmd.foundOption("v")) { theHandler.getPowerSupply(cPowerSupply)->getChannel(cChannel)->setVoltage(cVolts); }
        LOG(INFO) << BOLDWHITE << cPowerSupply << " status of channel " << cChannel << ":" RESET;
        std::string channelName(cChannel);
        bool        isOn       = theHandler.getPowerSupply(cPowerSupply)->getChannel(cChannel)->isOn();
        std::string isOnResult = isOn ? "1" : "0";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::string voltageCompliance = std::to_string(theHandler.getPowerSupply(cPowerSupply)->getChannel(cChannel)->getVoltageCompliance());
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::string voltage = std::to_string(theHandler.getPowerSupply(cPowerSupply)->getChannel(cChannel)->getVoltage());
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::string currentCompliance = std::to_string(theHandler.getPowerSupply(cPowerSupply)->getChannel(cChannel)->getCurrentCompliance());
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::string current = "-";
        if(isOn) { current = std::to_string(theHandler.getPowerSupply(cPowerSupply)->getChannel(cChannel)->getCurrent()); }
        LOG(INFO) << "\tIsOn:\t\t" << BOLDWHITE << isOn << RESET;
        LOG(INFO) << "\tV_max(set):\t\t" << BOLDWHITE << voltageCompliance << RESET;
        LOG(INFO) << "\tV(meas):\t" << BOLDWHITE << voltage << RESET;
        LOG(INFO) << "\tI_max(set):\t" << BOLDWHITE << currentCompliance << RESET;
        LOG(INFO) << "\tI(meas):\t" << BOLDWHITE << current << RESET;
        gui::data((channelName + ">IsOn").c_str(), isOnResult.c_str());
        gui::data((channelName + ">v_max_set").c_str(), voltageCompliance.c_str());
        gui::data((channelName + ">v_meas").c_str(), voltage.c_str());
        gui::data((channelName + ">i_max_set").c_str(), currentCompliance.c_str());
        gui::data((channelName + ">i_meas").c_str(), current.c_str());
    }
    else // No channel given
    {
        if(cTurnOff) // No channel given but turn off called -> Turn off power supply master output
        {
            LOG(INFO) << "Turn off all channels" << cPowerSupply;
            for(auto channelName: channelNames) { theHandler.getPowerSupply(cPowerSupply)->getChannel(channelName.first)->turnOff(); }
        }
        if(cTurnOn) // No channel given but turn off called -> Turn off power supply master output
        {
            LOG(INFO) << "Turn on all channels" << cPowerSupply;
            for(auto channelName: channelNames) { theHandler.getPowerSupply(cPowerSupply)->getChannel(channelName.first)->turnOn(); }
        }
        if(cmd.foundOption("report"))
        {
            // Give complete status reoort for all channels in the power supply
            for(auto channelName: channelNames)
            {
                if(channelName.second)
                {
                    LOG(INFO) << BOLDWHITE << cPowerSupply << " status of channel " << channelName.first << ":" RESET;
                    bool        isOn       = theHandler.getPowerSupply(cPowerSupply)->getChannel(channelName.first)->isOn();
                    std::string isOnResult = isOn ? "1" : "0";
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    std::string voltageCompliance = std::to_string(theHandler.getPowerSupply(cPowerSupply)->getChannel(channelName.first)->getVoltageCompliance());
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    std::string voltage = std::to_string(theHandler.getPowerSupply(cPowerSupply)->getChannel(channelName.first)->getVoltage());
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    std::string currentCompliance = std::to_string(theHandler.getPowerSupply(cPowerSupply)->getChannel(channelName.first)->getCurrentCompliance());
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    std::string current = "-";
                    if(isOn) { current = std::to_string(theHandler.getPowerSupply(cPowerSupply)->getChannel(channelName.first)->getCurrent()); }
                    LOG(INFO) << "\tIsOn:\t\t" << BOLDWHITE << isOnResult << RESET;
                    LOG(INFO) << "\tV_max(set):\t\t" << BOLDWHITE << voltageCompliance << RESET;
                    LOG(INFO) << "\tV(meas):\t" << BOLDWHITE << voltage << RESET;
                    LOG(INFO) << "\tI_max(set):\t" << BOLDWHITE << currentCompliance << RESET;
                    LOG(INFO) << "\tI(meas):\t" << BOLDWHITE << current << RESET;
                    gui::data((channelName.first + ">IsOn").c_str(), isOnResult.c_str());
                    gui::data((channelName.first + ">v_max_set").c_str(), voltageCompliance.c_str());
                    gui::data((channelName.first + ">v_meas").c_str(), voltage.c_str());
                    gui::data((channelName.first + ">i_max_set").c_str(), currentCompliance.c_str());
                    gui::data((channelName.first + ">i_meas").c_str(), current.c_str());
                }
            }
        }
    }
#endif

    return 0;
}
