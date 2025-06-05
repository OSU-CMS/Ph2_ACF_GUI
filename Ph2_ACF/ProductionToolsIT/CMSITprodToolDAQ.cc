#include "DQMUtils/DQMInterface.h"
#include "System/SystemController.h"
#include "Utils/ConfigureInfo.h"
#include "Utils/MiddlewareInterface.h"
#include "Utils/RD53Shared.h"
#include "Utils/argvparser.h"

#include "ProductionToolsIT/RD53EyeDiag.h"
#include "ProductionToolsIT/RD53EyeScanOptimization.h"

#include <chrono>
#include <sys/wait.h>
#include <thread>

#if defined(__USE_ROOT__)
#include "TApplication.h"
#endif

// ##################
// # Default values #
// ##################
#define RUNNUMBER 0
#define FILERUNNUMBER "./RunNumber.txt"
#define BASEDIR "PH2ACF_BASE_DIR"

INITIALIZE_EASYLOGGINGPP

using namespace Ph2_System;
using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;

int main(int argc, char** argv)
{
    // #############################
    // # Initialize command parser #
    // #############################
    CommandLineProcessing::ArgvParser cmd;

    cmd.setIntroductoryDescription("@@@ CMSIT Middleware System Test Application @@@");

    cmd.setHelpOption("h", "help", "Print this help page");

    cmd.defineOption("file", "Hardware description file", CommandLineProcessing::ArgvParser::OptionRequiresValue);
    cmd.defineOptionAlternative("file", "f");

    cmd.defineOption("calib",
                     "Which calibration to run [latency pixelalive noise scurve gain threqu gainopt thrmin thradj "
                     "injdelay clkdelay datarbopt datatrtest physics eudaq bertest voltagetuning, gendacdac]",
                     CommandLineProcessing::ArgvParser::OptionRequiresValue);
    cmd.defineOptionAlternative("calib", "c");

    cmd.defineOption("reset", "Reset the backend board", CommandLineProcessing::ArgvParser::NoOptionAttribute);
    cmd.defineOptionAlternative("reset", "r");

    int result = cmd.parse(argc, argv);
    if(result != CommandLineProcessing::ArgvParser::NoParserError)
    {
        LOG(INFO) << cmd.parseErrorDescription(result);
        exit(EXIT_FAILURE);
    }

    // ###################
    // # Read run number #
    // ###################
    int           runNumber = RUNNUMBER;
    std::ifstream fileRunNumberIn;
    fileRunNumberIn.open(FILERUNNUMBER, std::ios::in);
    if(fileRunNumberIn.is_open() == true) fileRunNumberIn >> runNumber;
    fileRunNumberIn.close();
    system(std::string("mkdir -p " + std::string(RD53Shared::RESULTDIR)).c_str());

    // ####################
    // # Retrieve options #
    // ####################
    std::string configFile = cmd.foundOption("file") == true ? cmd.optionValue("file") : "";
    std::string whichCalib = cmd.foundOption("calib") == true ? cmd.optionValue("calib") : "";
    bool        reset      = cmd.foundOption("reset") == true ? true : false;

    // ########################
    // # Configure the logger #
    // ########################
    std::string fileName("logs/CMSITminiDAQ" + RD53Shared::fromInt2Str(runNumber));
    if(whichCalib != "") fileName += "_" + whichCalib;
    fileName += ".log";
    el::Configurations conf(std::string(std::getenv(BASEDIR)) + "/settings/logger.conf");
    conf.set(el::Level::Global, el::ConfigurationType::Format, "|%datetime{%h:%m:%s}|%levshort|%msg");
    conf.set(el::Level::Global, el::ConfigurationType::Filename, fileName);
    el::Loggers::reconfigureAllLoggers(conf);

    SystemController mySysCntr;

    if(reset == true)
    {
        // ######################################
        // # Reset hardware or read binary file #
        // ######################################

        std::stringstream outp;
        mySysCntr.InitializeHw(configFile, outp);
        mySysCntr.InitializeSettings(configFile, outp);
        if(mySysCntr.fDetectorContainer->getFirstObject()->getFirstObject()->flpGBT == nullptr)
            static_cast<RD53FWInterface*>(mySysCntr.fBeBoardFWMap[mySysCntr.fDetectorContainer->getFirstObject()->getId()])->ResetSequence("160");
        else
            static_cast<RD53FWInterface*>(mySysCntr.fBeBoardFWMap[mySysCntr.fDetectorContainer->getFirstObject()->getId()])->ResetSequence("320");
        exit(EXIT_SUCCESS);
    }
    else
    {
        // #######################
        // # Initialize Hardware #
        // #######################

        LOG(INFO) << BOLDMAGENTA << "@@@ Initializing the Hardware @@@" << RESET;
        ConfigureInfo theConfigureInfo;
        theConfigureInfo.setConfigurationFiles(configFile);
        mySysCntr.Configure(theConfigureInfo);
        LOG(INFO) << BOLDMAGENTA << "@@@ Hardware initialization done @@@" << RESET;
    }

    std::cout << std::endl;

    // ###################
    // # Run Calibration #
    // ###################
    if(whichCalib == "eyescan")
    {
#ifdef __USE_ROOT__
        // ##################################
        // # Run Eye Scan optimization      #
        // ##################################
        LOG(INFO) << BOLDMAGENTA << "@@@ Performing Eye Scan Optimization Optimization @@@" << RESET;

        std::string         fileName("Run" + RD53Shared::fromInt2Str(runNumber) + "_EyeDiagramScan");
        EyeScanOptimization eso;
        eso.Inherit(&mySysCntr);
        eso.localConfigure(fileName, runNumber);
        eso.Running();
        eso.draw();
#endif
    }
    else if(whichCalib == "eyescan2d")
    {
#ifdef __USE_ROOT__
        // ##################################
        // # Run Eye Scan optimization      #
        // ##################################
        LOG(INFO) << BOLDMAGENTA << "@@@ Performing Eye Scan Optimization Optimization in 2D @@@" << RESET;

        std::string         fileName("Run" + RD53Shared::fromInt2Str(runNumber) + "_EyeDiagramScan");
        EyeScanOptimization eso;
        eso.Inherit(&mySysCntr);
        eso.localConfigure(fileName, runNumber, true);
        eso.Running();
        eso.draw();
#endif
    }

    else if(whichCalib == "eyediag")
    {
#ifdef __USE_ROOT__
        // ##################################
        // # Run Eye Scan optimization      #
        // ##################################
        LOG(INFO) << BOLDMAGENTA << "@@@ Performing Eye Diagram @@@" << RESET;

        std::string fileName("Run" + RD53Shared::fromInt2Str(runNumber) + "_EyeDiagram");
        EyeDiag     ed;
        ed.Inherit(&mySysCntr);
        ed.localConfigure(fileName, runNumber);
        ed.Running();
        ed.draw();
#endif
    }
    else if(whichCalib != "")
    {
        LOG(ERROR) << BOLDRED << "Option not recognized: " << BOLDYELLOW << whichCalib << RESET;
        exit(EXIT_FAILURE);
    }

    // ###########################
    // # Copy configuration file #
    // ###########################
    const auto configFileBasename = configFile.substr(configFile.find_last_of("/\\") + 1);
    const auto outputConfigFile   = std::string(RD53Shared::RESULTDIR) + "/Run" + RD53Shared::fromInt2Str(runNumber) + "_" + configFileBasename;
    system(("cp " + configFile + " " + outputConfigFile).c_str());

    // #####################
    // # Update run number #
    // #####################
    std::ofstream fileRunNumberOut;
    runNumber++;
    fileRunNumberOut.open(FILERUNNUMBER, std::ios::out);
    if(fileRunNumberOut.is_open() == true) fileRunNumberOut << RD53Shared::fromInt2Str(runNumber) << std::endl;
    fileRunNumberOut.close();

    // #############################
    // # Destroy System Controller #
    // #############################
    mySysCntr.Destroy();

    LOG(INFO) << BOLDMAGENTA << "@@@ End of CMSIT miniDAQ @@@" << RESET;

    return EXIT_SUCCESS;
}
