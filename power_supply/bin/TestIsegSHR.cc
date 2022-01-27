/*!
 * \author Massimiliano Marchisone <mmarchis.@cern.ch>
 * \date Jul 30 2020
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
#include "IsegSHR.h"

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
* Reading module identification.
************************************************
*/
void ModuleTest(IsegSHRChannel* IsegSHRchannel)
{
    cout << "Asking for device ID" << endl;
    cout << IsegSHRchannel->getDeviceID() << endl;
    cout << "Asking for module temperature" << endl;
    cout << IsegSHRchannel->getDeviceTemperature() << endl;
}

/*!
************************************************
* Test of HV power cycle.
************************************************
*/
void OnOffTest(IsegSHRChannel* IsegSHRchannel)
{
    cout << "switching on" << endl;
    IsegSHRchannel->turnOn();
    cout << IsegSHRchannel->isOn() << endl;
    wait();
    cout << "switching off" << endl;
    IsegSHRchannel->turnOff();
    cout << IsegSHRchannel->isOn() << endl;
    wait();
    cout << "switching off" << endl;
    IsegSHRchannel->turnOff();
    cout << IsegSHRchannel->isOn() << endl;
    wait();
    cout << "switching on" << endl;
    IsegSHRchannel->turnOn();
    cout << IsegSHRchannel->isOn() << endl;
    wait();
    cout << "switching off" << endl;
    IsegSHRchannel->turnOff();
    cout << IsegSHRchannel->isOn() << endl;
    wait();
    cout << "switching on" << endl;
    IsegSHRchannel->turnOn();
    cout << IsegSHRchannel->isOn() << endl;
    wait();
    cout << "switching on" << endl;
    IsegSHRchannel->turnOn();
    cout << IsegSHRchannel->isOn() << endl;
    wait();
    cout << "switching off" << endl;
    IsegSHRchannel->turnOff();
    cout << IsegSHRchannel->isOn() << endl;
}

/*!
************************************************
* Test of polarity switch.
************************************************
*/
void PolarityTest(IsegSHRChannel* IsegSHRchannel)
{
    cout << "Turning off" << endl;
    IsegSHRchannel->turnOff();
    wait();
    cout << "setting polarity to N" << endl;
    IsegSHRchannel->setPolarity("n");
    cout << IsegSHRchannel->getPolarity() << endl;
    wait();
    cout << "setting polarity to P" << endl;
    IsegSHRchannel->setPolarity("p");
    cout << IsegSHRchannel->getPolarity() << endl;
    wait();
    cout << "setting polarity to N" << endl;
    IsegSHRchannel->setPolarity("n");
    cout << IsegSHRchannel->getPolarity() << endl;
    wait();
    cout << "setting polarity to N" << endl;
    IsegSHRchannel->setPolarity("n");
    cout << IsegSHRchannel->getPolarity() << endl;
    wait();
    cout << "setting polarity to P" << endl;
    IsegSHRchannel->setPolarity("p");
    cout << IsegSHRchannel->getPolarity() << endl;
    wait();
    cout << "setting polarity to N" << endl;
    IsegSHRchannel->setPolarity("n");
    cout << IsegSHRchannel->getPolarity() << endl;
    wait();
    cout << "setting polarity to P" << endl;
    IsegSHRchannel->setPolarity("p");
    cout << IsegSHRchannel->getPolarity() << endl;
    wait();
    cout << "setting polarity to P" << endl;
    IsegSHRchannel->setPolarity("p");
    cout << IsegSHRchannel->getPolarity() << endl;
    wait();
    cout << "setting polarity to N" << endl;
    IsegSHRchannel->setPolarity("n");
    cout << IsegSHRchannel->getPolarity() << endl;
}

