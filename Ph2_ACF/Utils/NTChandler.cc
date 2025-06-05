#include "Utils/NTChandler.h"
#include "Utils/ConsoleColor.h"
#include "Utils/easylogging++.h"
#include <algorithm>
#include <fstream>
#include <vector>

void NTChandler::addNTCtable(const std::string& theNTCtype, const std::string& theNTCtableFileName)
{
    if(fNTCtypeAndFilename.find(theNTCtype) != fNTCtypeAndFilename.end())
    {
        if(fNTCtypeAndFilename[theNTCtype] != theNTCtableFileName)
        {
            LOG(ERROR) << BOLDRED << "Error: NTC type " << theNTCtype << " already loaded from file " << fNTCtypeAndFilename[theNTCtype] << " but trying to reload it from the file "
                       << theNTCtableFileName << ", throwing exception";
            throw std::runtime_error("Trying to reload NTC from a different file");
        }
        return;
    }
    fNTCtypeAndFilename[theNTCtype] = theNTCtableFileName;
    NTCtable& theNTCtable           = fNTCtableMap[theNTCtype];

    std::ifstream file(theNTCtableFileName);
    if(file.is_open())
    {
        std::string line;
        std::string delimiter = ",";
        while(std::getline(file, line))
        {
            // get temp and resistance from line string
            size_t             pos = 0;
            std::string        token;
            std::vector<float> cLineValues(0);
            while((pos = line.find(delimiter)) != std::string::npos)
            {
                token = line.substr(0, pos);
                line.erase(0, pos + delimiter.length());
                cLineValues.push_back(stof(token));
            }
            theNTCtable[cLineValues.at(2)] = cLineValues.at(0);
        }
        file.close();
    }
}

/*!
 * @brief converts the resistance into temperature
 * @param theNTCtype type of the NT as specified in the xml file
 * @param resistance resistance in Ohm
 * @return temperature in Celsius
 */
float NTChandler::getTemperature(const std::string& theNTCtype, float resistance)
{
    float kiloOhmResistance = resistance / 1000.;
    if(fNTCtypeAndFilename.find(theNTCtype) == fNTCtypeAndFilename.end())
    {
        LOG(ERROR) << BOLDRED << "Error: no NTC type " << theNTCtype << " loaded";
        throw std::runtime_error("No NTC data available for requested type");
    }

    NTCtable& theNTCtable = fNTCtableMap[theNTCtype];
    if(kiloOhmResistance < theNTCtable.begin()->first || kiloOhmResistance > theNTCtable.rbegin()->first)
    {
        LOG(WARNING) << BOLDRED << "Out of range resistence = " << kiloOhmResistance << " for NTC type " << theNTCtype << RESET;
        return 999.;
    }

    std::pair<float, float> lowerPoint;
    std::pair<float, float> upperPoint;
    for(const auto& resistanceAndtemperature: theNTCtable)
    {
        if(resistanceAndtemperature.first <= kiloOhmResistance) upperPoint = resistanceAndtemperature;
        if(resistanceAndtemperature.first >= kiloOhmResistance)
        {
            lowerPoint = resistanceAndtemperature;
            break;
        }
    }

    if(upperPoint == lowerPoint) return upperPoint.second;

    float slope     = (upperPoint.second - lowerPoint.second) / (upperPoint.first - lowerPoint.first);
    float intercept = upperPoint.second - slope * upperPoint.first;
    return slope * kiloOhmResistance + intercept;
}

std::string NTChandler::getNTCfile(const std::string& theNTCtype) { return fNTCtypeAndFilename[theNTCtype]; }
