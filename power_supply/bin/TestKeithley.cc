/*!
 * \authors Mattia Lizzo <mattia.lizzo@cern.ch>, INFN-Firenze
 * \authors Francesco Fiori <francesco.fiori@cern.ch>, INFN-Firenze
 * \authors Antonio Cassese <antonio.cassese@cern.ch>, INFN-Firenze
 * \date Sep 2 2019
 */

// Libraries
#include "DeviceHandler.h"
#include "PowerSupply.h"
#include "PowerSupplyChannel.h"
#include <boost/program_options.hpp> //!For command line arg parsing
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>

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
    PowerSupply*        powerSupply;
    PowerSupplyChannel* channel1;
    PowerSupplyChannel* channel2;
    try
    {
        powerSupply = theHandler.getPowerSupply("MyKeithley");
        channel1    = powerSupply->getChannel("Front");
        channel2    = powerSupply->getChannel("Back");
    }
    catch(const std::out_of_range& oor)
    {
        std::cerr << "Out of Range error: " << oor.what() << '\n';
        throw std::out_of_range(oor.what());
    }
    channel1->turnOn();
    std::cout << "voltage on channel 1: " << channel1->getOutputVoltage() << "V\tcurrent on channel 1: " << channel1->getCurrent() << "A\tIs on: " << channel1->isOn() << std::endl;
    ;
    std::cout << "voltage on channel 2: " << channel2->getOutputVoltage() << "V\tcurrent on channel 2: " << channel2->getCurrent() << "A\tIs on: " << channel2->isOn() << std::endl;
    ;

    channel1->rampToVoltage(0);

    std::cout << "voltage on channel 1: " << channel1->getOutputVoltage() << "V\tcurrent on channel 1: " << channel1->getCurrent() << "A\tIs on: " << channel1->isOn() << std::endl;
    ;
    std::cout << "voltage on channel 2: " << channel2->getOutputVoltage() << "V\tcurrent on channel 2: " << channel2->getCurrent() << "A\tIs on: " << channel2->isOn() << std::endl;
    ;

    channel1->rampToVoltage(-40);
    std::cout << "voltage on channel 1: " << channel1->getOutputVoltage() << "V\tcurrent on channel 1: " << channel1->getCurrent() << "A\tIs on: " << channel1->isOn() << std::endl;
    ;
    std::cout << "voltage on channel 2: " << channel2->getOutputVoltage() << "V\tcurrent on channel 2: " << channel2->getCurrent() << "A\tIs on: " << channel2->isOn() << std::endl;
    ;
    channel1->rampToVoltage(-300);

    std::cout << "voltage on channel 1: " << channel1->getOutputVoltage() << "V\tcurrent on channel 1: " << channel1->getCurrent() << "A\tIs on: " << channel1->isOn() << std::endl;
    ;
    std::cout << "voltage on channel 2: " << channel2->getOutputVoltage() << "V\tcurrent on channel 2: " << channel2->getCurrent() << "A\tIs on: " << channel2->isOn() << std::endl;
    ;

    channel1->rampToVoltage(0);

    std::cout << "voltage on channel 1: " << channel1->getOutputVoltage() << "V\tcurrent on channel 1: " << channel1->getCurrent() << "A\tIs on: " << channel1->isOn() << std::endl;
    ;
    std::cout << "voltage on channel 2: " << channel2->getOutputVoltage() << "V\tcurrent on channel 2: " << channel2->getCurrent() << "A\tIs on: " << channel2->isOn() << std::endl;
    ;

    channel1->rampToVoltage(-300);

    std::cout << "voltage on channel 1: " << channel1->getOutputVoltage() << "V\tcurrent on channel 1: " << channel1->getCurrent() << "A\tIs on: " << channel1->isOn() << std::endl;
    ;
    std::cout << "voltage on channel 2: " << channel2->getOutputVoltage() << "V\tcurrent on channel 2: " << channel2->getCurrent() << "A\tIs on: " << channel2->isOn() << std::endl;
    ;
    /*

    channel1->rampDownToVoltage(-17,10,3000);
    channel1->rampDownToVoltage(-1.13,10,3000);
    channel1->rampDownToVoltage(-10.54,10,3000);
    */

    /*
    channel4->turnOn();
    wait();
    channel4->turnOff();
    wait();
    channel4->turnOn();
    wait();
    channel4->turnOff();
    */
    /*    channel1->turnOff();
        channel2->turnOff();

        channel3->turnOff();
        channel4->turnOff();
    */

    return 0;
}
