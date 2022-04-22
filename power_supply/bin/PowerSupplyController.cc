#include "PowerSupplyInterface.h"
#include <boost/program_options.hpp> //!For command line arg parsing
#include <cstdlib>
#include <iostream>
#include <stdexcept>

#define PORT 7000 // The server listening port
                  /*!
                   ************************************************
                   * Argument parser.
                   ************************************************
                   */
boost::program_options::variables_map process_program_options(const int argc, const char* const argv[])
{
    boost::program_options::options_description desc("Allowed options");

    desc.add_options()("help,h", "produce help message")("config,c",
                                                         boost::program_options::value<std::string>()->default_value("default"),
                                                         "set configuration file path (default files defined for each test) "
                                                         "...")("verbose,v", boost::program_options::value<std::string>()->implicit_value("0"), "verbosity level")
                                                         ("port,p",boost::program_options::value<int>()->implicit_value(PORT),"set server listening port");

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

/*!
 ************************************************
 * Main.
 ************************************************
 */
int main(int argc, char** argv)
{
    boost::program_options::variables_map v_map = process_program_options(argc, argv);

    PowerSupplyInterface thePowerSupplyInterface(v_map["port"].as<int>(), v_map["config"].as<std::string>());
    thePowerSupplyInterface.startAccept();

    while(true) { std::this_thread::sleep_for(std::chrono::milliseconds(1000)); }

    return EXIT_SUCCESS;
}
