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
* Test informations print.
************************************************
*/
void PrintTestInfo(std::string pathConfig, std::string infoString) { std::cout << infoString << std::endl << "Loading configuration file: " << pathConfig << std::endl; }

/*!
************************************************
 * Test options handler.
 \param test Test name.
 \param pathConfig Confiuration file path.
 \param verbosity Verbosity level.
 \return True if everything works well.
************************************************
*/
int test(std::string test, std::string pathConfig, int verbosity)
{
    if(!test.compare("ivkeithley"))
    {
        if(!pathConfig.compare("default")) pathConfig = "config/iv_keithley_config.xml";
        PrintTestInfo(pathConfig, "Executing IV curve using Keithley as Power Supply and Multimeter");
        std::string commandString = "sudo bin/IV_keithley_2410 -c " + pathConfig;
        system(commandString.c_str());
        return 0;
    }
    else if(!test.compare("iseg"))
    {
        if(!pathConfig.compare("default")) pathConfig = "../config/config_iseg.xml";
        PrintTestInfo(pathConfig, "Executing simple tests on ISEG power supply");
        std::string commandString = "./TestIsegSHR -c " + pathConfig;
        system(commandString.c_str());
        return 0;
    }
    else if(!test.compare("caen"))
    {
        if(!pathConfig.compare("default")) pathConfig = "../config/config_caen.xml";
        PrintTestInfo(pathConfig, "Executing simple tests on CAEN SY4527 module");
        std::string commandString = "./TestCAEN_SY4527 -c " + pathConfig;
        system(commandString.c_str());
        return 0;
    }
    else if(!test.compare("rands"))
    {
        if(!pathConfig.compare("default")) pathConfig = "config/configRohdeSchwartz.xml";
        PrintTestInfo(pathConfig, "Executing simple tests on Rhode&Schwarz power supply");
        std::string commandString = "sudo bin/TestRohdeSchwarz -c " + pathConfig;
        system(commandString.c_str());
        return 0;
    }
    else if(!test.compare("tti"))
    {
        if(!pathConfig.compare("default")) pathConfig = "config/config_tti.xml";
        PrintTestInfo(pathConfig, "Executing simple tests on TTi power supply");
        std::string commandString = "sudo bin/TestTTi -c " + pathConfig;
        system(commandString.c_str());
        return 0;
    }
    // else if(!test.compare("itivcurve"))
    //{
    //    if(!pathConfig.compare("default")) pathConfig = "config/iv_it_sldo.xml";
    //    PrintTestInfo(pathConfig, "Executing IV curve for Inner Tracker");
    //    return PowerSupply::ExecuteTest(pathConfig, "iv_curve", verbosity);
    //}
    else
    {
        std::cout << "Test " << test << " not defined, aborting exectution." << std::endl;
        return -1;
    }
}

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
         "...")

            ("test,t",
             po::value<string>(),
             "set the test to be executed."
             "\nPossible values:\n"
             // "  itivcurve: \tPerforms IV curve for IT flavours. Default config "
             // "file is config/iv_it_sldo.xml\n"
             "  ivkeithley: \tPerforms IV curve using keithley as power supply and Multimeter. Default config file is config/iv_keithley_config.xml\n"
             "  iseg: \tPerforms simple tests on ISEG power supply. Default config file is config/config_iseg.xml\n"
             "  caen: \tPerforms simple tests on CAEN SY4527 module. Default config file is config/config_caen.xml\n"
             "  rands: \tPerforms simple tests on Rhode&Schwarz power supply. Default config file is config/configRohdeSchwartz.xml\n"
             "  tti: \tPerforms simple tests on TTi power supply. Default config file is config/config_tti.xml\n"
             // "  Value2: \tdoes something else, bla bla bla bla bla bla bla
             // bla bla bla bla bla bla bla bla\n"
             )

                ("verbose,v", po::value<string>()->implicit_value("0"), "verbosity level");

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

    // Test type execution
    if(vm.count("test")) test(vm["test"].as<string>(), vm["config"].as<string>(), vm.count("verbose"));
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
    return 0;
}
