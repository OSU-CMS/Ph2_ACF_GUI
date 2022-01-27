/*!
 * \authors Monica Scaringella <monica.scaringella@fi.infn.it>, INFN-Firenze
 * \date July 24 2020
 */

// Libraries
#include "DeviceHandler.h"
#include "Keithley.h"
#include "PowerSupply.h"
#include "PowerSupplyChannel.h"
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp> //!For command line arg parsing
#include <errno.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>

// Namespaces
using namespace std;
namespace po = boost::program_options;

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

/*!
************************************************
* Prints present voltage and current
************************************************
*/
void printPresentVoltageAndCurrent(PowerSupplyChannel* psChannel)
{
    std::cout << "Present voltage is: " << psChannel->getSetVoltage() << " V\t current is: " << psChannel->getCurrent() << " A" << std::endl;
}

/*!
************************************************
* Sets polarity.
************************************************
*/
int setPolarity(std::string pol_string)
{
    int polarity;
    if(pol_string == "positive")
        polarity = 1; // positive polarity
    else if(pol_string == "negative")
        polarity = -1; // negative polarity
    else
    {
        std::stringstream error;
        error << "polarity " << pol_string
              << " is not valid, "
                 "please check the xml configuration file.";
        throw std::runtime_error(error.str());
    }
    return polarity;
}

/*!
************************************************
* Checks massimum voltage configuration and
* returns proper vRange.
************************************************
*/
float getVRangeFromVmax(int vMax)
{
    float vRange;
    if(vMax > 1000)
    {
        std::stringstream error;
        error << "vMax " << vMax
              << " is not valid, "
                 "please check the xml configuration file.";
        throw std::runtime_error(error.str());
    }
    else if(vMax <= 0.2)
        vRange = 0.2;
    else if(vMax <= 20)
        vRange = 20;
    else if(vMax <= 200)
        vRange = 200;
    else
        vRange = 1000;

    return vRange;
}

/*!
************************************************
* Safe file opening.
************************************************
*/
FILE* fileOpen(std::string totalFileName)
{
    FILE* f_data = fopen((totalFileName + ".csv").c_str(), "w+");
    if(f_data == 0)
    {
        char              buffer[256];
        char*             errorMsg = strerror_r(errno, buffer, 256);
        std::stringstream error;
        error << "Error while opening " << totalFileName << ": " << errorMsg;
        throw std::runtime_error(error.str());
    }
    return f_data;
}

/*!
************************************************
* Preliminary power supply settings.
************************************************
*/
void preliminaryPSSettings(KeithleyChannel* channel, const int currentIntegration, const int voltageIntegration)
{
    float dummy = 0;
    // channel->setParameter("command OUTP:SMOD NORM", dummy);           // Permits discharging of capacitive load when output is off
    // channel->setParameter("command :SYST:AZER ON", dummy);            // Enables Auto-Zeroing of ADCs
    channel->setParameter("command :source:function volt", dummy);       // Configure source voltage
    channel->setParameter("command :source:volt:mode fixed", dummy);     // Set fixed voltage mode (aka not sweep)
    channel->setParameter("command :sense:function \'curr:dc\'", dummy); // Set measurement to DC current
    // channel->setParameter("command :func:conc on", dummy);            // Turn concurrent mode on, i.e. allow readout of voltage and current simultaneously
    if(currentIntegration >= 0) channel->setParameter("command :sens:curr:nplc " + std::to_string(currentIntegration), dummy); // Set current integration period
    if(voltageIntegration >= 0) channel->setParameter("command :sens:volt:nplc " + std::to_string(voltageIntegration), dummy); // Set voltage integration period
}

