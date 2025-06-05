#include "Utils/argvparser.h"
#include "tools/CicFEAlignment.h"
#include "tools/MultiplexingSetup.h"
#include <cstring>

#ifdef __USE_ROOT__
#include "TApplication.h"
#include "TROOT.h"
#endif

#include "Utils/Timer.h"

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
    cmd.setIntroductoryDescription("CMS Ph2_ACF d19c Testboard Firmware Test Application");
    // error codes
    cmd.addErrorCode(0, "Success");
    cmd.addErrorCode(1, "Error");
    // options
    cmd.setHelpOption("h", "help", "Print this help page");
    cmd.defineOption("file", "Hw Description File . Default value: settings/D19CHWDescription.xml", ArgvParser::OptionRequiresValue /*| ArgvParser::OptionRequired*/);
    cmd.defineOptionAlternative("file", "f");
    cmd.defineOption("mux_disconnect", "Disable setup with multiplexing backplane", ArgvParser::NoOptionAttribute);
    cmd.defineOption("mux_scan", "Scan setup with multiplexing backplane", ArgvParser::NoOptionAttribute);
    cmd.defineOption("mux_configure", "Configure setup with multiplexing backplane", ArgvParser::OptionRequiresValue /*| ArgvParser::OptionRequired*/);
    cmd.defineOption("mux_auto_configure", "Configure all available cards one by one", ArgvParser::NoOptionAttribute);
    cmd.defineOption("enable_fmc_power", "Initialization of FMC power", ArgvParser::NoOptionAttribute);

    int result = cmd.parse(argc, argv);

    if(result != ArgvParser::NoParserError)
    {
        LOG(INFO) << cmd.parseErrorDescription(result);
        exit(1);
    }

    // now query the parsing results
    std::string cHWFile = (cmd.foundOption("file")) ? cmd.optionValue("file") : "settings/D19CHWDescription.xml";

    std::stringstream outp;
    Tool              cTool;
    cTool.setInterfaceInitialization(0);
    cTool.InitializeHw(cHWFile, outp);
    cTool.InitializeSettings(cHWFile, outp);
    LOG(INFO) << outp.str();
#ifdef __MULTIPLEXING__
    bool cMuxDisconnect    = (cmd.foundOption("mux_disconnect")) ? true : false;
    bool cMuxScan          = (cmd.foundOption("mux_scan")) ? true : false;
    bool cMuxConfigure     = (cmd.foundOption("mux_configure")) ? true : false;
    bool cMuxAutoConfigure = (cmd.foundOption("mux_auto_configure")) ? true : false;

    MultiplexingSetup cMuxControl;
    cMuxControl.Inherit(&cTool);
    cMuxControl.Initialise();
    if(cMuxDisconnect) { cMuxControl.Disconnect(); }
    else if(cMuxScan) { cMuxControl.Scan(); }
    else if(cMuxConfigure)
    {
        std::string       sBPNumCardNum = cmd.optionValue("mux_configure");
        std::vector<int>  vBPNumCardNum;
        std::stringstream ssBPNumCardNum(sBPNumCardNum);
        int               i;
        while(ssBPNumCardNum >> i)
        {
            vBPNumCardNum.push_back(i);
            if(ssBPNumCardNum.peek() == ',') ssBPNumCardNum.ignore();
        };
        uint8_t cBackplaneNum = vBPNumCardNum.at(0);
        uint8_t cCardNum      = vBPNumCardNum.at(1);
        cMuxControl.ConfigureSingleCard(cBackplaneNum, cCardNum);
    }
    else if(cMuxAutoConfigure) { cMuxControl.ConfigureAll(); }
#endif //__MULTIPLEXING__
    LOG(INFO) << "*** End of the operation ***";
    // cTool.Destroy();

    return 0;
}