/*!
************************************************
* Test of voltage settings and readings.
************************************************
*/
void VoltageTest(IsegSHRChannel* IsegSHRchannel)
{
    cout << "Setting polarity to N" << endl;
    IsegSHRchannel->setPolarity("n");
    cout << IsegSHRchannel->getPolarity() << endl;
    wait();
    cout << "Turning on" << endl;
    IsegSHRchannel->turnOn();
    wait();
    cout << "Setting voltage to 0 V" << endl;
    IsegSHRchannel->setVoltage(0);
    cout << IsegSHRchannel->getSetVoltage() << endl;
    wait();
    cout << IsegSHRchannel->getOutputVoltage() << endl;
    wait();
    cout << "Setting voltage to -100 V" << endl;
    IsegSHRchannel->setVoltage(-100);
    cout << IsegSHRchannel->getSetVoltage() << endl;
    wait();
    cout << IsegSHRchannel->getOutputVoltage() << endl;
    wait();
    cout << "Setting voltage to -200 V" << endl;
    IsegSHRchannel->setVoltage(-200);
    cout << IsegSHRchannel->getSetVoltage() << endl;
    wait();
    cout << IsegSHRchannel->getOutputVoltage() << endl;
    wait();
    cout << "Setting voltage to -300 V" << endl;
    IsegSHRchannel->setVoltage(-300);
    cout << IsegSHRchannel->getSetVoltage() << endl;
    wait();
    cout << IsegSHRchannel->getOutputVoltage() << endl;
    wait();
    cout << "Setting voltage to 0 V" << endl;
    IsegSHRchannel->setVoltage(0);
    cout << IsegSHRchannel->getSetVoltage() << endl;
    wait();
    cout << IsegSHRchannel->getOutputVoltage() << endl;
    wait();
    cout << "Turning off" << endl;
    IsegSHRchannel->turnOff();
    wait();
    cout << "Setting polarity to P" << endl;
    IsegSHRchannel->setPolarity("p");
    cout << IsegSHRchannel->getPolarity() << endl;
    wait();
    cout << "Turning on" << endl;
    IsegSHRchannel->turnOn();
    wait();
    cout << "Setting voltage to +100 V" << endl;
    IsegSHRchannel->setVoltage(100);
    cout << IsegSHRchannel->getSetVoltage() << endl;
    wait();
    cout << IsegSHRchannel->getOutputVoltage() << endl;
    wait();
    cout << "Setting voltage to +200 V" << endl;
    IsegSHRchannel->setVoltage(200);
    cout << IsegSHRchannel->getSetVoltage() << endl;
    wait();
    cout << IsegSHRchannel->getOutputVoltage() << endl;
    wait();
    cout << "Setting voltage to +300 V" << endl;
    IsegSHRchannel->setVoltage(300);
    cout << IsegSHRchannel->getSetVoltage() << endl;
    wait();
    cout << IsegSHRchannel->getOutputVoltage() << endl;
    wait();
    cout << "Setting voltage to 0 V" << endl;
    IsegSHRchannel->setVoltage(0);
    cout << IsegSHRchannel->getSetVoltage() << endl;
    wait();
    cout << IsegSHRchannel->getOutputVoltage() << endl;
    wait();
    cout << "Turning off" << endl;
    IsegSHRchannel->turnOff();
    wait();
    cout << "Getting voltage limits" << endl;
    cout << IsegSHRchannel->getVoltageCompliance() << endl;
    wait();
    cout << IsegSHRchannel->getOverVoltageProtection() << endl;
    wait();
    cout << IsegSHRchannel->getVoltageCompliance() << endl;
    wait();
    cout << IsegSHRchannel->getOverVoltageProtection() << endl;
    wait();
    cout << "Setting voltage limits" << endl;
    IsegSHRchannel->setVoltageCompliance(123);
    wait();
    IsegSHRchannel->setVoltageCompliance(456);
    wait();
    IsegSHRchannel->setOverVoltageProtection(4519);
    wait();
    IsegSHRchannel->setOverVoltageProtection(-4519);
}

void ChannelStatusTest(IsegSHRChannel* IsegSHRchannel)
{
    cout << "Setting polarity to N" << endl;
    IsegSHRchannel->setPolarity("n");
    cout << IsegSHRchannel->getPolarity() << endl;
    wait();
    cout << "Setting voltage to 0 V" << endl;
    IsegSHRchannel->setVoltage(0);
    cout << IsegSHRchannel->getSetVoltage() << endl;
    wait();
    cout << "Turning on" << endl;
    IsegSHRchannel->turnOn();
    wait();
    cout << "Setting voltage to -150 V" << endl;
    IsegSHRchannel->setVoltage(-150);
    cout << IsegSHRchannel->getSetVoltage() << endl;
    wait();
    for(int i = 0; i < 10; i++)
    {
        cout << IsegSHRchannel->getChannelStatus() << "   " << IsegSHRchannel->isRamping() << "   " << IsegSHRchannel->getOutputVoltage() << endl;
        wait();
    }

    cout << "Turning off" << endl;
    IsegSHRchannel->turnOff();
    wait();
    for(int i = 0; i < 10; i++)
    {
        cout << IsegSHRchannel->getChannelStatus() << "   " << IsegSHRchannel->isRamping() << "   " << IsegSHRchannel->getOutputVoltage() << endl;
        wait();
    }
    cout << "Setting voltage to 0 V" << endl;
    IsegSHRchannel->setVoltage(0);
    cout << IsegSHRchannel->getSetVoltage() << endl;
    wait();
    cout << IsegSHRchannel->getChannelStatus() << endl;
}

