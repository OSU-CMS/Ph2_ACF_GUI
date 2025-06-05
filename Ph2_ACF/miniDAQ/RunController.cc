#include "MiddlewareController.h"
#include <boost/program_options.hpp>

INITIALIZE_EASYLOGGINGPP

namespace po = boost::program_options;

po::variables_map process_program_options(const int argc, const char* const argv[])
{
    po::options_description desc("Allowed options");

    desc.add_options()("help,h", "produce help message")

        ("port,p",
         po::value<uint16_t>()->default_value(0),
         "Server port shift (default = 0) "
         "...");

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
        std::cout << desc << "\n";
        return 0;
    }

    return vm;
}

int main(int argc, char** argv)
{
    boost::program_options::variables_map v_map = process_program_options(argc, argv);

    uint16_t portShift = v_map["port"].as<uint16_t>();

    std::string loggerConfigFile;
    if(std::getenv("PH2ACF_BASE_DIR") == nullptr) throw std::runtime_error("No PH2ACF_BASE_DIR environment variables have been set. You need to source some settings file!");

    loggerConfigFile = std::string(std::getenv("PH2ACF_BASE_DIR")) + "/settings/logger.conf";
    el::Configurations conf(loggerConfigFile);
    el::Loggers::reconfigureAllLoggers(conf);

    MiddlewareController theMiddlewareController(portShift);
    theMiddlewareController.startAccept();

    while(true) { std::this_thread::sleep_for(std::chrono::milliseconds(1000)); }

    return EXIT_SUCCESS;
}
