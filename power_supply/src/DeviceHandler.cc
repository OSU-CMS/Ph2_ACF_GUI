/*!
 * \authors Lorenzo Uplegger <uplegger@fnal.gov>, Fermilab
 * \authors Antonio Cassese <antonio.cassese@cern.ch>, INFN-Firenze
 * \authors Stefan Maier <s.maier@kit.edu>, KIT-Karlsruhe
 * \date jul 23 2020
 */

#include "DeviceHandler.h"
#include "CAENelsFastPS.h"
#include "Multimeter.h"
#include "PowerSupply.h"
#include "Scope.h"

#ifdef __CAEN__
#include "CAEN.h"
#endif
#include "ArduinoKIRA.h"
#include "Hameg7044.h"
#include "IsegSHR.h"
#include "Keithley.h"
#include "KeithleyMultimeter.h"
#include "KeySight.h"
#include "RohdeSchwarz.h"
#include "ScopeAgilent.h"
#include "TTi.h"
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept> // std::out_of_range

/*!
************************************************
* DeviceHandler constructor.
************************************************
*/
DeviceHandler::DeviceHandler() {}

/*!
************************************************
* DeviceHandler distructor.
************************************************
*/
DeviceHandler::~DeviceHandler()
{
    for(auto it: fPowerSupplyMap) delete it.second;
    fPowerSupplyMap.clear();

    for(auto it: fArduinoMap) delete it.second;
    fArduinoMap.clear();

    for(auto it: fMultimeterMap) delete it.second;
    fMultimeterMap.clear();

    for(auto it: fScopeMap) delete it.second;
    fScopeMap.clear();
}

/*!
************************************************
 * Load xml file.
 \param docPath Path to the xml input file.
 \param doc Pugi document to be uploaded.
 \return Pugi parser result.
************************************************
*/
pugi::xml_parse_result DeviceHandler::loadXml(const std::string& docPath, pugi::xml_document& doc)
{
    pugi::xml_parse_result result = doc.load_file(docPath.c_str());
    return result;
}

/*!
****************************** ******************
 * Load xml file error handler.
 \param docPath Input xml file name.
 \param result Pugi parser result.
 \return False if no error occurred.
************************************************
*/
bool DeviceHandler::loadXmlErrorHandler(const std::string& docPath, const pugi::xml_parse_result& result)
{
    std::cout << "Xml parse results for " << docPath << std::endl << "Parse error: " << result.description() << ", character pos = " << result.offset << std::endl;
    return false;
}