/*!
************************************************
* Test of voltage ramp-up and ramp-down settings.
************************************************
*/
void RampTest(IsegSHRChannel* IsegSHRchannel)
{
    cout << "Reading previous ramp up setting" << endl;
    cout << IsegSHRchannel->getVoltageRampUpSpeed() << endl;
    wait();
    cout << "Reading previous ramp down setting" << endl;
    cout << IsegSHRchannel->getVoltageRampDownSpeed() << endl;
    wait();
    cout << "Setting ramp up value to 10 V/s" << endl;
    IsegSHRchannel->setVoltageRampUpSpeed(10);
    cout << IsegSHRchannel->getVoltageRampUpSpeed() << endl;
    wait();
    cout << "Setting ramp down value to 15 V/s" << endl;
    IsegSHRchannel->setVoltageRampDownSpeed(15);
    cout << IsegSHRchannel->getVoltageRampDownSpeed() << endl;
    wait();
    cout << "Setting ramp up value to -8 V/s" << endl;
    IsegSHRchannel->setVoltageRampUpSpeed(-8);
    cout << IsegSHRchannel->getVoltageRampUpSpeed() << endl;
    wait();
    cout << "Setting ramp down value to -31 V/s" << endl;
    IsegSHRchannel->setVoltageRampDownSpeed(-31);
    cout << IsegSHRchannel->getVoltageRampDownSpeed() << endl;
    wait();
    cout << "Setting ramp up value to 600 V/s" << endl;
    IsegSHRchannel->setVoltageRampUpSpeed(600);
    cout << IsegSHRchannel->getVoltageRampUpSpeed() << endl;
    wait();
    cout << "Setting ramp down value to 500 V/s" << endl;
    IsegSHRchannel->setVoltageRampDownSpeed(500);
    cout << IsegSHRchannel->getVoltageRampDownSpeed() << endl;
}

/*!
************************************************
* Test of current settings and readings.
************************************************
*/
void CurrentTest(IsegSHRChannel* IsegSHRchannel)
{
    cout << "Getting current limits" << endl;
    cout << IsegSHRchannel->getCurrentCompliance() << endl;
    wait();
    cout << IsegSHRchannel->getOverCurrentProtection() << endl;
    wait();
    cout << IsegSHRchannel->getCurrentCompliance() << endl;
    wait();
    cout << IsegSHRchannel->getOverCurrentProtection() << endl;
    wait();
    cout << "Setting polarity to N" << endl;
    IsegSHRchannel->setPolarity("n");
    cout << IsegSHRchannel->getPolarity() << endl;
    wait();
    cout << "Setting voltage to -10 V" << endl;
    IsegSHRchannel->setVoltage(-10);
    cout << IsegSHRchannel->getSetVoltage() << endl;
    wait();
    cout << "Setting current to 0.3 mA" << endl;
    IsegSHRchannel->setCurrent(0.3e-3);
    cout << IsegSHRchannel->getSetCurrent() << endl;
    cout << IsegSHRchannel->getCurrent() << endl;
    wait();
    cout << "Setting polarity to P" << endl;
    IsegSHRchannel->setPolarity("p");
    cout << IsegSHRchannel->getPolarity() << endl;
    wait();
    cout << "Setting voltage to 15 V" << endl;
    IsegSHRchannel->setVoltage(15);
    cout << IsegSHRchannel->getSetVoltage() << endl;
    wait();
    cout << "Setting current to 0.4 mA" << endl;
    IsegSHRchannel->setCurrent(0.4e-3);
    cout << IsegSHRchannel->getSetCurrent() << endl;
    cout << IsegSHRchannel->getCurrent() << endl;
    wait();
    cout << "Setting current to -4.2 mA" << endl;
    IsegSHRchannel->setCurrent(-4.2e-3);
    cout << IsegSHRchannel->getSetCurrent() << endl;
    cout << IsegSHRchannel->getCurrent() << endl;
    wait();
    cout << "Setting current limits" << endl;
    IsegSHRchannel->setCurrentCompliance(5e-3);
    wait();
    IsegSHRchannel->setCurrentCompliance(3e-4);
    wait();
    IsegSHRchannel->setOverCurrentProtection(5e-3);
    wait();
    IsegSHRchannel->setOverCurrentProtection(3e-4);
}

