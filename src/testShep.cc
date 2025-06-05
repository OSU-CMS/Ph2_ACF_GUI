#include "DQMUtils/DQMInterface.h"
#include "MonitorDQM/MonitorDQMInterface.h"
#include "Utils/ConfigureInfo.h"
#include "Utils/MiddlewareInterface.h"
#include "Utils/StartInfo.h"
#include "Utils/argvparser.h"
#include <google/protobuf/descriptor.h>

#include <cstring>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <TApplication.h>

#include "Utils/easylogging++.h"

using namespace CommandLineProcessing;

INITIALIZE_EASYLOGGINGPP

static bool controlC = false;

void interruptHandler(int handler)
{
    std::cout << __PRETTY_FUNCTION__ << " Sig handler: " << handler << std::endl;
    exit(EXIT_FAILURE);

    controlC = true;
}

int returnRunNumber(std::string cFileName)
{
    std::string   cLine;
    int           cRunNumber = -1;
    std::ifstream cStream(cFileName);
    if(cStream.is_open())
    {
        while(std::getline(cStream, cLine))
        {
            std::istringstream cIStream(cLine);
            cIStream >> cRunNumber;
            // LOG(INFO) << BOLDMAGENTA << cRunNumber << RESET;
        }
    }

    cRunNumber++;
    std::ofstream cRunLog;
    cRunLog.open(cFileName, std::fstream::app);
    cRunLog << cRunNumber << "\n";
    cRunLog.close();

    return cRunNumber;
}

