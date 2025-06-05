#include "Utils/Utilities.h"
#include "Utils/argvparser.h"
#include "tools/Eudaq2Producer.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;
using namespace CommandLineProcessing;
INITIALIZE_EASYLOGGINGPP

////////////////////////////////////////////
// Mauro: needs update to new EUDAQ (9/2021)
////////////////////////////////////////////

int main(int argc, char** argv)
{
#ifdef __EUDAQ__
    // configure the logger
    el::Configurations conf(std::string(std::getenv("PH2ACF_BASE_DIR")) + "/settings/logger.conf");
    el::Loggers::reconfigureAllLoggers(conf);
    ArgvParser cmd;

    // init
    cmd.setIntroductoryDescription("CMS Ph2_ACF EUDAQ Producer for Test Beam Operations");
    // error codes
    cmd.addErrorCode(0, "Success");
    cmd.addErrorCode(1, "Error");

    // options
    cmd.setHelpOption("h", "help", "Print this help page");
    cmd.defineOption("runcontrol", "The RunControl address. Default value: tcp://localhost:44000", ArgvParser::OptionRequiresValue);
    cmd.defineOptionAlternative("runcontrol", "r");
    cmd.defineOption("name", "Producer code name. Default value: ph2producer", ArgvParser::OptionRequiresValue);
    cmd.defineOptionAlternative("name", "n");
    cmd.defineOption("save", "Save the data to a raw file.  ", ArgvParser::OptionRequiresValue);
    cmd.defineOptionAlternative("save", "s");

    // parse
    int result = cmd.parse(argc, argv);
    if(result != ArgvParser::NoParserError)
    {
        LOG(INFO) << cmd.parseErrorDescription(result);
        exit(1);
    }

    // get values
    std::string cRunControlAddress = (cmd.foundOption("runcontrol")) ? cmd.optionValue("runcontrol") : "tcp://localhost:44000";
    std::string cName              = (cmd.foundOption("name")) ? cmd.optionValue("name") : "ph2producer";
    std::string cOutputFile;

    Eudaq2Producer cProducer(cName, cRunControlAddress);
    try
    {
        LOG(INFO) << "Trying to connect to RunControl";
        cProducer.Connect();
    }
    catch(...)
    {
        LOG(ERROR) << "Can not connect to RunControl at " << cRunControlAddress.c_str();
        return -1;
    }
    LOG(INFO) << "Connected";

    while(cProducer.IsConnected()) { std::this_thread::sleep_for(std::chrono::seconds(1)); }

    // as well damn if u want, cdz
    // cProducer.Destroy();
#endif
    return 0;
}
