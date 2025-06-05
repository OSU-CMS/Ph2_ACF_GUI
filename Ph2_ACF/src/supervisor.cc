#include "DQMUtils/DQMInterface.h"
#include "HWDescription/BeBoard.h"
#include "HWDescription/Chip.h"
#include "HWDescription/Definition.h"
#include "HWDescription/Hybrid.h"
#include "HWInterface/BeBoardInterface.h"
#include "HWInterface/ChipInterface.h"
#include "MonitorDQM/MonitorDQMInterface.h"
#include "Parser/ParserDefinitions.h"
#include "Utils/ConfigureInfo.h"
#include "Utils/MiddlewareInterface.h"
#include "Utils/StartInfo.h"
#include "Utils/Utilities.h"
#include "Utils/argvparser.h"
#include "miniDAQ/CombinedCalibrationFactory.h"

#include <cstring>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "TROOT.h"
#include <TApplication.h>

#include "Utils/easylogging++.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;
using namespace CommandLineProcessing;

INITIALIZE_EASYLOGGINGPP

static bool  controlC            = false;
static pid_t runControllerPid    = -1;
static int   runControllerStatus = 0;

void interruptHandler(int handler)
{
    std::cout << __PRETTY_FUNCTION__ << " Sig handler: " << handler << std::endl;
    std::cout << __PRETTY_FUNCTION__ << "Run controller pid: " << runControllerPid << " status: " << runControllerStatus << std::endl;
    if(runControllerStatus != 0 && runControllerPid > 0)
    {
        std::cout << __PRETTY_FUNCTION__ << "Killing run controller pid: " << runControllerPid << " status: " << runControllerStatus << std::endl;
        kill(runControllerPid, SIGKILL);
    }
    exit(EXIT_FAILURE);

    controlC = true;
}