int main(int argc, char* argv[])
{
    if(std::getenv("PH2ACF_BASE_DIR") == nullptr)
    {
        std::cout << "You must source setup.sh or export the PH2ACF_BASE_DIR environmental variable. Exiting..." << std::endl;
        exit(EXIT_FAILURE);
    }
    std::string baseDir = std::string(std::getenv("PH2ACF_BASE_DIR")) + "/";
    std::string binDir  = baseDir + "bin/";

    // configure the logger
    el::Configurations conf(baseDir + "settings/logger.conf");
    el::Loggers::reconfigureAllLoggers(conf);

    ArgvParser cmd;

    // init
    cmd.setIntroductoryDescription("CMS Ph2_ACF  calibration routine using K. Uchida's algorithm or a fast algorithm");
    // error codes
    cmd.addErrorCode(0, "Success");
    cmd.addErrorCode(1, "Error");
    // options
    cmd.setHelpOption("h", "help", "Print this help page");

    cmd.defineOption("file", "Hw Description File", ArgvParser::OptionRequiresValue | ArgvParser::OptionRequired);
    cmd.defineOptionAlternative("file", "f");

    std::string calibrationHelpMessage = "Calibration to run. List of available calibrations:\n";

    cmd.defineOption("calibration", calibrationHelpMessage, ArgvParser::OptionRequiresValue | ArgvParser::OptionRequired);
    cmd.defineOptionAlternative("calibration", "c");

    cmd.defineOption("output", "Output Directory. Default value: Results", ArgvParser::OptionRequiresValue /*| ArgvParser::OptionRequired*/);
    cmd.defineOptionAlternative("output", "o");

    cmd.defineOption("allChan", "Do calibration using all channels? Default: false", ArgvParser::NoOptionAttribute);
    cmd.defineOptionAlternative("allChan", "a");

    cmd.defineOption("batch", "Run the application in batch mode", ArgvParser::NoOptionAttribute);
    cmd.defineOptionAlternative("batch", "b");

    cmd.defineOption("port", "Port shift for TCP servers 0", ArgvParser::OptionRequiresValue /*| ArgvParser::OptionRequired*/);
    cmd.defineOptionAlternative("port", "p");

    int result = cmd.parse(argc, argv);

    if(result != ArgvParser::NoParserError)
    {
        LOG(INFO) << cmd.parseErrorDescription(result);
        exit(EXIT_FAILURE);
    }

    // now query the parsing results
    std::string cHWFile    = cmd.optionValue("file");
    std::string cDirectory = (cmd.foundOption("output")) ? cmd.optionValue("output") : "Results/";
    cDirectory += cmd.optionValue("calibration");

    bool batchMode = (cmd.foundOption("batch")) ? true : false;

    struct sigaction act;
    act.sa_handler = interruptHandler;
    sigaction(SIGINT, &act, NULL);

    enum
    {
        INITIAL,
        HALTED,
        CONFIGURED,
        RUNNING,
        STOPPED
    };

    int                 stateMachineStatus = INITIAL;
    MiddlewareInterface theMiddlewareInterface("127.0.0.1", 5000);
    theMiddlewareInterface.initialize();

    // int main ( int argc, char* argv[] )
    // std::cout << argc << "-" << argv[2] << std::endl;
    // exit(0);
    int   tAppArgc = 1;
    char* tAppArgv[2];
    tAppArgv[0] = argv[0];
    tAppArgv[1] = (char*)"-b";
    if(batchMode) tAppArgc = 2;
    TApplication theApp("App", &tAppArgc, tAppArgv);

    DQMInterface        theDQMInterface;
    MonitorDQMInterface theMonitorDQMInterface;

    stateMachineStatus = HALTED;

    bool done = false;
    while(!done)
    {
        try
        {
            switch(stateMachineStatus)
            {
            case HALTED:
            {
                std::cout << __PRETTY_FUNCTION__ << "Supervisor Sending Configure!!!" << std::endl;
                std::string   calibrationName   = cmd.optionValue("calibration");
                std::string   configurationFile = cmd.optionValue("file");
                ConfigureInfo theConfigureInfo;
                theConfigureInfo.setConfigurationFiles(configurationFile);
                theConfigureInfo.setCalibrationName(calibrationName);
                theMiddlewareInterface.configure(theConfigureInfo);
                theDQMInterface.configure(theConfigureInfo);
                theMonitorDQMInterface.configure(theConfigureInfo);
                stateMachineStatus = CONFIGURED;
                break;
            }
            case CONFIGURED:
            {
                int       runNumber = returnRunNumber("RunNumbers.dat");
                StartInfo theStartInfo;
                theStartInfo.setRunNumber(runNumber);
                std::cout << __PRETTY_FUNCTION__ << "Supervisor Sending Start!!!" << std::endl;
                std::cout << __PRETTY_FUNCTION__ << __LINE__ << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
                theDQMInterface.startProcessingData(theStartInfo);
                std::cout << __PRETTY_FUNCTION__ << __LINE__ << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
                theMonitorDQMInterface.startProcessingData();
                std::cout << __PRETTY_FUNCTION__ << __LINE__ << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
                theMiddlewareInterface.start(theStartInfo);
                std::cout << __PRETTY_FUNCTION__ << __LINE__ << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
                stateMachineStatus = RUNNING;
                std::cout << __PRETTY_FUNCTION__ << __LINE__ << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
                break;
            }
            case RUNNING:
            {
                std::cout << __PRETTY_FUNCTION__ << __LINE__ << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
                if(cmd.optionValue("calibration") != "psphysics" && cmd.optionValue("calibration") != "2sphysics")
                {
                    std::cout << __PRETTY_FUNCTION__ << __LINE__ << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
                    theMiddlewareInterface.status();
                    std::cout << __PRETTY_FUNCTION__ << __LINE__ << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
                    while(theMiddlewareInterface.status() != "Done")
                    {
                        std::cout << __PRETTY_FUNCTION__ << __LINE__ << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
                        usleep(5e5);
                    }
                }
                else
                    usleep(20e6);
                std::cout << __PRETTY_FUNCTION__ << "Supervisor Sending Stop!!!" << std::endl;
                usleep(2e6);
                theMiddlewareInterface.stop();
                usleep(1e6);
                stateMachineStatus = STOPPED;
                break;
            }
            case STOPPED:
            {
                theDQMInterface.stopProcessingData();
                usleep(5e6);
                theMiddlewareInterface.halt();
                std::cout << __PRETTY_FUNCTION__ << "Supervisor Everything Stopped!!! Exiting..." << std::endl;
                done = true;
                break;
            }
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            return EXIT_FAILURE;
        }

        if(!done)
        {
            std::cout << __PRETTY_FUNCTION__ << "Supervisor SLEEPING!!!" << std::endl;
            usleep(1000000);
        }
    }

    theMonitorDQMInterface.stopProcessingData();

    theApp.Run();

    return EXIT_SUCCESS;
}
