#include "OTTemperature.h"
#include "Utils/NTChandler.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

std::string OTTemperature::fCalibrationDescription = "Read Module temperatures";

OTTemperature::OTTemperature() : OTTool() {}

OTTemperature::~OTTemperature() {}

// Initialization function
void OTTemperature::Initialise()
{
    Prepare();
    SetName("OTTemperature");
}

// State machine control functions
void OTTemperature::Running()
{
    Initialise();
    fSuccess = true;
    ReadModuleTemperatures();
    Reset();
}
// Set current
void OTTemperature::SetCurrents(std::vector<uint8_t> pCurrents)
{
    fCurrentDACs.clear();
    for(auto cCurrent: pCurrents) fCurrentDACs.push_back(cCurrent);
}
void OTTemperature::ReadThermistors(const OpticalGroup* pOpticalGroup)
{
    // auto& clpGBT = pOpticalGroup->flpGBT;
    std::map<std::string, std::string> cNTCMap = pOpticalGroup->getNTCMap();
    // std::map<std::string, std::string>::iterator it;
    for(auto it = cNTCMap.begin(); it != cNTCMap.end(); it++)
    {
        std::string theNTCtype  = it->first;
        std::string adc         = it->second;
        float       temperature = ReadThermistor(pOpticalGroup, adc, theNTCtype);
        LOG(INFO) << BOLDBLUE << theNTCtype << " (" << adc << ") Temperature: " << temperature << "°C" << RESET;
    }
}

// Read thermistor temperature
float OTTemperature::ReadThermistor(const OpticalGroup* pOpticalGroup, std::string pADC, std::string theNTCtype)
{
    auto& theLpGBT = pOpticalGroup->flpGBT;
    if(theLpGBT == nullptr) return -1;

    flpGBTInterface->CdacSetCurrent(theLpGBT, pADC, flpGBTInterface->_CdacCodeToCurrent(theLpGBT, pADC, 0xaa));
    float theResistanceValue = flpGBTInterface->MeasureResistance(theLpGBT, pADC, 1000, true);

    LOG(DEBUG) << "Resistance in Ohms: " << theResistanceValue << RESET;

    float temperature = NTChandler::getInstance().getTemperature(theNTCtype, theResistanceValue);

    LOG(DEBUG) << BOLDBLUE << "NTC Resistance is " << theResistanceValue << " Ohms ---- Temperature of NTC is " << temperature << "°C" << RESET;

    // Current time
    auto               t  = std::time(nullptr);
    auto               tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
    auto cTime = oss.str();

    // Write temperature to file
    std::ofstream cOutputfile;
    int           cOGID           = pOpticalGroup->getId();
    std::string   cOutputfilename = "./Temperatures/Temps_OG" + std::to_string(cOGID) + ".txt";
    cOutputfile.open(cOutputfilename, std::ios_base::app); // append instead of overwrite
    cOutputfile << cTime << "," << temperature << "," << theResistanceValue << "\n";

    return temperature;
}

float OTTemperature::ReadInternalThermistor(const OpticalGroup* pOpticalGroup)
{
    auto& clpGBT = pOpticalGroup->flpGBT;
    if(clpGBT == nullptr) return -1;
    auto cLpgbtTempInADC = flpGBTInterface->GetInternalTemperature(clpGBT);

    uint16_t cOffset = flpGBTInterface->GetADCOffset(clpGBT, 0);
    float    cGain   = flpGBTInterface->GetADCGain(clpGBT, 0);
    LOG(DEBUG) << "Offset: " << +cOffset << " --- Gain: " << cGain << RESET;
    float vPos = (cLpgbtTempInADC - cOffset * (1 - cGain / 2.0)) / (cGain * 512);
    // V = m * T + V0  where V0 is voltage at zero degrees and m is temperature coefficien and T the current temperature, resulting in a Voltage V
    // -> T = (V-V0 ) / m
    std::pair<float, float> coeff = clpGBT->getTemperatureCoefficients();
    float                   m     = coeff.first;
    float                   v0    = coeff.second;
    LOG(DEBUG) << " V0: " << v0 << RESET;
    LOG(DEBUG) << " vPos: " << vPos << RESET;

    float temperature = (vPos - v0) / m;
    LOG(INFO) << BOLDBLUE << "APPROXIMATED internal lpGBT Temperature: " << +temperature << "°C" << RESET;
    return temperature;
}
// Read module temperatures
void OTTemperature::ReadModuleTemperatures()
{
    for(const auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            auto& clpGBT = cOpticalGroup->flpGBT;
            if(clpGBT == nullptr) continue;

            // first some sanity checks
            flpGBTInterface->ConfigureInternalMonitoring(clpGBT, 1);
            std::vector<std::string> cVoltages = {"VREF/2", "VDDIO", "VDD", "VDDA"};
            std::vector<float>       cVoltageADCReadings;
            for(auto cVoltageADC: cVoltages)
            {
                std::vector<float> cMeasurements(0);
                for(uint8_t cIndx = 0; cIndx < 10; cIndx++) { cMeasurements.push_back(flpGBTInterface->ReadADC(clpGBT, cVoltageADC, "VREF/2", fGain)); }
                float cMean    = std::accumulate(cMeasurements.begin(), cMeasurements.end(), 0.) / cMeasurements.size();
                float cVoltage = (cMean) * (fVref / 1023);
                LOG(INFO) << BOLDBLUE << "Gain of " << +fGain << "\t" << cVoltageADC << " ADC reading " << cMean << " converted voltage " << cVoltage << RESET;
                cVoltageADCReadings.push_back(cMean);
            }
            do {
                ReadInternalThermistor(cOpticalGroup);
                flpGBTInterface->ConfigureInternalMonitoring(clpGBT, 0);
                // read ADC value of temperature sensor on the sensor
                ReadThermistors(cOpticalGroup);
            } while(fLoopReadout);
        }
    }
}

void OTTemperature::Stop() {}

void OTTemperature::Pause() {}

void OTTemperature::Resume() {}

void OTTemperature::writeObjects() {}