/*!
************************************************
* Switches PS off if it was on.
************************************************
*/
void switchPSOff(KeithleyChannel* psChannel, float voltStep, float timeStep)
{
    if(!psChannel->isOn()) return;

    const int polarity = (psChannel->getSetVoltage() > 0) ? 1 : -1;
    voltStep *= polarity;
    float currentVoltage = psChannel->getSetVoltage();

    while(currentVoltage * polarity > fabs(voltStep))
    {
        currentVoltage -= voltStep;
        psChannel->setVoltage(currentVoltage);
        sleep(timeStep);
        currentVoltage = psChannel->getSetVoltage();
        printPresentVoltageAndCurrent(psChannel);
    }
    psChannel->setVoltage(0);
    printPresentVoltageAndCurrent(psChannel);
    psChannel->turnOff();
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

    std::string docPath = v_map["config"].as<string>();

    std::cout << "docPath: " << docPath << std::endl;

    // Taking configuration from xml file
    pugi::xml_document docSettings;

    DeviceHandler theHandler;
    theHandler.readSettings(docPath, docSettings);

    pugi::xml_node options = docSettings.child("Options");
    pugi::xml_node plot    = docSettings.child("Plot");

    // Retriving plot options
    bool        plotFlag      = plot.attribute("PlotFlag").as_bool();
    bool        saveFlag      = plot.attribute("SaveFlag").as_bool();
    std::string saveFormatStr = plot.attribute("SaveFormat").value();
    std::string folderName    = plot.attribute("SaveFolder").value();
    std::string fileName      = plot.attribute("FileName").value();
    std::string fileHeader    = plot.attribute("FileHeader").value();
    bool        mplFlag       = plot.attribute("MatplotlibFlag").as_bool();

    // Retriving measurement options
    float       voltStepSize         = options.attribute("VoltStepSize").as_float();
    float       voltStepSizeRampDown = options.attribute("VoltStepSizeRampDown").as_float();
    float       vMax                 = options.attribute("Vmax").as_float();
    float       waitTime             = options.attribute("waitTimeSeconds").as_float();
    float       compliance           = options.attribute("Compliance").as_float();
    float       currentReadRange     = options.attribute("CurrentReadRangeAmp").as_float();
    int         currentIntegration   = options.attribute("CurrentIntegration").as_int();
    int         voltageIntegration   = options.attribute("VoltageIntegration").as_int();
    int         nPoints              = options.attribute("nPointsPerVolt").as_int();
    std::string pol_string           = options.attribute("Polarity").value();
    std::string panel_string         = options.attribute("Panel").value();

    // Usefull variables
    int   polarity;
    float dummy = 0;
    float voltage, current, currentSquare, stdCurrent;
    float vRange;

    // File handling variables
    FILE*  f_data;
    FILE*  pipe = NULL;
    float* x    = NULL;
    float* y    = NULL;
    float* z    = NULL;

    // Directories and save file handling
    time_t     rawtime;
    struct tm* timeinfo;
    char       dateString[15];
    char       timeString[15];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(dateString, sizeof(dateString), "%Y_%m_%d/", timeinfo);
    strftime(timeString, sizeof(timeString), "%H_%M_%S_", timeinfo);
    folderName += dateString;
    boost::filesystem::path dir(folderName.c_str());
    boost::filesystem::create_directories(dir);
    std::string totalFileName = folderName + timeString + fileName;
    std::string baseFileName  = timeString + fileName;

    std::replace(fileHeader.begin(), fileHeader.end(), ',', '\t');

    // Gnuplot save configuration
    std::stringstream        saveFormatStringStream(saveFormatStr);
    std::vector<std::string> saveFormatVector;
    saveFormatVector.push_back("");
    int vIterator = 0;

    for(char vStr; saveFormatStringStream >> vStr;)
    {
        saveFormatVector.at(vIterator).push_back(vStr);
        if(saveFormatStringStream.peek() == ',')
        {
            saveFormatStringStream.ignore();
            saveFormatVector.push_back("");
            ++vIterator;
        }
    }

    // Printing out configuration
    std::cout << "************* Measurment settings summary *************" << std::endl;
    std::cout << "Polarity = " << pol_string << std::endl;
    std::cout << "Compliance = " << compliance << std::endl;
    std::cout << "Current integration = " << currentIntegration << std::endl;
    std::cout << "Voltage integration = " << voltageIntegration << std::endl;
    std::cout << "Wait time in seconds = " << waitTime << std::endl;
    std::cout << "Step size in volts = " << voltStepSize << std::endl;
    std::cout << "Step size in volts for ramp down = " << voltStepSizeRampDown << std::endl;
    std::cout << "vMax = " << vMax << std::endl;
    std::cout << "panel = " << panel_string << std::endl;
    std::cout << "totalFileName = " << totalFileName << std::endl;
    std::cout << "Save format = " << std::endl;
    for(unsigned int vVectorString = 0; vVectorString < saveFormatVector.size(); ++vVectorString) std::cout << "\t\t" << saveFormatVector.at(vVectorString) << std::endl;
    std::cout << "Header = #" << fileHeader << std::endl;
    std::cout << "************************* END *************************" << std::endl;

    std::vector<float> vVoltagePoints;
    for(int v = 0; v < floor(vMax / voltStepSize); ++v) vVoltagePoints.push_back(voltStepSize * (v + 1));
    if(vVoltagePoints.back() < vMax) vVoltagePoints.push_back(vMax);

    // Power supply and power supply channel initialization
    PowerSupply*        powerSupply;
    PowerSupplyChannel* channelTmp;
    KeithleyChannel*    channel;
    try
    {
        powerSupply = theHandler.getPowerSupply("MyKeithley");
        if(panel_string == "front")
            channelTmp = powerSupply->getChannel("Front");
        else if(panel_string == "rear")
            channelTmp = powerSupply->getChannel("Rear");
        else
        {
            std::stringstream error;
            error << "Panel " << panel_string
                  << " is not valid, "
                     "please check the xml configuration file.";
            throw std::runtime_error(error.str());
        }
        channel = dynamic_cast<KeithleyChannel*>(channelTmp);
        switchPSOff(channel, voltStepSizeRampDown, waitTime);                   // Switches PS off if output is on
        polarity = setPolarity(pol_string);                                     // Setting polarity
        vRange   = getVRangeFromVmax(vMax);                                     // Checks vMax and returns proper vRange
        f_data   = fileOpen(totalFileName);                                     // Opening save file
        preliminaryPSSettings(channel, currentIntegration, voltageIntegration); // Preliminary PS settings
        channel->setCurrentCompliance(compliance);                              // Set compliance
        channel->setParameter("Vsrc_range", vRange);                            // Configure voltage source range

        fprintf(f_data, "#%s\n", fileHeader.c_str());

        // configure current range auto
        if(currentReadRange <= 0)
            channel->setParameter("autorange", true);
        else if(currentReadRange > 0)
        {
            channel->setParameter("autorange", false);
            channel->setParameter("command_arg :sens:curr:range", currentReadRange);
        }

        if(plotFlag)
        {
            int n_points = vVoltagePoints.size();
            x            = new float[n_points];
            y            = new float[n_points];
            z            = new float[n_points];
            pipe         = popen("gnuplot", "w");
        }

        std::cout << "Starting measurement" << std::endl;

        voltStepSize = voltStepSize * polarity;
        vMax         = vMax * polarity;

        if(plotFlag)
        {
            char line_gnu[50];
            fprintf(pipe, "set xrange [0:%f]\n", vMax * 1.1);
            fprintf(pipe, "unset key\n");
            fprintf(pipe, "set autoscale y\n");
            fprintf(pipe, "set offset graph 0.10, 0.10\n");
            sprintf(line_gnu, "set format y \"%%.1e\"\n");
            fprintf(pipe, "%s", line_gnu);
            fprintf(pipe, "set style line 1 linecolor rgb '#0060ad' linetype 1 linewidth 2 pointtype 7 pointsize 1.5\n");
            fprintf(pipe, "set xlabel \"Voltage [V]\"\n");
            fprintf(pipe, "show xlabel\n");
            fprintf(pipe, "set ylabel \"Current [A]\"\n");
            fprintf(pipe, "show ylabel\n");
        }

        channel->turnOn();
        channel->setVoltage(0);
        voltage = channel->getSetVoltage();

        for(unsigned int v = 0; v < vVoltagePoints.size(); ++v)
        {
            float vVoltage = vVoltagePoints.at(v);
            if(vVoltage > vRange) { std::cout << "Error, Voltage out of range" << std::endl; }

            channel->setVoltage(vVoltage);

            sleep(waitTime);
            channel->getCurrent();
            channel->getOutputVoltage();

            current       = 0;
            currentSquare = 0;
            float currentTmp, voltageTmp;

            for(int vRead = 0; vRead < nPoints; ++vRead)
            {
                sleep(0.1);
                currentTmp = channel->getCurrent();
                voltageTmp = channel->getOutputVoltage();
                current += currentTmp;
                voltage += voltageTmp;
                currentSquare += pow(currentTmp, 2);
                std::cout << "Point " << vRead << " current: " << currentTmp << "\tvoltage: " << voltageTmp << std::endl;
                fprintf(f_data, "%e\t%e\t", voltageTmp, currentTmp);
            }
            fprintf(f_data, "\n");
            current /= (float)nPoints;
            voltage /= (float)nPoints;
            stdCurrent = sqrtf((currentSquare / (float)nPoints) - (pow(current, 2)));

            if(plotFlag)
            {
                x[v] = vVoltage;
                y[v] = current;
                z[v] = stdCurrent;

                fprintf(pipe, "plot '-' with yerrorlines\n");

                for(unsigned int ii = 0; ii <= v; ii++) { fprintf(pipe, "%f %e %e\n", x[ii], y[ii], z[ii]); }

                fprintf(pipe, "e\n"); // finally, e
                fflush(pipe);         // flush the pipe to update the plot
            }
        }

        std::cout << "Measurement done, press enter to continue" << std::endl;

        getchar();

        if(plotFlag) fclose(pipe);
        switchPSOff(channel, voltStepSizeRampDown, waitTime);  // Ramping down
        channel->setParameter("command :system:local", dummy); // Putting commands in local mode

        fclose(f_data);
        std::cout << "Copying " + totalFileName + ".csv to " + folderName + "LastIV.csv" << std::endl;
        sleep(1);
        boost::filesystem::copy_file((totalFileName + ".csv").c_str(),                   // From
                                     (folderName + "LastIV.csv").c_str(),                // to
                                     boost::filesystem::copy_option::overwrite_if_exists // Copy options: overwrite if it exist
        );

        if(mplFlag)
        {
            std::cout << "Execution of python script for matplotlib plot of IV" << std::endl;
            int v = system(("plotIVKeithely.py -v -d " + folderName + " -n " + std::to_string(nPoints) + " -k").c_str());
            std::cout << "System return value after execution " << v << std::endl;
            if(saveFlag)
            {
                std::string imageFolderName = folderName + "/Images/";
                std::string imageName       = imageFolderName + baseFileName + ".";
                std::string saveFormat;
                for(unsigned int vSaveType = 0; vSaveType < saveFormatVector.size(); ++vSaveType)
                {
                    saveFormat = saveFormatVector.at(vSaveType);
                    std::cout << "Copying image file " << imageFolderName << "LastIV." + saveFormat + " into " << imageName << saveFormat << std::endl;
                    boost::filesystem::copy_file((imageFolderName + "LastIV." + saveFormat).c_str(), // From
                                                 (imageName + saveFormat).c_str(),                   // to
                                                 boost::filesystem::copy_option::overwrite_if_exists // Copy options: overwrite if it exist
                    );
                }
            }
        }
    }
    catch(const std::out_of_range& oor)
    {
        std::cerr << "Out of Range error: " << oor.what() << '\n';
        throw std::out_of_range(oor.what());
    }
    if(x != nullptr) delete x;
    if(y != nullptr) delete y;
    if(z != nullptr) delete z;
    return 0;
}