/*!
************************************************
* Test of reset command (turn all HV off, set all HV to 0, set all currents to nominal).
************************************************
*/
void ResetTest(IsegSHRChannel* IsegSHRchannel0, IsegSHRChannel* IsegSHRchannel1)
{
    cout << "Setting polarity to N" << endl;
    IsegSHRchannel0->setPolarity("n");
    cout << IsegSHRchannel0->getPolarity() << endl;
    IsegSHRchannel1->setPolarity("n");
    cout << IsegSHRchannel1->getPolarity() << endl;
    wait();
    cout << "Setting voltage to -30 V" << endl;
    IsegSHRchannel0->setVoltage(-30);
    cout << IsegSHRchannel0->getSetVoltage() << endl;
    IsegSHRchannel1->setVoltage(-30);
    cout << IsegSHRchannel1->getSetVoltage() << endl;
    wait();
    cout << "Setting current to 1 mA" << endl;
    IsegSHRchannel0->setCurrent(1e-3);
    cout << IsegSHRchannel0->getSetCurrent() << endl;
    IsegSHRchannel1->setCurrent(1e-3);
    cout << IsegSHRchannel1->getSetCurrent() << endl;
    wait();
    cout << "Switching on" << endl;
    IsegSHRchannel0->turnOn();
    cout << IsegSHRchannel0->isOn() << endl;
    IsegSHRchannel1->turnOn();
    cout << IsegSHRchannel1->isOn() << endl;
    wait();
    cout << "Reset" << endl;
    IsegSHRchannel0->reset(); // one command for all channels
}

/*!
************************************************
* Test more than 1 module.
************************************************
*/
void TestTwoModules(IsegSHRChannel* IsegSHRchannel0, IsegSHRChannel* IsegSHRchannel1)
{
    ModuleTest(IsegSHRchannel0);
    cout << endl;
    ModuleTest(IsegSHRchannel1);

    cout << "switching on ch0" << endl;
    IsegSHRchannel0->turnOn();
    cout << IsegSHRchannel0->isOn() << endl;
    wait();
    cout << "switching on ch1" << endl;
    IsegSHRchannel1->turnOn();
    cout << IsegSHRchannel1->isOn() << endl;
    wait();
    cout << "switching off ch0" << endl;
    IsegSHRchannel0->turnOff();
    cout << IsegSHRchannel0->isOn() << endl;
    wait();
    cout << "switching off ch1" << endl;
    IsegSHRchannel1->turnOff();
    cout << IsegSHRchannel1->isOn() << endl;
    wait();
}