bool checkExitStatus(int status, std::string programName)
{
    if(WIFEXITED(status) && !WEXITSTATUS(status))
    {
        std::cout << __PRETTY_FUNCTION__ << programName << " executed successfully." << std::endl;
        return true;
    }
    else if(WIFEXITED(status) && WEXITSTATUS(status))
    {
        if(WEXITSTATUS(status) == 127)
        {
            // execv failed
            std::cout << __PRETTY_FUNCTION__ << programName << " execv failed." << std::endl;
            return false;
        }
        else
        {
            std::cout << __PRETTY_FUNCTION__ << programName << " terminated normally, but returned a non-zero status." << std::endl;
            return true;
        }
    }
    else
    {
        std::cout << __PRETTY_FUNCTION__ << programName << " didn't terminate normally. Status: " << status << std::endl;
        return false;
    }
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

    cmd.defineOption("file", "Hw Description File", ArgvParser::OptionRequiresValue);
    cmd.defineOptionAlternative("file", "f");

    cmd.defineOption("last", "Use HW Description in the result directory of the last run");
    cmd.defineOptionAlternative("last", "l");

    cmd.defineOption("run", "Use HW Description in the result directory of the specified run", ArgvParser::OptionRequiresValue);
    cmd.defineOptionAlternative("run", "r");

    CombinedCalibrationFactory theCombinedCalibrationFactory;
    std::stringstream          calibrationHelpMessage;
    calibrationHelpMessage << "Calibration to run. List of available calibrations:\n";
    for(const auto& calibrationList: theCombinedCalibrationFactory.getAvailableCalibrations())
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

    cmd.defineOption("port", "Port shift for TCP servers 0", ArgvParser::OptionRequiresValue /*| ArgvParser::OptionRequired*/);
    cmd.defineOptionAlternative("port", "p");

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

    // pid_t  runControllerPid = -1;
    // pid_t  dqmControllerPid = -1;
    int runControllerPidStatus = 0;

    std::cout << __PRETTY_FUNCTION__ << "Forking RunController" << std::endl;
    runControllerPid = fork();
    if(runControllerPid == -1) // pid == -1 means error occurred
    {
        LOG(ERROR) << "Can't fork RunController, error occurred";
        exit(EXIT_FAILURE);
    }
    else if(runControllerPid == 0) // pid == 0 means child process created
    {
        // getpid() returns process id of calling process
        // printf("Child runControllerPid, pid = %u\n",getpid());

        // the argv list first argument should point to
        // filename associated with file being executed
        // the array pointer must be terminated by NULL
        // pointer
        char* argv[] = {(char*)"RunController", NULL};

        // the execv() only return if error occurred.
        // The return value is -1
        execv((binDir + "RunController").c_str(), argv);
        LOG(ERROR) << "Can't run RunController, error occurred";
        exit(EXIT_FAILURE);
    }
    // usleep(10000000);
    //	std::cout << "forking dqm" << std::endl;
    //	dqmControllerPid = fork();
    //	if (dqmControllerPid == -1)// pid == -1 means error occurred
    //	{
    //		LOG (ERROR) << "Can't fork DQMHistogrammer, error occurred";
    //		exit(EXIT_FAILURE);
    //	}
    //
    //	else if (dqmControllerPid == 0)// pid == 0 means child process created
    //	{
    //		char * argv[] = {"DQMController", NULL};
    //		execv((binDir + "DQMController").c_str(),NULL);
    //		LOG (ERROR) << "Can't run DQMController, error occurred";
    //		exit(EXIT_FAILURE);
    //	}
    //
    //	// a positive number is returned for the pid of
    //	// parent process
    //	// getppid() returns process id of parent of
    //	// calling process
    //	printf("Parent process, pid = %u\n",getppid());

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
    TApplication cApp("Root Application", &argc, argv);

    if(batchMode)
        gROOT->SetBatch(true);
    else
        TQObject::Connect("TCanvas", "Closed()", "TApplication", &cApp, "Terminate()");

    DQMInterface        theDQMInterface;
    MonitorDQMInterface theMonitorDQMInterface;

    stateMachineStatus = HALTED;

    // int runControllerPidStatus = 0;
    // int dqmControllerPidStatus = 0;
    std::cout << __PRETTY_FUNCTION__ << "runControllerPid: " << runControllerPid << std::endl;
    std::cout << __PRETTY_FUNCTION__ << "runControllerPid: " << runControllerPid << std::endl;
    std::cout << __PRETTY_FUNCTION__ << "runControllerPid: " << runControllerPid << std::endl;
    std::cout << __PRETTY_FUNCTION__ << "runControllerPid: " << runControllerPid << std::endl;
    std::cout << __PRETTY_FUNCTION__ << "runControllerPid: " << runControllerPid << std::endl;
    std::cout << __PRETTY_FUNCTION__ << "runControllerPid: " << runControllerPid << std::endl;
    std::cout << __PRETTY_FUNCTION__ << "runControllerPid: " << runControllerPid << std::endl;
    std::cout << __PRETTY_FUNCTION__ << "runControllerPid: " << runControllerPid << std::endl;
    std::cout << __PRETTY_FUNCTION__ << "runControllerPid: " << runControllerPid << std::endl;
    std::cout << __PRETTY_FUNCTION__ << "runControllerPid: " << runControllerPid << std::endl;
    std::cout << __PRETTY_FUNCTION__ << "runControllerPid: " << runControllerPid << std::endl;
    bool done = false;
    while(!done)
    {
        if(runControllerPidStatus == 0 && (runControllerPidStatus = waitpid(runControllerPid, &runControllerStatus, WNOHANG)) != 0)
        {
            std::cout << __PRETTY_FUNCTION__ << " Run Controller status: " << runControllerStatus << std::endl;
            if(!checkExitStatus(runControllerStatus, "RunController")) exit(EXIT_FAILURE);
        }
        //		if(dqmControllerPidStatus == 0 && (dqmControllerPidStatus = waitpid(dqmControllerPid,
        //&dqmControllerStatus, WNOHANG)) != 0)
        //		{
        //			if(!checkExitStatus(dqmControllerStatus,"DQMController"))
        //			{
        //	    		kill(runControllerPid,SIGKILL);
        //	    		exit(EXIT_FAILURE);
        //			}
        //
        //		}
        //    	if(runControllerPidStatus != 0)// && dqmControllerPidStatus != 0)
        //    	{
        //    		std::cout << __PRETTY_FUNCTION__ << "3Run Controller pid status: " << runControllerPidStatus <<
        //    std::endl; 		done = true;
        //    	}
        else
        {
            try
            {
                std::cout << __PRETTY_FUNCTION__ << "Supervisor Run Controller status: " << runControllerStatus << std::endl;
                switch(stateMachineStatus)
                {
                case HALTED:
                {
                    std::cout << __PRETTY_FUNCTION__ << "Supervisor Sending Configure!!!" << std::endl;
                    std::string   calibrationName = cmd.optionValue("calibration");
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
                    int runNumber = returnAndIncreaseRunNumber("RunNumbers.dat");
                    std::cout << __PRETTY_FUNCTION__ << " [" << __LINE__ << "] RunNumber = " << runNumber << std::endl;
                    StartInfo theStartInfo;
                    theStartInfo.setRunNumber(runNumber);
                    std::cout << __PRETTY_FUNCTION__ << "Supervisor Sending Start!!!" << std::endl;
                    theDQMInterface.startProcessingData(theStartInfo);
                    theMonitorDQMInterface.startProcessingData();
                    theMiddlewareInterface.start(theStartInfo);
                    stateMachineStatus = RUNNING;
                    break;
                }
                case RUNNING:
                {
                    if(cmd.optionValue("calibration") != "psphysics" && cmd.optionValue("calibration") != "2sphysics")
                    {
                        std::string status = theMiddlewareInterface.status();

                        while(status != "Done")
                        {
                            usleep(2e6);
                            status = theMiddlewareInterface.status();
                            if(status == "Error")
                            {
                                std::cout << "An error occurred, Aborting..." << std::endl;
                                abort();
                            }
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
                kill(runControllerPid, SIGKILL);
                return EXIT_FAILURE;
            }
        }
        if(!done)
        {
            std::cout << __PRETTY_FUNCTION__ << "Supervisor SLEEPING!!!" << std::endl;
            usleep(1000000);
        }
    }

    theMonitorDQMInterface.stopProcessingData();

    std::cout << __PRETTY_FUNCTION__ << "Out of supervisor state machine!. Run Controller status: " << runControllerStatus << std::endl;
    checkExitStatus(runControllerStatus, "RunController");
    // checkExitStatus(dqmControllerStatus,"DQMController");

    if(!batchMode) cApp.Run();
    kill(runControllerPid, SIGKILL);

    return EXIT_SUCCESS;
}
