#include "HWInterface/D19cDebugFWInterface.h"
#include "Utils/StartInfo.h"
#include "Utils/Timer.h"
#include "Utils/Utilities.h"
#include "Utils/argvparser.h"
#include "boost/format.hpp"
#include "tools/BeamTestCheck.h"
#include "tools/CBCPulseShape.h"
#include "tools/CheckCbcNeighbors.h"
#include "tools/CicFEAlignment.h"
#include "tools/ECVLinkAlignmentOT.h"
#include "tools/KIRA.h"
#include "tools/LatencyScan.h"
#include "tools/LinkAlignmentOT.h"
#include "tools/OTCMNoise.h"
#include "tools/OTLightTransmission.h"
#include "tools/OTQuickNoise.h"
#include "tools/OTSensorTemperature.h"
#include "tools/OTTemperature.h"
#include "tools/OTVTRXLightOff.h"
#include "tools/OTlpGBTID.h"
#include "tools/PSAlignment.h"
#include "tools/PSPixelAlive.h"
#include "tools/PedeNoise.h"
#include "tools/PedestalEqualization.h"
#include "tools/PhaseScan.h"
#include "tools/RegisterTester.h"
#include "tools/StubBackEndAlignment.h"
#include <cstring>

#ifdef __POWERSUPPLY__
// Libraries
#include "DeviceHandler.h"
#include "PowerSupply.h"
#include "PowerSupplyChannel.h"
#endif

#ifdef __USE_ROOT__
#include "TApplication.h"
#include "TROOT.h"
#endif

#define __NAMEDPIPE__

#ifdef __NAMEDPIPE__
#include "Utils/gui_logger.h"
#endif

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;
using namespace CommandLineProcessing;
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
        theTool->Destroy();
        abort();
    }
}

uint16_t returnRunNumber(std::string cFileName)
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
    return (uint16_t)(cRunNumber + 1);
}

std::vector<uint8_t> getArgs(std::string pArgsStr)
{
    std::vector<uint8_t> cSides;
    std::stringstream    cArgsSS(pArgsStr);
    int                  i;
    while(cArgsSS >> i)
    {
        cSides.push_back(i);
        if(cArgsSS.peek() == ',') cArgsSS.ignore();
    };
    return cSides;
}