/*!
************************************************
* I vs V curve.
************************************************
*/
void IVCurve(IsegSHRChannel* IsegSHRchannel)
{
    time_t     t   = time(0);
    struct tm* now = localtime(&t);
    char       fileName[80];
    strftime(fileName, 80, "../../Test/IVscan/IVscan_%y%m%d_%H%M%S.txt", now);
    ofstream outFile;
    outFile.open(fileName);

    const useconds_t sleeping_time = 1e6;   /// allowed range [0,1e6]
    const float      Vmin          = -300;  /// it must be negative, in V
    const float      deltaV        = 10;    /// in V
    const float      iLim          = -1e-6; /// min current tolerated in A (it must be negative)

    if(IsegSHRchannel->getPolarity() != "n")
    {
        if(IsegSHRchannel->isOn() == 1) { IsegSHRchannel->turnOff(); }
        IsegSHRchannel->setPolarity("n");
    }

    if(IsegSHRchannel->getSetVoltage() != 0.) { IsegSHRchannel->setVoltage(0.); }

    if(IsegSHRchannel->getVoltageRampUpSpeed() != 20) { IsegSHRchannel->setVoltageRampUpSpeed(20); }

    if(IsegSHRchannel->getVoltageRampDownSpeed() != 20) { IsegSHRchannel->setVoltageRampDownSpeed(20); }

    cout << "***********************************" << endl;
    cout << "High voltage power supply settings:" << endl;
    cout << "Polarity: " << IsegSHRchannel->getPolarity() << endl;
    cout << "Set voltage (V): " << IsegSHRchannel->getSetVoltage() << endl;
    cout << "Ramp up speed (V/s): " << IsegSHRchannel->getVoltageRampUpSpeed() << endl;
    cout << "Ramp down speed (V/s): " << IsegSHRchannel->getVoltageRampDownSpeed() << endl;
    cout << "Current limit (A): " << IsegSHRchannel->getOverCurrentProtection() << endl;
    cout << "***********************************" << endl << endl;

    wait();

    IsegSHRchannel->turnOn();
    cout << "Ready to start the I(V) scan" << endl;

    wait();

    float Vset = 0.;
    while(Vset >= Vmin) /// >= because Vmin is negative
    {
        IsegSHRchannel->setVoltage(Vset);
        usleep(6e5);
        while(IsegSHRchannel->isRamping()) { usleep(sleeping_time); }

        cout << "Waiting for 4 s" << endl;
        usleep(sleeping_time);
        usleep(sleeping_time);
        usleep(sleeping_time);
        usleep(sleeping_time);

        cout << IsegSHRchannel->getSetVoltage() << "\t" << IsegSHRchannel->getOutputVoltage() << "\t" << IsegSHRchannel->getCurrent() << endl;
        outFile << IsegSHRchannel->getSetVoltage() << "\t" << IsegSHRchannel->getOutputVoltage() << "\t" << IsegSHRchannel->getCurrent() << endl;

        if(IsegSHRchannel->getCurrent() <= iLim) /// <= because current is negative
        {
            cout << "Current too high (limit is " << iLim << " A), aborting scan" << endl;
            break;
        }

        Vset -= deltaV; /// voltage must be negative
    }

    Vset = Vmin;
    while(Vset <= 0) /// <= because Vset is negative
    {
        IsegSHRchannel->setVoltage(Vset);
        usleep(6e5);
        while(IsegSHRchannel->isRamping()) { usleep(sleeping_time); }

        cout << "Waiting for 4 s" << endl;
        usleep(sleeping_time);
        usleep(sleeping_time);
        usleep(sleeping_time);
        usleep(sleeping_time);

        cout << IsegSHRchannel->getSetVoltage() << "\t" << IsegSHRchannel->getOutputVoltage() << "\t" << IsegSHRchannel->getCurrent() << endl;
        outFile << IsegSHRchannel->getSetVoltage() << "\t" << IsegSHRchannel->getOutputVoltage() << "\t" << IsegSHRchannel->getCurrent() << endl;

        if(IsegSHRchannel->getCurrent() <= iLim) /// <= because current is negative
        {
            cout << "Current too high (limit is " << iLim << " A), aborting scan" << endl;
            break;
        }

        Vset += deltaV; /// voltage must be negative
    }

    outFile.close();

    cout << endl << "I(V) scan completed, ramping down" << endl;
    IsegSHRchannel->setVoltage(0.);
    while(IsegSHRchannel->isRamping()) usleep(sleeping_time);

    IsegSHRchannel->turnOff();
    cout << "Results are in file " << fileName << endl;
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
        theHandler.getPowerSupply("MyIsegSHR4220");
    }
    catch(const std::out_of_range& oor)
    {
        std::cerr << "Out of Range error: " << oor.what() << '\n';
    }

    PowerSupply*        PS              = theHandler.getPowerSupply("MyIsegSHR4220");
    IsegSHR*            myIsegSHR       = dynamic_cast<IsegSHR*>(PS);
    PowerSupplyChannel* channel0        = myIsegSHR->getChannel("HV_Module1"); // ID = 0
    IsegSHRChannel*     IsegSHRchannel0 = dynamic_cast<IsegSHRChannel*>(channel0);

    //  PowerSupplyChannel* channel1        = myIsegSHR->getChannel("HV_Module2"); // ID = 1
    //  IsegSHRChannel*     IsegSHRchannel1 = dynamic_cast<IsegSHRChannel*>(channel1);

    cout << "---------------------------------------------------" << endl;

    //  ModuleTest(IsegSHRchannel0);
    //  OnOffTest(IsegSHRchannel0);
    //  PolarityTest(IsegSHRchannel0);
    //  VoltageTest(IsegSHRchannel0);
    //  ChannelStatusTest(IsegSHRchannel0);
    //  TestTwoModules(IsegSHRchannel0,IsegSHRchannel1);
    //  CurrentTest(IsegSHRchannel0);
    //  RampTest(IsegSHRchannel0);
    //  ResetTest(IsegSHRchannel0,IsegSHRchannel1);
    IVCurve(IsegSHRchannel0);

    return 0;
}