void DeviceHandler::readSettings(const std::string& docPath, pugi::xml_document& docSettings, bool verbose)
{
    pugi::xml_parse_result result = DeviceHandler::loadXml(docPath, docSettings);
    if(verbose)
    {
        if(result) { std::cout << "Xml config file loaded: " << result.description() << std::endl; }
        else
        {
            DeviceHandler::loadXmlErrorHandler(docPath, result);
        }
    }
    fDocumentRoot = docSettings.child("Devices");

    // I do not know why but somehow the loop for the Arduinos must be made BEFORE the loop for the power supplies.....  (Stefan Sep 2020)
    for(pugi::xml_node arduino = fDocumentRoot.child("Arduino"); arduino; arduino = arduino.next_sibling("Arduino"))
    {
        std::string inUse = arduino.attribute("InUse").value();
        if(inUse.compare("Yes") != 0 && inUse.compare("yes") != 0) continue;
        std::string id    = arduino.attribute("ID").value();
        std::string model = arduino.attribute("Model").value();

        if(model.compare("ArduinoKIRA") == 0) { fArduinoMap.emplace(id, new ArduinoKIRA(arduino)); }
        else
        {
            std::stringstream error;
            error << "The Model: " << model
                  << " is not an available arduino and won't be initialized, "
                     "please check the xml configuration file.";
            throw std::runtime_error(error.str());
        }
    }

    for(pugi::xml_node multimeter = fDocumentRoot.child("Multimeter"); multimeter; multimeter = multimeter.next_sibling("Multimeter"))
    {
        std::string inUse = multimeter.attribute("InUse").value();
        if(inUse.compare("Yes") != 0 && inUse.compare("yes") != 0) continue;
        std::string id    = multimeter.attribute("ID").value();
        std::string model = multimeter.attribute("Model").value();

        if(model.compare("KeithleyMultimeter") == 0) { fMultimeterMap.emplace(id, new KeithleyMultimeter(multimeter)); }
        else
        {
            std::stringstream error;
            error << "The Model: " << model
                  << " is not an available multimeter and won't be initialized, "
                     "please check the xml configuration file.";
            throw std::runtime_error(error.str());
        }
    }
    for(pugi::xml_node scope = fDocumentRoot.child("Scope"); scope; scope = scope.next_sibling("Scope"))
    {
        std::string inUse = scope.attribute("InUse").value();
        if(inUse.compare("Yes") != 0 && inUse.compare("yes") != 0) continue;
        std::string id    = scope.attribute("ID").value();
        std::string model = scope.attribute("Model").value();
        if(model.compare("Agilent") == 0)
            fScopeMap.emplace(id, new ScopeAgilent(scope));
        else
        {
            std::stringstream error;
            error << "The Model: " << model
                  << " is not an scope supply and won't be initialized, "
                     "please check the xml configuration file.";
            throw std::runtime_error(error.str());
        }
    }
    for(pugi::xml_node powerSupply = fDocumentRoot.child("PowerSupply"); powerSupply; powerSupply = powerSupply.next_sibling("PowerSupply"))
    {
        std::string inUse = powerSupply.attribute("InUse").value();
        if(inUse.compare("Yes") != 0 && inUse.compare("yes") != 0) continue;
        std::string id    = powerSupply.attribute("ID").value();
        std::string model = powerSupply.attribute("Model").value();
        if(model.compare("CAENelsFastPS") == 0) { fPowerSupplyMap.emplace(id, new CAENelsFastPS(powerSupply)); }
#ifdef __CAEN__
        else if(model.compare("CAEN") == 0)
        {
            fPowerSupplyMap.emplace(id, new CAEN(powerSupply));
        }
#endif
        else if(model.compare("TTi") == 0)
        {
            fPowerSupplyMap.emplace(id, new TTi(powerSupply));
        }
        else if(model.compare("RohdeSchwarz") == 0)
        {
            fPowerSupplyMap.emplace(id, new RohdeSchwarz(powerSupply));
        }
        else if(model.compare("Hameg7044") == 0)
        {
            fPowerSupplyMap.emplace(id, new Hameg7044(powerSupply));
        }
        else if(model.compare("Keithley") == 0)
        {
            fPowerSupplyMap.emplace(id, new Keithley(powerSupply));
        }
        else if(model.compare("KeySight") == 0)
        {
            fPowerSupplyMap.emplace(id, new KeySight(powerSupply));
        }
        else if(model.compare("IsegSHR4220") == 0)
        {
            fPowerSupplyMap.emplace(id, new IsegSHR(powerSupply));
        }
        // else if (std::strcmp(Model.c_str(), "Keithley2410") == 0) {
        //   ps_map.emplace(ID, new Keithley2410(PSparameters));
        // }
        else
        {
            std::stringstream error;
            error << "The Model: " << model
                  << " is not an available power supply and won't be initialized, "
                     "please check the xml configuration file.";
            throw std::runtime_error(error.str());
        }
    }
}

PowerSupply* DeviceHandler::getPowerSupply(const std::string& id)
{
    if(fPowerSupplyMap.find(id) == fPowerSupplyMap.end()) { throw std::out_of_range("No power supply with id " + id + " has been configured!"); }
    return fPowerSupplyMap.find(id)->second;
}

Arduino* DeviceHandler::getArduino(const std::string& id)
{
    if(fArduinoMap.find(id) == fArduinoMap.end()) { throw std::out_of_range("No arduino with id " + id + " has been configured!"); }
    return fArduinoMap.find(id)->second;
}

Multimeter* DeviceHandler::getMultimeter(const std::string& id)
{
    if(fMultimeterMap.find(id) == fMultimeterMap.end()) { throw std::out_of_range("No multimeter with id " + id + " has been configured!"); }
    return fMultimeterMap.find(id)->second;
}

Scope* DeviceHandler::getScope(const std::string& id)
{
    if(fScopeMap.find(id) == fScopeMap.end()) { throw std::out_of_range("No scope with id " + id + " has been configured!"); }
    return fScopeMap.find(id)->second;
}

std::vector<std::string> DeviceHandler::getReadoutList()
{
    std::vector<std::string> readoutIdList;
    for(const auto& powerSupplyId: fPowerSupplyMap)
    {
        for(const auto& channelId: powerSupplyId.second->getChannelList())
        {
            readoutIdList.push_back(powerSupplyId.first + "_" + channelId + "_Voltage");
            readoutIdList.push_back(powerSupplyId.first + "_" + channelId + "_Current");
        }
    }
    return readoutIdList;
}

std::vector<std::string> DeviceHandler::getStatus()
{
    std::vector<std::string> readoutValueList;
    for(const auto& powerSupplyId: fPowerSupplyMap)
    {
        for(const auto& channelId: powerSupplyId.second->getChannelList())
        {
            readoutValueList.push_back(powerSupplyId.first + "_" + channelId + "_Voltage:" + std::to_string(powerSupplyId.second->getChannel(channelId)->getOutputVoltage()));
            std::ostringstream currentStringStream;
            currentStringStream << std::fixed << std::setprecision(11) << powerSupplyId.second->getChannel(channelId)->getCurrent();
            readoutValueList.push_back(powerSupplyId.first + "_" + channelId + "_Current:" + currentStringStream.str());
        }
    }
    return readoutValueList;
}