int main(int argc, char* argv[])
{
    // configure the logger
    el::Configurations conf(std::string(std::getenv("PH2ACF_BASE_DIR")) + "/settings/logger.conf");
    el::Loggers::reconfigureAllLoggers(conf);

    ArgvParser cmd;

    // init
    cmd.setIntroductoryDescription("CMS Ph2_ACF  Commissioning tool to perform the following procedures:\n-Timing / "
                                   "Latency scan\n-Threshold Scan\n-Stub Latency Scan");
    // error codes
    cmd.addErrorCode(0, "Success");
    cmd.addErrorCode(1, "Error");
    // options
    cmd.setHelpOption("h", "help", "Print this help page");

    cmd.defineOption("file", "Hw Description File . Default value: settings/Commission_2CBC.xml", ArgvParser::OptionRequiresValue /*| ArgvParser::OptionRequired*/);
    cmd.defineOptionAlternative("file", "f");

    cmd.defineOption("tuneOffsets", "tune offsets on readout chips connected to CIC.");
    cmd.defineOptionAlternative("tuneOffsets", "t"); // trimming
    cmd.defineOption("linkTest", "Check data coming over link....", ArgvParser::OptionRequiresValue);
    cmd.defineOption("measurePedeNoise", "measure pedestal and noise on readout chips connected to CIC."); // Scurve
    cmd.defineOptionAlternative("measurePedeNoise", "m");

    cmd.defineOption("pixelAlive", "measure pixel occupancy and mask pixels below threshold");
    cmd.defineOptionAlternative("pixelAlive", "p");

    cmd.defineOption("cmNoise", "measure common mode noise");

    cmd.defineOption("read", "Read data from a raw file.  ", ArgvParser::OptionRequiresValue);
    cmd.defineOption("save", "Save the data to a raw file.  ", ArgvParser::NoOptionAttribute);
    cmd.defineOption("skipAlignment", "Skip the back-end alignment step ", ArgvParser::OptionRequiresValue);

    cmd.defineOption("batch", "Run the application in batch mode", ArgvParser::NoOptionAttribute);
    cmd.defineOptionAlternative("batch", "b");

    cmd.defineOption("allChan", "Do pedestal and noise measurement using all channels? Default: false", ArgvParser::NoOptionAttribute);
    cmd.defineOptionAlternative("allChan", "a");

    cmd.defineOption("reconfigure", "Reconfigure Hardware and align");
    cmd.defineOption("configure", "Configure Hardware");
    cmd.defineOption("reload", "Reload settings files and board registers");
    cmd.defineOption("realign", "Re-align module [SSA-MPA] and/or [BE]");
    cmd.defineOption("phaseScan", "Phase Scan");

    cmd.defineOption("ecv", "Perform Electronic Chain Validation Scans", ArgvParser::NoOptionAttribute);

    cmd.defineOption("moduleId", "Serial Number of module . Default value: xxxx", ArgvParser::OptionRequiresValue /*| ArgvParser::OptionRequired*/);
    cmd.defineOption("alignPS", "Perform SSA-MPA alignment steps", ArgvParser::NoOptionAttribute);
    cmd.defineOption("checkClusters", "Check CIC2 sparsification... ", ArgvParser::NoOptionAttribute);
    cmd.defineOption("psDataTest", "....", ArgvParser::NoOptionAttribute);
    cmd.defineOption("checkSLink", "Check S-link ... data saved to file ", ArgvParser::OptionRequiresValue);
    cmd.defineOption("checkStubs", "Check Stubs... ", ArgvParser::NoOptionAttribute);
    cmd.defineOption("checkReadData", "Check ReadData method... ", ArgvParser::NoOptionAttribute);
    cmd.defineOption("checkAsync", "Check Async readout methods [PS objects only]... ", ArgvParser::NoOptionAttribute);
    cmd.defineOption("checkReadNEvents", "Check ReadNEvents method... ", ArgvParser::NoOptionAttribute);
    cmd.defineOption("noiseInjection", "Check noise injection...", ArgvParser::NoOptionAttribute);
    cmd.defineOption("calibrateADC", "Calibrate ADC on lpGBT....", ArgvParser::NoOptionAttribute);
    cmd.defineOption("readIDs", "Read chip ids....", ArgvParser::NoOptionAttribute);
    cmd.defineOption("completeDataCheck", "Complete data check for the following CBCs", ArgvParser::OptionRequiresValue);

    cmd.defineOption("pageToTest", "Page to test", ArgvParser::OptionRequiresValue);
    cmd.defineOption("registerTestWrite", "Test I2C registers on Chips", ArgvParser::OptionRequiresValue);
    cmd.defineOption("registerTestWriteAndToggle", "Test I2C registers on Chips", ArgvParser::OptionRequiresValue);
    cmd.defineOption("registerTestRead", "Test I2C registers on Chips", ArgvParser::OptionRequiresValue);
    cmd.defineOption("registerTestReadAndToggle", "Test I2C registers on Chips", ArgvParser::OptionRequiresValue);
    cmd.defineOption("sortOrder", "Sort order for CBC registers  : 0 - no sort other than page; 1 - page then increasing addresss; 2 - page then decreasing addresss", ArgvParser::OptionRequiresValue);
    cmd.defineOption("bitToFlip", "Bit to flip when testing register write", ArgvParser::OptionRequiresValue);
    cmd.defineOption("testAttempts", "Number of attempts", ArgvParser::OptionRequiresValue);
    cmd.defineOption("returnToDefPage", "Return to Def Page", ArgvParser::OptionRequiresValue);
    cmd.defineOption("manualScan", "Manual scan of threshold", ArgvParser::NoOptionAttribute);
    cmd.defineOption("injectionTest", "Manual scan of threshold", ArgvParser::OptionRequiresValue);
    //
    cmd.defineOption("DataMonitor", "Data monitor", ArgvParser::OptionRequiresValue);
    cmd.defineOption("TestPulseCheck", "Test pulse check - inject with TP and perform latency scan", ArgvParser::NoOptionAttribute);
    cmd.defineOption("ExternalCheck", "External trigger check - run with external triggers", ArgvParser::NoOptionAttribute);
    cmd.defineOption("TLUCheck", "External trigger check - run with external triggers", ArgvParser::NoOptionAttribute);
    cmd.defineOption("InternalCheck", "External trigger check - run with external triggers", ArgvParser::NoOptionAttribute);
    cmd.defineOption("continuousReadout", "Readout triggers as they come : argument to provide is how often to poll the readout [in us]", ArgvParser::OptionRequiresValue);
    cmd.defineOption("scanLatencies", "Scan L1+Stub Latencies ", ArgvParser::NoOptionAttribute);
    cmd.defineOption("scanL1", "Scan L1 Latency ", ArgvParser::NoOptionAttribute);
    cmd.defineOption("scanStubs", "Scan Stub Latency ", ArgvParser::NoOptionAttribute);
    cmd.defineOption("limitTriggers", "Only accept exactly the correct number of triggers", ArgvParser::NoOptionAttribute);
    cmd.defineOption("checkCICAlignment", "Manually scan CIC input aligner", ArgvParser::NoOptionAttribute);
    cmd.defineOption("checkLink", "Check that I can receive constant pattern from link", ArgvParser::OptionRequiresValue);
    cmd.defineOption("kira", "Perform KIRA scan", ArgvParser::NoOptionAttribute);
    cmd.defineOption("kiraport", "Specify port of power supply package", ArgvParser::OptionRequiresValue);
    cmd.defineOption("kiraid", "Specify id of arduino in power supply package", ArgvParser::OptionRequiresValue);
    cmd.defineOption("kiracalibration", "Perform KIRA calibration", ArgvParser::NoOptionAttribute);
    //
    cmd.defineOption("readTemperatures", "Read temperature sensors available on module [lpGBT internal; sensor thermistory]", ArgvParser::OptionRequiresValue);
    cmd.defineOption("readMonitors", "Read internal monitors on lpGBT [lpGBT internal; sensor thermistory]", ArgvParser::OptionRequiresValue);
    cmd.defineOption("pulseShape", "Scan the threshold and fit for signal Vcth", ArgvParser::NoOptionAttribute);
    cmd.defineOption("checkSharedStubs", "Check stubs at boundary between chips", ArgvParser::NoOptionAttribute);
    cmd.defineOption("vtrxLightOff", "Turnoff the light output of the VTRX+ to perform an IV curve while the LV is still powered", ArgvParser::NoOptionAttribute);
    //
    cmd.defineOption("writeJson", "Write numeric results and/or progress in json format to given output file", ArgvParser::OptionRequiresValue);
    cmd.defineOption("readlpGBTIDs", "Read IDs of lpGBTs", ArgvParser::NoOptionAttribute);
    cmd.defineOption("readSensorTemperature", "Read sensor temperature", ArgvParser::NoOptionAttribute);
    cmd.defineOption("measureQuickNoise", "measure occupancy of all channels");
    cmd.defineOption("measureChannelTransmission", "measure data about transmission for given acquisition card channel", ArgvParser::OptionRequiresValue);
    cmd.defineOption("latency", "scan the trigger latency", ArgvParser::NoOptionAttribute);
    cmd.defineOption("stublatency", "scan the stub latency", ArgvParser::NoOptionAttribute);

    int result = cmd.parse(argc, argv);

    if(result != ArgvParser::NoParserError)
    {
        LOG(INFO) << cmd.parseErrorDescription(result);
        exit(1);
    }

    // now query the parsing results
    std::string cHWFile             = (cmd.foundOption("file")) ? cmd.optionValue("file") : "settings/Commissioning.xml";
    bool        batchMode           = (cmd.foundOption("batch")) ? true : false;
    bool        cSaveToFile         = cmd.foundOption("save");
    std::string cSkip               = (cmd.foundOption("skipAlignment")) ? cmd.optionValue("skipAlignment") : "";
    std::string cInjectionSource    = (cmd.foundOption("injectionTest")) ? cmd.optionValue("injectionTest") : "digital";
    std::string cSrcLnkTst          = (cmd.foundOption("linkTest")) ? cmd.optionValue("linkTest") : "lpGBT";
    std::string cModuleId           = (cmd.foundOption("moduleId")) ? cmd.optionValue("moduleId") : "ModuleOT";
    int         cKiraPort           = std::stoi((cmd.foundOption("kiraport")) ? cmd.optionValue("kiraport") : "7010");
    std::string cKiraID             = (cmd.foundOption("kiraid")) ? cmd.optionValue("kiraid") : "myArduino";
    bool        cKiraCalibration    = cmd.foundOption("kiracalibration");
    std::string cDirectory          = (cmd.foundOption("output")) ? cmd.optionValue("output") : "Results/";
    bool        cPulseShape         = (cmd.foundOption("pulseShape")) ? true : false;
    int         cTansmissionChannel = (cmd.foundOption("measureChannelTransmission")) ? convertAnyInt(cmd.optionValue("measureChannelTransmission").c_str()) : -1;
    bool        cLatency            = (cmd.foundOption("latency")) ? true : false;
    bool        cStubLatency        = (cmd.foundOption("stublatency")) ? true : false;
    bool        cEcv                = (cmd.foundOption("ecv")) ? true : false;

    uint16_t cRunNumber = 666;
    if(!cmd.foundOption("read"))
    {
        std::ofstream cRunLog;
        cRunNumber = returnRunNumber("RunNumbers.dat");
        cRunLog.open("RunNumbers.dat", std::fstream::app);
        cRunLog << cRunNumber << "\n";
        cRunLog.close();
        LOG(INFO) << BOLDBLUE << "Run number is " << +cRunNumber << RESET;
        cDirectory += Form("OT_ModuleTest_%s_Run%d", cModuleId.c_str(), cRunNumber);
    }
    else
    {
        std::string cRawFileName = cmd.foundOption("read") ? cmd.optionValue("read") : "";
        cDirectory += Form("Raw_%s", cRawFileName.substr(0, cRawFileName.find(".raw")).c_str());
    }
    TApplication cApp("Root Application", &argc, argv);

    if(batchMode)
        gROOT->SetBatch(true);
    else
        TQObject::Connect("TCanvas", "Closed()", "TApplication", &cApp, "Terminate()");

    std::string cResultfile = "Hybrid";
    Timer       t;
    Timer       cGlobalTimer;
    cGlobalTimer.start();

    std::stringstream outp;
    Tool              cTool;

    std::thread softKillThread(killProcessFunction, &cTool);
    softKillThread.detach();

    struct sigaction act;
    act.sa_handler = interruptHandler;
    sigaction(SIGINT, &act, NULL);

    if(cSaveToFile)
    {
        char cRawFileName[80];
        std::snprintf(cRawFileName, sizeof(cRawFileName), "Run%.05d.raw", cRunNumber);
        std::string cRawFile = cRawFileName;
        cTool.addFileHandler(cRawFile, 'w');
        LOG(INFO) << BOLDBLUE << "Writing Binary Rawdata to:   " << cRawFile;
    }
    cTool.InitializeHw(cHWFile, outp);
    cTool.InitializeSettings(cHWFile, outp);
    LOG(INFO) << outp.str();
    cTool.CreateResultDirectory(cDirectory, false, false);
    cTool.InitResultFile(cResultfile);
    cTool.initializeExceptionHandler();
    BeBoard* pBoard = static_cast<BeBoard*>(cTool.fDetectorContainer->getFirstObject());
    cTool.fBeBoardInterface->getBoardInfo(pBoard);

    if(cmd.foundOption("vtrxLightOff"))
    {
        cTool.ConfigureHw();
        LOG(INFO) << BOLDBLUE << "Turn off light output of VTRX..." << RESET;
        OTVTRXLightOff cLightOff;
        cLightOff.Inherit(&cTool);
        StartInfo theStartInfo;
        theStartInfo.setRunNumber(cRunNumber);
        cLightOff.Start(theStartInfo);
        cLightOff.waitForRunToBeCompleted();
    }

    if(cmd.foundOption("writeJson"))
    {
        std::ofstream* outStream = new std::ofstream(cmd.optionValue("writeJson"));
        cTool.setOfStream(outStream);
    }

    if(cmd.foundOption("readTemperatures"))
    {
        LOG(INFO) << BOLDBLUE << "Reading internal monitors from lpGBT-ADCs.." << RESET;
        auto          cGain = (cmd.foundOption("readMonitors")) ? convertAnyInt(cmd.optionValue("readMonitors").c_str()) : 0;
        OTTemperature cTemperatureReader;
        cTemperatureReader.Inherit(&cTool);
        cTemperatureReader.SetGain(cGain);
        cTemperatureReader.LoopReadout(true);
        StartInfo theStartInfo;
        theStartInfo.setRunNumber(cRunNumber);
        cTemperatureReader.Start(theStartInfo);
        cTemperatureReader.waitForRunToBeCompleted();
    }

    if(cmd.foundOption("readSensorTemperature"))
    {
        LOG(INFO) << BOLDBLUE << "Reading sensor temperature" << RESET;
        OTSensorTemperature cSensorTemperature;
        cSensorTemperature.Inherit(&cTool);
        StartInfo theStartInfo;
        theStartInfo.setRunNumber(cRunNumber);
        cSensorTemperature.Start(theStartInfo);
        cSensorTemperature.waitForRunToBeCompleted();
    }

    if(cmd.foundOption("readlpGBTIDs"))
    {
        LOG(INFO) << BOLDBLUE << "Reading lpGBT IDs" << RESET;
        OTlpGBTID clpGBTDIReader;
        clpGBTDIReader.Inherit(&cTool);
        StartInfo theStartInfo;
        theStartInfo.setRunNumber(cRunNumber);
        clpGBTDIReader.Start(theStartInfo);
        clpGBTDIReader.waitForRunToBeCompleted();
    }

    if(cmd.foundOption("measureQuickNoise"))
    {
        LOG(INFO) << BOLDBLUE << "Computing occupancy over 10000 triggers" << RESET;
        OTQuickNoise cQuickNoiseReader;
        cQuickNoiseReader.Inherit(&cTool);
        StartInfo theStartInfo;
        theStartInfo.setRunNumber(cRunNumber);
        cQuickNoiseReader.Start(theStartInfo);
        cQuickNoiseReader.waitForRunToBeCompleted();
    }

    if(cTansmissionChannel > 0)
    {
        LOG(INFO) << BOLDBLUE << "Getting transciever data" << RESET;
        OTLightTransmission cLightTransmissionReader(cTansmissionChannel);
        cLightTransmissionReader.Inherit(&cTool);
        StartInfo theStartInfo;
        theStartInfo.setRunNumber(cRunNumber);
        cLightTransmissionReader.Start(theStartInfo);
        cLightTransmissionReader.waitForRunToBeCompleted();
    }

    if(cmd.foundOption("calibrateADC"))
    {
        LOG(INFO) << BOLDBLUE << "Calibrating ADC.." << RESET;
        for(const auto cBoard: *cTool.fDetectorContainer)
        {
            for(auto cOpticalGroup: *cBoard)
            {
                auto& clpGBT = cOpticalGroup->flpGBT;
                if(clpGBT == nullptr) continue;

                // use Vin as the reference
                // this I know does not change with anything
                std::vector<std::string> cADCs_VoltageMonitors{"ADC2"};
                std::vector<float>       cADCs_Refs{10.4 * 0.49 / 10.0};
                size_t                   cIndx = cADCs_VoltageMonitors.size() - 1;
                // static_cast<D19clpGBTInterface*>(cTool.flpGBTInterface)->WriteChipReg(clpGBT,"ADCMon", (1 << 4 ) );
                // find correction
                std::vector<float>   cVals(10, 0);
                uint8_t              cEnableVref = 1;
                std::string          cADCsel     = cADCs_VoltageMonitors[cIndx];
                std::vector<uint8_t> cRefPoints{0, 0x05, 0x10, 0x20, 0x3F};
                std::vector<float>   cMeasurements(0);
                std::vector<float>   cSlopes(0);
                for(auto cRef: cRefPoints)
                {
                    static_cast<D19clpGBTInterface*>(cTool.flpGBTInterface)->ConfigureVref(clpGBT, cEnableVref, cRef);
                    // wait until Vref is stable
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    for(size_t cM = 0; cM < cVals.size(); cM++) { cVals[cM] = static_cast<D19clpGBTInterface*>(cTool.flpGBTInterface)->ReadADC(clpGBT, cADCsel) * CONVERSION_FACTOR; }
                    float cMean         = std::accumulate(cVals.begin(), cVals.end(), 0.) / cVals.size();
                    float cDifference_V = (cADCs_Refs[cIndx] - cMean);
                    // LOG (DEBUG) << BOLDBLUE << "ADC_" << cADCsel << " reading from lpGBT "
                    //         << " correction applied is " << +cRef
                    //         << " reading [mean] is "
                    //         << +cMean*1e3
                    //         << " milli-volts."
                    //         << "\t...Difference between expected and measured "
                    //         << " values is "
                    //         << cDifference_V*1e3
                    //         << " milli-volts." << RESET;
                    cMeasurements.push_back(cDifference_V);
                    if(cMeasurements.size() > 1)
                    {
                        for(int cI = cMeasurements.size() - 2; cI >= 0; cI--)
                        {
                            float cSlope = (cMeasurements[cMeasurements.size() - 1] - cMeasurements[cI]) / (cRefPoints[cMeasurements.size() - 1] - cRefPoints[cI]);
                            LOG(DEBUG) << BOLDBLUE << "Index " << +(cMeasurements.size() - 1) << " -- index " << cI << " slope is " << cSlope << RESET;
                            cSlopes.push_back(cSlope);
                        }
                    }
                }
                float cMeanSlope = std::accumulate(cSlopes.begin(), cSlopes.end(), 0.) / cSlopes.size();
                float cIntcpt    = cMeasurements[0];
                int   cCorr      = std::min(std::floor(-1.0 * cIntcpt / cMeanSlope), 63.);
                // LOG (INFO) << BOLDMAGENTA << "Mean slope is " << cMeanSlope
                //     << " , intercept is " << cIntcpt
                //     << " correction is " << cCorr
                //     << RESET;
                // apply correction and check
                static_cast<D19clpGBTInterface*>(cTool.flpGBTInterface)->ConfigureVref(clpGBT, cEnableVref, (uint8_t)cCorr);
                // wait until Vref is stable
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                for(size_t cM = 0; cM < cVals.size(); cM++) { cVals[cM] = static_cast<D19clpGBTInterface*>(cTool.flpGBTInterface)->ReadADC(clpGBT, cADCsel) * CONVERSION_FACTOR; }
                float cMeanValue = std::accumulate(cVals.begin(), cVals.end(), 0.) / cVals.size();
                LOG(INFO) << BOLDMAGENTA << "Measured V_min after correction is " << std::setprecision(2) << std::fixed << cMeanValue * 1e3 << " mV , expected value is " << cADCs_Refs[cIndx] * 1e3
                          << " difference is " << std::fabs(cMeanValue - cADCs_Refs[cIndx]) * 1e3 << " mV, correction needed to acheive this was  " << +cCorr << RESET;

                // turn off ADC mon
                // static_cast<D19clpGBTInterface*>(cTool.flpGBTInterface)->WriteChipReg(clpGBT,"ADCMon", 0x00 );
            } // configure lpGBT
        }
    }

    // read chip ids
    if(cmd.foundOption("readIDs"))
    {
        for(const auto cBoard: *cTool.fDetectorContainer)
        {
            for(auto cOpticalGroup: *cBoard)
            {
                for(auto cHybrid: *cOpticalGroup)
                {
                    for(auto cChip: *cHybrid)
                    {
                        auto cFusedId = cTool.fReadoutChipInterface->ReadChipFuseID(cChip);
                        LOG(INFO) << BOLDMAGENTA << "Hybrid#" << +cChip->getId() << " CBC#" << +cChip->getId() << " Fused Id is " << +cFusedId << RESET;
                    }
                }
            }
        }
    }

    if(cmd.foundOption("registerTestWrite"))
    {
        uint8_t cNregistersToCheck = (cmd.foundOption("registerTestWrite")) ? convertAnyInt(cmd.optionValue("registerTestWrite").c_str()) : 1;
        // 0 - no sorting other than page; 1 - page then increasing addresss ; 2 - page then decreasing address
        uint8_t  cSortOder  = (cmd.foundOption("sortOrder")) ? convertAnyInt(cmd.optionValue("sortOrder").c_str()) : 0;
        uint8_t  cBitToFlip = (cmd.foundOption("bitToFlip")) ? convertAnyInt(cmd.optionValue("bitToFlip").c_str()) : 0;
        uint32_t cAttempts  = (cmd.foundOption("testAttempts")) ? convertAnyInt(cmd.optionValue("testAttempts").c_str()) : 10;
        uint8_t  cPage      = (cmd.foundOption("pageToTest")) ? convertAnyInt(cmd.optionValue("pageToTest").c_str()) : 1;

        RegisterTester cRegTester;
        cRegTester.Inherit(&cTool);
        cRegTester.SetSortOrder(cSortOder);
        cRegTester.SetBitToFlip(cBitToFlip);
        cRegTester.Initialise();
        for(size_t cAttempt = 0; cAttempt < cAttempts; cAttempt++)
        {
            LOG(INFO) << BOLDBLUE << "Read - test#" << +cAttempt << RESET;
            cRegTester.CheckWriteRegisters(cPage, cNregistersToCheck);
        }
        cRegTester.writeObjects();
    }

    if(cmd.foundOption("registerTestWriteAndToggle"))
    {
        uint8_t cNregistersToCheck = (cmd.foundOption("registerTestWriteAndToggle")) ? convertAnyInt(cmd.optionValue("registerTestWriteAndToggle").c_str()) : 1;
        // 0 - no sorting other than page; 1 - page then increasing addresss ; 2 - page then decreasing address
        uint8_t  cSortOder        = (cmd.foundOption("sortOrder")) ? convertAnyInt(cmd.optionValue("sortOrder").c_str()) : 0;
        uint8_t  cBitToFlip       = (cmd.foundOption("bitToFlip")) ? convertAnyInt(cmd.optionValue("bitToFlip").c_str()) : 0;
        uint8_t  cReturnToDefPage = (cmd.foundOption("returnToDefPage")) ? convertAnyInt(cmd.optionValue("returnToDefPage").c_str()) : 1;
        uint32_t cAttempts        = (cmd.foundOption("testAttempts")) ? convertAnyInt(cmd.optionValue("testAttempts").c_str()) : 10;
        uint8_t  cPage            = (cmd.foundOption("pageToTest")) ? convertAnyInt(cmd.optionValue("pageToTest").c_str()) : 1;

        LOG(INFO) << BOLDBLUE << "Will run register test " << cAttempts << " times..." << RESET;
        RegisterTester cRegTester;
        cRegTester.Inherit(&cTool);
        cRegTester.SetSortOrder(cSortOder);
        cRegTester.SetBitToFlip(cBitToFlip);
        cRegTester.SetReturnToDefPage(cReturnToDefPage);
        cRegTester.Initialise();
        for(size_t cAttempt = 0; cAttempt < cAttempts; cAttempt++)
        {
            LOG(INFO) << BOLDBLUE << "Page switch with read - test#" << +cAttempt << RESET;
            cRegTester.CheckPageSwitchWrite(cPage, cNregistersToCheck);
        }
        cRegTester.writeObjects();
    }

    if(cmd.foundOption("registerTestRead"))
    {
        uint8_t cNregistersToCheck = (cmd.foundOption("registerTestRead")) ? convertAnyInt(cmd.optionValue("registerTestRead").c_str()) : 1;
        // 0 - no sorting other than page; 1 - page then increasing addresss ; 2 - page then decreasing address
        uint8_t  cSortOder = (cmd.foundOption("sortOrder")) ? convertAnyInt(cmd.optionValue("sortOrder").c_str()) : 0;
        uint32_t cAttempts = (cmd.foundOption("testAttempts")) ? convertAnyInt(cmd.optionValue("testAttempts").c_str()) : 10;
        uint8_t  cPage     = (cmd.foundOption("pageToTest")) ? convertAnyInt(cmd.optionValue("pageToTest").c_str()) : 1;

        RegisterTester cRegTester;
        cRegTester.Inherit(&cTool);
        cRegTester.SetSortOrder(cSortOder);
        cRegTester.Initialise();
        for(size_t cAttempt = 0; cAttempt < cAttempts; cAttempt++)
        {
            LOG(INFO) << BOLDBLUE << "Read - test#" << +cAttempt << RESET;
            cRegTester.CheckReadRegisters(cPage, cNregistersToCheck);
        }
        cRegTester.writeObjects();
    }
    if(cmd.foundOption("registerTestReadAndToggle"))
    {
        uint8_t cNregistersToCheck = (cmd.foundOption("registerTestReadAndToggle")) ? convertAnyInt(cmd.optionValue("registerTestReadAndToggle").c_str()) : 1;
        // 0 - no sorting other than page; 1 - page then increasing addresss ; 2 - page then decreasing address
        uint8_t  cSortOder        = (cmd.foundOption("sortOrder")) ? convertAnyInt(cmd.optionValue("sortOrder").c_str()) : 0;
        uint8_t  cReturnToDefPage = (cmd.foundOption("returnToDefPage")) ? convertAnyInt(cmd.optionValue("returnToDefPage").c_str()) : 1;
        uint32_t cAttempts        = (cmd.foundOption("testAttempts")) ? convertAnyInt(cmd.optionValue("testAttempts").c_str()) : 10;
        uint8_t  cPage            = (cmd.foundOption("pageToTest")) ? convertAnyInt(cmd.optionValue("pageToTest").c_str()) : 1;

        RegisterTester cRegTester;
        cRegTester.Inherit(&cTool);
        cRegTester.SetSortOrder(cSortOder);
        cRegTester.SetReturnToDefPage(cReturnToDefPage);
        cRegTester.Initialise();
        for(size_t cAttempt = 0; cAttempt < cAttempts; cAttempt++)
        {
            LOG(INFO) << BOLDBLUE << "Page switch with read - test#" << +cAttempt << RESET;
            cRegTester.CheckPageSwitchRead(cPage, cNregistersToCheck);
        }
        cRegTester.writeObjects();
    }

    if(cEcv)
    {
        cTool.ConfigureHw(true);

        ECVLinkAlignmentOT cLinkAlignment;
        cLinkAlignment.Inherit(&cTool);

        StartInfo theStartInfo;
        theStartInfo.setRunNumber(cRunNumber);
        cLinkAlignment.Start(theStartInfo);
        cLinkAlignment.waitForRunToBeCompleted();
        cLinkAlignment.writeObjects();
        cLinkAlignment.dumpConfigFiles();
    }

    // align CIC-lpGBT-BE
    if(!cmd.foundOption("read") && cmd.foundOption("reconfigure"))
    {
        cTool.ConfigureHw();
        // just to check
        // for(const auto cBoard: *cTool.fDetectorContainer)
        // {
        //     D19cDebugFWInterface* cDebugInterface   = static_cast<D19cDebugFWInterface*>(cTool.fBeBoardInterface->getFirmwareInterface(cBoard));
        //     cDebugInterface->L1ADebug();
        //     cTool.ReadNEvents(cBoard, 10);
        // }
        // exit(0);

        // for(const auto cBoard: *cTool.fDetectorContainer)
        // {
        //     cTool.ReadNEvents(cBoard, 10);
        // }

        // map MPA outputs for PS module
        StartInfo theStartInfo;
        theStartInfo.setRunNumber(cRunNumber);

        PSAlignment cPSAlignment;
        cPSAlignment.Inherit(&cTool);
        cPSAlignment.Start(theStartInfo);
        cPSAlignment.waitForRunToBeCompleted();

        // Alignment of a pattern between CIC and FC7
        LOG(INFO) << BOLDRED << "LinkAlignmentOT" << RESET;

        LinkAlignmentOT cLinkAlignment;
        cLinkAlignment.Inherit(&cTool);
        try
        {
            StartInfo theStartInfo;
            theStartInfo.setRunNumber(cRunNumber);
            cLinkAlignment.Start(theStartInfo);
        }
        catch(const std::exception& e)
        {
            LOG(INFO) << BOLDRED << "Could not align link in the BE... stopping here." << RESET;
            return (666);
        }
        cLinkAlignment.waitForRunToBeCompleted();
        cLinkAlignment.dumpConfigFiles();
        if(!cLinkAlignment.getStatus())
        {
            LOG(INFO) << BOLDRED << "Could not align link in the BE... stopping here." << RESET;
            return (666);
        }
        // align FEs - CIC
        LOG(INFO) << BOLDRED << "CicFEAlignment" << RESET;
        CicFEAlignment cCicAligner;
        cCicAligner.Inherit(&cTool);

        // Doesnt work PSv2
        cCicAligner.Start(theStartInfo);
        cCicAligner.waitForRunToBeCompleted();
        //\Doesnt work PSv2

        cCicAligner.dumpConfigFiles();

        // quickly check ReadData
        // for(const auto cBoard: *cTool.fDetectorContainer)
        // {
        //     cTool.fBeBoardInterface->Start(cBoard);
        //     std::this_thread::sleep_for(std::chrono::milliseconds(10));
        //     std::vector<uint32_t> cData;
        //     bool                cWait    = false;
        //     cTool.ReadData(cBoard, cData, cWait);
        //     cTool.fBeBoardInterface->Stop(cBoard);
        // }
    }
    // reconfigure hardware (ie reload chip registers) without running the alignment
    if(!cmd.foundOption("read") && cmd.foundOption("configure")) { cTool.ConfigureHw(); }
    // reload settings on-to FE chips
    if(!cmd.foundOption("read") && cmd.foundOption("reload"))
    {
        cTool.ConfigureHw();

        // map MPA outputs for PS module
        PSAlignment cPSAlignment;
        cPSAlignment.Inherit(&cTool);
        cPSAlignment.Initialise();
        cPSAlignment.MapMPAOutputs();
        cPSAlignment.Reset();

        LinkAlignmentOT cLinkAlignment;
        cLinkAlignment.Inherit(&cTool);
        try
        {
            StartInfo theStartInfo;
            theStartInfo.setRunNumber(cRunNumber);
            cLinkAlignment.Start(theStartInfo);
        }
        catch(const std::exception& e)
        {
            LOG(INFO) << BOLDRED << "Could not align link in the BE... stopping here." << RESET;
            return (666);
        }
        cLinkAlignment.waitForRunToBeCompleted();
        cLinkAlignment.dumpConfigFiles();
        if(!cLinkAlignment.getStatus())
        {
            LOG(INFO) << BOLDRED << "Could not align link in the BE... stopping here." << RESET;
            return (666);
        }
    }
    if(!cmd.foundOption("read") && cmd.foundOption("realign"))
    {
        // re-align stub package
        bool cSkipStubPkg = (cmd.foundOption("skipAlignment")) && (cSkip.find("all") != std::string::npos || cSkip.find("stubPackage") != std::string::npos);
        if(cSkipStubPkg)
            LOG(INFO) << BOLDBLUE << "Will skip time alignment of stub package in the BE  " << RESET;
        else
        {
            LOG(INFO) << BOLDBLUE << "Performing time alignment of stub data with L1 data in the BE " << RESET;
            LinkAlignmentOT cLinkAlignment;
            cLinkAlignment.Inherit(&cTool);
            cLinkAlignment.Initialise();
            cLinkAlignment.AlignStubPackage();
            cLinkAlignment.Reset();
        }

        // time align stubs with L1 data in the BE
        bool cSkipBEstubs = (cmd.foundOption("skipAlignment")) && (cSkip.find("all") != std::string::npos || cSkip.find("beStubs") != std::string::npos);
        if(cSkipBEstubs)
            LOG(INFO) << BOLDBLUE << "Will skip time alignment of stub data with L1 data in the BE " << RESET;
        else
        {
            LOG(INFO) << BOLDBLUE << "Performing time alignment of stub data with L1 data in the BE " << RESET;
            StubBackEndAlignment cStubBackEndAligner;
            LOG(INFO) << BOLDBLUE << "1 " << RESET;
            cStubBackEndAligner.Inherit(&cTool);
            LOG(INFO) << BOLDBLUE << "2 " << RESET;
            StartInfo theStartInfo;
            theStartInfo.setRunNumber(cRunNumber);
            cStubBackEndAligner.Start(theStartInfo);
            LOG(INFO) << BOLDBLUE << "3 " << RESET;
            cStubBackEndAligner.waitForRunToBeCompleted();
            LOG(INFO) << BOLDBLUE << "4 " << RESET;
        }
        LOG(INFO) << BOLDRED << "PSAlignment " << RESET;
        // now align data between SSA-MPA
        bool cSkipMPAin = (cmd.foundOption("skipAlignment")) && (cSkip.find("all") != std::string::npos || cSkip.find("mpaInputs") != std::string::npos);
        if(cSkipMPAin)
            LOG(INFO) << BOLDBLUE << "Will skip alignment of SSA output data (L1+stubs) to MPAs " << RESET;
        else
        {
            LOG(INFO) << BOLDRED << "PSAlignment! " << RESET;
            // map MPA outputs for PS module
            PSAlignment cPSAlignment;
            cPSAlignment.Inherit(&cTool);
            cPSAlignment.Initialise();
            cPSAlignment.MapMPAOutputs();
            LOG(INFO) << BOLDBLUE << "Performing alignment of SSA output data (L1+stubs) to MPAs " << RESET;
            cPSAlignment.Align();
            cPSAlignment.Reset();
            cPSAlignment.dumpConfigFiles();
            LOG(INFO) << BOLDRED << "PSAlignment DONE! " << RESET;
        }
    }

    if(!cmd.foundOption("read") && cmd.foundOption("checkCICAlignment"))
    {
        // align FEs - CIC
        CicFEAlignment cCicAligner;
        cCicAligner.Inherit(&cTool);
        cCicAligner.Initialise();
        cCicAligner.InputLineScan();
        cCicAligner.Reset();
    }
    // LOG (INFO) << BOLDBLUE << "Performing time alignment of stub data with L1 data in the BE " << RESET;
    // StubBackEndAlignment cStubBackEndAligner;
    // cStubBackEndAligner.Inherit(&cTool);
    // cStubBackEndAligner.Start(cRunNumber);
    // cStubBackEndAligner.waitForRunToBeCompleted();

    // equalize thresholds on readout chips

    if(cmd.foundOption("tuneOffsets") && !cmd.foundOption("read"))
    {
        bool cAllChan = (cmd.foundOption("allChan")) ? true : false;
        t.start();
        // now create a PedestalEqualization object
        PedestalEqualization cPedestalEqualization;
        cPedestalEqualization.Inherit(&cTool);
        cPedestalEqualization.Initialise(cAllChan, true);
        cPedestalEqualization.FindVplus();
        cPedestalEqualization.FindOffsets();
        cPedestalEqualization.Reset();
        // second parameter disables stub logic on CBC3
        cPedestalEqualization.writeObjects();
        cPedestalEqualization.dumpConfigFiles();
        cPedestalEqualization.resetPointers();
        t.stop();
        t.show("Time to tune the front-ends on the system: ");
        // // reset
        // cTool.fDetectorContainer->resetReadoutChipQueryFunction();
    }

    if(cmd.foundOption("injectionTest") && !cmd.foundOption("read"))
    {
        // auto cNevents          = findValueInSettings("Check2STPamplitude", 255);
        for(auto board: *cTool.fDetectorContainer)
        {
            cTool.setSameDacBeBoard(static_cast<BeBoard*>(board), "ReadoutMode", 0);
            if(cInjectionSource.find("digital") != std::string::npos)
            {
                // configure trigger
                uint8_t                                       cTriggerSource   = 6;
                uint16_t                                      cDelayAfterReset = 100;
                uint16_t                                      cDelayTillNext   = 400;
                std::vector<std::string>                      cFcmdRegs{"trigger_source", "test_pulse.delay_after_fast_reset", "test_pulse.delay_before_next_pulse"};
                std::vector<uint16_t>                         cFcmdRegVals{cTriggerSource, cDelayAfterReset, cDelayTillNext};
                std::vector<uint16_t>                         cFcmdRegOrigVals(cFcmdRegs.size(), 0);
                std::vector<std::pair<std::string, uint32_t>> cRegVec;
                cRegVec.clear();
                for(size_t cIndx = 0; cIndx < cFcmdRegs.size(); cIndx++)
                {
                    std::string cRegName    = "fc7_daq_cnfg.fast_command_block." + cFcmdRegs[cIndx];
                    cFcmdRegOrigVals[cIndx] = cTool.fBeBoardInterface->ReadBoardReg(board, cRegName);
                    cRegVec.push_back({cRegName, cFcmdRegVals[cIndx]});
                }
                cRegVec.push_back({"fc7_daq_cnfg.fast_command_block.triggers_to_accept", 0});
                cRegVec.push_back({"fc7_daq_ctrl.fast_command_block.control.load_config", 0x1});
                cTool.fBeBoardInterface->WriteBoardMultReg(board, cRegVec);
                cTool.fBeBoardInterface->WriteBoardReg(board, "fc7_daq_cnfg.tlu_block.tlu_enabled", 0);

                // first inject digitally
                Injection              cInjection;
                std::vector<Injection> cInjections;
                // inject one cluster into each MPA-SSA pair
                cInjection.fRow    = 10;
                cInjection.fColumn = 2;
                cInjections.push_back(cInjection); // 0
                // inject
                for(auto cOpticalReadout: *board)
                {
                    for(auto cHybrid: *cOpticalReadout)
                    {
                        for(auto cChip: *cHybrid)
                        {
                            // and that readout mode is set
                            // make sure L1 latency is configured
                            if(cChip->getFrontEndType() == FrontEndType::MPA2) { (static_cast<PSInterface*>(cTool.fReadoutChipInterface))->digiInjection(cChip, cInjections, 0x01); }
                            if(cChip->getFrontEndType() == FrontEndType::SSA2)
                            {
                                cTool.fReadoutChipInterface->WriteChipReg(cChip, "ENFLAGS_ALL", 0x0);
                                cTool.fReadoutChipInterface->WriteChipReg(cChip, "CalPulse_duration", 0x01);
                                cTool.fReadoutChipInterface->WriteChipReg(cChip, "DigCalibPattern_L_ALL", 0x01);
                                cTool.fReadoutChipInterface->WriteChipReg(cChip, "DigCalibPattern_H_ALL", 0x00);
                                for(auto cInjection: cInjections) { cTool.fReadoutChipInterface->WriteChipReg(cChip, "ENFLAGS_S" + std::to_string(cInjection.fRow), 0x9); }
                            }
                        } // chip
                    } // hybrid
                } // optica]l group
            }
            else if(cInjectionSource.find("analogue") != std::string::npos)
            {
                // configure trigger
                uint8_t                                       cTriggerSource   = 6;
                uint16_t                                      cDelayAfterReset = 100;
                uint16_t                                      cDelayTillNext   = 400;
                std::vector<std::string>                      cFcmdRegs{"trigger_source", "test_pulse.delay_after_fast_reset", "test_pulse.delay_before_next_pulse"};
                std::vector<uint16_t>                         cFcmdRegVals{cTriggerSource, cDelayAfterReset, cDelayTillNext};
                std::vector<uint16_t>                         cFcmdRegOrigVals(cFcmdRegs.size(), 0);
                std::vector<std::pair<std::string, uint32_t>> cRegVec;
                cRegVec.clear();
                for(size_t cIndx = 0; cIndx < cFcmdRegs.size(); cIndx++)
                {
                    std::string cRegName    = "fc7_daq_cnfg.fast_command_block." + cFcmdRegs[cIndx];
                    cFcmdRegOrigVals[cIndx] = cTool.fBeBoardInterface->ReadBoardReg(board, cRegName);
                    cRegVec.push_back({cRegName, cFcmdRegVals[cIndx]});
                }
                cRegVec.push_back({"fc7_daq_ctrl.fast_command_block.control.load_config", 0x1});
                cTool.fBeBoardInterface->WriteBoardMultReg(board, cRegVec);
                cTool.fBeBoardInterface->WriteBoardReg(board, "fc7_daq_cnfg.tlu_block.tlu_enabled", 0);

                int cPSmoduleSSAth = cTool.findValueInSettings<double>("PSmoduleSSAthreshold", 100);
                int cPSmoduleMPAth = cTool.findValueInSettings<double>("PSmoduleMPAthreshold", 100);
                int cInjectionAmpl = cTool.findValueInSettings<double>("PSOccupancyPulseAmplitude", 0xFF);
                int cSamplingSSA   = cTool.findValueInSettings<double>("SamplingModeSSA", 0);
                int cSamplingMPA   = cTool.findValueInSettings<double>("SamplingModeMPA", 0);
                // analogue injection
                cTool.setSameDacBeBoard(static_cast<BeBoard*>(board), "InjectedCharge", cInjectionAmpl);
                cTool.setSameDacBeBoard(static_cast<BeBoard*>(board), "AnalogueSync", 1);
                LOG(INFO) << BOLDBLUE << cInjectionAmpl << " injected charge, " << cPSmoduleSSAth << " as SSA threshold " << cPSmoduleMPAth << " as MPA threshold " << RESET;
                // disable injection on all pixels MPAs
                for(auto opticalGroup: *board)
                {
                    for(auto hybrid: *opticalGroup)
                    {
                        for(auto chip: *hybrid)
                        {
                            cTool.fReadoutChipInterface->WriteChipReg(chip, "ENFLAGS_ALL", 0x00, false); // 0x5E
                        }
                    }
                }
                // enable a few
                std::vector<std::pair<uint16_t, uint16_t>> cPxls;
                uint16_t                                   cFirstRow = 10;
                uint16_t                                   cNCols    = 2;
                uint16_t                                   cNRows    = 1;
                std::vector<uint16_t>                      cStrps;
                for(uint16_t cNRow = 0; cNRow < cNRows; cNRow++)
                {
                    uint16_t cRow = cFirstRow + cNRow * 2;
                    cStrps.push_back(cRow);
                    for(uint16_t cCol = 0; cCol < cNCols; cCol++) { cPxls.push_back({cNRow, cCol}); }
                }
                LOG(INFO) << BOLDBLUE << "Enabling analogue injection in " << cPxls.size() << " pixels and " << cStrps.size() << " strips." << RESET;
                for(auto opticalGroup: *board)
                {
                    for(auto hybrid: *opticalGroup)
                    {
                        for(auto chip: *hybrid)
                        {
                            if(chip->getFrontEndType() == FrontEndType::MPA2)
                            {
                                for(auto cPxl: cPxls)
                                {
                                    std::stringstream cRegName;
                                    cRegName << "ENFLAGS_C" << +cPxl.second << "_R" << +cPxl.first;
                                    cTool.fReadoutChipInterface->WriteChipReg(chip, cRegName.str(), 0x4F, false);
                                    std::stringstream cRegNameTrim;
                                    cRegNameTrim << "TrimDAC_C" << +cPxl.second << "_R" << +cPxl.first;
                                    cTool.fReadoutChipInterface->WriteChipReg(chip, cRegNameTrim.str(), 0x0);
                                }
                            }
                            if(chip->getFrontEndType() == FrontEndType::SSA2)
                            {
                                for(auto sStrp: cStrps)
                                {
                                    std::stringstream cRegName;
                                    cRegName << "ENFLAGS_S" << +sStrp;
                                    // cTool.fReadoutChipInterface->WriteChipReg(chip, cRegName.str(), 0x13, false);
                                    cTool.fReadoutChipInterface->WriteChipReg(chip, cRegName.str(), 0x11, false);
                                }
                            }
                        }
                    }
                }

                for(auto opticalGroup: *board)
                {
                    for(auto hybrid: *opticalGroup)
                    {
                        for(auto chip: *hybrid)
                        {
                            if(chip->getFrontEndType() == FrontEndType::SSA2)
                            {
                                cTool.fReadoutChipInterface->WriteChipReg(chip, "Threshold", cPSmoduleSSAth);
                                cTool.fReadoutChipInterface->WriteChipReg(chip, "SAMPLINGMODE_ALL", cSamplingSSA);
                                cTool.fReadoutChipInterface->WriteChipReg(chip, "CalPulse_duration", 0x01);
                            }
                            if(chip->getFrontEndType() == FrontEndType::MPA2)
                            {
                                cTool.fReadoutChipInterface->WriteChipReg(chip, "Threshold", cPSmoduleMPAth);
                                cTool.fReadoutChipInterface->WriteChipReg(chip, "ModeSel_ALL", cSamplingMPA);
                            }
                        }
                    }
                }
            }
            else if(cInjectionSource.find("external") != std::string::npos)
            {
                // configure trigger
                uint8_t                                       cTriggerSource   = 4;
                uint16_t                                      cDelayAfterReset = 100;
                uint16_t                                      cDelayTillNext   = 400;
                std::vector<std::string>                      cFcmdRegs{"trigger_source", "test_pulse.delay_after_fast_reset", "test_pulse.delay_before_next_pulse"};
                std::vector<uint16_t>                         cFcmdRegVals{cTriggerSource, cDelayAfterReset, cDelayTillNext};
                std::vector<uint16_t>                         cFcmdRegOrigVals(cFcmdRegs.size(), 0);
                std::vector<std::pair<std::string, uint32_t>> cRegVec;
                cRegVec.clear();
                for(size_t cIndx = 0; cIndx < cFcmdRegs.size(); cIndx++)
                {
                    std::string cRegName    = "fc7_daq_cnfg.fast_command_block." + cFcmdRegs[cIndx];
                    cFcmdRegOrigVals[cIndx] = cTool.fBeBoardInterface->ReadBoardReg(board, cRegName);
                    cRegVec.push_back({cRegName, cFcmdRegVals[cIndx]});
                }
                cRegVec.push_back({"fc7_daq_ctrl.fast_command_block.control.load_config", 0x1});
                cTool.fBeBoardInterface->WriteBoardMultReg(board, cRegVec);
                cTool.fBeBoardInterface->WriteBoardReg(board, "fc7_daq_cnfg.tlu_block.tlu_enabled", 1);
                cTool.fBeBoardInterface->WriteBoardReg(board, "fc7_daq_cnfg.tlu_block.handshake_mode", 2);

                int cPSmoduleSSAth = cTool.findValueInSettings<double>("PSmoduleSSAthreshold", 100);
                int cPSmoduleMPAth = cTool.findValueInSettings<double>("PSmoduleMPAthreshold", 100);
                int cInjectionAmpl = cTool.findValueInSettings<double>("PSOccupancyPulseAmplitude", 0xFF);
                int cSamplingSSA   = cTool.findValueInSettings<double>("SamplingModeSSA", 0);
                int cSamplingMPA   = cTool.findValueInSettings<double>("SamplingModeMPA", 0);
                // analogue injection
                cTool.setSameDacBeBoard(static_cast<BeBoard*>(board), "InjectedCharge", cInjectionAmpl);
                cTool.setSameDacBeBoard(static_cast<BeBoard*>(board), "AnalogueSync", 1);
                LOG(INFO) << BOLDBLUE << cInjectionAmpl << " injected charge, " << cPSmoduleSSAth << " as SSA threshold " << cPSmoduleMPAth << " as MPA threshold " << RESET;
                // disable injection on all pixels MPAs
                for(auto opticalGroup: *board)
                {
                    for(auto hybrid: *opticalGroup)
                    {
                        for(auto chip: *hybrid)
                        {
                            if(chip->getFrontEndType() == FrontEndType::MPA2)
                            {
                                cTool.fReadoutChipInterface->WriteChipReg(chip, "ENFLAGS_ALL", 0xF, false); // 0x5E
                            }
                            if(chip->getFrontEndType() == FrontEndType::SSA2)
                            {
                                cTool.fReadoutChipInterface->WriteChipReg(chip, "ENFLAGS_ALL", 0x1, false); // 0x5E
                            }
                        }
                    }
                }

                for(auto opticalGroup: *board)
                {
                    for(auto hybrid: *opticalGroup)
                    {
                        for(auto chip: *hybrid)
                        {
                            if(chip->getFrontEndType() == FrontEndType::SSA2)
                            {
                                cTool.fReadoutChipInterface->WriteChipReg(chip, "Threshold", cPSmoduleSSAth);
                                cTool.fReadoutChipInterface->WriteChipReg(chip, "SAMPLINGMODE_ALL", cSamplingSSA);
                            }
                            if(chip->getFrontEndType() == FrontEndType::MPA2)
                            {
                                cTool.fReadoutChipInterface->WriteChipReg(chip, "Threshold", cPSmoduleMPAth);
                                cTool.fReadoutChipInterface->WriteChipReg(chip, "ModeSel_ALL", cSamplingMPA);
                            }
                        }
                    }
                }
            }
            else
            {
                auto cTriggerSource = cTool.fBeBoardInterface->ReadBoardReg(board, "fc7_daq_cnfg.fast_command_block.trigger_source");
                LOG(INFO) << BOLDBLUE << "Injection test with trigger source " << +cTriggerSource << RESET;

                int cPSmoduleSSAth = cTool.findValueInSettings<double>("PSmoduleSSAthreshold", 100);
                int cPSmoduleMPAth = cTool.findValueInSettings<double>("PSmoduleMPAthreshold", 100);
                int cSamplingSSA   = cTool.findValueInSettings<double>("SamplingModeSSA", 0);
                int cSamplingMPA   = cTool.findValueInSettings<double>("SamplingModeMPA", 0);

                // force TP to be off
                cTool.setSameDacBeBoard(static_cast<BeBoard*>(board), "AnalogueSync", 0);
                // analogue injection
                for(auto opticalGroup: *board)
                {
                    for(auto hybrid: *opticalGroup)
                    {
                        for(auto chip: *hybrid)
                        {
                            if(chip->getFrontEndType() == FrontEndType::SSA2)
                            {
                                cTool.fReadoutChipInterface->WriteChipReg(chip, "SAMPLINGMODE_ALL", cSamplingSSA);
                                cTool.fReadoutChipInterface->WriteChipReg(chip, "Threshold", cPSmoduleSSAth);
                            }
                            if(chip->getFrontEndType() == FrontEndType::MPA2)
                            {
                                cTool.fReadoutChipInterface->WriteChipReg(chip, "ModeSel_ALL", cSamplingMPA);
                                cTool.fReadoutChipInterface->WriteChipReg(chip, "Threshold", cPSmoduleMPAth);
                            }
                        }
                    }
                }
            }
        }
        LatencyScan cLatencyScan;
        cLatencyScan.Inherit(&cTool);
        cLatencyScan.Initialize();
        cLatencyScan.ScanLatency();

        if(cmd.foundOption("phaseScan"))
        {
            PhaseScan cPhaseScan;
            cPhaseScan.Inherit(&cTool);
            cPhaseScan.Initialize();
            cPhaseScan.ScanPhase();
        }
    }
    // measure noise on FE chips
    if(cmd.foundOption("measurePedeNoise") && !cmd.foundOption("read")) // S-curves
    {
        bool cAllChan = (cmd.foundOption("allChan")) ? true : false;
        LOG(INFO) << BOLDMAGENTA << "Measuring pedestal and noise" << RESET;
        t.start();
        // if this is true, I need to create an object of type PedeNoise from the members of Calibration
        // tool provides an Inherit(Tool* pTool) for this purpose
        PedeNoise cPedeNoise;
        cPedeNoise.Inherit(&cTool);
        cPedeNoise.Initialise(cAllChan, true); // canvases etc. for fast calibration
        cPedeNoise.measureNoise();
        cPedeNoise.Validate();
        cPedeNoise.writeObjects();
        cPedeNoise.dumpConfigFiles();
        cPedeNoise.Reset();
        t.stop();
        t.show("Time to Scan Pedestals and Noise");
    }
    if(cmd.foundOption("pixelAlive") && !cmd.foundOption("read"))
    {
        LOG(INFO) << BOLDMAGENTA << "Measuring Pixel Occupancy and Masking Pixels Below Threshold" << RESET;
        t.start();
        PSPixelAlive cPixelAlive;
        cPixelAlive.Inherit(&cTool);
        cPixelAlive.Initialise();
        cPixelAlive.measurePixels();
        cPixelAlive.writeObjects();
        LOG(INFO) << BOLDBLUE << "Dumping PixelAlive Registers" << RESET;
        cPixelAlive.dumpConfigFiles();
        cPixelAlive.Reset();
        t.stop();
        t.show("Time to Measure Pixel Occupancy and Mask Pixels");
        LOG(INFO) << BOLDMAGENTA << "PixelAlive Finished" << RESET;
    }

    if(cmd.foundOption("cmNoise") && !cmd.foundOption("read"))
    {
        LOG(INFO) << "OT_MODULE_TEST:: Measuring CM Noise" << RESET;

        OTCMNoise cTester;
        cTester.Inherit(&cTool);
        cTester.Initialize();
        LOG(INFO) << "OT_MODULE_TEST:: Setting thresholds" << RESET;
        cTester.SetThresholds();

        LOG(INFO) << "OT_MODULE_TEST:: Taking measurements " << RESET;
        cTester.TakeData();
    }

    uint8_t cScanL1    = (cmd.foundOption("scanL1") || cmd.foundOption("scanLatencies")) ? 1 : 0;
    uint8_t cScanStubs = (cmd.foundOption("scanStubs") || cmd.foundOption("scanLatencies")) ? 1 : 0;
    if(!cmd.foundOption("read") && cmd.foundOption("TestPulseCheck"))
    {
        std::ofstream cGoodRuns;
        cGoodRuns.open("GoodRunNumbers.dat", std::fstream::app);
        cGoodRuns << cRunNumber << "\n";
        cGoodRuns.close();

        PrintConfig cCng;
        cCng.fVerbose    = 1;
        cCng.fPrintEvery = 1;

        BeamTestCheck cBeamTestCheck;
        cBeamTestCheck.Inherit(&cTool);
        cBeamTestCheck.Initialise();

        cBeamTestCheck.ConfigureScans(cScanL1, cScanStubs);
        cBeamTestCheck.ConfigurePrintout(cCng);
        cBeamTestCheck.CheckWithTP();
        cBeamTestCheck.writeObjects();
        // cBeamTestCheck.Reset();
    }

    if(!cmd.foundOption("read") && cmd.foundOption("ExternalCheck"))
    {
        std::ofstream cGoodRuns;
        cGoodRuns.open("GoodRunNumbers.dat", std::fstream::app);
        cGoodRuns << cRunNumber << "\n";
        cGoodRuns.close();

        BeamTestCheck cBeamTestCheck;
        cBeamTestCheck.Inherit(&cTool);
        cBeamTestCheck.Initialise();
        cBeamTestCheck.ConfigureScans(cScanL1, cScanStubs);
        // check with TP
        cBeamTestCheck.CheckWithExternal();
        cBeamTestCheck.writeObjects();
        cBeamTestCheck.Reset();
    }

    if(!cmd.foundOption("read") && cmd.foundOption("InternalCheck"))
    {
        std::ofstream cGoodRuns;
        cGoodRuns.open("GoodRunNumbers.dat", std::fstream::app);
        cGoodRuns << cRunNumber << "\n";
        cGoodRuns.close();

        BeamTestCheck cBeamTestCheck;
        cBeamTestCheck.Inherit(&cTool);
        cBeamTestCheck.Initialise();
        cBeamTestCheck.ConfigureScans(cScanL1, cScanStubs);
        PrintConfig cCng;
        cCng.fVerbose    = 1;
        cCng.fPrintEvery = 10000;
        cBeamTestCheck.ConfigurePrintout(cCng);
        cBeamTestCheck.CheckWithInternal();
        cBeamTestCheck.writeObjects();
        cBeamTestCheck.Reset();
    }

    if(!cmd.foundOption("read") && cmd.foundOption("TLUCheck"))
    {
        std::ofstream cGoodRuns;
        cGoodRuns.open("GoodRunNumbers.dat", std::fstream::app);
        cGoodRuns << cRunNumber << "\n";
        cGoodRuns.close();

        BeamTestCheck cBeamTestCheck;
        cBeamTestCheck.Inherit(&cTool);
        cBeamTestCheck.Initialise();
        cBeamTestCheck.ConfigureScans(cScanL1, cScanStubs);
        cBeamTestCheck.CheckWithTLU();
        cBeamTestCheck.writeObjects();
        cBeamTestCheck.Reset();
    }

    if(!cmd.foundOption("read") && cmd.foundOption("kira"))
    {
        std::ofstream cGoodRuns;
        cGoodRuns.open("GoodRunNumbers.dat", std::fstream::app);
        cGoodRuns << cRunNumber << "\n";
        cGoodRuns.close();
        KIRA cKira;
        cKira.Inherit(&cTool);
        cKira.Initialise(cKiraPort, cKiraID);
        cKira.determineLatency();
        if(cKiraCalibration) cKira.calibrateIntensity();
        cKira.performKIRATest();
    }

    if(!cmd.foundOption("read") && cmd.foundOption("DataMonitor"))
    {
        std::ofstream cGoodRuns;
        cGoodRuns.open("GoodRunNumbers.dat", std::fstream::app);
        cGoodRuns << cRunNumber << "\n";
        cGoodRuns.close();

        uint8_t       cDisableFEs = (cmd.foundOption("DataMonitor")) ? convertAnyInt(cmd.optionValue("DataMonitor").c_str()) : 0;
        BeamTestCheck cBeamTestCheck;
        cBeamTestCheck.Inherit(&cTool);
        cBeamTestCheck.Initialise();
        if(cDisableFEs == 1) cBeamTestCheck.DisableAllFEs();

        // uint8_t cContinuousReadout = cmd.foundOption("continuousReadout") ? 1 : 0;
        // int     cReadoutPause      = (cmd.foundOption("continuousReadout")) ? convertAnyInt(cmd.optionValue("continuousReadout").c_str()) : 10;
        // cBeamTestCheck.SetReadoutPause(cReadoutPause);

        if(cmd.foundOption("TLUCheck")) cBeamTestCheck.CheckWithTLU();
        if(cmd.foundOption("ExternalCheck")) cBeamTestCheck.CheckWithExternal();
        if(cmd.foundOption("TestPulseCheck")) cBeamTestCheck.CheckWithTP();

        cBeamTestCheck.writeObjects();
        cBeamTestCheck.Reset();
    }

    if(cmd.foundOption("read"))
    {
        std::string   cRawFileName = cmd.foundOption("read") ? cmd.optionValue("read") : "";
        BeamTestCheck cBeamTestCheck;
        cBeamTestCheck.SetReadoutMode(1);
        cBeamTestCheck.Inherit(&cTool);
        cBeamTestCheck.Initialise();
        PrintConfig cCng;
        cCng.fVerbose    = 1;
        cCng.fPrintEvery = 1;
        cBeamTestCheck.ConfigurePrintout(cCng);
        cBeamTestCheck.SaveHitDataTree();
        cBeamTestCheck.ReadDataFromFile(cRawFileName);
        cBeamTestCheck.ValidateRaw();
        cBeamTestCheck.writeObjects();
        // cBeamTestCheck.Reset();
    }
    if(!cmd.foundOption("read")) { cTool.dumpConfigFiles(); }

    if(cPulseShape)
    {
        Timer t;
        t.start();
        CBCPulseShape cCBCPulseShape;
        cCBCPulseShape.Inherit(&cTool);
        cCBCPulseShape.Initialise();
        cCBCPulseShape.runCBCPulseShape();
        cCBCPulseShape.writeObjects();
        t.stop();
        t.show("Time for pulseShape plot measurement");
        t.reset();
    }

    if(cLatency || cStubLatency)
    {
        LatencyScan cLatencyScan;
        cLatencyScan.Inherit(&cTool);
        cLatencyScan.Initialize();
        if(cLatency) cLatencyScan.ScanLatency();
        if(cStubLatency) cLatencyScan.StubLatencyScan();
        cLatencyScan.writeObjects();
    }

    cTool.SaveResults();
    cTool.WriteRootFile();
    cTool.CloseResultFile();
    cTool.Destroy();
    signal(SIGINT, SIG_DFL);
    runCompleted = 1;
    if(!batchMode) cApp.Run();
    cGlobalTimer.stop();
    cGlobalTimer.show("Total execution time: ");

    return 0;
}
