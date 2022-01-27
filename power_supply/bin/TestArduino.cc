/*!
 * \authors Mattia Lizzo <mattia.lizzo@cern.ch>, INFN-Firenze
 * \authors Francesco Fiori <francesco.fiori@cern.ch>, INFN-Firenze
 * \authors Antonio Cassese <antonio.cassese@cern.ch>, INFN-Firenze
 * \date Sep 2 2019
 */

// Libraries
#include "Arduino.h"
#include "DeviceHandler.h"

#include <boost/program_options.hpp> //!For command line arg parsing
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>

// Custom Libraries
#include "ArduinoKIRA.h"

// Namespaces
using namespace std;
namespace po = boost::program_options;

/*!
************************************************
* A simple "wait for input".
************************************************
*/
void wait()
{
    cout << "Press ENTER to continue...";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

/*!
************************************************
* Argument parser.
************************************************
*/
po::variables_map process_program_options(const int argc, const char* const argv[])
{
    po::options_description desc("Allowed options");

    desc.add_options()("help,h", "produce help message")

        ("config,c",
         po::value<string>()->default_value("default"),
         "set configuration file path (default files defined for each test) "
         "...")("verbose,v", po::value<string>()->implicit_value("0"), "verbosity level");

    po::variables_map vm;
    try
    {
        po::store(po::parse_command_line(argc, argv, desc), vm);
    }
    catch(po::error const& e)
    {
        std::cerr << e.what() << '\n';
        exit(EXIT_FAILURE);
    }
    po::notify(vm);

    // Help
    if(vm.count("help"))
    {
        cout << desc << "\n";
        return 0;
    }

    // Power supply object option
    if(vm.count("object")) { cout << "Object to initialize set to " << vm["object"].as<string>() << endl; }

    return vm;
}

/*!
 ************************************************
 * Main.
 ************************************************
 */
int main(int argc, char* argv[])
{
    boost::program_options::variables_map v_map = process_program_options(argc, argv);
    std::cout << "Initializing ..." << std::endl;

    std::string        docPath = v_map["config"].as<string>();
    pugi::xml_document docSettings;

    DeviceHandler theHandler;
    theHandler.readSettings(docPath, docSettings);
    Arduino* arduino;

    try
    {
        arduino = theHandler.getArduino("MyArduino");
    }
    catch(const std::out_of_range& oor)
    {
        std::cerr << "Out of Range error: " << oor.what() << '\n';
        throw std::out_of_range(oor.what());
    }

    std::cout << "get humidity: " << theHandler.getArduino("MyArduino")->getParameterFloat("get_humidity") << std::endl;
    std::cout << "get temperature: " << theHandler.getArduino("MyArduino")->getParameterFloat("get_temperature") << std::endl;
    /*
    // std::cout << "Resetting" << std::endl;
    // powerSupply->reset();
    //    std::cout << "On: " << channel1->isOn() << std::endl;
    // std::cout << "Setting voltage" << std::endl;
    // channel1->setVoltage(2.2);

    // std::cout << "Setting OVP" << std::endl;
    // channel1->setOverVoltageProtection(3);

    // std::cout << "Setting current compliance" << std::endl;
    // channel1->setCurrentCompliance(0.9);

    // std::cout << "Turning on" << std::endl;
    // channel1->turnOn();
    // std::cout << "On: " << channel1->isOn() << std::endl;

    // std::cout << "Setting voltage" << std::endl;
    channel1->setVoltage(1.11);
    channel2->setVoltage(2.22);
    channel3->setVoltage(3.33);
    channel1->setCurrent(0.11);
    channel2->setCurrent(0.22);
    channel3->setCurrent(0.33);
    // wait();
    // channel1->setParameter("INST:NSEL",1);
    // std::cout << channel1->getParameterFloat("VOLT?") << std::endl;

    std::cout << "Channel 1 On: " << channel1->isOn()
              << std::endl
              //    << "OVP: " << channel1->getOverVoltageProtection() << std::endl
              << "Voltage: " << channel1->getOutputVoltage() << std::endl
              << "Current: " << channel1->getCurrent() << std::endl;
    wait();
    std::cout << "Channel 2 On: " << channel2->isOn()
              << std::endl
              //    << "OVP: " << channel2->getOverVoltageProtection() << std::endl
              << "Voltage: " << channel2->getOutputVoltage() << std::endl
              << "Current: " << channel2->getCurrent() << std::endl;
    wait();
    std::cout << "Channel 3 On: " << channel3->isOn()
              << std::endl
              //    << "OVP: " << channel3->getOverVoltageProtection() << std::endl
              << "Voltage: " << channel3->getOutputVoltage() << std::endl
              << "Current: " << channel3->getCurrent() << std::endl;
    wait();
    // std::cout
    //     << "Channel 4 On: "
    //     << channel4->isOn()
    //     << std::endl
    //   //    << "OVP: " << channel4->getOverVoltageProtection() << std::endl
    //     << "Voltage: " << channel4->getOutputVoltage() << std::endl
    //     << "Current: " << channel4->getCurrent() << std::endl;
    wait();
    std::cout << "Turning off" << std::endl;
    channel1->turnOff();
    std::cout << "On: " << channel1->isOn() << std::endl;
    */
    return 0;
}
