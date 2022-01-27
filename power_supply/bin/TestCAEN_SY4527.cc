/*!
 * \author Massimiliano Marchisone <mmarchis.@cern.ch>
 * \date Feb 16 2021
 */

// Libraries
#include "DeviceHandler.h"
#include "PowerSupply.h"
#include "PowerSupplyChannel.h"
#include <algorithm>
#include <boost/program_options.hpp> //!For command line arg parsing
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <unistd.h>

// Custom Libraries
#include "CAEN.h"

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
* Argument parser.
************************************************
*/
po::variables_map process_program_options(const int argc, const char* const argv[])
{
    po::options_description desc("Allowed options");

    desc.add_options()("help,h", "produce help message")("config,c",
                                                         po::value<string>()->default_value("default"),
                                                         "set configuration file path (default files defined for each test) "
                                                         "...")("verbose,v", po::value<string>()->implicit_value("0"), "verbosity level");

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
    cout << "Initializing ..." << endl;

    std::string        docPath = v_map["config"].as<string>();
    pugi::xml_document docSettings;

    DeviceHandler theHandler;
    theHandler.readSettings(docPath, docSettings);
    try
    {
        theHandler.getPowerSupply("CAEN_SY4527");
    }
    catch(const std::out_of_range& oor)
    {
        std::cerr << "Out of Range error: " << oor.what() << '\n';
    }

    PowerSupply* PS     = theHandler.getPowerSupply("CAEN_SY4527");
    CAEN*        myCAEN = dynamic_cast<CAEN*>(PS);

    const int           totChannels_LV = 8;
    const int           totChannels_HV = 12;
    PowerSupplyChannel* channel        = 0x0;
    CAENChannel*        CAENChannels_LV[totChannels_LV];
    CAENChannel*        CAENChannels_HV[totChannels_HV];
    char                buffer[20];

    /// declaration of 8 LV channels (1 CAEN module A2519C)
    for(int iCh = 0; iCh < totChannels_LV; iCh++)
    {
        sprintf(buffer, "LV_1-%d", iCh);
        channel              = myCAEN->getChannel(buffer);
        CAENChannels_LV[iCh] = dynamic_cast<CAENChannel*>(channel);
    }

    /// declaration of 12 HV channels (1 CAEN module A7435)
    for(int iCh = 0; iCh < totChannels_HV; iCh++)
    {
        sprintf(buffer, "HV_1-%d", iCh);
        channel              = myCAEN->getChannel(buffer);
        CAENChannels_HV[iCh] = dynamic_cast<CAENChannel*>(channel);
    }

    cout << "---------------------------------------------------" << endl;

    const useconds_t sleeping_time = 1e6; /// allowed range [0,1e6], time in usec
    bool             alarmFlag     = false;
    for(int iCh = 0; iCh < totChannels_LV; iCh++)
    {
        cout << CAENChannels_LV[iCh]->getChannelStatus() << endl;
        if(CAENChannels_LV[iCh]->getChannelStatus() != 0 && alarmFlag == false) alarmFlag = true;
    }

    if(alarmFlag)
    {
        cout << "Clearing alarms..." << endl;
        myCAEN->clearAlarm();
    }

    wait();

    for(int iCh = 0; iCh < totChannels_LV; iCh++)
    {
        CAENChannels_LV[iCh]->turnOn();
        usleep(sleeping_time);
    }

    for(int iCh = 0; iCh < totChannels_HV; iCh++)
    {
        CAENChannels_HV[iCh]->turnOn();
        usleep(sleeping_time);
    }

    bool isBoardRamping = true;
    int  totChannelsRamping;
    while(isBoardRamping)
    {
        totChannelsRamping = 0;
        for(int iCh = 0; iCh < totChannels_HV; iCh++) totChannelsRamping += CAENChannels_HV[iCh]->isChannelRampingUp();
        if(!totChannelsRamping) isBoardRamping = false;
    }

    wait();

    for(int iCh = 0; iCh < totChannels_LV; iCh++) cout << CAENChannels_LV[iCh]->getOutputVoltage() << endl;
    cout << endl;
    for(int iCh = 0; iCh < totChannels_HV; iCh++) cout << CAENChannels_HV[iCh]->getOutputVoltage() << endl;

    wait();

    for(int iCh = 0; iCh < totChannels_LV; iCh++)
    {
        CAENChannels_LV[iCh]->turnOff();
        usleep(sleeping_time);
    }

    for(int iCh = 0; iCh < totChannels_HV; iCh++)
    {
        CAENChannels_HV[iCh]->turnOff();
        usleep(sleeping_time);
    }

    isBoardRamping = true;
    while(isBoardRamping)
    {
        totChannelsRamping = 0;
        for(int iCh = 0; iCh < totChannels_HV; iCh++) totChannelsRamping += CAENChannels_HV[iCh]->isChannelRampingDown();
        if(!totChannelsRamping) isBoardRamping = false;
    }

    wait();

    for(int iCh = 0; iCh < totChannels_LV; iCh++) cout << CAENChannels_LV[iCh]->isOn() << endl;
    cout << endl;
    for(int iCh = 0; iCh < totChannels_HV; iCh++) cout << CAENChannels_HV[iCh]->isOn() << endl;

    wait();
    return 0;
}
