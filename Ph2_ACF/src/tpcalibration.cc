#include <cstring>

#include "Utils/Timer.h"
#include "Utils/Utilities.h"
#include "tools/LatencyScan.h"
#include "tools/PedeNoise.h"
#include "tools/SignalScan.h"
#include "tools/SignalScanFit.h"
#include "tools/TPCalibration.h"
#include "tools/Tool.h"

#include "TApplication.h"
#include "TROOT.h"
#include "Utils/argvparser.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;
using namespace CommandLineProcessing;

INITIALIZE_EASYLOGGINGPP

int main(int argc, char* argv[])
{
    // configure the logger
    el::Configurations conf(std::string(std::getenv("PH2ACF_BASE_DIR")) + "/settings/logger.conf");
    el::Loggers::reconfigureAllLoggers(conf);

    ArgvParser cmd;

    // init
    cmd.setIntroductoryDescription("CMS Ph2_ACF test pulse calibration tool to perform the calibration of threshold gain calibration");
    // error codes
    cmd.addErrorCode(0, "Success");
    cmd.addErrorCode(1, "Error");
    // options
    cmd.setHelpOption("h", "help", "Print this help page");

    cmd.defineOption("file", "Hw Description File . Default value: settings/d19c_test.xml", ArgvParser::OptionRequiresValue /*| ArgvParser::OptionRequired*/);
    cmd.defineOptionAlternative("file", "f");

    cmd.defineOption("start", "start value for test pulse amplitude, default: 165", ArgvParser::OptionRequiresValue);
    cmd.defineOptionAlternative("start", "s");

    cmd.defineOption("end", "end of test pulse amplitudes to be tested, default: 255", ArgvParser::OptionRequiresValue);
    cmd.defineOptionAlternative("end", "e");

    cmd.defineOption("stepsize", "stepsize for the test pulse calibration, default: 10", ArgvParser::OptionRequiresValue);

    cmd.defineOption("output", "Output Directory . Default value: Results/", ArgvParser::OptionRequiresValue /*| ArgvParser::OptionRequired*/);
    cmd.defineOptionAlternative("output", "o");

    cmd.defineOption("batch", "Run the application in batch mode", ArgvParser::NoOptionAttribute);
    cmd.defineOptionAlternative("batch", "b");

    int result = cmd.parse(argc, argv);

    if(result != ArgvParser::NoParserError)
    {
        LOG(INFO) << cmd.parseErrorDescription(result);
        exit(1);
    }

    // now query the parsing results
    std::string cHWFile = (cmd.foundOption("file")) ? cmd.optionValue("file") : "settings/d19c_test.xml";

    std::string cDirectory = (cmd.foundOption("output")) ? cmd.optionValue("output") : "Results/TPCalibration";

    bool batchMode = (cmd.foundOption("batch")) ? true : false;

    TApplication cApp("Root Application", &argc, argv);
    if(batchMode)
        gROOT->SetBatch(true);
    else
        TQObject::Connect("TCanvas", "Closed()", "TApplication", &cApp, "Terminate()");

    std::string cResultfile = "TPCalibration";

    // Record the Pedestals for each Testpulse amplitude and then do a fit
    std::stringstream outp;

    LOG(INFO) << BOLDBLUE << "Do test pulse calibration for the range:" << RESET;

    Timer t;
    t.start();

#ifdef __USE_ROOT__
    // reasonable start and end values for the CBC3 (for amplitudes lower than 165,
    // larger test pulse charges, the behavior is not linear anymore)
    int cStartAmp = (cmd.foundOption("start")) ? convertAnyInt(cmd.optionValue("start").c_str()) : 165;
    int cEndAmp   = (cmd.foundOption("end")) ? convertAnyInt(cmd.optionValue("end").c_str()) : 255;
    int cStepSize = (cmd.foundOption("stepsize")) ? convertAnyInt(cmd.optionValue("stepsize").c_str()) : 10;

    TPCalibration cTBCalibrate;
    cTBCalibrate.InitializeHw(cHWFile, outp);
    cTBCalibrate.InitializeSettings(cHWFile, outp);
    LOG(INFO) << outp.str();
    cTBCalibrate.CreateResultDirectory(cDirectory);
    cTBCalibrate.InitResultFile(cResultfile);
    cTBCalibrate.StartHttpServer();
    cTBCalibrate.ConfigureHw();
    cTBCalibrate.Initialise(false);
    cTBCalibrate.Init(cStartAmp, cEndAmp, cStepSize);

    // Now do the actual calirbation
    cTBCalibrate.RunCalibration();

    cTBCalibrate.SaveResults();
#endif

    t.stop();
    t.show("Time to run the test pulse calibration ");
    return 0;
}
