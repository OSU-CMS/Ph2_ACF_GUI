#include <cstring>

#include "Parser/ParserDefinitions.h"
#include "Utils/ConfigureInfo.h"
#include "Utils/StartInfo.h"
#include "Utils/Timer.h"
#include "Utils/Utilities.h"
#include "Utils/argvparser.h"
#include "Utils/easylogging++.h"
#include "boost/format.hpp"
#include "miniDAQ/MiddlewareStateMachine.h"

#include "TApplication.h"
#include "TROOT.h"

using namespace CommandLineProcessing;
INITIALIZE_EASYLOGGINGPP

sig_atomic_t killProcess  = 0;
sig_atomic_t runCompleted = 0;

void interruptHandler(int handler) { killProcess = 1; }

void killProcessFunction(MiddlewareStateMachine* theMiddlewareStateMachine)
{
    while(1)
    {
        usleep(250000);
        if(killProcess || runCompleted) break;
    }
    if(killProcess)
    {
        theMiddlewareStateMachine->abort();
        abort();
    }
}

int main(int argc, char* argv[])
{
    MiddlewareStateMachine theMiddlewareStateMachine;

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
    cmd.setIntroductoryDescription("CMS Ph2_ACF main script");
    // error codes
    cmd.addErrorCode(0, "Success");
    cmd.addErrorCode(1, "Error");
    // options
    cmd.setHelpOption("h", "help", "Print this help page");

    cmd.defineOption("file", "Hw Description File", ArgvParser::OptionRequiresValue);
    cmd.defineOptionAlternative("file", "f");

    cmd.defineOption("last", "Use HW Description in the result directory of the last run");
    cmd.defineOptionAlternative("last", "l");

    cmd.defineOption("run", "Use HW Description in the result directory of the specified run", ArgvParser::OptionRequiresValue);
    cmd.defineOptionAlternative("run", "r");

    std::stringstream calibrationHelpMessage;
    calibrationHelpMessage << "Calibration to run. List of available calibrations:\n";
    for(const auto& calibrationList: theMiddlewareStateMachine.getCombinedCalibrationFactory().getAvailableCalibrations())
    {
        calibrationHelpMessage << "--------------------------------------------------------------------------" << std::endl;
        calibrationHelpMessage << BOLDGREEN << calibrationList.first << " Calibrations" << RESET << std::endl;
        calibrationHelpMessage << "--------------------------------------------------------------------------" << std::endl;
        for(const auto& calibration: calibrationList.second)
        {
            calibrationHelpMessage << BOLDBLUE << "\t" << calibration.first << RESET << std::endl;
            for(const auto& subCalibration: calibration.second)
            {
                calibrationHelpMessage << "\t\t" << subCalibration.first;
                if(subCalibration.second != "") calibrationHelpMessage << ": " << subCalibration.second;
                calibrationHelpMessage << std::endl;
            }
        }
    }

    cmd.defineOption("calibration", calibrationHelpMessage.str(), ArgvParser::OptionRequiresValue | ArgvParser::OptionRequired);
    cmd.defineOptionAlternative("calibration", "c");

    cmd.defineOption("batch", "Run the application in batch mode", ArgvParser::NoOptionAttribute);
    cmd.defineOptionAlternative("batch", "b");

    int result = cmd.parse(argc, argv);

    if(result != ArgvParser::NoParserError)
    {
        LOG(INFO) << cmd.parseErrorDescription(result);
        exit(EXIT_FAILURE);
    }

    std::string configurationFile;
    int         numberOfConfiguratioFileOptions = 0;
    if(cmd.foundOption("file"))
    {
        ++numberOfConfiguratioFileOptions;
        configurationFile = cmd.optionValue("file");
    }
    if(cmd.foundOption("run"))
    {
        ++numberOfConfiguratioFileOptions;
        int runNumber     = stoi(cmd.optionValue("run"));
        configurationFile = expandEnvironmentVariables("${PH2ACF_BASE_DIR}/") + getResultDirectoryName(runNumber) + "/" + OUTPUT_CONFIGURATION_FILE;
        LOG(INFO) << "Using configuration file from run " << runNumber << ": " << configurationFile << std::endl;
    }
    if(cmd.foundOption("last"))
    {
        ++numberOfConfiguratioFileOptions;
        int runNumber     = returnPreviousRunNumber("RunNumbers.dat");
        configurationFile = expandEnvironmentVariables("${PH2ACF_BASE_DIR}/") + getResultDirectoryName(runNumber) + "/" + OUTPUT_CONFIGURATION_FILE;
        LOG(INFO) << "Using configuration file from last run (" << runNumber << "): " << configurationFile << std::endl;
    }

    if(numberOfConfiguratioFileOptions == 0)
    {
        LOG(ERROR) << BOLDRED << "ERROR: HW configuration file needs to be specified using options file, run or last" << RESET;
        exit(1);
    }
    if(numberOfConfiguratioFileOptions > 1)
    {
        LOG(ERROR) << BOLDRED << "ERROR: options file, run and last are mutually exclusive, please use just one of them" << RESET;
        exit(1);
    }

    bool batchMode = (cmd.foundOption("batch")) ? true : false;

    Timer cGlobalTimer;
    cGlobalTimer.start();

    std::thread softKillThread(killProcessFunction, &theMiddlewareStateMachine);
    softKillThread.detach();

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
    int stateMachineStatus = INITIAL;

    theMiddlewareStateMachine.initialize();

    int   tAppArgc = 1;
    char* tAppArgv[2];
    tAppArgv[0] = argv[0];
    tAppArgv[1] = (char*)"-b";
    if(batchMode) tAppArgc = 2;
    TApplication theApp("App", &tAppArgc, tAppArgv);

    stateMachineStatus = HALTED;

    bool done = false;
    while(!done)
    {
        switch(stateMachineStatus)
        {
        case HALTED:
        {
            std::string   calibrationName = cmd.optionValue("calibration");
            ConfigureInfo theConfigureInfo;
            theConfigureInfo.setConfigurationFiles(configurationFile);
            theConfigureInfo.setCalibrationName(calibrationName);
            // theConfigureInfo.enableOpticalGroup(0,0,"pippo");
            // theConfigureInfo.enableHybrid(0,0,0, "pippo");
            // theConfigureInfo.enableHybrid(0,2,4, "pluto");
            theMiddlewareStateMachine.configure(theConfigureInfo);
            stateMachineStatus = CONFIGURED;
            break;
        }
        case CONFIGURED:
        {
            int       runNumber = returnAndIncreaseRunNumber("RunNumbers.dat");
            StartInfo theStartInfo;
            theStartInfo.setRunNumber(runNumber);
            theMiddlewareStateMachine.start(theStartInfo);
            stateMachineStatus = RUNNING;
            break;
        }
        case RUNNING:
        {
            if(cmd.optionValue("calibration") != "psphysics" && cmd.optionValue("calibration") != "2sphysics")
            {
                while(theMiddlewareStateMachine.status() == MiddlewareStateMachine::Status::RUNNING) { usleep(5e5); }
            }
            else
                usleep(20e6);
            std::cout << __PRETTY_FUNCTION__ << "Supervisor Sending Stop!!!" << std::endl;
            theMiddlewareStateMachine.stop();
            stateMachineStatus = STOPPED;
            break;
        }
        case STOPPED:
        {
            theMiddlewareStateMachine.halt();
            done = true;
            break;
        }
        }
    }

    signal(SIGINT, SIG_DFL);
    runCompleted = 1;

    if(!batchMode) theApp.Run();

    cGlobalTimer.stop();
    cGlobalTimer.show("Total execution time: ");

    return EXIT_SUCCESS;
}
