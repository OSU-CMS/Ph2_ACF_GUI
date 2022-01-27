/*!
 * \authors Antonio Cassese <antonio.cassese@fi.infn.it>, INFN-Firenze
 * \date November 09 2020
 */

// Libraries
#include "DeviceHandler.h"
#include "PowerSupply.h"
#include "PowerSupplyChannel.h"
#include <boost/program_options.hpp> //!For command line arg parsing
#include <errno.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>

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
* Prints present voltage and current
************************************************
*/
void printPresentVoltageAndCurrent(PowerSupplyChannel* psChannel)
{
    std::cout << "Present voltage is: " << psChannel->getSetVoltage() << " V\t current is: " << psChannel->getCurrent() << " A" << std::endl;
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
* Ramp down routine.
************************************************
*/
void rampPSDown(PowerSupplyChannel* psChannel, float voltStep, float timeStep)
{
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
        if(fabs(psChannel->getCurrent()) > fabs(psChannel->getCurrentCompliance()))
            if(!askForYesOrNo("Present current higher than Compliance, do you want to continue anyway?")) return;
    }
    psChannel->setVoltage(0);
    printPresentVoltageAndCurrent(psChannel);
}

/*!
************************************************
* Ramp up routine.
************************************************
*/
void rampPSUp(PowerSupplyChannel* psChannel, float voltStep, float timeStep, float voltageFinalValue)
{
    float     currentVoltage = psChannel->getSetVoltage();
    const int polarity       = (voltageFinalValue > currentVoltage) ? 1 : -1;
    voltStep *= polarity;
    const float voltageFinalValueLastStep = voltageFinalValue - voltStep;

    while(fabs(currentVoltage) < fabs(voltageFinalValueLastStep))
    {
        currentVoltage += voltStep;
        psChannel->setVoltage(currentVoltage);
        sleep(timeStep);
        currentVoltage = psChannel->getSetVoltage();
        printPresentVoltageAndCurrent(psChannel);
        if(fabs(psChannel->getCurrent()) > fabs(psChannel->getCurrentCompliance()))
            if(!askForYesOrNo("Present current higher than Compliance, do you want to continue anyway?")) return;
    }
    psChannel->setVoltage(voltageFinalValue);
    printPresentVoltageAndCurrent(psChannel);
}

/*!
************************************************
* Switch off of bias and low voltage.
************************************************
*/
void startQuestion()
{
    std::cout << "What do you want to do? Choose among the following options:" << std::endl
              << "[0] Switch first off then on" << std::endl
              << "[1] Switch off only" << std::endl
              << "[2] Switch on only" << std::endl
              << "[3] Exit program" << std::endl;
}

/*!
 ************************************************
 * Switch off of bias and low voltage.
 ************************************************
 */
bool switchOff(PowerSupplyChannel* lowVoltagePSChannel, PowerSupplyChannel* biasVoltagePSChannel, float biasVoltageVoltsStep, float biasVoltageTimeStep)
{
    if(biasVoltagePSChannel->isOn())
    {
        if(biasVoltagePSChannel->getSetVoltage() != 0)
        {
            bool flagRampDown = askForYesOrNo("The bias PS output is not zero, do you want to turn it to zero?");
            if(flagRampDown)
            {
                std::cout << "Ramping bias PS to 0V..." << std::endl;
                rampPSDown(biasVoltagePSChannel, biasVoltageVoltsStep, biasVoltageTimeStep);
            }
            else
            {
                std::cout << "You choose not to switch off bias PS, exiting program execution..." << std::endl;
                return 0;
            }
        }
    }
    else
    {
        if(biasVoltagePSChannel->getSetVoltage() != 0)
        {
            std::cout << "Bias power supply is off, but with voltage set to " << biasVoltagePSChannel->getSetVoltage() << " ... Setting it to 0" << std::endl;
            biasVoltagePSChannel->setVoltage(0);
        }
    }
    bool fSwitchOff = askForYesOrNo("Bias voltage is now off, do you want to turn low voltage off?");
    if(fSwitchOff)
    {
        lowVoltagePSChannel->turnOff();
        return true;
    }
    else
    {
        std::cout << "You choose not to switch off low voltage PS... Exiting program execution..." << std::endl;
        return false;
    }
    return false;
}

