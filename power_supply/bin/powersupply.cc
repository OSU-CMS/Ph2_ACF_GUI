#include "utils/ConsoleColor.h"
#include "utils/easylogging++.h"
#include "utils/gui_logger.h"
#include <boost/program_options.hpp> //!For command line arg parsing

#include <chrono>
#include <thread>

// Libraries
#include "DeviceHandler.h"
#include "PowerSupply.h"
#include "PowerSupplyChannel.h"

INITIALIZE_EASYLOGGINGPP

boost::program_options::variables_map process_program_options(const int argc, const char* const argv[])
{
    boost::program_options::options_description desc("CMS Ph2_ACF  system test application");

    desc.add_options()("help,h", "produce help message")("name,n", boost::program_options::value<std::string>(), "Name of the power supply as described in the HW file")(
        "channel,c", boost::program_options::value<std::string>(), "Name of the power supply as described in the HW file")(
        "voltage,v", boost::program_options::value<float>(), "Voltage to be set at given channel")(
        "i_max,seti_max", boost::program_options::value<float>(), "Maximum allowed current to be set at given channel")(
        "v_max,setv_max", boost::program_options::value<float>(), "Voltage protection setting for the given channel")(
        "off,turnoff", "Turn off channel of power supply, if no channel is given, turn off all channels")("on,turnon", "Turn on channel of power supply, requires a channel")(
        "file,f", boost::program_options::value<std::string>(), "Hw Description File . Default value: settings/D19CDescription_Cic2.xml")(
        "gui,g", boost::program_options::value<std::string>(), "Named pipe for GUI communication");

    boost::program_options::variables_map vm;
    try
    {
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
    }
    catch(boost::program_options::error const& e)
    {
        std::cerr << e.what() << '\n';
        exit(EXIT_FAILURE);
    }
    boost::program_options::notify(vm);

    // Help
    if(vm.count("help"))
    {
        std::cout << desc << "\n";
        return 0;
    }
    return vm;
}

int main(int argc, char** argv)
{
    // configure the logger
    el::Configurations conf(std::string(std::getenv("PH2ACF_BASE_DIR")) + "/settings/logger.conf");
    el::Loggers::reconfigureAllLoggers(conf);

    el::Helpers::installLogDispatchCallback<gui::LogDispatcher>("GUILogDispatcher");

    boost::program_options::variables_map v_map = process_program_options(argc, argv);

    // now query the parsing results
    std::string cHWFile      = v_map["file"].as<std::string>();
    std::string cPowerSupply = v_map["name"].as<std::string>();
    std::string cChannel     = v_map["channel"].as<std::string>();
    double      cVoltsLimit  = v_map.count("v_max") ? v_map["v_max"].as<float>() : 10.5;
    double      cAmpsLimit   = v_map.count("i_max") ? v_map["i_max"].as<float>() : 1.3;
    double      cVolts       = v_map.count("v") ? v_map["v"].as<float>() : 0.0;

    bool cTurnOff = v_map.count("off");
    bool cTurnOn  = v_map.count("on");
    // Avoid undefined state
    if(cTurnOn && cTurnOff)
    {
        cTurnOff = true;
        cTurnOn  = false;
    }

    // Check if there is a gui involved, if not dump information in a dummy pipe
    std::string guiPipe = (v_map.count("gui")) ? v_map["gui"].as<std::string>() : "/tmp/guiDummyPipe";
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

    if(v_map.count("channel"))
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
        if(v_map.count("v_max")) { theHandler.getPowerSupply(cPowerSupply)->getChannel(cChannel)->setOverVoltageProtection(cVoltsLimit); }
        if(v_map.count("i_max")) { theHandler.getPowerSupply(cPowerSupply)->getChannel(cChannel)->setCurrentCompliance(cAmpsLimit); }
        if(v_map.count("v")) { theHandler.getPowerSupply(cPowerSupply)->getChannel(cChannel)->setVoltage(cVolts); }
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

    return 0;
}
