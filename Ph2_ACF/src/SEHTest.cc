
#include "Utils/Timer.h"
#include "Utils/Utilities.h"
#include "Utils/argvparser.h"
#include "tools/Tool.h"

#include "tools/SEHTester.h"

#ifdef __USE_ROOT__
#include "TApplication.h"
#include "TROOT.h"
#endif

#define __NAMEDPIPE__

#ifdef __NAMEDPIPE__
#include "Utils/gui_logger.h"
#endif

#include <cstring>
#include <fstream>
#include <inttypes.h>

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;
using namespace CommandLineProcessing;

using namespace std;
INITIALIZE_EASYLOGGINGPP

#define CHIPSLAVE 4
sig_atomic_t killProcess  = 0;
sig_atomic_t runCompleted = 0;

void interruptHandler(int handler) { killProcess = 1; }

void killProcessFunction(Tool* theTool)
{
    while(1)
    {
        usleep(250000);
        if(killProcess || runCompleted) break;
    }
    if(killProcess)
    {
        theTool->SaveResults();
        theTool->WriteRootFile();
        theTool->CloseResultFile();
        theTool->Destroy();
        abort();
    }
}

int main(int argc, char* argv[])
{
#if defined(__TCUSB__) && defined(__USE_ROOT__)
    // configure the logger
    el::Configurations conf(std::string(std::getenv("PH2ACF_BASE_DIR")) + "/settings/logger.conf");
    el::Loggers::reconfigureAllLoggers(conf);

    ArgvParser cmd;

    // init
    cmd.setIntroductoryDescription("Binary to test a 2S-SEH with a test card");
    // error codes
    cmd.addErrorCode(0, "Success");
    cmd.addErrorCode(1, "Error");
    // options
    cmd.setHelpOption("h", "help", "Print this help page");
    cmd.defineOption("file", "Hw Description file. Default value: settings/D19CDescription_ROH_EFC7.xml", ArgvParser::OptionRequiresValue /*| ArgvParser::OptionRequired*/);
    cmd.defineOptionAlternative("file", "f");
    // Load pattern
    cmd.defineOption("test-internal-pattern", "Internally Generated LpGBT Pattern", ArgvParser::OptionRequiresValue /*| ArgvParser::OptionRequires*/);
    cmd.defineOptionAlternative("test-internal-pattern", "ip");
    cmd.defineOption("test-external-pattern",
                     "Externally Generated LpGBT Pattern using the Data Player for Control FC7; Also, an automated comparision is performed",
                     ArgvParser::OptionRequiresValue /*| ArgvParser::OptionRequires*/);
    cmd.defineOptionAlternative("test-external-pattern", "ep");

    cmd.defineOption("cic-pattern", "Externally Generated LpGBT Pattern using CIC output", ArgvParser::NoOptionAttribute /*| ArgvParser::OptionRequires*/);
    cmd.defineOptionAlternative("cic-pattern", "cp");

    // Test Reset lines
    cmd.defineOption("test-reset", "Test Reset lines");
    cmd.defineOptionAlternative("test-reset", "r");
    // test I2C Masters
    cmd.defineOption("test-i2c", "Test I2C LpGBT Masters on SEH", ArgvParser::OptionRequiresValue /*| ArgvParser::OptionRequires*/);
    cmd.defineOptionAlternative("test-i2c", "i2c");
    // test ADC channels
    cmd.defineOption("test-adc", "Test LpGBT ADCs on SEH");
    cmd.defineOptionAlternative("test-adc", "a");
    cmd.defineOption("calibrate-adc", "Calibrate LpGBT ADCs on SEH via testcard");
    cmd.defineOptionAlternative("calibrate-adc", "c");
    // run Eye Opening Monitor
    cmd.defineOption("test-eom", "Run Eye Opening Monitor test");
    cmd.defineOptionAlternative("test-eom", "eom");
    //
    cmd.defineOption("eq-attenuation", "EQ attenuation for eye opening measurement", ArgvParser::OptionRequiresValue);
    cmd.defineOptionAlternative("eq-attenuation", "eqa");
    // clock test
    cmd.defineOption("test-clock", "Run clock tests", ArgvParser::NoOptionAttribute);
    // fast command test
    cmd.defineOption("test-fcmd", "Scope fast commands [de-serialized]");
    cmd.defineOption("fcmd-pattern", "Injected pattern (simulates FCMD) on the DownLink", ArgvParser::OptionRequiresValue /*| ArgvParser::OptionRequires*/);
    cmd.defineOptionAlternative("fcmd-pattern", "fp");
    //
    cmd.defineOption("fcmd-test", "Run fast command tests", ArgvParser::NoOptionAttribute);
    cmd.defineOption("fcmd-test-start-pattern", "Fast command FSM test start pattern", ArgvParser::OptionRequiresValue);
    cmd.defineOption("fcmd-test-userfile", "User file with fastcommands for testing", ArgvParser::OptionRequiresValue);
    cmd.defineOption("test-ber", "Run Bit Error Rate test");
    cmd.defineOptionAlternative("test-ber", "ber");
    cmd.defineOption("ber-pattern", "Define pattern to be used for Bit Error Rate test", ArgvParser::OptionRequiresValue /*| ArgvParser::OptionRequires*/);
    cmd.defineOptionAlternative("ber-pattern", "bp");
    // FCMD check in BRAM
    cmd.defineOption("bramfcmd-check", "Access to written data in BRAM", ArgvParser::OptionRequiresValue);
    // Write reference patterns to BRAM
    cmd.defineOption("bramreffcmd-write", "Write reference patterns to BRAM", ArgvParser::OptionRequiresValue);
    // convert user file to fw format
    cmd.defineOption("convert-userfile", "Convert user defined file to fw compliant format", ArgvParser::OptionRequiresValue);
    // read single ref FCMD BRAM addr
    cmd.defineOption("read-ref-bram", "Read single ref FCMD BRAM address", ArgvParser::OptionRequiresValue);
    // read single check FCMD BRAM addr
    cmd.defineOption("read-check-bram", "Read single check FCMD BRAM address", ArgvParser::OptionRequiresValue);
    // Flush check BRAM
    cmd.defineOption("clear-check-bram", "", ArgvParser::NoOptionAttribute);
    // Flush ref BRAM
    cmd.defineOption("clear-ref-bram", "", ArgvParser::NoOptionAttribute);
    // Test VTRx+ registers
    cmd.defineOption("test-vtrx", "Test testVTRx+ slow control");
    cmd.defineOptionAlternative("test-vtrx", "v");
    // Efficiency
    cmd.defineOption("test-efficiency", "Measure the DC/DC efficiency");
    cmd.defineOption("measure-input-iv", "Measure input currents and voltages on test card");
    cmd.defineOptionAlternative("measure-input-iv", "iv");
    cmd.defineOption("powersupply", "Use remote control of the 10V power supply in order to ramp up the voltage", ArgvParser::NoOptionAttribute);
    cmd.defineOptionAlternative("powersupply", "ps");
    // Bias voltage leakage current
    cmd.defineOption("test-leak", "Measure the Bias voltage leakage current ", ArgvParser::OptionRequiresValue);
    cmd.defineOption("test-ext-leak", "Measure the Bias voltage leakage current using an external power supply", ArgvParser::OptionRequiresValue);
    cmd.defineOption("test-leak-parallel", "Runs the HV leak test in parallel", ArgvParser::NoOptionAttribute);

    // Bias voltage on sensor side
    cmd.defineOption("test-bias", "Measure the Bias voltage on sensor side ", ArgvParser::NoOptionAttribute);
    cmd.defineOption("test-ext-bias", "Measure the Bias voltage on sensor side using an external power supply", ArgvParser::NoOptionAttribute);

    // Load values defining a test from file
    cmd.defineOption("rightLoad", "right load 1 step = 635uA 0xfff = 2.6A ", ArgvParser::OptionRequiresValue);
    cmd.defineOption("leftLoad", "left load 1 step = 635uA 0xfff = 2.6A ", ArgvParser::OptionRequiresValue);
    // Bias voltage on sensor side
    // Load values defining a test from file
    cmd.defineOption("test-parameter", "Use user file with test parameters, otherwise (or if file is missing it) default parameters will be used", ArgvParser::OptionRequiresValue);

    // general
    cmd.defineOption("batch", "Run the application in batch mode", ArgvParser::NoOptionAttribute);
    cmd.defineOptionAlternative("batch", "b");

    //
    cmd.defineOption("hybridId", "Name or serial number of SEH", ArgvParser::OptionRequiresValue);

    //
    cmd.defineOption("output", "Output directory. Default: Results/", ArgvParser::OptionRequiresValue);
    cmd.defineOptionAlternative("output", "o");

    cmd.defineOption("LVPowerSupplyId", "External low voltage (SEH input voltage) power supply ID", ArgvParser::OptionRequiresValue);
    cmd.defineOption("LVChannelId", "External low voltage (SEH input voltage) channel ID", ArgvParser::OptionRequiresValue);
    cmd.defineOption("HVPowerSupplyId", "External high voltage (sensor bias voltage) power supply ID", ArgvParser::OptionRequiresValue);
    cmd.defineOption("HVChannelId", "External high voltage (sensor bias voltage) channel ID", ArgvParser::OptionRequiresValue);
    cmd.defineOption("channelsFromFile", "Extract external power supply information from settings file", ArgvParser::NoOptionAttribute);

    //
    cmd.defineOption("USBBus", "USB device bus number", ArgvParser::OptionRequiresValue);
    cmd.defineOption("USBDev", "USB device device number", ArgvParser::OptionRequiresValue);
    cmd.defineOption("linkId", "Optical link Id", ArgvParser::OptionRequiresValue);
    cmd.defineOption("fmcId", "Optical fmc Id", ArgvParser::OptionRequiresValue);
    cmd.defineOption("useGui",
                     "Support for running the test from the gui for hybrids testing. The named pipe for communication needs to be passed as the last parameter. Default: false",
                     ArgvParser::NoOptionAttribute);
    int result = cmd.parse(argc, argv);
    if(result != ArgvParser::NoParserError)
    {
        LOG(INFO) << cmd.parseErrorDescription(result);
        exit(1);
    }

    std::string cHWFile    = (cmd.foundOption("file")) ? cmd.optionValue("file") : "settings/D19CDescription_ROH_OFC7.xml";
    bool        batchMode  = (cmd.foundOption("batch")) ? true : false;
    std::string cDirectory = (cmd.foundOption("output")) ? cmd.optionValue("output") : "Results/";
    std::string cHybridId  = (cmd.foundOption("hybridId")) ? cmd.optionValue("hybridId") : "xxxx";
    // Test to perform
    bool        cFCMDTest              = (cmd.foundOption("fcmd-test")) ? true : false;
    std::string cFCMDTestStartPattern  = (cmd.foundOption("fcmd-test-start-pattern")) ? cmd.optionValue("fcmd-test-start-pattern") : "11000001";
    std::string cFCMDTestUserFileName  = (cmd.foundOption("fcmd-test-userfile")) ? cmd.optionValue("fcmd-test-userfile") : "fcmd_file.txt";
    std::string cBRAMFCMDLine          = (cmd.foundOption("bramfcmd-check")) ? cmd.optionValue("bramfcmd-check") : "fe_for_ps_roh_fcmd_SSA_l_check";
    std::string cBRAMFCMDFileName      = (cmd.foundOption("bramreffcmd-write")) ? cmd.optionValue("bramreffcmd-write") : "fcmd_file.txt";
    std::string cConvertUserFileName   = (cmd.foundOption("convert-userfile")) ? cmd.optionValue("convert-userfile") : "fcmd_file.txt";
    std::string cRefBRAMAddr           = (cmd.foundOption("read-ref-bram")) ? cmd.optionValue("read-ref-bram") : "0";
    std::string cCheckBRAMAddr         = (cmd.foundOption("read-check-bram")) ? cmd.optionValue("read-check-bram") : "0";
    uint8_t     cFCMDPattern           = (cmd.foundOption("fcmd-pattern")) ? convertAnyInt(cmd.optionValue("fcmd-pattern").c_str()) : 0;
    std::string cTestParameterFileName = (cmd.foundOption("test-parameter")) ? cmd.optionValue("test-parameter") : "testParameters.txt";
    uint16_t    cLeakVoltage           = (cmd.foundOption("test-leak")) ? convertAnyInt(cmd.optionValue("test-leak").c_str()) : 0;
    uint16_t    cExtLeakVoltage        = (cmd.foundOption("test-ext-leak")) ? convertAnyInt(cmd.optionValue("test-ext-leak").c_str()) : 0;
    uint16_t    cLeftLoad              = (cmd.foundOption("leftLoad")) ? convertAnyInt(cmd.optionValue("leftLoad").c_str()) : 0;
    uint16_t    cRightLoad             = (cmd.foundOption("rightLoad")) ? convertAnyInt(cmd.optionValue("rightLoad").c_str()) : 0;
    std::string cLVPowerSupplyId       = (cmd.foundOption("LVPowerSupplyId")) ? cmd.optionValue("LVPowerSupplyId") : "MyRohdeSchwarz";
    std::string cLVChannelId           = (cmd.foundOption("LVChannelId")) ? cmd.optionValue("LVChannelId") : "LV_Module3";
    std::string cHVPowerSupplyId       = (cmd.foundOption("HVPowerSupplyId")) ? cmd.optionValue("HVPowerSupplyId") : "MyIsegSHR4220";
    std::string cHVChannelId           = (cmd.foundOption("HVChannelId")) ? cmd.optionValue("HVChannelId") : "HV_Module1";
    // To use from the GUI
    uint32_t cUsbBus = (cmd.foundOption("USBBus")) ? (uint32_t)(std::stoi(cmd.optionValue("USBBus"))) : 0; // Default option?
    uint8_t  cUsbDev = (cmd.foundOption("USBDev")) ? (uint32_t)(std::stoi(cmd.optionValue("USBDev"))) : 0; // Default option?
    bool     cGui    = (cmd.foundOption("useGui"));

    uint8_t            linkId = (cmd.foundOption("linkId")) ? (uint32_t)(std::stoi(cmd.optionValue("linkId"))) : 0;
    std::string        fmcId  = (cmd.foundOption("fmcId")) ? cmd.optionValue("fmcId") : "L12";
    pugi::xml_document doc;
    if(!doc.load_file(cHWFile.c_str())) return -1;
    pugi::xml_node     cDescription = doc.child("HwDescription");
    std::vector<float> cVoltages    = {5., 5.2, 5.4, 6., 7., 8., 9., 9.1, 9.2, 9.3, 9.4, 9.5, 9.6, 9.7, 9.8, 9.9, 10., 10.5, 10., 9., 8., 7., 6.6, 6.4, 6.2, 6.0, 5.8, 5};
    for(pugi::xml_node devices = cDescription.first_child(); devices; devices = devices.next_sibling())
    {
        for(pugi::xml_node ps = devices.first_child(); ps; ps = ps.next_sibling())
        {
            if(cmd.foundOption("linkId") && cmd.foundOption("fmcId"))
            {
                if(static_cast<std::string>(ps.name()) == "OpticalGroup")
                {
                    LOG(INFO) << BOLDBLUE << "Identified optical group" << RESET;
                    pugi::xml_attribute attr = ps.attribute("Id");
                    attr.set_value(linkId);
                    attr = ps.attribute("FMCId");
                    attr.set_value(fmcId.c_str());
                }
            }
            if((static_cast<std::string>(ps.name()) == "InputVoltages") && (static_cast<std::string>(devices.name()) == "TestParameters"))
            {
                string loadedVector = ps.child_value("Voltage");
                if(loadedVector != "")
                {
                    int         pos = 0, new_pos = 0;
                    std::string sep = ",";
                    cVoltages.clear();
                    loadedVector += ",";
                    while(true)
                    {
                        new_pos = loadedVector.find(sep, pos);
                        if(new_pos == -1) { break; }
                        cVoltages.push_back(stof(loadedVector.substr(pos, new_pos - pos)));
                        pos = new_pos + 1;
                    }
                }
            }
            std::string stringID(ps.attribute("ID").value());
            std::string stringType(ps.attribute("Type").value());
            if(stringType == "LV")
            {
                for(pugi::xml_node channel = ps.child("Channel"); channel; channel = channel.next_sibling("Channel"))
                {
                    std::string stringChannel(channel.attribute("ID").value());
                    cLVPowerSupplyId = stringID;
                    cLVChannelId     = stringChannel;
                    LOG(INFO) << BOLDBLUE << "Identified LV Power Supply " << stringID << " and channel " << stringChannel << RESET;
                }
            }
            if(stringType == "HV")
            {
                for(pugi::xml_node channel = ps.child("Channel"); channel; channel = channel.next_sibling("Channel"))
                {
                    std::string stringChannel(channel.attribute("ID").value());
                    cHVPowerSupplyId = stringID;
                    cHVChannelId     = stringChannel;
                    LOG(INFO) << BOLDBLUE << "Identified HV Power Supply " << stringID << " and channel " << stringChannel << RESET;
                }
            }
        }
    }
    if(cmd.foundOption("linkId") && cmd.foundOption("fmcId")) doc.save_file((cHWFile + "_copy").c_str());

    cDirectory += Form("2S_SEH_%s", cHybridId.c_str());

    TApplication cApp("Root Application", &argc, argv);
    if(batchMode)
        gROOT->SetBatch(true);
    else
        TQObject::Connect("TCanvas", "Closed()", "TApplication", &cApp, "Terminate()");

    std::string cResultfile = "Hybrid";
    // Timer t;

    if(cGui)
    {
        // Initialize gui communication with named pipe
        gui::init(argv[argc - 1]);

        gui::status("Initializing test");
        gui::progress(0 / 10.0);
    }
    // Initialize and Configure Back-End (Optical) FC7
    Tool cTool;

    std::thread softKillThread(killProcessFunction, &cTool);
    softKillThread.detach();

    struct sigaction act;
    act.sa_handler = interruptHandler;
    sigaction(SIGINT, &act, NULL);

    struct sigaction act2;
    act2.sa_handler = interruptHandler;
    sigaction(SIGTERM, &act2, NULL);

    std::stringstream outp;
    LOG(INFO) << BOLDYELLOW << "Initializing FC7" << RESET;
    if(cmd.foundOption("linkId") && cmd.foundOption("fmcId"))
    {
        cTool.InitializeHw((cHWFile + "_copy").c_str(), outp);
        cTool.InitializeSettings((cHWFile + "_copy").c_str(), outp);
        remove((cHWFile + "_copy").c_str());
    }
    else
    {
        cTool.InitializeHw((cHWFile).c_str(), outp);
        cTool.InitializeSettings((cHWFile).c_str(), outp);
    }
    LOG(INFO) << outp.str();
    outp.str("");
    cTool.CreateResultDirectory(cDirectory, true, true);
    cTool.InitResultFile(cResultfile);
    cTool.initializeExceptionHandler();
    cTool.bookSummaryTree();

    LOG(INFO) << BOLDYELLOW << "Monitoring file name " << cTool.GetMonitorFileName() << RESET;
    LOG(INFO) << BOLDYELLOW << "Configuring FC7" << RESET;
    // Initialize SEH tester
    if(cGui)
    {
        gui::message("");
        gui::status("Establishing optical link");
        gui::progress(0 / 10.0);

        gui::data("ResultsDirectory", cTool.getDirectoryName().c_str());
        // gui::data("MonitoringFile", cTool.GetMonitorFileName().c_str());
        LOG(DEBUG) << BOLDBLUE << cTool.getDirectoryName().c_str() << RESET;
        LOG(DEBUG) << BOLDBLUE << cTool.GetMonitorFileName().c_str() << RESET;
    }
    // Choose USB interface by Dev and Bus, actually (only) works because of (evil) global variables in the tcusb
    // ¯\_(ツ)_/¯
    if(cmd.foundOption("USBBus") && cmd.foundOption("USBDev")) { TC_2SSEH cTC_2SSEH(cUsbBus, cUsbDev); }

    SEHTester cSEHTester;
    cSEHTester.Inherit(&cTool);
    cSEHTester.InitialiseTestCard(true);
    if(cGui) { std::this_thread::sleep_for(std::chrono::milliseconds(3000)); }
    cSEHTester.RunHybridETest();
    cTool.fillSummaryTree("setup_type", (cGui) ? 1 : 0);

    if(cmd.foundOption("measure-input-iv"))
    {
        LOG(INFO) << BOLDYELLOW << "Switching on SEH using remote power supply control and perform I-V scan" << RESET;
        cSEHTester.TurnOn(0, 0, false, true);
        if(!cSEHTester.CheckShort(cLVPowerSupplyId, cLVChannelId))
        {
            LOG(INFO) << BOLDBLUE << "Stop test due to possible short" << RESET;
            cTool.fillSummaryTree("has_short", 1);
            cTool.SaveResults();
            cTool.WriteRootFile();
            cTool.CloseResultFile();
            cTool.Destroy();
            abort();
        }
        cTool.fillSummaryTree("has_short", 0);
        cSEHTester.RampPowerSupply(cLVPowerSupplyId, cLVChannelId, cVoltages);
        cSEHTester.TurnOn(cRightLoad, cLeftLoad, true, false);
    }
    else
    {
        LOG(INFO) << BOLDYELLOW << "Switching on SEH without remote power supply control" << RESET;
        cSEHTester.TurnOn(cRightLoad, cLeftLoad, true, true);
    }
    if(cmd.foundOption("test-ext-leak"))
    {
        if(cmd.foundOption("test-leak-parallel"))
        {
            LOG(INFO) << BOLDBLUE << "Measuring leakage current with external power supply in parallel" << RESET;
            cSEHTester.SetupExternalTestLeakageCurrent(cExtLeakVoltage, cHVPowerSupplyId, cHVChannelId);
        }
    }

    // establishes an optical link and configures the lpgbt over the optical cable
    uint8_t  cExternalPattern = (cmd.foundOption("test-external-pattern")) ? convertAnyInt(cmd.optionValue("test-external-pattern").c_str()) : 0;
    BeBoard* pBoard           = static_cast<BeBoard*>(cTool.fDetectorContainer->getFirstObject());
    cTool.fBeBoardInterface->getBoardInfo(pBoard);

    for(int i = 0; i < 8; i++)
    {
        std::cout << "###--------------l8---------------###" << std::endl;
        dynamic_cast<D19cFWInterface*>(cTool.fBeBoardInterface->getFirmwareInterface(pBoard))->GetSFPParameter_L8("T", i);
        dynamic_cast<D19cFWInterface*>(cTool.fBeBoardInterface->getFirmwareInterface(pBoard))->GetSFPParameter_L8("V", i);
        dynamic_cast<D19cFWInterface*>(cTool.fBeBoardInterface->getFirmwareInterface(pBoard))->GetSFPParameter_L8("I", i);
        dynamic_cast<D19cFWInterface*>(cTool.fBeBoardInterface->getFirmwareInterface(pBoard))->GetSFPParameter_L8("TX", i);
        dynamic_cast<D19cFWInterface*>(cTool.fBeBoardInterface->getFirmwareInterface(pBoard))->GetSFPParameter_L8("RX", i);
        std::cout << "###--------------l12--------------###" << std::endl;
        dynamic_cast<D19cFWInterface*>(cTool.fBeBoardInterface->getFirmwareInterface(pBoard))->GetSFPParameter_L12("T", i);
        dynamic_cast<D19cFWInterface*>(cTool.fBeBoardInterface->getFirmwareInterface(pBoard))->GetSFPParameter_L12("V", i);
        dynamic_cast<D19cFWInterface*>(cTool.fBeBoardInterface->getFirmwareInterface(pBoard))->GetSFPParameter_L12("I", i);
        dynamic_cast<D19cFWInterface*>(cTool.fBeBoardInterface->getFirmwareInterface(pBoard))->GetSFPParameter_L12("TX", i);
        dynamic_cast<D19cFWInterface*>(cTool.fBeBoardInterface->getFirmwareInterface(pBoard))->GetSFPParameter_L12("RX", i);
    }
    // Initialize tester
    std::string MonitorFileName = cTool.GetMonitorFileName().c_str();
    std::string DirectoryName   = cSEHTester.getDirectoryName().c_str();
    try
    {
        cTool.ConfigureHw();
        cSEHTester.CheckConfiguredHw();
        cSEHTester.ReadChipIds();
        cSEHTester.LpGBTInjectULExternalPattern(true, cExternalPattern);
        cSEHTester.Initialise();

        // std::this_thread::sleep_for(std::chrono::milliseconds(30000));
        if(cmd.foundOption("test-parameter"))
        {
            cSEHTester.readTestParameters(cTestParameterFileName);
            LOG(INFO) << BOLDYELLOW << "You are using the parameters from " << cTestParameterFileName << " if provided there" << RESET;
        }
        else { LOG(INFO) << BOLDYELLOW << "You are using the default parameter set stored in fDefaultParameters" << RESET; }
        /*******************/
        /*   TEST UPLINK   */
        /* E-links CIC_OUT */
        /*******************/
        if(cmd.foundOption("test-internal-pattern") || cmd.foundOption("test-external-pattern"))
        {
            if(cGui)
            {
                gui::message("");
                gui::status("Testing uplink");
                gui::progress(1 / 10.0);

                // gui::data("ResultsDirectory", cSEHTester.getDirectoryName().c_str());
                // gui::data("MonitoringFile", cTool.GetMonitorFileName().c_str());
                LOG(DEBUG) << BOLDBLUE << cSEHTester.getDirectoryName().c_str() << RESET;
                LOG(DEBUG) << BOLDBLUE << cTool.GetMonitorFileName().c_str() << RESET;
            }
            /* INTERNALLY GENERATED PATTERN */
            if(cmd.foundOption("test-internal-pattern"))
            {
                uint8_t  cInternalPattern8  = (cmd.foundOption("test-internal-pattern")) ? convertAnyInt(cmd.optionValue("test-internal-pattern").c_str()) : 0;
                uint32_t cInternalPattern32 = cInternalPattern8 << 24 | cInternalPattern8 << 16 | cInternalPattern8 << 8 | cInternalPattern8 << 0;
                cSEHTester.LpGBTInjectULInternalPattern(cInternalPattern32);
                cSEHTester.LpGBTCheckULPattern(false, cInternalPattern8);
            }
            /* EXTERNALLY GENERATED PATTERN */
            else if(cmd.foundOption("test-external-pattern"))
            {
                bool cStatus = true;
                // while(true){
                cStatus = cSEHTester.LpGBTCheckULPattern(true, cExternalPattern);
                // cSEHTester.LpGBTCheckULPatternv2(true, cExternalPattern);
                // std::this_thread::sleep_for(std::chrono::milliseconds(1500));
                //}
#ifdef __USE_ROOT__
                cTool.fillSummaryTree("status_CicOutTest", (cStatus) ? 1 : 0);
#endif
                if(cStatus) { LOG(INFO) << BOLDGREEN << "CIC_Out test passed." << RESET; }
                else { LOG(INFO) << BOLDRED << "CIC_Out test failed." << RESET; }
            }
        }
        /****************************/
        /* TEST RESET LINES (GPIOs) */
        /*     And test GPIs        */
        /****************************/
        if(cmd.foundOption("test-reset"))
        {
            if(cGui)
            {
                gui::message("CIC OUT test finished");
                gui::status("Testing reset lines");
                gui::progress(2 / 10.0);
            }
            bool cStatus = cSEHTester.LpGBTTestResetLines();
#ifdef __USE_ROOT__
            cTool.fillSummaryTree("status_ResetTest", (cStatus) ? 1 : 0);
#endif
            if(cStatus) { LOG(INFO) << BOLDGREEN << "Reset test passed." << RESET; }
            else { LOG(INFO) << BOLDRED << "Reset test failed." << RESET; }
            cStatus = cSEHTester.LpGBTTestGPILines();
#ifdef __USE_ROOT__
            cTool.fillSummaryTree("status_PowerGoodTest", (cStatus) ? 1 : 0);
#endif
            if(cStatus) { LOG(INFO) << BOLDGREEN << "Power Good test passed." << RESET; }
            else { LOG(INFO) << BOLDRED << "Power Good test failed." << RESET; }
        }

        /****************************/
        /*  Test VTRx+ slow control */
        /****************************/
        if(cmd.foundOption("test-vtrx"))
        {
            if(cGui)
            {
                gui::message("Reset lines test finished");
                gui::status("Testing VTRX+ slow control lines");
                gui::progress(3 / 10.0);
            }
            bool cStatus = cSEHTester.LpGBTTestVTRx();
#ifdef __USE_ROOT__
            cTool.fillSummaryTree("status_vtrxplusslowcontrol", (cStatus) ? 1 : 0);
#endif
            if(cStatus)
                LOG(INFO) << BOLDGREEN << "VTRx+ slow control test passed." << RESET;
            else
                LOG(INFO) << BOLDRED << "VTRx+ slow control test failed." << RESET;
        }

        /****************************/
        /*  Test LpGBT I2C Masters  */
        /****************************/
        if(cmd.foundOption("test-i2c"))
        {
            if(cGui)
            {
                gui::message("VTRX+ test finished");
                gui::status("Testing I2C Masters on the lpGBT");
                gui::progress(4 / 10.0);
            }
            int                  pNTries  = convertAnyInt(cmd.optionValue("test-i2c").c_str());
            std::vector<uint8_t> cMasters = {0, 2};
            bool                 cStatus  = cSEHTester.LpGBTTestI2CMaster(cMasters, pNTries);
#ifdef __USE_ROOT__
            cTool.fillSummaryTree("status_i2cmasters", (cStatus) ? 1 : 0);
#endif
            if(cStatus)
                LOG(INFO) << BOLDBLUE << "I2C test " << BOLDGREEN << " passed" << RESET;
            else { LOG(INFO) << BOLDBLUE << "I2C test " << BOLDRED << " failed" << RESET; }
        }

        /**********************************/
        /* TEST ANALOG-DIGITAL-CONVERTERS */
        /**********************************/
        if(cmd.foundOption("test-adc"))
        {
            if(cGui)
            {
                gui::message("I2C Masters test finished");
                gui::status("Testing ADC lines on the lpGBT");
                gui::progress(5 / 10.0);
            }
            bool                     cCalibrate = cmd.foundOption("calibrate-adc");
            std::vector<std::string> cADCs      = {"ADC0", "ADC3"};
            cSEHTester.LpGBTTestADC(cADCs, 0, 3720, 600, cCalibrate); // 12 bit DAC with 1V reference, ROH is 12 bit something, needs to be included somewhere
            cSEHTester.LpGBTTestFixedADCs();
        }

        /********************/
        /* TEST EYE OPENING */
        /********************/
        if(cmd.foundOption("test-eom"))
        {
            // if(cGui)
            // {
            //     gui::message("ADC test finished");
            //     gui::status("Measuring eye opening");
            //     gui::progress(7.5 / 10.0);
            // }
            uint8_t cEQAttenuation = cmd.foundOption("eq-attenuation") ? convertAnyInt(cmd.optionValue("eq-attenuation").c_str()) : 3;
            cSEHTester.LpGBTRunEyeOpeningMonitor(7, cEQAttenuation);
        }

        /***********************/
        /* TEST BIT ERROR RATE */
        /***********************/
        if(cmd.foundOption("test-ber"))
        {
            uint32_t cBERTPattern32 = cmd.foundOption("ber-pattern") ? convertAnyInt(cmd.optionValue("ber-pattern").c_str()) : 0x00000000;
            // FIXME still hard coded
            uint8_t cCoarseSource = 1, cFineSource = 4, cMeasTime = 5;
            cSEHTester.LpGBTRunBitErrorRateTest(cCoarseSource, cFineSource, cMeasTime, cBERTPattern32);
        }

        /***************/
        /* TEST CLOCKS */
        /***************/
        if(cmd.foundOption("test-clock"))
        {
            if(cGui)
            {
                gui::message("Eye opening monitoring finished");
                gui::status("Testing clock lines");
                gui::progress(6 / 10.0);
            }
            LOG(INFO) << BOLDBLUE << "Clock test" << RESET;
            bool cStatus = cSEHTester.LpGBTCheckClocks();
            if(cStatus)
                LOG(INFO) << BOLDBLUE << "Clock test " << BOLDGREEN << " passed" << RESET;
            else { LOG(INFO) << BOLDBLUE << "Clock test " << BOLDRED << " failed" << RESET; }
#ifdef __USE_ROOT__
            cTool.fillSummaryTree("status_clocktest", (cStatus) ? 1 : 0);
#endif
        }

        int cFmcdCounter = 0;
        int cFcmdTries   = 1;
        /*********************/
        /* TEST FAST COMMAND */
        /*********************/
        if(cmd.foundOption("test-fcmd"))
        {
            if(cGui)
            {
                gui::message("Clock test finished");
                gui::status("Testing FCMD lines");
                gui::progress(7 / 10.0);
            }
            if(cmd.foundOption("fcmd-pattern"))
            {
                LOG(INFO) << BOLDBLUE << "FCMD pattern test" << RESET;
                // Align lines in the back-end with selected pattern
                cSEHTester.LpGBTInjectDLInternalPattern(cFCMDPattern);
                // Now this has switched to idle frames!

                for(int i = 0; i < cFcmdTries; i++)
                {
                    if(!cSEHTester.LpGBTFastCommandChecker(7)) cFmcdCounter += 1;
                }
                LOG(INFO) << BOLDRED << "FCMD pattern test failed " << +cFmcdCounter << " times" << RESET;
#ifdef __USE_ROOT__
                cTool.fillSummaryTree("fcmd_tries", cFcmdTries);
                cTool.fillSummaryTree("fcmd_failures", cFmcdCounter);
#endif
            }
            else
            {
                for(int i = 0; i < cFcmdTries; i++)
                {
                    if(!cSEHTester.LpGBTFastCommandChecker(7)) cFmcdCounter += 1;
                }
                LOG(INFO) << BOLDRED << "FCMD pattern test failed " << +cFmcdCounter << " times" << RESET;
            }
        }
        /*********************/
        /* TEST Efficiency   */
        /*********************/
        if(cmd.foundOption("test-efficiency"))
        {
            if(cGui)
            {
                gui::message("FCMD test finished");
                gui::status("Testing efficiency");
                gui::progress(8 / 10.0);
            }
            LOG(INFO) << BOLDBLUE << "Efficiency Test" << RESET;
            cSEHTester.TestEfficiency(0, 2502, 417);
        }

        cTool.StopMonitoring();

        if(cmd.foundOption("test-ext-leak") & cmd.foundOption("test-leak-parallel"))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(3000));
            LOG(INFO) << BOLDBLUE << "Ending leakage current with external power supply in parallel" << RESET;
            cSEHTester.EndExternalTestLeakageCurrent(cHVPowerSupplyId, cHVChannelId);
        }
        // Legacy HV leakage test
        if(cmd.foundOption("test-leak"))
        {
            LOG(INFO) << BOLDBLUE << "Measuring leakage current" << RESET;
            cSEHTester.TestLeakageCurrent(cLeakVoltage, 150);
        }
        if(cmd.foundOption("test-ext-leak") & !cmd.foundOption("test-leak-parallel"))
        {
            LOG(INFO) << BOLDBLUE << "Measuring leakage current with external power supply" << RESET;
            cSEHTester.ExternalTestLeakageCurrent(cExtLeakVoltage, 150, cHVPowerSupplyId, cHVChannelId);
        }

        /*********************/
        /* TEST Bias Voltage */
        /*********************/

        if(cmd.foundOption("test-bias"))
        {
            LOG(INFO) << BOLDBLUE << "Measuring bias voltage on sensor side" << RESET;
            cSEHTester.TestBiasVoltage();
        }

        if(cmd.foundOption("test-ext-bias"))
        {
            if(cGui)
            {
                gui::message("Efficiency test finished");
                gui::status("Testing bias voltage");
                gui::progress(9 / 10.0);
            }
            LOG(INFO) << BOLDBLUE << "Measuring bias voltage on sensor side with external power supply" << RESET;
            cSEHTester.ExternalTestBiasVoltage(cHVPowerSupplyId, cHVChannelId);
        }

        if(cFCMDTest && !cFCMDTestStartPattern.empty() && !cFCMDTestUserFileName.empty())
        {
            LOG(INFO) << BOLDBLUE << "Fast command test" << RESET;
            cSEHTester.CheckFastCommands(cFCMDTestStartPattern, cFCMDTestUserFileName);
        }

        if(cmd.foundOption("bramfcmd-check") && !cBRAMFCMDLine.empty())
        {
            LOG(INFO) << BOLDBLUE << "Access to written data in BRAM" << RESET;
            cSEHTester.CheckFastCommandsBRAM(cBRAMFCMDLine);
        }

        if(cmd.foundOption("bramreffcmd-write") && !cBRAMFCMDFileName.empty())
        {
            LOG(INFO) << BOLDBLUE << "Write reference patterns to BRAM" << RESET;
            cSEHTester.WritePatternToBRAM(cBRAMFCMDFileName);
        }

        if(cmd.foundOption("convert-userfile") && !cConvertUserFileName.empty())
        {
            LOG(INFO) << BOLDBLUE << "Convert user file to fw compliant format" << RESET;
            cSEHTester.UserFCMDTranslate(cConvertUserFileName);
        }

        if(cmd.foundOption("read-ref-bram"))
        {
            int cAddr = std::atoi(cRefBRAMAddr.c_str());
            LOG(INFO) << BOLDBLUE << "Read single ref FCMD BRAM address: " << cmd.optionValue("read-ref-bram") << RESET;
            cSEHTester.ReadRefAddrBRAM(cAddr);
        }

        if(cmd.foundOption("read-check-bram"))
        {
            int cAddr = std::atoi(cCheckBRAMAddr.c_str());
            LOG(INFO) << BOLDBLUE << "Read single check FCMD BRAM address: " << cmd.optionValue("read-check-bram") << RESET;
            cSEHTester.ReadCheckAddrBRAM(cAddr);
        }

        if(cmd.foundOption("clear-ref-bram"))
        {
            LOG(INFO) << BOLDBLUE << "Flushing ref BRAM!" << RESET;
            cSEHTester.ClearBRAM(std::string("ref"));
        }

        if(cmd.foundOption("clear-check-bram"))
        {
            LOG(INFO) << BOLDBLUE << "Flushing check BRAM!" << RESET;
            cSEHTester.ClearBRAM(std::string("test"));
        }
        // cSEHTester.calibrateADC();
        cSEHTester.SetLoad(0, 0);
        cSEHTester.LpGBTInjectULExternalPattern(false, 170);

        if(cGui)
        {
            gui::message("Bias voltage test finished");
            gui::status("Test finished");
            gui::data("ResultsDirectory", cTool.getDirectoryName().c_str());
            gui::progress(10.0 / 10.0);
        }
    }
    catch(std::exception const& e)
    {
        std::stringstream cExitMessage;
        cExitMessage << "Exception Message : " << e.what();
        cTool.fillSummaryTree(cExitMessage.str(), 1);
        if(cGui) gui::data("Exception", e.what());
        LOG(ERROR) << BOLDRED << "Exception: " << cExitMessage.str() << RESET;
        LOG(INFO) << BOLDYELLOW << "Will save test results obtained so far and.. stop test procedure" << RESET;
        cTool.StopMonitoring();
        if(cmd.foundOption("test-ext-leak") & cmd.foundOption("test-leak-parallel"))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(3000));
            LOG(INFO) << BOLDBLUE << "Stop leakage current with external power supply in parallel due to error" << RESET;
            cSEHTester.EndExternalTestLeakageCurrent(cHVPowerSupplyId, cHVChannelId);
        }
    }
    if(cGui)
    {
        gui::message("Done");
        gui::status("Test complete");
        gui::progress(10.0 / 10.0);
    }

    // Save Result File
    cTool.SaveResults();
    cTool.WriteRootFile();
    cTool.CloseResultFile();
    // Destroy Tools
    cTool.Destroy();

    if(!MonitorFileName.empty())
    {
        LOG(INFO) << GREEN << "Attempting to copy monitoring file : " << BOLDYELLOW << MonitorFileName << RESET;

        std::string cCommand = "cp " + MonitorFileName + " " + DirectoryName + "/Monitoring.root";
        try
        {
            system(cCommand.c_str());
        }
        catch(std::exception& e)
        {
            LOG(ERROR) << BOLDRED << "Exceptin when trying to move Monitoring File to Directory: " << DirectoryName << RESET;
        }
    }

    runCompleted = 1;

    if(!batchMode) cApp.Run();
#endif
    return 0;
}