/*!
************************************************
* Switch on of bias and low voltage.
************************************************
*/
bool switchOn(PowerSupplyChannel* lowVoltagePSChannel, PowerSupplyChannel* biasVoltagePSChannel, float biasVoltageVoltsStep, float biasVoltageTimeStep, float biasVoltageVolts)
{
    std::cout << "Switching on low voltage" << std::endl;
    lowVoltagePSChannel->turnOn();
    bool flagRampUp = askForYesOrNo("Low voltage is on, do you want to switch bias voltage on?");
    if(flagRampUp)
    {
        if(!biasVoltagePSChannel->isOn())
        {
            if(!askForYesOrNo("Bias voltage ps output is off, do you want to turn it on?"))
            {
                std::cout << "Bias voltage remains off, terminating program execution..." << std::endl;
                return false;
            }
            else
            {
                biasVoltagePSChannel->setVoltage(0);
                biasVoltagePSChannel->turnOn();
            }
        }
        std::cout << "Switching on bias voltage up to " << biasVoltageVolts << " volts" << std::endl;
        rampPSUp(biasVoltagePSChannel, biasVoltageVoltsStep, biasVoltageTimeStep, biasVoltageVolts);
        return true;
    }
    else
    {
        std::cout << "You choose not to switch on bias voltage." << std::endl;
        return false;
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

    std::string        configPath = v_map["config"].as<string>();
    pugi::xml_document settings;
    std::cout << "configPath" << configPath << std::endl;

    DeviceHandler theHandler;
    theHandler.readSettings(configPath, settings);

    // pugi::xml_node fDocumentRoot = settings.child("Devices");
    pugi::xml_node options = settings.child("Options");

    float       biasVoltageVoltsStep   = options.attribute("BiasVoltageVoltsStep").as_float();
    float       biasVoltageTimeStep    = options.attribute("BiasVoltageTimeStep").as_float();
    float       biasMicroAmpCompliance = options.attribute("BiasMicroAmpCompliance").as_float() * 1e-6;
    float       biasVoltageVolts       = options.attribute("BiasVoltageVolts").as_float();
    std::string lowVoltagePSName       = options.attribute("LowVoltagePS").as_string();
    std::string biasVoltagePSName      = options.attribute("BiasVoltagePS").as_string();

    std::cout << "LowVoltagePS: " << lowVoltagePSName << std::endl << "BiasVoltagePS: " << biasVoltagePSName << std::endl;

    PowerSupplyChannel* lowVoltagePSChannel;
    PowerSupplyChannel* biasVoltagePSChannel;

    try
    {
        lowVoltagePSChannel  = theHandler.getPowerSupply(lowVoltagePSName)->getChannel("first");
        biasVoltagePSChannel = theHandler.getPowerSupply(biasVoltagePSName)->getChannel("REAR");
    }
    catch(const std::out_of_range& oor)
    {
        std::cerr << "Out of Range error: " << oor.what() << '\n';
        return 0;
    }

    bool        flagSwitchOn  = false;
    bool        flagSwitchOff = false;
    std::string ans;
    startQuestion();
    while(std::getline(std::cin, ans))
    {
        if(ans == "0")
        {
            flagSwitchOn  = true;
            flagSwitchOff = true;
            break;
        }
        else if(ans == "1")
        {
            flagSwitchOn  = false;
            flagSwitchOff = true;
            break;
        }
        else if(ans == "2")
        {
            flagSwitchOn  = true;
            flagSwitchOff = false;
            break;
        }
        else if(ans == "3")
            return 0;
        else
        {
            std::cout << ans << " wrong option, chose among" << std::endl;
            startQuestion();
        }
    }

    // Setting current compliance
    if(biasMicroAmpCompliance != biasVoltagePSChannel->getCurrentCompliance())
    {
        bool fSetCurrentCompliance = askForYesOrNo("Current compliance is not " + std::to_string(biasMicroAmpCompliance) + " A, do you want to set to this value?");
        if(fSetCurrentCompliance)
            biasVoltagePSChannel->setCurrentCompliance(biasMicroAmpCompliance);
        else
        {
            fSetCurrentCompliance = askForYesOrNo("Current compliance is " + std::to_string(biasVoltagePSChannel->getCurrentCompliance()) + " A are you happy with this?");
            if(!fSetCurrentCompliance)
                std::cout << "Please change configuration file " << configPath << " accordingly with the current compliance needed. Terminating program execution..." << std::endl;
            return 0;
        }
    }

    // Switch off
    if(flagSwitchOff)
    {
        if(!switchOff(lowVoltagePSChannel, biasVoltagePSChannel, biasVoltageVoltsStep, biasVoltageTimeStep)) return 0;
        if(flagSwitchOn)
        {
            if(!askForSpecificInput("Bias and low voltage are now off... If you want to switch everything back on please enter", "MM")) { return 0; }
        }
    }
    // Switch on
    if(flagSwitchOn) switchOn(lowVoltagePSChannel, biasVoltagePSChannel, biasVoltageVoltsStep, biasVoltageTimeStep, biasVoltageVolts);
    std::cout << "Going back to local control" << std::endl;
    biasVoltagePSChannel->setParameter("local", true);
    std::cout << "Done" << std::endl;
}
