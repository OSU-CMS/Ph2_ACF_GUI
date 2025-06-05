#include "OTSensorTemperature.h"
#include "Utils/NTChandler.h"
#ifdef __USE_ROOT__
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#endif
using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;
using namespace Ph2_System;

OTSensorTemperature::OTSensorTemperature() : OTTool() {}

OTSensorTemperature::~OTSensorTemperature() {}

// Initialization function
void OTSensorTemperature::Initialise()
{
    Prepare();
    SetName("OTSensorTemperature");
}

// State machine control functions
void OTSensorTemperature::Running()
{
    Initialise();
    fSuccess = true;
    ReadModuleTemperatures();
    Reset();
}

void OTSensorTemperature::ReadThermistors(const OpticalGroup* pOpticalGroup)
{
    std::map<std::string, std::string> cNTCMap = pOpticalGroup->getNTCMap();
    for(auto it = cNTCMap.begin(); it != cNTCMap.end(); it++)
    {
        std::string theNTCtype  = it->first;
        std::string adc         = it->second;
        float       temperature = ReadThermistor(pOpticalGroup, adc, theNTCtype);
        LOG(INFO) << BOLDBLUE << theNTCtype << " (" << adc << ") Temperature: " << temperature << "°C" << RESET;
#ifdef __USE_ROOT__
        if(fOfStream != nullptr)
        {
            json j;
            j["type"]                = "data";
            j["data"]["temperature"] = temperature;
            *(fOfStream) << j << std::endl;
        }
#endif
    }
}

// Read thermistor temperature
float OTSensorTemperature::ReadThermistor(const OpticalGroup* pOpticalGroup, std::string pADC, std::string theNTCtype)
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

// Read module temperatures
void OTSensorTemperature::ReadModuleTemperatures()
{
    for(const auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            auto& clpGBT = cOpticalGroup->flpGBT;
            if(clpGBT == nullptr) continue;
            ReadThermistors(cOpticalGroup);
        }
    }
}

void OTSensorTemperature::Stop() {}

void OTSensorTemperature::Pause() {}

void OTSensorTemperature::Resume() {}

void OTSensorTemperature::writeObjects() {}
