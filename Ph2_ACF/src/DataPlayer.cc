#include "HWInterface/DPInterface.h"
#include "Utils/Timer.h"
#include "Utils/argvparser.h"
#include "tools/Tool.h"
#include <csignal>

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;

using namespace CommandLineProcessing;
using namespace std;

INITIALIZE_EASYLOGGINGPP

// Interrupt function
void signalHandler(int signum)
{
    LOG(INFO) << BOLDYELLOW << "Interrupt Signal [RECEIVED]" << RESET;
    exit(signum);
}

int main(int argc, char* argv[])
{
    // Signal Handler for interrupt
    signal(SIGABRT, signalHandler);

    ArgvParser cmd;
    cmd.addErrorCode(0, "Success");
    cmd.addErrorCode(1, "Error");
    cmd.setHelpOption("h", "help", "Print this help page");
    cmd.defineOption("file", "Hw Description File . Default value: settings/Calibration8CBC.xml", ArgvParser::OptionRequiresValue /*| ArgvParser::OptionRequired*/);
    cmd.defineOptionAlternative("file", "f");
    cmd.defineOption("pattern", "Data Player Pattern", ArgvParser::OptionRequiresValue /*| ArgvParser::OptionRequires*/);
    cmd.defineOptionAlternative("pattern", "p");

    int result = cmd.parse(argc, argv);
    if(result != ArgvParser::NoParserError)
    {
        LOG(INFO) << cmd.parseErrorDescription(result);
        exit(1);
    }

    // FIXME
    std::string cHWFile  = (cmd.foundOption("file")) ? cmd.optionValue("file") : "settings/Calibration8CBC.xml";
    uint8_t     pPattern = (cmd.foundOption("pattern")) ? convertAnyInt(cmd.optionValue("pattern").c_str()) : 0;

    Tool              cTool;
    std::stringstream outp;
    cTool.InitializeHw(cHWFile, outp);
    cTool.InitializeSettings(cHWFile, outp);
    LOG(INFO) << outp.str();
    outp.str("");

    DPInterface         cDPInterfacer;
    BeBoardFWInterface* pInterface = dynamic_cast<BeBoardFWInterface*>(cTool.fBeBoardFWMap.find(0)->second);

    // Check if Emulator is running
    // DP type == 1 for ROH
    uint8_t cDPtype = 1;
    if(cDPInterfacer.IsRunning(pInterface, cDPtype))
    {
        LOG(INFO) << BOLDBLUE << " STATUS : Data Player is running and will be stopped " << RESET;
        cDPInterfacer.Stop(pInterface);
    }

    // Configure and Start DataPlayer
    cDPInterfacer.Configure(pInterface, pPattern);
    cDPInterfacer.Start(pInterface, cDPtype);
    if(cDPInterfacer.IsRunning(pInterface, cDPtype))
        LOG(INFO) << BOLDBLUE << "FE data player " << BOLDGREEN << " running correctly!" << RESET;
    else
        LOG(INFO) << BOLDRED << "Could not start FE data player" << RESET;

    cTool.Destroy();
    return 0;
}
