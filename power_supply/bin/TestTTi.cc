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

// Custom Libraries
#include "TTi.h"

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
    PowerSupply* powerSupply;
    try
    {
        theHandler.getPowerSupply("MyTTiEthernet");
    }
    catch(const std::out_of_range& oor)
    {
        std::cerr << "Out of Range error: " << oor.what() << '\n';
    }
    powerSupply                        = theHandler.getPowerSupply("MyTTiEthernet");
    TTi*                ttiPowerSupply = dynamic_cast<TTi*>(powerSupply);
    PowerSupplyChannel* ttiChannel     = ttiPowerSupply->getChannel("main");

    std::cout << "Resetting" << std::endl;
    ttiPowerSupply->reset();
    std::cout << "On: " << ttiChannel->isOn() << std::endl;
    std::cout << "Setting voltage" << std::endl;
    ttiChannel->setVoltage(2);
    // ttiChannel->setVoltage(60); // For voltage maximum value exceed error
    // testing.
    std::cout << "Setting OVP" << std::endl;
    ttiChannel->setOverVoltageProtection(4);
    std::cout << "Setting current compliance" << std::endl;
    ttiChannel->setCurrentCompliance(0.5);
    std::cout << "Setting DELTAV" << std::endl;
    ttiChannel->setParameter("DELTAV", (float)1.);
    // ttiChannel->setParameter("DELTAP",(float) 1.); // For error syntax testing
    std::cout << "Turning on" << std::endl;
    ;
    ttiChannel->turnOn();
    std::cout << "Setting voltage" << std::endl;
    ttiChannel->setVoltage(0.3);
    // ttiChannel->setVoltage(2); // For error in setting voltage with control
    // testing
    wait();
    std::cout << "On: " << ttiChannel->isOn() << std::endl
              << "OVP: " << ttiChannel->getOverVoltageProtection() << std::endl
              << "Voltage: " << ttiChannel->getOutputVoltage() << std::endl
              << "Current: " << ttiChannel->getCurrent() << std::endl
              << "DeltaV: " << ttiChannel->getParameterFloat("DELTAV") << std::endl;
    wait();
    ttiChannel->turnOff();
    return 0;
}
