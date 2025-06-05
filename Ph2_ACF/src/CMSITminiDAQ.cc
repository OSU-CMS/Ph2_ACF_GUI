/*!
  \file                  CMSITminiDAQ.cc
  \brief                 Mini DAQ to test RD53 readout chip
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#include "System/SystemController.h"
#include "Utils/ConfigureInfo.h"
#include "Utils/argvparser.h"
#include "tools/RD53BERtest.h"
#include "tools/RD53ClockDelay.h"
#include "tools/RD53DataReadbackOptimization.h"
#include "tools/RD53Gain.h"
#include "tools/RD53GainOptimization.h"
#include "tools/RD53GenericDacDacScan.h"
#include "tools/RD53InjectionDelay.h"
#include "tools/RD53Latency.h"
#include "tools/RD53LpGBTeyeOpening.h"
#include "tools/RD53Physics.h"
#include "tools/RD53PixelAlive.h"
#include "tools/RD53SCurve.h"
#include "tools/RD53ThrAdjustment.h"
#include "tools/RD53ThrEqualization.h"
#include "tools/RD53ThrMinimization.h"
#include "tools/RD53VTRxLightYieldScan.h"
#include "tools/RD53VoltageTuning.h"

#ifdef __EUDAQ__
#include "TROOT.h"
#include "tools/RD53eudaqProducer.h"
#endif

// ##################
// # Default values #
// ##################
#define RUNNUMBER 0
#define FILERUNNUMBER "./RunNumber.txt"
#define BASEDIR "PH2ACF_BASE_DIR"
#define TESTSUBDETECTOR false

INITIALIZE_EASYLOGGINGPP

using namespace Ph2_System;
using namespace Ph2_HwInterface;

void introBanner()
{
    // #######################
    // # Introductory banner #
    // #######################
    LOG(INFO) << BOLDGREEN << "       ____  _     ____         _    ____ _____" << RESET;
    LOG(INFO) << BOLDGREEN << "      |  _ \\| |__ |___ \\       / \\  / ___|  ___|" << RESET;
    LOG(INFO) << BOLDGREEN << "      | |_) | '_ \\  __) |____ / _ \\| |   | |_" << RESET;
    LOG(INFO) << BOLDGREEN << "      |  __/| | | |/ __/_____/ ___ \\ |___|  _|" << RESET;
    LOG(INFO) << BOLDGREEN << "      |_|   |_| |_|_____|   /_/   \\_\\____|_|\n" << RESET;
    LOG(INFO) << BOLDGREEN << "  ____ __  __ ____ ___ _____          _       _ ____    _    ___" << RESET;
    LOG(INFO) << BOLDGREEN << " / ___|  \\/  / ___|_ _|_   _| __ ___ (_)_ __ (_)  _ \\  / \\  / _ \\" << RESET;
    LOG(INFO) << BOLDGREEN << "| |   | |\\/| \\___ \\| |  | || '_ ` _ \\| | '_ \\| | | | |/ _ \\| | | |" << RESET;
    LOG(INFO) << BOLDGREEN << "| |___| |  | |___) | |  | || | | | | | | | | | | |_| / ___ \\ |_| |" << RESET;
    LOG(INFO) << BOLDGREEN << " \\____|_|  |_|____/___| |_||_| |_| |_|_|_| |_|_|____/_/   \\_\\__\\_\\\n" << RESET;
}

void readBinaryData(const std::string& binaryFile, SystemController& mySysCntr, std::vector<RD53Event>& decodedEvents)
{
    size_t                errors = 0;
    std::vector<uint32_t> data;

    RD53Event::ForkDecodingThreads();

    LOG(INFO) << BOLDMAGENTA << "@@@ Decoding binary data file @@@" << RESET;
    mySysCntr.addFileHandler(binaryFile, 'r');
    LOG(INFO) << BOLDBLUE << "\t--> Data are being readout from binary file" << RESET;
    mySysCntr.readFile(data, 0);

    uint32_t status;
    RD53Event::DecodeEventsMultiThreads(data, decodedEvents, status);
    LOG(INFO) << GREEN << "Total number of 32-bit words read from binary file: " << BOLDYELLOW << data.size() << RESET;
    LOG(INFO) << GREEN << "Total number of events (i.e. bunch crossings) decoded from binary file: " << BOLDYELLOW << decodedEvents.size() << RESET;

    for(auto i = 0u; i < decodedEvents.size(); i++)
        if(RD53Event::EvtErrorHandler(decodedEvents[i].eventStatus) == false)
        {
            LOG(ERROR) << BOLDBLUE << "\t--> Corrupted bunch crossing n. " << BOLDYELLOW << i << RESET;
            errors++;
            RD53Event::PrintEvents({decodedEvents[i]});
        }

    if(decodedEvents.size() != 0)
    {
        LOG(INFO) << GREEN << "Corrupted bunch crossings: " << BOLDYELLOW << std::fixed << std::setprecision(3) << errors << " (" << 1. * errors / decodedEvents.size() * 100. << "%)"
                  << std::setprecision(-1) << RESET;
        int avgEventSize = data.size() / decodedEvents.size();
        LOG(INFO) << GREEN << "Average bunch crossing size is " << BOLDYELLOW << avgEventSize * RD53FWEvtEncoder::NBIT_EVT_WORD << RESET << GREEN << " bits over " << BOLDYELLOW << decodedEvents.size()
                  << RESET << GREEN << " events" << RESET;
    }

    std::string fileName(binaryFile);
    if(RD53Event::MakeNtuple(fileName.replace(fileName.find(".raw"), 4, ".root"), decodedEvents) == true) LOG(INFO) << GREEN << "Saving raw data into ROOT ntuple: " << BOLDYELLOW << fileName << RESET;

    mySysCntr.closeFileHandler();
}

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

    cmd.defineOption("calibSettingsFile", "Calibration settings override file", CommandLineProcessing::ArgvParser::OptionRequiresValue);
    cmd.defineOptionAlternative("calibSettingsFile", "s");

    cmd.defineOption("calib",
                     "Which calibration to run [latency pixelalive noise scurve gain threqu gainopt thrmin thradj "
                     "injdelay clkdelay datarbopt physics eudaq bertest voltagetuning gendacdac vtrx eye]",
                     CommandLineProcessing::ArgvParser::OptionRequiresValue);
    cmd.defineOptionAlternative("calib", "c");

    cmd.defineOption("binary", "Binary file to decode", CommandLineProcessing::ArgvParser::OptionRequiresValue);
    cmd.defineOptionAlternative("binary", "b");

    cmd.defineOption("prog", "Just program the system components", CommandLineProcessing::ArgvParser::NoOptionAttribute);
    cmd.defineOptionAlternative("prog", "p");

    cmd.defineOption("skipcfg", "Skip entire configuration sequence", CommandLineProcessing::ArgvParser::NoOptionAttribute);
    cmd.defineOptionAlternative("skipcfg", "k");

    cmd.defineOption("eudaqRunCtr", "EUDAQ-IT run control address (e.g. tcp://localhost:44000)", CommandLineProcessing::ArgvParser::OptionRequiresValue);

    cmd.defineOption("prodName", "Name of the EUDAQ producer in run controler", CommandLineProcessing::ArgvParser::OptionRequiresValue);
    cmd.defineOptionAlternative("prodName", "n");

    cmd.defineOption("reset", "Reset the backend board", CommandLineProcessing::ArgvParser::NoOptionAttribute);
    cmd.defineOptionAlternative("reset", "r");

    cmd.defineOption("dump", "Dump frontend chips register content", CommandLineProcessing::ArgvParser::NoOptionAttribute);
    cmd.defineOptionAlternative("dump", "d");

    cmd.defineOption("capture", "Capture communication with board (extension .bin)", CommandLineProcessing::ArgvParser::OptionRequiresValue);

    cmd.defineOption("replay", "Replay previously captured communication (extension .bin)", CommandLineProcessing::ArgvParser::OptionRequiresValue);

    cmd.defineOption("runtime", "Set running time for physics mode (in seconds)", CommandLineProcessing::ArgvParser::OptionRequiresValue);
    cmd.defineOptionAlternative("runtime", "t");

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
    std::ifstream theFileIn;
    theFileIn.open(FILERUNNUMBER, std::ios::in);
    if(theFileIn.is_open() == true) theFileIn >> runNumber;
    theFileIn.close();
    system(std::string("mkdir -p " + std::string(RD53Shared::RESULTDIR)).c_str());

    // ####################
    // # Retrieve options #
    // ####################
    std::string configFile        = cmd.foundOption("file") == true ? cmd.optionValue("file") : "";
    std::string calibSettingsFile = cmd.foundOption("calibSettingsFile") == true ? cmd.optionValue("calibSettingsFile") : configFile;
    std::string whichCalib        = cmd.foundOption("calib") == true ? cmd.optionValue("calib") : "";
    std::string EUDAQproducerNAME = cmd.foundOption("prodName") == true ? cmd.optionValue("prodName") : "";
    std::string binaryFile        = cmd.foundOption("binary") == true ? cmd.optionValue("binary") : "";
    std::string eudaqRunCtr       = cmd.foundOption("eudaqRunCtr") == true ? cmd.optionValue("eudaqRunCtr") : "tcp://localhost:44000";
    bool        program           = cmd.foundOption("prog") == true ? true : false;
    bool        skipcfg           = cmd.foundOption("skipcfg") == true ? true : false;
    bool        reset             = cmd.foundOption("reset") == true ? true : false;
    bool        dumpRegs          = cmd.foundOption("dump") == true ? true : false;
    int         runtime           = cmd.foundOption("runtime") == true ? stoi(cmd.optionValue("runtime")) : -1;
    if(cmd.foundOption("capture") == true)
        RegManager::enableCapture(cmd.optionValue("capture").insert(0, std::string(RD53Shared::RESULTDIR) + "/Run" + RD53Shared::fromInt2Str(runNumber) + "_"));
    else if(cmd.foundOption("replay") == true)
        RegManager::enableReplay(cmd.optionValue("replay"));

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

    introBanner();
    SystemController mySysCntr;

    // ##################################
    // # Configure the SystemController #
    // ##################################
    if((reset == true) || (dumpRegs == true) || (binaryFile != ""))
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

        // ####################
        // # Read binary file #
        // ####################
        else if(binaryFile != "")
            readBinaryData(binaryFile, mySysCntr, RD53Event::decodedEvents);
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
    if(whichCalib == "latency")
    {
        // ###################
        // # Run LatencyScan #
        // ###################
        LOG(INFO) << BOLDMAGENTA << "@@@ Performing Latency scan @@@" << RESET;

        std::string fileName("Run" + RD53Shared::fromInt2Str(runNumber) + "_Latency");
        Latency     la;
        la.Inherit(&mySysCntr);
        la.localConfigure(fileName, runNumber);
        la.run();
        la.analyze();
        la.draw();
    }
    else if(whichCalib == "datarbopt")
    {
        // ##################################
        // # Run Data Readback Optimization #
        // ##################################
        LOG(INFO) << BOLDMAGENTA << "@@@ Performing Data Readback Optimization @@@" << RESET;

        std::string              fileName("Run" + RD53Shared::fromInt2Str(runNumber) + "_DataReadbackOptimization");
        DataReadbackOptimization dro;
        dro.Inherit(&mySysCntr);
        dro.localConfigure(fileName, runNumber);
        dro.run();
        dro.draw();
    }
    else if(whichCalib == "pixelalive")
    {
        // ##################
        // # Run PixelAlive #
        // ##################
        LOG(INFO) << BOLDMAGENTA << "@@@ Performing PixelAlive scan @@@" << RESET;

        std::string fileName("Run" + RD53Shared::fromInt2Str(runNumber) + "_PixelAlive");
        PixelAlive  pa;
        pa.Inherit(&mySysCntr);
        pa.localConfigure(fileName, runNumber);

        // #############################################
        // # Address different subsets of the detector #
        // #############################################
        int  evenORodd = 0;
        bool doTwice   = false;
        do {
            if(TESTSUBDETECTOR == true)
            {
                if(pa.fDetectorContainer->size() != 1)
                {
                    auto boardSubset = [evenORodd](const BoardContainer* theBoard) { return (theBoard->getId() % 2 == evenORodd); };
                    pa.fDetectorContainer->addBoardQueryFunction(boardSubset, "boardSubset");
                    doTwice = true;
                }
                else if(pa.fDetectorContainer->getFirstObject()->size() != 1)
                {
                    auto optoGroupSubset = [evenORodd](const OpticalGroupContainer* theOpticalGroup) { return (theOpticalGroup->getId() % 2 == evenORodd); };
                    pa.fDetectorContainer->addOpticalGroupQueryFunction(optoGroupSubset, "opticalGroupSubset");
                    doTwice = true;
                }
                else if(pa.fDetectorContainer->getFirstObject()->getFirstObject()->size() != 1)
                {
                    auto hybridSubset = [evenORodd](const HybridContainer* theHybrid) { return (theHybrid->getId() % 2 == evenORodd); };
                    pa.fDetectorContainer->addHybridQueryFunction(hybridSubset, "moduleSubset");
                    doTwice = true;
                }
                else if(pa.fDetectorContainer->getFirstObject()->getFirstObject()->getFirstObject()->size() != 1)
                {
                    auto chipSubset = [evenORodd](const ChipContainer* theChip) { return (theChip->getId() % 2 == evenORodd); };
                    pa.fDetectorContainer->addReadoutChipQueryFunction(chipSubset, "chipSubset");
                    doTwice = true;
                }
            }

            pa.run();
            pa.analyze();
            pa.draw();
            RD53RunProgress::current() = 0;

            pa.fDetectorContainer->resetReadoutChipQueryFunction();
            pa.fDetectorContainer->resetHybridQueryFunction();
            pa.fDetectorContainer->resetOpticalGroupQueryFunction();
            pa.fDetectorContainer->resetBoardQueryFunction();

            evenORodd++;
        } while((doTwice == true) && (evenORodd < 2));
    }
    else if(whichCalib == "noise")
    {
        // #############
        // # Run Noise #
        // #############
        LOG(INFO) << BOLDMAGENTA << "@@@ Performing Noise scan @@@" << RESET;

        std::string fileName("Run" + RD53Shared::fromInt2Str(runNumber) + "_NoiseScan");
        PixelAlive  pa;
        pa.Inherit(&mySysCntr);
        pa.localConfigure(fileName, runNumber);
        pa.run();
        pa.analyze();
        pa.draw();
    }
    else if(whichCalib == "scurve")
    {
        // ##############
        // # Run SCurve #
        // ##############
        LOG(INFO) << BOLDMAGENTA << "@@@ Performing SCurve scan @@@" << RESET;

        std::string fileName("Run" + RD53Shared::fromInt2Str(runNumber) + "_SCurve");
        SCurve      sc;
        sc.Inherit(&mySysCntr);
        sc.localConfigure(fileName, runNumber);
        sc.run();
        sc.analyze();
        sc.draw();
    }
    else if(whichCalib == "gain")
    {
        // ############
        // # Run Gain #
        // ############
        LOG(INFO) << BOLDMAGENTA << "@@@ Performing Gain scan @@@" << RESET;

        std::string fileName("Run" + RD53Shared::fromInt2Str(runNumber) + "_Gain");
        Gain        ga;
        ga.Inherit(&mySysCntr);
        ga.localConfigure(fileName, runNumber);
        ga.run();
        ga.analyze();
        ga.draw();
    }
    else if(whichCalib == "gainopt")
    {
        // #########################
        // # Run Gain Optimization #
        // #########################
        LOG(INFO) << BOLDMAGENTA << "@@@ Performing Gain Optimization @@@" << RESET;

        std::string      fileName("Run" + RD53Shared::fromInt2Str(runNumber) + "_GainOptimization");
        GainOptimization go;
        go.Inherit(&mySysCntr);
        go.localConfigure(fileName, runNumber);
        go.run();
        go.analyze();
        go.draw();
    }
    else if(whichCalib == "threqu")
    {
        // ##############################
        // # Run Threshold Equalization #
        // ##############################
        LOG(INFO) << BOLDMAGENTA << "@@@ Performing Threshold Equalization @@@" << RESET;

        std::string     fileName("Run" + RD53Shared::fromInt2Str(runNumber) + "_ThrEqualization");
        ThrEqualization te;
        te.Inherit(&mySysCntr);
        te.localConfigure(fileName, runNumber);
        te.run();
        te.analyze();
        te.draw();
    }
    else if(whichCalib == "thrmin")
    {
        // ##############################
        // # Run Threshold Minimization #
        // ##############################
        LOG(INFO) << BOLDMAGENTA << "@@@ Performing Threshold Minimization @@@" << RESET;

        std::string     fileName("Run" + RD53Shared::fromInt2Str(runNumber) + "_ThrMinimization");
        ThrMinimization tm;
        tm.Inherit(&mySysCntr);
        tm.localConfigure(fileName, runNumber);
        tm.run();
        tm.analyze();
        tm.draw();
    }
    else if(whichCalib == "thradj")
    {
        // ##############################
        // # Run Threshold Minimization #
        // ##############################
        LOG(INFO) << BOLDMAGENTA << "@@@ Performing Threshold Adjustment @@@" << RESET;

        std::string   fileName("Run" + RD53Shared::fromInt2Str(runNumber) + "_ThrAdjustment");
        ThrAdjustment ta;
        ta.Inherit(&mySysCntr);
        ta.localConfigure(fileName, runNumber);
        ta.run();
        ta.analyze();
        ta.draw();
    }
    else if(whichCalib == "injdelay")
    {
        // #######################
        // # Run Injection Delay #
        // #######################
        LOG(INFO) << BOLDMAGENTA << "@@@ Performing Injection Delay scan @@@" << RESET;

        std::string    fileName("Run" + RD53Shared::fromInt2Str(runNumber) + "_InjectionDelay");
        InjectionDelay id;
        id.Inherit(&mySysCntr);
        id.localConfigure(fileName, runNumber);
        id.run();
        id.analyze();
        id.draw();
    }
    else if(whichCalib == "clkdelay")
    {
        // ###################
        // # Run Clock Delay #
        // ###################
        LOG(INFO) << BOLDMAGENTA << "@@@ Performing Clock Delay scan @@@" << RESET;

        std::string fileName("Run" + RD53Shared::fromInt2Str(runNumber) + "_ClockDelay");
        ClockDelay  cd;
        cd.Inherit(&mySysCntr);
        cd.localConfigure(fileName, runNumber);
        cd.run();
        cd.analyze();
        cd.draw();
    }
    else if(whichCalib == "bertest")
    {
        // ################
        // # Run BER test #
        // ################
        LOG(INFO) << BOLDMAGENTA << "@@@ Performing Bit Error Rate test @@@" << RESET;

        std::string fileName("Run" + RD53Shared::fromInt2Str(runNumber) + "_BERtest");
        BERtest     bt;
        bt.Inherit(&mySysCntr);
        bt.localConfigure(fileName, runNumber);
        bt.run();
        bt.draw();
    }
    else if(whichCalib == "voltagetuning")
    {
        // ######################
        // # Run Voltage Tuning #
        // ######################
        LOG(INFO) << BOLDMAGENTA << "@@@ Performing Voltage Tuning @@@" << RESET;

        std::string   fileName("Run" + RD53Shared::fromInt2Str(runNumber) + "_VoltageTuning");
        VoltageTuning vt;
        vt.Inherit(&mySysCntr);
        vt.localConfigure(fileName, runNumber);
        vt.run();
        vt.analyze();
        vt.draw();
    }
    else if(whichCalib == "gendacdac")
    {
        // ############################
        // # Run Generic DAC-DAC Scan #
        // ############################
        LOG(INFO) << BOLDMAGENTA << "@@@ Performing Generic DAC-DAC scan @@@" << RESET;

        std::string       fileName("Run" + RD53Shared::fromInt2Str(runNumber) + "_GenericDacDac");
        GenericDacDacScan gs;
        gs.Inherit(&mySysCntr);
        gs.localConfigure(fileName, runNumber);
        gs.run();
        gs.analyze();
        gs.draw();
    }
    else if(whichCalib == "vtrx")
    {
        // #############################
        // # Run VTRx Light Yield Scan #
        // #############################
        LOG(INFO) << BOLDMAGENTA << "@@@ Performing VTRx Light Yield scan @@@" << RESET;

        std::string        fileName("Run" + RD53Shared::fromInt2Str(runNumber) + "_VTRxScan");
        VTRxLightYieldScan vs;
        vs.Inherit(&mySysCntr);
        vs.localConfigure(fileName, runNumber);
        vs.run();
        vs.draw();
    }
    else if(whichCalib == "eye")
    {
        // ##############################
        // # Run LpGBT Eye Opening Scan #
        // ##############################
        LOG(INFO) << BOLDMAGENTA << "@@@ Performing LpGBT Eye Opening scan @@@" << RESET;

        std::string     fileName("Run" + RD53Shared::fromInt2Str(runNumber) + "_LpGBTeyeScan");
        LpGBTeyeOpening es;
        es.Inherit(&mySysCntr);
        es.localConfigure(fileName, runNumber);
        es.run();
        es.draw();
    }
    else if(whichCalib == "physics")
    {
        // ###############
        // # Run Physics #
        // ###############
        LOG(INFO) << BOLDMAGENTA << "@@@ Performing Physics data taking @@@" << RESET;

        Physics ph;
        ph.Inherit(&mySysCntr);
        if(binaryFile == "")
        {
            std::string fileName("Run" + RD53Shared::fromInt2Str(runNumber) + "_Physics");

            ph.localConfigure(fileName, runNumber);
            StartInfo theStartInfo;
            theStartInfo.setRunNumber(runNumber);
            ph.Start(theStartInfo);
            if(runtime == -1)
            {
                do {
                    LOG(INFO) << BOLDBLUE << "\t--> Press '" << BOLDYELLOW << "Enter" << BOLDBLUE << "' key to stop the run ..." << RESET;
                } while(std::cin.get() != '\n');
            }
            else
                std::this_thread::sleep_for(std::chrono::seconds(runtime));
            ph.Stop();
        }
        else
        {
            std::string fileName(binaryFile);
            fileName.erase(0, (fileName.find_last_of("/\\") == std::string::npos ? 0 : fileName.find_last_of("/\\")));
            fileName  = fileName.erase(fileName.find(".raw") - 8, 12) + "fromRaw";
            runNumber = atof(fileName.substr(fileName.find("Run") + 3, 6).c_str());
            ph.setValueInSettings<double>("SaveBinaryData", false);

            ph.localConfigure(fileName, runNumber);
            ph.analyze(true);
            ph.draw();
        }
    }
    else if(whichCalib == "eudaq")
    {
#ifdef __EUDAQ__
        // ######################
        // # Run EUDAQ producer #
        // ######################
        LOG(INFO) << BOLDMAGENTA << "@@@ Performing EUDAQ data taking @@@" << RESET;

        gROOT->SetBatch(true);

        auto theEUDAQproducer = eudaq::Producer::Make(EUDAQ::EUDAQproducerNAME, EUDAQproducerNAME == "" ? EUDAQ::EUDAQproducerNAME : EUDAQproducerNAME, eudaqRunCtr);

        if(!theEUDAQproducer)
        {
            LOG(ERROR) << BOLDRED << "Unknown Producer: " << EUDAQ::EUDAQproducerNAME << " - " << EUDAQproducerNAME << RESET;
            exit(EXIT_FAILURE);
        }

        static_cast<RD53eudaqProducer*>(theEUDAQproducer.get())->Creator(mySysCntr, configFile);

        try
        {
            theEUDAQproducer->Connect();
        }
        catch(...)
        {
            LOG(ERROR) << BOLDRED << "Could not connect to RunControl: " << eudaqRunCtr << RESET;
            exit(EXIT_FAILURE);
        }

        static_cast<RD53eudaqProducer*>(theEUDAQproducer.get())->MainLoop();
        runNumber = static_cast<RD53eudaqProducer*>(theEUDAQproducer.get())->RD53sysCntrPhys.theCurrentRun;
#else
        LOG(WARNING) << BOLDBLUE << "EUDAQ flag was OFF during compilation" << RESET;
        exit(EXIT_FAILURE);
#endif
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

    // ######################################################
    // # Disable all channels and destroy System Controller #
    // ######################################################
    std::string monitorFileName(mySysCntr.fDetectorMonitor != nullptr ? mySysCntr.fDetectorMonitor->getMonitorFileName() : "");
    bool        splitFile = mySysCntr.findValueInSettings<double>("DoSplitByBoardHybrid", false);
    if(binaryFile == "") mySysCntr.disableAllChannels();
    mySysCntr.Destroy();

    // ###########################
    // # Copy configuration file #
    // ###########################
    auto copyFile = [&](const std::string& fileName, const std::string& fileReName = "")
    {
        const std::string fileBaseName = fileName.substr(fileName.find_last_of("/\\") + 1);
        std::string       fileBaseReName(fileBaseName);
        if(fileReName != "") fileBaseReName = fileReName.substr(fileReName.find_last_of("/\\") + 1);
        const std::string outputFile = std::string(RD53Shared::RESULTDIR) + "/Run" + RD53Shared::fromInt2Str(runNumber) + "_" + (fileReName == "" ? fileBaseName : fileBaseReName);
        system(("cp " + fileName + " " + outputFile).c_str());
    };
    copyFile(configFile);
    if(configFile != calibSettingsFile) copyFile(calibSettingsFile);

    // ##########################
    // # Retrieve last DQM file #
    // ##########################
    if(monitorFileName != "")
    {
        std::string monitorFileNameNew(monitorFileName);
        monitorFileNameNew.erase(monitorFileNameNew.find("_"), monitorFileNameNew.find(".root") - monitorFileNameNew.find("_"));
        copyFile(monitorFileName, monitorFileNameNew);

        // ##########################################
        // # Rename and move split monitoring files #
        // ##########################################
        if(splitFile == true)
        {
            std::string monitorFileNameSplit(monitorFileName);
            monitorFileNameSplit.insert(monitorFileNameSplit.find(".root"), "_Board_*");
            system(("find " + monitorFileNameSplit + " > input.txt").c_str());
            system(("find " + monitorFileNameSplit + " | sed -E -re 's/(MonitorDQM_)(.*)(Board)/\\1\\3/g\' > output.txt").c_str());
            system(("sed -E -ri 's/(.*)(MonitorDQM)/" + std::string(RD53Shared::RESULTDIR) + "\\/Run" + RD53Shared::fromInt2Str(runNumber) + "_\\2/g\' output.txt").c_str());

            std::string   lineIn, lineOut;
            std::ifstream inFile("input.txt");
            std::ifstream outFile("output.txt");
            if(inFile.is_open() && outFile.is_open())
            {
                while(getline(inFile, lineIn) && getline(outFile, lineOut)) system(("cp " + lineIn + " " + lineOut).c_str());
                inFile.close();
                outFile.close();
            }

            system("rm input.txt output.txt");
        }
    }

    // #####################
    // # Update run number #
    // #####################
    std::ofstream theFileOut;
    runNumber++;
    theFileOut.open(FILERUNNUMBER, std::ios::out);
    if(theFileOut.is_open() == true) theFileOut << RD53Shared::fromInt2Str(runNumber) << std::endl;
    theFileOut.close();

    LOG(INFO) << BOLDMAGENTA << "@@@ End of CMSIT miniDAQ @@@" << RESET;

    return EXIT_SUCCESS;
}
