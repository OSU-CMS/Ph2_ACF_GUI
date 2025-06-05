#include "System/SystemController.h"
#include "Utils/CommonVisitors.h"
#include "Utils/Timer.h"
#include "Utils/argvparser.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;
using namespace CommandLineProcessing;
INITIALIZE_EASYLOGGINGPP

int main(int argc, char** argv)
{
    // configure the logger
    el::Configurations conf(std::string(std::getenv("PH2ACF_BASE_DIR")) + "/settings/logger.conf");
    el::Loggers::reconfigureAllLoggers(conf);

    ArgvParser cmd;

    // init
    cmd.setIntroductoryDescription("CMS Ph2_ACF  system test application");
    // error codes
    cmd.addErrorCode(0, "Success");
    cmd.addErrorCode(1, "Error");
    // options
    cmd.setHelpOption("h", "help", "Print this help page");

    cmd.defineOption("file", "Hw Description File . Default value: settings/HWDescription_2CBC.xml", ArgvParser::OptionRequiresValue /*| ArgvParser::OptionRequired*/);
    cmd.defineOptionAlternative("file", "f");

    cmd.defineOption("configure", "Configure HW", ArgvParser::NoOptionAttribute);
    cmd.defineOptionAlternative("configure", "c");

    int result = cmd.parse(argc, argv);

    if(result != ArgvParser::NoParserError)
    {
        LOG(INFO) << cmd.parseErrorDescription(result);
        exit(1);
    }

    // now query the parsing results
    std::string cHWFile    = (cmd.foundOption("file")) ? cmd.optionValue("file") : "settings/HWDescription_2CBC.xml";
    bool        cConfigure = (cmd.foundOption("configure")) ? true : false;

    std::stringstream outp;
    SystemController  cSystemController;
    cSystemController.InitializeHw(cHWFile, outp);
    cSystemController.InitializeSettings(cHWFile, outp);

    LOG(INFO) << outp.str();
    outp.str("");

    if(cConfigure) { cSystemController.ConfigureHw(); }

    LOG(INFO) << "*** End of the System test ***";
    cSystemController.Destroy();
    return 0;
}
