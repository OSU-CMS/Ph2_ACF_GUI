/*!
  \file                  TEPXTools.cc
  \brief                 TEPX tools for module testing
  \author                PSI team
  \version               1.0
  \date                  04/03/2024
  Support:               none
*/

#include "RD53BMuxReader.h"
#include "TEPXQuadNTC.h"
#include <vector>

#include "HWInterface/RD53FWInterface.h"
#include "HWInterface/RegManager.h"
#include "System/SystemController.h"
#include "Utils/ConfigureInfo.h"
#include "Utils/StartInfo.h"
#include "Utils/argvparser.h"

// ##################
// # Default values #
// ##################
#define RUNNUMBER 0
#define FILERUNNUMBER "./RunNumber.txt"
#define BASEDIR "PH2ACF_BASE_DIR"
#define TESTSUBDETECTOR false

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

    cmd.setIntroductoryDescription("@@@ TEPX Tools Application @@@");

    cmd.setHelpOption("h", "help", "Print this help page");

    cmd.defineOption("file", "Hardware description file", CommandLineProcessing::ArgvParser::OptionRequiresValue);
    cmd.defineOptionAlternative("file", "f");

    cmd.defineOption("calibSettingsFile", "Calibration settings override file", CommandLineProcessing::ArgvParser::OptionRequiresValue);
    cmd.defineOptionAlternative("calibSettingsFile", "s");

    cmd.defineOption("calib", "Which calibration to run [muxreader ntc]", CommandLineProcessing::ArgvParser::OptionRequiresValue);
    cmd.defineOptionAlternative("calib", "c");

    cmd.defineOption("comment", "Operator comment to be printed to log", CommandLineProcessing::ArgvParser::OptionRequiresValue);
    cmd.defineOptionAlternative("comment", "m");

    cmd.defineOption("prog", "Just program the system components", CommandLineProcessing::ArgvParser::NoOptionAttribute);
    cmd.defineOptionAlternative("prog", "p");

    cmd.defineOption("skipcfg", "Skip entire configuration sequence", CommandLineProcessing::ArgvParser::NoOptionAttribute);
    cmd.defineOptionAlternative("skipcfg", "k");

    cmd.defineOption("reset", "Reset the backend board", CommandLineProcessing::ArgvParser::NoOptionAttribute);
    cmd.defineOptionAlternative("reset", "r");

    cmd.defineOption("dump", "Dump frontend chips register content", CommandLineProcessing::ArgvParser::NoOptionAttribute);
    cmd.defineOptionAlternative("dump", "d");

    cmd.defineOption("capture", "Capture communication with board (extension .bin)", CommandLineProcessing::ArgvParser::OptionRequiresValue);

    cmd.defineOption("replay", "Replay previously captured communication (extension .bin)", CommandLineProcessing::ArgvParser::OptionRequiresValue);

    int result = cmd.parse(argc, argv);
    if(result != CommandLineProcessing::ArgvParser::NoParserError)
    {
        LOG(INFO) << cmd.parseErrorDescription(result);
        exit(EXIT_FAILURE);
    }

    // ###################
    // # Read run number #
    // ###################
    unsigned int  runNumber = RUNNUMBER;
    std::ifstream fileRunNumberIn;
    fileRunNumberIn.open(FILERUNNUMBER, std::ios::in);
    if(fileRunNumberIn.is_open() == true) fileRunNumberIn >> runNumber;
    fileRunNumberIn.close();
    system(std::string("mkdir -p " + std::string(RD53Shared::RESULTDIR)).c_str());

    // ####################
    // # Retrieve options #
    // ####################
    std::string configFile        = cmd.foundOption("file") == true ? cmd.optionValue("file") : "";
    std::string calibSettingsFile = cmd.foundOption("calibSettingsFile") == true ? cmd.optionValue("calibSettingsFile") : configFile;
    std::string whichCalib        = cmd.foundOption("calib") == true ? cmd.optionValue("calib") : "";
    std::string operatorComment   = cmd.foundOption("comment") == true ? cmd.optionValue("comment") : "";
    bool        program           = cmd.foundOption("prog") == true ? true : false;
    bool        skipcfg           = cmd.foundOption("skipcfg") == true ? true : false;
    bool        reset             = cmd.foundOption("reset") == true ? true : false;
    bool        dumpRegs          = cmd.foundOption("dump") == true ? true : false;
    if(cmd.foundOption("capture") == true)
        RegManager::enableCapture(cmd.optionValue("capture").insert(0, std::string(RD53Shared::RESULTDIR) + "/Run" + RD53Shared::fromInt2Str(runNumber) + "_"));
    else if(cmd.foundOption("replay") == true)
        RegManager::enableReplay(cmd.optionValue("replay"));

    // ########################
    // # Configure the logger #
    // ########################
    std::string fileName("logs/TEPXTools" + RD53Shared::fromInt2Str(runNumber));
    if(whichCalib != "") fileName += "_" + whichCalib;
    fileName += ".log";
    el::Configurations conf(std::string(std::getenv(BASEDIR)) + "/settings/logger.conf");
    conf.set(el::Level::Global, el::ConfigurationType::Format, "|%datetime{%h:%m:%s}|%levshort|%msg");
    conf.set(el::Level::Global, el::ConfigurationType::Filename, fileName);
    el::Loggers::reconfigureAllLoggers(conf);

    if(operatorComment != "") LOG(INFO) << "Operator comment:" << operatorComment << RESET;

    SystemController mySysCntr;

    // ##################################
    // # Configure the SystemController #
    // ##################################
    if((reset == true) || (dumpRegs == true))
    {
        std::stringstream outp;
        mySysCntr.InitializeHw(configFile, outp);
        mySysCntr.InitializeSettings(calibSettingsFile, outp);

        // ##################
        // # Reset hardware #
        // ##################
        if(reset == true)
        {
            if(mySysCntr.fDetectorContainer->getFirstObject()->getFirstObject()->flpGBT == nullptr)
                static_cast<RD53FWInterface*>(mySysCntr.fBeBoardFWMap[mySysCntr.fDetectorContainer->getFirstObject()->getId()])->ResetSequence("160");
            else
                static_cast<RD53FWInterface*>(mySysCntr.fBeBoardFWMap[mySysCntr.fDetectorContainer->getFirstObject()->getId()])->ResetSequence("320");
            exit(EXIT_SUCCESS);
        }

        // ##########################################
        // # Dump FW and frontend registers content #
        // ##########################################
        else if(dumpRegs == true)
        {
            LOG(INFO) << BOLDMAGENTA << "@@@ Dumping frontend registers @@@" << RESET;
            mySysCntr.DumpRegisters();
        }
    }
    else
    {
        // #######################
        // # Initialize Hardware #
        // #######################
        LOG(INFO) << BOLDMAGENTA << "@@@ Initializing the Hardware @@@" << RESET;
        ConfigureInfo theConfigureInfo;
        theConfigureInfo.setConfigurationFiles(configFile, calibSettingsFile);
        theConfigureInfo.setCalibrationName(whichCalib);
        mySysCntr.Configure(theConfigureInfo, !skipcfg);
        LOG(INFO) << BOLDMAGENTA << "@@@ Hardware initialization done @@@" << RESET;
    }

    LOG(INFO) << RESET;

    // ###################
    // # Run Calibration #
    // ###################
    if(whichCalib == "muxreader")
    {
        // ####################
        // # Read multiplexer #
        // ####################
        LOG(INFO) << BOLDMAGENTA << "@@@ Reading out multiplexers @@@" << RESET;

        std::string    fileName("Run" + RD53Shared::fromInt2Str(runNumber) + "_MuxReader");
        RD53BMuxReader mux;
        mux.Inherit(&mySysCntr);
        mux.localConfigure(fileName, runNumber);
        mux.run();
    }
    else if(whichCalib == "ntc")
    {
        // ###################
        // #  Read out NTCs  #
        // ###################
        LOG(INFO) << BOLDMAGENTA << "@@@ Reading out NTCs @@@" << RESET;

        std::string fileName("Run" + RD53Shared::fromInt2Str(runNumber) + "_NTC");
        TEPXQuadNTC ntc;
        ntc.Inherit(&mySysCntr);
        ntc.localConfigure(fileName, runNumber);
        ntc.run();
    }
    else if((program == false) && (dumpRegs == false))
    {
        if(whichCalib == "")
            LOG(ERROR) << BOLDRED << "Error: calibration not specified" << RESET;
        else
            LOG(ERROR) << BOLDRED << "Error: option not recognized (" << BOLDYELLOW << whichCalib << BOLDRED << ")" << RESET;

        mySysCntr.Destroy();
        exit(EXIT_FAILURE);
    }

    // ###########################
    // # Copy configuration file #
    // ###########################
    auto copyConfigFile = [&](const std::string& fileName)
    {
        const auto fileBasename = fileName.substr(fileName.find_last_of("/\\") + 1);
        const auto outputFile   = std::string(RD53Shared::RESULTDIR) + "/Run" + RD53Shared::fromInt2Str(runNumber) + "_" + fileBasename;
        system(("cp " + fileName + " " + outputFile).c_str());
    };
    copyConfigFile(configFile);
    if(configFile != calibSettingsFile) copyConfigFile(calibSettingsFile);

    // #####################
    // # Update run number #
    // #####################
    std::ofstream fileRunNumberOut;
    runNumber++;
    fileRunNumberOut.open(FILERUNNUMBER, std::ios::out);
    if(fileRunNumberOut.is_open() == true) fileRunNumberOut << RD53Shared::fromInt2Str(runNumber) << std::endl;
    fileRunNumberOut.close();

    // ######################################################
    // # Disable all channels and destroy System Controller #
    // ######################################################
    mySysCntr.disableAllChannels();
    mySysCntr.Destroy();

    LOG(INFO) << BOLDMAGENTA << "@@@ End of TEPX Tools Application @@@" << RESET;

    return EXIT_SUCCESS;
}
