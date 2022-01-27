/*!
 * \authors Antonio Cassese <antonio.cassese@fi.infn.it>, INFN-Firenze
 * \date November 09 2020
 */

// Libraries
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp> //!For command line arg parsing
#include <cmath>
#include <cstdlib>
#include <errno.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <pugixml.hpp>
#include <sstream>
#include <string>
#include <vector>

#include "DeviceHandler.h"
#include "KeithleyMultimeter.h"
#include "Multimeter.h"
#include "PowerSupply.h"
#include "PowerSupplyChannel.h"

using namespace std;
namespace po = boost::program_options;

// Global Variables
PowerSupply*        fPowerSupply;
PowerSupplyChannel* fPowerSupplyChannel;
KeithleyMultimeter* fKeithleyMultimeter;
bool                fVerbose;
std::string         fMultimeterClassName;
std::string         fSaveFileName;
std::string         fSaveDir;
std::ofstream*      fSaveFile;

/*!
************************************************
* A simple "wait for input".
************************************************
*/
void wait()
{
    cout << "Press ENTER to continue...";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

/*! \brief Clean strings.
************************************************
*  Cleans string from child_value from
*  unwanted characters (as spaces).
* \param strClean String to be clean.
* \return Cleaned string.
************************************************
*/
std::string ChildValClean(std::string strClean)
{
    boost::trim(strClean);
    return strClean;
}

/*!
 ************************************************
 * Creates dicrectory and file streamer from
 * settings in config file.
 \param saveNode Node with saving settings.
************************************************
*/
void ConfigureSaving(pugi::xml_node* saveFileNode)
{
    std::string saveDir      = saveFileNode->attribute("path").as_string();
    std::string saveFileName = saveFileNode->attribute("file").as_string();
    time_t      rawtime;
    struct tm*  timeinfo;
    char        dateString[15];
    char        timeString[15];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(dateString, sizeof(dateString), "%Y_%m_%d/", timeinfo);
    strftime(timeString, sizeof(timeString), "%H_%M_%S_", timeinfo);
    saveDir += dateString;
    fSaveDir = saveDir;
    boost::filesystem::path dir(saveDir.c_str());
    boost::filesystem::create_directories(dir);
    fSaveFileName = saveDir + timeString + saveFileName;
    if(fVerbose) std::cout << "\nOpening file: " << fSaveFileName << " ..." << std::endl;
    fSaveFile->open(fSaveFileName, std::ofstream::out | std::ofstream::app);
    if(fVerbose) std::cout << "File is open: " << fSaveFile->is_open() << std::endl << std::endl;
}

/*!
************************************************
 * Configure instruments described in xml file
 * and specified in instruments config file(s).
 \param instrumentNode Node with instruments
 list.
************************************************
*/
void ConfigureInstruments(pugi::xml_node* instrumentNode, DeviceHandler* mulHandler, DeviceHandler* psHandler)
{
    pugi::xml_document multimeterDocSettings;
    pugi::xml_document powerSupplyDocSettings;
    pugi::xml_node     multimeter            = instrumentNode->child("Multimeter");
    pugi::xml_node     powerSupply           = instrumentNode->child("PowerSupply");
    std::string        multimeterConfigFile  = multimeter.attribute("ConfigFile").as_string();
    std::string        powerSupplyConfigFile = powerSupply.attribute("ConfigFile").as_string();
    fMultimeterClassName                     = multimeter.attribute("ClassName").as_string();
    if(fVerbose)
    {
        std::cout << "fMultimeterClassName: " << fMultimeterClassName << std::endl;
        std::cout << "Multimeter configuration file is: " << multimeterConfigFile << std::endl;
        std::cout << "Power supply configuration file is: " << powerSupplyConfigFile << std::endl;
    }
    mulHandler->readSettings(multimeterConfigFile, multimeterDocSettings, fVerbose);
    psHandler->readSettings(powerSupplyConfigFile, powerSupplyDocSettings, fVerbose);

    if(fMultimeterClassName == "KeithleyMultimeter")
        fKeithleyMultimeter = dynamic_cast<KeithleyMultimeter*>(mulHandler->getMultimeter(multimeter.attribute("ID").as_string()));
    else
    {
        std::stringstream error;
        error << "Code not implemented for " << fMultimeterClassName << " multimeter type, aborting...";
        throw std::runtime_error(error.str());
    }
    fPowerSupply        = psHandler->getPowerSupply(powerSupply.attribute("ID").as_string());
    fPowerSupplyChannel = fPowerSupply->getChannel(powerSupply.attribute("ChannelID").as_string());
}

/*!
************************************************
 * Writes the file header starting from channel
 * map informations.
 \param std::map<int, std::string> & channelMap, channel
*       map informations.
************************************************
*/

void PrepareFileHeaderScanner(pugi::xml_node* multimeterReadTypeNode, std::map<int, std::string>& channelMap)
{
    int nChannels = multimeterReadTypeNode->attribute("nChannels").as_int();
    *fSaveFile << "Current"
               << ",Vps";
    for(int v = 1; v < nChannels + 1; ++v)
    {
        if(channelMap.count(v) != 1)
        {
            // std::cout << "Entering here: " << v << std::endl;
            *fSaveFile << ",";
            if(channelMap.count(v) > 1) std::cout << "More than one channel_" << v << " defined, please check config file" << std::endl;
            continue;
        }
        *fSaveFile << "," << channelMap[v];
        ;
    }
    *fSaveFile << std::endl;
}

/*!
************************************************
 * Fills vector for data acquisition with the
 * right direction.
 \param data Vector pointer for acquisition
 data points filling.
 \param first First point.
 \param last Last point.
 \param stepSize Step size between points.
************************************************
*/
void FillScanPointsVector(std::vector<double>* data, double first, double last, double stepSize)
{
    double step;
    int    nStep;
    step = fabs(last - first);
    step /= fabs(stepSize);
    nStep = (int)std::floor(step);
    nStep += 1;
    for(int v = 0; v < nStep; ++v) data->push_back(first + v * stepSize);
    if(data->back() != last) data->push_back(last);
}

/*!
************************************************
 * Creates data taking point starting from
 * config file.
 \return Returns vector with data taking
 points.
************************************************
*/
std::vector<double> PrepareScanPoints(pugi::xml_node* settingNode)
{
    double      vScanLow   = settingNode->attribute("Lower").as_double();
    double      vScanHigh  = settingNode->attribute("Higher").as_double();
    double      vScanStep  = settingNode->attribute("StepSize").as_double();
    std::string vDirection = settingNode->attribute("Direction").as_string();

    std::vector<double> dataTakingPoints;

    if(fVerbose) std::cout << "Direction: " << vDirection << std::endl;

    if(vDirection == "up")
        FillScanPointsVector(&dataTakingPoints, vScanLow, vScanHigh, vScanStep);
    else if(vDirection == "down")
        FillScanPointsVector(&dataTakingPoints, vScanHigh, vScanLow, -vScanStep);
    else if(vDirection == "both")
    {
        FillScanPointsVector(&dataTakingPoints, vScanLow, vScanHigh, vScanStep);
        FillScanPointsVector(&dataTakingPoints, vScanHigh, vScanLow, -vScanStep);
    }
    else
    {
        std::stringstream error;
        error << "Direction: " << vDirection << " in configuration file for IT IV curve not defined";
        throw std::runtime_error(error.str());
    }
    return dataTakingPoints;
}

/*!
************************************************
 * Performs starting routines on multimeter.
 \return True if everything works.
************************************************
*/
void StartMultimeter()
{
    if(fMultimeterClassName == "KeithleyMultimeter")
    {
        fKeithleyMultimeter->reset();
        fKeithleyMultimeter->enableInternalScan();
    }
}

/*!
************************************************
 * Performs starting routines on power supply.
 \return True if everything works.
************************************************
*/
void StartPowerSupply(std::string curveType, pugi::xml_node* settingNode)
{
    fPowerSupply->reset();
    sleep(1);
    float compliance = settingNode->attribute("Compliance").as_float();
    float protection = settingNode->attribute("Protection").as_float();
    if(curveType == "Current")
    {
        fPowerSupplyChannel->setOverVoltageProtection(protection);
        fPowerSupplyChannel->setVoltageCompliance(compliance);
        fPowerSupplyChannel->setCurrent(0);
    }
    else if(curveType == "Voltage")
    {
        fPowerSupplyChannel->setOverCurrentProtection(protection);
        fPowerSupplyChannel->setCurrentCompliance(compliance);
        fPowerSupplyChannel->setVoltage(0);
    }
    else
    {
        std::stringstream error;
        error << "Code not implemented for " << curveType << " curve type, aborting...";
        throw std::runtime_error(error.str());
    }

    fPowerSupplyChannel->turnOn();
}

/*!
************************************************
 * Acquisition for multimeter with scanner
 * card.
 \return bool returns true if acquisition
 worked properly.
************************************************
*/
void ScannerCardAcquisition(pugi::xml_node* settingNode, pugi::xml_node* multimeterReadTypeNode)
{
    try
    {
        std::map<int, std::string> channelMap;
        fKeithleyMultimeter->createScannerCardMap(multimeterReadTypeNode, channelMap, fVerbose);
        PrepareFileHeaderScanner(multimeterReadTypeNode, channelMap);
        std::string         flagZero     = settingNode->attribute("Zeroing").as_string();
        std::vector<double> vScan        = PrepareScanPoints(settingNode);
        int                 firstChannel = channelMap.begin()->first;
        int                 lastChannel  = channelMap.end()->first;
        std::cout << "First channel: " << firstChannel << std::endl << "Last channel: " << lastChannel << std::endl;
        for(unsigned int v = 0; v < vScan.size(); ++v)
        {
            if(fVerbose) std::cout << "ScanPoint: " << vScan[v] << std::endl;
            if(vScan[v] > 0.01)
                fPowerSupplyChannel->setCurrent(vScan[v]);
            else
                fPowerSupplyChannel->setCurrent(0.01);
            sleep(1); // timeout s
            *fSaveFile << fPowerSupplyChannel->getCurrent() << "," << fPowerSupplyChannel->getOutputVoltage() << "," << fKeithleyMultimeter->scanChannels(firstChannel, lastChannel);
            if(flagZero == "no" || flagZero == "No" || flagZero == "NO") continue;
            fPowerSupplyChannel->setCurrent(0.01); // Sets current to zero
            sleep(1);
            if(fVerbose)
            {
                std::cout << "Zeroing current" << std::endl;
                std::cout << "Current value after zeroing: " << fPowerSupplyChannel->getCurrent() << " A" << std::endl;
                std::cout << "Voltage value after zeroing: " << fPowerSupplyChannel->getOutputVoltage() << " V" << std::endl;
            }
        }
        fPowerSupplyChannel->turnOff();
    }
    catch(const std::out_of_range& oor)
    {
        std::cerr << "Out of Range error: " << oor.what() << '\n';
    }
}

/*!
************************************************
* Starts the acquisition of data.
************************************************
*/
void StartAcquisition(pugi::xml_node* settingNode, pugi::xml_node* multimeterReadTypeNode, pugi::xml_node* instrumentNode)
{
    std::string readTypeStr = instrumentNode->child("Multimeter").attribute("ReadType").as_string();
    if(readTypeStr == "ScannerCard")
        ScannerCardAcquisition(settingNode, multimeterReadTypeNode);
    else
    {
        std::stringstream error;
        error << "Code not implemented for " << readTypeStr << " multimeter reading type, aborting...";
        throw std::runtime_error(error.str());
    }
}

/*!
************************************************
* A simple "Yes or No" input handler.
************************************************
*/
bool askForYesOrNo(std::string question)
{
    std::string ans;
    std::cout << question << " [Yn]" << std::endl;
    while(std::getline(std::cin, ans))
    {
        if(ans.empty() || ans == "y" || ans == "Y")
            return true;
        else if(ans == "n" || ans == "N")
            return false;
        else
            std::cout << ans << " wrong option, choose among [Yn]" << std::endl;
    }
    return false;
}

/*!
************************************************
* Wait for specific string input.
************************************************
*/
bool askForSpecificInput(std::string question, std::string expectedAnswer)
{
    std::string ans;
    std::cout << question << " " << expectedAnswer << " n otherwise" << std::endl;
    while(std::getline(std::cin, ans))
    {
        if(ans == expectedAnswer)
            return true;
        else if(ans == "n" || ans == "N")
            return false;
        else
            std::cout << ans << " wrong option, choose among " << expectedAnswer << " and n" << std::endl;
    }
    return false;
}

/*!
************************************************
* Argument parser.
************************************************
*/
po::variables_map process_program_options(const int argc, const char* const argv[])
{
    po::options_description desc("Allowed options");

    desc.add_options()("help,h", "produce help message")

        ("config,c",
         po::value<string>()->default_value("default"),
         "set configuration file path (default files defined for each test) "
         "...")

            ("verbose,v", po::value<string>()->implicit_value("0"), "verbosity level");

    po::variables_map vm;
    try
    {
        po::store(po::parse_command_line(argc, argv, desc), vm);
    }
    catch(po::error const& e)
    {
        std::cerr << e.what() << '\n';
        exit(EXIT_FAILURE);
    }
    po::notify(vm);

    // Help
    if(vm.count("help"))
    {
        cout << desc << "\n";
        return 0;
    }

    // Power supply object option
    if(vm.count("object")) { cout << "Object to initialize set to " << vm["object"].as<string>() << endl; }

    return vm;
}

/*!
 ************************************************
 * Main.
 ************************************************
 */
int main(int argc, char* argv[])
{
    boost::program_options::variables_map v_map = process_program_options(argc, argv);
    std::cout << "Initializing ..." << std::endl;

    fSaveFile                     = new std::ofstream;
    fVerbose                      = v_map.count("verbose");
    std::string        configPath = v_map["config"].as<string>();
    pugi::xml_document settings;
    if(configPath == "default") configPath = "config/ivITsldo.xml";
    std::cout << "configPath" << configPath << std::endl;

    DeviceHandler theHandler;
    theHandler.readSettings(configPath, settings);

    pugi::xml_node testNode               = settings.child("IVCurve");
    pugi::xml_node saveFileNode           = testNode.child("Save");
    pugi::xml_node instrumentNode         = testNode.child("Devices");
    std::string    curveType              = ChildValClean(testNode.child("CurveType").child_value());
    std::string    strSearchTPNode        = "//TestPoints[@Type='" + curveType + "']";
    pugi::xml_node testPointNode          = testNode.select_node(strSearchTPNode.c_str()).node();
    pugi::xml_node settingNode            = testPointNode.child("Settings");
    pugi::xml_node multimeterReadTypeNode = testNode.child(instrumentNode.child("Multimeter").attribute("ReadType").as_string());

    DeviceHandler mulHandler;
    DeviceHandler psHandler;

    try
    {
        ConfigureSaving(&saveFileNode);
        ConfigureInstruments(&instrumentNode, &mulHandler, &psHandler);
        StartMultimeter();
        StartPowerSupply(curveType, &settingNode);
        StartAcquisition(&settingNode, &multimeterReadTypeNode, &instrumentNode);
        if(fSaveFile != NULL && fSaveFile->is_open())
        {
            if(fVerbose) std::cout << "Closing save data file..." << std::endl;
            fSaveFile->close();
            std::cout << ("Copying " + fSaveFileName + " to " + fSaveDir + "lastScan.csv").c_str() << std::endl;
            sleep(1);
            boost::filesystem::copy_file(fSaveFileName.c_str(),                              // From
                                         (fSaveDir + "lastScan.csv").c_str(),                // to
                                         boost::filesystem::copy_option::overwrite_if_exists // Copy options: overwrite if it exist
            );
        }
    }
    catch(const std::out_of_range& oor)
    {
        std::cerr << "Out of Range error: " << oor.what() << '\n';
    }
}
