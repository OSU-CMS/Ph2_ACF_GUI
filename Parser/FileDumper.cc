#include "Parser/FileDumper.h"
#include "HWDescription/BeBoard.h"
#include "HWDescription/Definition.h"
#include "HWDescription/Hybrid.h"
#include "HWDescription/OpticalGroup.h"
#include "HWDescription/OuterTrackerHybrid.h"
#include "HWDescription/ReadoutChip.h"
#include "HWDescription/VTRx.h"
#include "HWDescription/lpGBT.h"
#include "Parser/CommunicationSettingConfig.h"
#include "Parser/FileParser.h"
#include "Parser/ParserDefinitions.h"
#include "Utils/ConsoleColor.h"
#include "Utils/Container.h"
#include "Utils/NTChandler.h"
#include <math.h>

using namespace Ph2_HwDescription;

FileDumper::FileDumper(const std::string& outputDirectory)
{
    // Overwrite if GUI is involved
    if(std::getenv("GIPHT_RESULT_FOLDER"))
    {
        LOG(INFO) << "OT Module GUI (GIPHT) result directory environmental variable set: " << std::getenv("GIPHT_RESULT_FOLDER") << RESET;
        fOutputDirectory = std::getenv("GIPHT_RESULT_FOLDER");
        LOG(INFO) << "Use " << fOutputDirectory << " for file dump" << RESET;
    }
    else if(!std::getenv("OTSDAQ_RESULTS_FOLDER")) { fOutputDirectory = std::string(getenv("PH2ACF_BASE_DIR")) + "/" + outputDirectory + "/"; }
}

FileDumper::~FileDumper() {}

std::stringstream FileDumper::dumpConfigurationFiles(DetectorContainer*             theDetectorContainer,
                                                     const Ph2_Parser::SettingsMap& theSettingMap,
                                                     CommunicationSettingConfig*    theCommunicationSettingConfig,
                                                     DetectorMonitorConfig*         theDetectorMonitorConfig)
{
    pugi::xml_document doc;

    // Add a declaration node
    pugi::xml_node declarationNode               = doc.prepend_child(pugi::node_declaration);
    declarationNode.append_attribute("version")  = "1.0";
    declarationNode.append_attribute("encoding") = "utf-8";

    pugi::xml_node hwDescriptionNode = doc.append_child(HW_DESCRIPTION_NODE_NAME);

    for(auto board: *theDetectorContainer) { dumpBoardConfigurationFile(hwDescriptionNode, board); }

    dumpSettings(hwDescriptionNode, theSettingMap);

    dumpMonitorSettings(hwDescriptionNode, theDetectorMonitorConfig);

    dumpCommunicationSettings(hwDescriptionNode, theCommunicationSettingConfig);

    std::string outputFileName = fOutputDirectory + OUTPUT_CONFIGURATION_FILE;

    LOG(INFO) << BOLDBLUE << "Configfiles for all Chips written to " << fOutputDirectory << RESET;

    if(doc.save_file(outputFileName.c_str())) { LOG(INFO) << BOLDYELLOW << "New configuration file saved: " << outputFileName << RESET; }
    else { LOG(ERROR) << BOLDRED << "Error saving the new configuration file." << RESET; }

    std::stringstream finalConfigurationStream;
    doc.save(finalConfigurationStream);
    return finalConfigurationStream;
}

void FileDumper::dumpBoardConfigurationFile(pugi::xml_node theMotherNode, BeBoard* theBoard)
{
    pugi::xml_node theBoardNode                             = theMotherNode.append_child(BEBOARD_NODE_NAME);
    theBoardNode.append_attribute(COMMON_ID_ATTRIBUTE_NAME) = std::to_string(theBoard->getId()).c_str();

    auto theBoardTypeAttribute = theBoardNode.append_attribute(BEBOARD_TYPE_ATTRIBUTE_NAME);
    if(theBoard->getBoardType() == BoardType::D19C)
        theBoardTypeAttribute = BEBOARD_TYPE_ATTRIBUTE_D19C_VALUE;
    else if(theBoard->getBoardType() == BoardType::RD53)
        theBoardTypeAttribute = BEBOARD_TYPE_ATTRIBUTE_RD53_VALUE;
    else
        throw std::runtime_error("FileDumper error: BeBoard type not recognized");

    auto theEventTypeAttribute = theBoardNode.append_attribute(BEBOARD_EVENT_TYPE_ATTRIBUTE_NAME);
    if(theBoard->getEventType() == EventType::ZS)
        theEventTypeAttribute = BEBOARD_EVENT_TYPE_ATTRIBUTE_ZS_VALUE;
    else if(theBoard->getEventType() == EventType::PSAS)
        theEventTypeAttribute = BEBOARD_EVENT_TYPE_ATTRIBUTE_VR_VALUE; // forcing it back to EventType::VR
    // theEventTypeAttribute = BEBOARD_EVENT_TYPE_ATTRIBUTE_PSAS_VALUE;
    else if(theBoard->getEventType() == EventType::VR2S)
        theEventTypeAttribute = BEBOARD_EVENT_TYPE_ATTRIBUTE_VR2S_VALUE;
    else if(theBoard->getEventType() == EventType::VR)
        theEventTypeAttribute = BEBOARD_EVENT_TYPE_ATTRIBUTE_VR_VALUE;
    else
        throw std::runtime_error("FileDumper error: Event type not recognized");

    theBoardNode.append_attribute(BEBOARD_LINKRESET_ATTRIBUTE_NAME)  = (theBoard->getLinkReset() > 0) ? "1" : "0";
    theBoardNode.append_attribute(BEBOARD_BOARDRESET_ATTRIBUTE_NAME) = (theBoard->getReset() > 0) ? "1" : "0";
    theBoardNode.append_attribute(BEBOARD_CONFIGURE_ATTRIBUTE_NAME)  = (theBoard->getToConfigure() > 0) ? "1" : "0";

    pugi::xml_node theBoardConnectionNode                                                    = theBoardNode.append_child(BEBOARD_CONNECTION_NODE_NAME);
    theBoardConnectionNode.append_attribute(BEBOARD_CONNECTION_ID_ATTRIBUTE_NAME)            = theBoard->getConnectionId().c_str();
    theBoardConnectionNode.append_attribute(BEBOARD_CONNECTION_URI_ATTRIBUTE_NAME)           = theBoard->getConnectionUri().c_str();
    theBoardConnectionNode.append_attribute(BEBOARD_CONNECTION_ADDRESS_TABLE_ATTRIBUTE_NAME) = theBoard->getAddressTable().c_str();

    std::string theFileName     = "BE" + std::to_string(theBoard->getId()) + "_Config.xml";
    std::string theFullFileName = fOutputDirectory + theFileName;
    LOG(DEBUG) << BOLDBLUE << "Dumping BeBoard configuration to " << theFullFileName << RESET;
    theBoard->saveRegMap(theFullFileName);

    pugi::xml_node theBoardConfigurationNode                                                   = theBoardNode.append_child(BEBOARD_CONFIGURATION_NODE_NAME);
    theBoardConfigurationNode.append_attribute(BEBOARD_CONFIGURATION_FILE_NAME_ATTRIBUTE_NAME) = theFullFileName.c_str();

    pugi::xml_node theBoardCDCENode                                          = theBoardNode.append_child(BEBOARD_CDCE_NODE_NAME);
    theBoardCDCENode.append_attribute(BEBOARD_CDCE_CONFIGURE_ATTRIBUTE_NAME) = "0"; // Always forcing it to 0 to avoid overriding eprom too many times
    theBoardCDCENode.append_attribute(BEBOARD_CDCE_CLOCKRATE_ATTRIBUTE_NAME) = theBoard->configCDCE().second;

    for(auto opticalGroup: *theBoard) { dumpOpticalGroupConfigurationFile(theBoardNode, opticalGroup); }
}

void FileDumper::dumpOpticalGroupConfigurationFile(pugi::xml_node theMotherNode, OpticalGroup* theOpticalGroup)
{
    pugi::xml_node theOpticalGroupNode                                 = theMotherNode.append_child(OPTICALGROUP_NODE_NAME);
    theOpticalGroupNode.append_attribute(COMMON_ID_ATTRIBUTE_NAME)     = std::to_string(theOpticalGroup->getId()).c_str();
    theOpticalGroupNode.append_attribute(COMMON_ENABLE_ATTRIBUTE_NAME) = "1"; // If it was disabled, it would not be here
    auto theOpticalGroupFMCidAttribute                                 = theOpticalGroupNode.append_attribute(OPTICALGROUP_FMCID_ATTRIBUTE_NAME);
    auto theFMCid                                                      = theOpticalGroup->getFMCId();
    if(theFMCid == 8)
        theOpticalGroupFMCidAttribute = OPTICALGROUP_FMCID_ATTRIBUTE_L8_VALUE;
    else if(theFMCid == 12)
        theOpticalGroupFMCidAttribute = OPTICALGROUP_FMCID_ATTRIBUTE_L12_VALUE;
    else
        throw std::runtime_error("FileDumper error: FMC Id not recognized");
    theOpticalGroupNode.append_attribute(COMMON_RESET_ATTRIBUTE_NAME) = (theOpticalGroup->getReset() > 0) ? "1" : "0";

    auto theNTCmap = theOpticalGroup->getNTCMap();
    for(auto& theNTC: theNTCmap)
    {
        auto theNTCptopertiesNode                                                       = theOpticalGroupNode.append_child(NTCPROPERTIES_NODE_NAME);
        theNTCptopertiesNode.append_attribute(NTCPROPERTIES_TYPE_ATTRIBUTE_NAME)        = theNTC.first.c_str();
        theNTCptopertiesNode.append_attribute(NTCPROPERTIES_ADC_ATTRIBUTE_NAME)         = theNTC.second.c_str();
        theNTCptopertiesNode.append_attribute(NTCPROPERTIES_LOOKUPTABLE_ATTRIBUTE_NAME) = NTChandler::getInstance().getNTCfile(theNTC.first).c_str();
    }

    auto& clpGBT = theOpticalGroup->flpGBT;
    if(clpGBT != nullptr)
    {
        std::string theLpGBTFilePathNodeName                              = std::string(LPGBT_NODE_NAME) + CHIP_FILES_APPEND_NODE_NAME;
        auto        theLpGBTFilePathNode                                  = theOpticalGroupNode.append_child(theLpGBTFilePathNodeName.c_str());
        theLpGBTFilePathNode.append_attribute(COMMON_PATH_ATTRIBUTE_NAME) = fOutputDirectory.c_str();

        auto theLpGBTNode                                           = theOpticalGroupNode.append_child(LPGBT_NODE_NAME);
        theLpGBTNode.append_attribute(COMMON_ID_ATTRIBUTE_NAME)     = std::to_string(clpGBT->getId()).c_str();
        theLpGBTNode.append_attribute(LPGBT_VERSION_ATTRIBUTE_NAME) = std::to_string(clpGBT->getVersion()).c_str();
        theLpGBTNode.append_attribute(LPGBT_OPTICAL_ATTRIBUTE_NAME) = clpGBT->isOptical() ? "1" : "0";

        auto cRegMap = clpGBT->getRegMap();

        std::string theFileName     = "BE" + std::to_string(theOpticalGroup->getBeBoardId()) + "_OG" + std::to_string(theOpticalGroup->getId()) + "_lpGBT" + std::to_string(clpGBT->getId()) + ".txt";
        std::string theFullFileName = fOutputDirectory + theFileName;
        LOG(DEBUG) << BOLDBLUE << "Dumping lpgbt configuration to " << theFullFileName << RESET;
        clpGBT->saveRegMap(theFullFileName);

        theLpGBTNode.append_attribute(COMMON_CONFIGFILE_ATTRIBUTE_NAME) = theFileName.c_str();
    }

    auto& theVTRx = theOpticalGroup->fVTRx;
    if(theVTRx != nullptr)
    {
        std::string theVTRxFilePathNodeName                              = std::string(VTRX_NODE_NAME) + CHIP_FILES_APPEND_NODE_NAME;
        auto        theVTRxFilePathNode                                  = theOpticalGroupNode.append_child(theVTRxFilePathNodeName.c_str());
        theVTRxFilePathNode.append_attribute(COMMON_PATH_ATTRIBUTE_NAME) = fOutputDirectory.c_str();

        auto theVTRxNode                                       = theOpticalGroupNode.append_child(VTRX_NODE_NAME);
        theVTRxNode.append_attribute(COMMON_ID_ATTRIBUTE_NAME) = std::to_string(theVTRx->getId()).c_str();

        auto cRegMap = theVTRx->getRegMap();

        std::string theFileName     = "BE" + std::to_string(theOpticalGroup->getBeBoardId()) + "_OG" + std::to_string(theOpticalGroup->getId()) + "_VTRx" + std::to_string(theVTRx->getId()) + ".txt";
        std::string theFullFileName = fOutputDirectory + theFileName;
        LOG(DEBUG) << BOLDBLUE << "Dumping VTRx configuration to " << theFullFileName << RESET;
        theVTRx->saveRegMap(theFullFileName);

        theVTRxNode.append_attribute(COMMON_CONFIGFILE_ATTRIBUTE_NAME) = theFileName.c_str();
    }

    for(auto hybrid: *theOpticalGroup) { dumpHybridConfigurationFile(theOpticalGroupNode, hybrid); }
}

void FileDumper::dumpHybridConfigurationFile(pugi::xml_node theMotherNode, Hybrid* theHybrid)
{
    pugi::xml_node theHybridNode                                 = theMotherNode.append_child(HYBRID_NODE_NAME);
    theHybridNode.append_attribute(COMMON_ID_ATTRIBUTE_NAME)     = std::to_string(theHybrid->getId() % 2).c_str();
    theHybridNode.append_attribute(COMMON_ENABLE_ATTRIBUTE_NAME) = "1"; // If it was disabled, it would not be here
    theHybridNode.append_attribute(COMMON_RESET_ATTRIBUTE_NAME)  = (theHybrid->getReset() > 0) ? "1" : "0";

    auto& cCic = static_cast<OuterTrackerHybrid*>(theHybrid)->fCic;
    if(cCic != NULL)
    {
        std::string theCICFilePathNodeName                              = std::string(CIC2_NODE_NAME) + CHIP_FILES_APPEND_NODE_NAME;
        auto        theCICFilePathNode                                  = theHybridNode.append_child(theCICFilePathNodeName.c_str());
        theCICFilePathNode.append_attribute(COMMON_PATH_ATTRIBUTE_NAME) = fOutputDirectory.c_str();

        std::string theFileName =
            "BE" + std::to_string(theHybrid->getBeBoardId()) + "_OG" + std::to_string(theHybrid->getOpticalGroupId()) + "_FE" + std::to_string(theHybrid->getId() % 2) + "_CIC.txt";
        std::string theFullFileName = fOutputDirectory + theFileName;
        LOG(DEBUG) << BOLDBLUE << "Dumping CIC configuration to " << theFullFileName << RESET;
        cCic->saveRegMap(theFullFileName);

        std::string CicNodeName                                       = CIC2_NODE_NAME;
        auto        theCICnode                                        = theHybridNode.append_child(CicNodeName.c_str());
        theCICnode.append_attribute(COMMON_ID_ATTRIBUTE_NAME)         = std::to_string(cCic->getId()).c_str();
        theCICnode.append_attribute(COMMON_ENABLE_ATTRIBUTE_NAME)     = "1";
        theCICnode.append_attribute(COMMON_CONFIGFILE_ATTRIBUTE_NAME) = theFileName.c_str();
    }

    bool cWithCBC = (std::find_if(theHybrid->begin(), theHybrid->end(), [](Ph2_HwDescription::Chip* x) { return x->getFrontEndType() == FrontEndType::CBC3; }) != theHybrid->end());
    bool cWithMPA = (std::find_if(theHybrid->begin(), theHybrid->end(), [](Ph2_HwDescription::Chip* x) { return (x->getFrontEndType() == FrontEndType::MPA2); }) != theHybrid->end());
    bool cWithSSA = (std::find_if(theHybrid->begin(), theHybrid->end(), [](Ph2_HwDescription::Chip* x) { return (x->getFrontEndType() == FrontEndType::SSA2); }) != theHybrid->end());

    auto appendReadoutChipConfigFilePath = [this, &theHybridNode](std::string theChipString)
    {
        std::string theCICFilePathNodeName                              = theChipString + CHIP_FILES_APPEND_NODE_NAME;
        auto        theCICFilePathNode                                  = theHybridNode.append_child(theCICFilePathNodeName.c_str());
        theCICFilePathNode.append_attribute(COMMON_PATH_ATTRIBUTE_NAME) = fOutputDirectory.c_str();
    };

    if(cWithCBC) appendReadoutChipConfigFilePath(CBC_NODE_NAME);
    if(cWithMPA) appendReadoutChipConfigFilePath(MPA2_NODE_NAME);
    if(cWithSSA) appendReadoutChipConfigFilePath(SSA2_NODE_NAME);

    for(auto chip: *theHybrid) { dumpChipConfigurationFile(theHybridNode, chip); }

    dumpLpGBTphasesForBypass(theHybridNode, theHybrid);
}

void FileDumper::dumpLpGBTphasesForBypass(pugi::xml_node theMotherNode, Ph2_HwDescription::Hybrid* theHybridContainer)
{
    auto           theCic                = static_cast<OuterTrackerHybrid*>(theHybridContainer)->fCic;
    pugi::xml_node theLpGBTphaseMainNode = theMotherNode.append_child(LPGBT_PHASES_FOR_CIC_BYPASS_MAIN_NODE_NAME);
    for(uint8_t phyPort = 0; phyPort < 12; ++phyPort)
    {
        std::string    thePhyPortNodeName = std::string(LPGBT_PHASES_FOR_CIC_BYPASS_PHYPORT_NODE_NAME) + std::to_string(phyPort);
        pugi::xml_node thePhyPortNode     = theLpGBTphaseMainNode.append_child(thePhyPortNodeName.c_str());
        for(uint8_t stubLine = 0; stubLine < 4; ++stubLine)
        {
            std::string theStubAttributeName                              = std::string(LPGBT_PHASES_FOR_CIC_BYPASS_LINE_ATTRIBUTE_NAME) + std::to_string(stubLine);
            thePhyPortNode.append_attribute(theStubAttributeName.c_str()) = std::to_string(+theCic->getLpGBTphaseForCICbypass(phyPort, stubLine)).c_str();
        }
    }
}

void FileDumper::dumpChipConfigurationFile(pugi::xml_node theMotherNode, ReadoutChip* theReadoutChip)
{
    std::string theReadoutChipNodeName;
    if(theReadoutChip->getFrontEndType() == FrontEndType::CBC3)
        theReadoutChipNodeName = CBC_NODE_NAME;
    else if(theReadoutChip->getFrontEndType() == FrontEndType::MPA2)
        theReadoutChipNodeName = MPA2_NODE_NAME;
    else if(theReadoutChip->getFrontEndType() == FrontEndType::SSA2)
        theReadoutChipNodeName = SSA2_NODE_NAME;
    else
        throw std::runtime_error("FileDumper error: Readout chip type not recognized");

    std::string theFileName = "BE" + std::to_string(theReadoutChip->getBeBoardId()) + "_OG" + std::to_string(theReadoutChip->getOpticalGroupId()) + "_FE" +
                              convertToString(theReadoutChip->getHybridId() % 2) + "_Chip" + convertToString(theReadoutChip->getId());
    if(theReadoutChip->getFrontEndType() == FrontEndType::SSA2) theFileName += "SSA";
    if(theReadoutChip->getFrontEndType() == FrontEndType::MPA2) theFileName += "MPA";
    theFileName += ".txt";
    std::string theFullFileName = fOutputDirectory + theFileName;
    LOG(DEBUG) << BOLDBLUE << "Dumping readout chip configuration to " << theFileName << RESET;
    theReadoutChip->saveRegMap(theFullFileName);

    pugi::xml_node theReadoutChipNode                                     = theMotherNode.append_child(theReadoutChipNodeName.c_str());
    theReadoutChipNode.append_attribute(COMMON_ID_ATTRIBUTE_NAME)         = std::to_string(theReadoutChip->getId()).c_str();
    theReadoutChipNode.append_attribute(COMMON_ENABLE_ATTRIBUTE_NAME)     = "1";
    theReadoutChipNode.append_attribute(COMMON_CONFIGFILE_ATTRIBUTE_NAME) = theFileName.c_str();
    theReadoutChipNode.append_attribute(CHIP_NOISE_ATTRIBUTE_NAME)        = std::to_string(theReadoutChip->getAverageNoise()).c_str();
    theReadoutChipNode.append_attribute(CHIP_PEDESTAL_ATTRIBUTE_NAME)     = std::to_string(theReadoutChip->getAveragePedestal()).c_str();
    if(theReadoutChip->getFrontEndType() == FrontEndType::SSA2 || theReadoutChip->getFrontEndType() == FrontEndType::MPA2)
    {
        theReadoutChipNode.append_attribute(CHIP_ADC_SLOPE_ATTRIBUTE_NAME)          = std::to_string(theReadoutChip->getADCCalibrationValue("ADC_SLOPE")).c_str();
        theReadoutChipNode.append_attribute(CHIP_ADC_OFFSET_ATTRIBUTE_NAME)         = std::to_string(theReadoutChip->getADCCalibrationValue("ADC_OFFSET")).c_str();
        theReadoutChipNode.append_attribute(CHIP_TEMPERATURE_SLOPE_ATTRIBUTE_NAME)  = std::to_string(theReadoutChip->getADCCalibrationValue("TEMP_SLOPE")).c_str();
        theReadoutChipNode.append_attribute(CHIP_TEMPERATURE_OFFSET_ATTRIBUTE_NAME) = std::to_string(theReadoutChip->getADCCalibrationValue("TEMP_OFFSET")).c_str();
    }
}

void FileDumper::dumpSettings(pugi::xml_node theMotherNode, const std::map<std::string, boost::any>& theSettingMap)
{
    pugi::xml_node theSettingMainNode = theMotherNode.append_child(SETTINGS_NODE_NAME);
    for(const auto& theSetting: theSettingMap)
    {
        pugi::xml_node theSettingNode                               = theSettingMainNode.append_child(SETTING_NODE_NAME);
        theSettingNode.append_attribute(COMMON_NAME_ATTRIBUTE_NAME) = theSetting.first.c_str();
        try
        {
            double theValue = boost::any_cast<double>(theSetting.second);
            // check if the value is an integer
            double integerPart;
            modf(theValue, &integerPart);
            if(integerPart == theValue) { theSettingNode.append_child(pugi::node_pcdata).set_value(std::to_string(int(theValue)).c_str()); }
            else
            {
                std::string stringValue = std::to_string(theValue);
                stringValue.erase(stringValue.find_last_not_of('0') + 1, std::string::npos);
                theSettingNode.append_child(pugi::node_pcdata).set_value(stringValue.c_str());
            }
        }
        catch(const std::exception& e)
        {
            try
            {
                // the any_cast to double failed, trying to cast to a string
                theSettingNode.append_child(pugi::node_pcdata).set_value(boost::any_cast<std::string>(theSetting.second).c_str());
            }
            catch(const std::exception& e)
            {
                throw std::runtime_error("FileDumper error: Setting value type not recognized");
            }
        }
    }
}

void FileDumper::dumpMonitorSettings(pugi::xml_node theMotherNode, DetectorMonitorConfig* theDetectorMonitorConfig)
{
    pugi::xml_node theMonitorSettingsNode = theMotherNode.append_child(MONITORINGSETTINGS_NODE_NAME);
    if(theDetectorMonitorConfig->fMonitoringType == MONITORING_NODE_TYPE_ATTRIBUTE_NONE_VALUE) return;

    pugi::xml_node theMonitoringNode                                          = theMonitorSettingsNode.append_child(MONITORING_NODE_NAME);
    theMonitoringNode.append_attribute(MONITORING_NODE_TYPE_ATTRIBUTE_NAME)   = theDetectorMonitorConfig->fMonitoringType.c_str();
    theMonitoringNode.append_attribute(MONITORING_NODE_ENABLE_ATTRIBUTE_NAME) = theDetectorMonitorConfig->fEnable ? "1" : "0";
    pugi::xml_node theMonitoringSleepTimeNode                                 = theMonitoringNode.append_child(MONITORINGSLEEPTIME_NODE_NAME);
    theMonitoringSleepTimeNode.append_child(pugi::node_pcdata).set_value(std::to_string(theDetectorMonitorConfig->fSleepTimeMs).c_str());

    const auto& theMonitorDeviceList = theDetectorMonitorConfig->fMonitorElementList;

    for(const auto& theMonitorElementList: theMonitorDeviceList)
    {
        for(const auto& theElementList: theMonitorElementList.second)
        {
            pugi::xml_node theMonitorElementNode                                              = theMonitoringNode.append_child(MONITORINGELEMENT_NODE_NAME);
            theMonitorElementNode.append_attribute(MONITORINGELEMENT_DEVICE_ATTRIBUTE_NAME)   = theMonitorElementList.first.c_str();
            theMonitorElementNode.append_attribute(MONITORINGELEMENT_REGISTER_ATTRIBUTE_NAME) = theElementList.first.c_str();
            theMonitorElementNode.append_attribute(MONITORING_NODE_ENABLE_ATTRIBUTE_NAME)     = theElementList.second ? "1" : "0";
        }
    }
}

void FileDumper::dumpCommunicationSettings(pugi::xml_node theMotherNode, CommunicationSettingConfig* theCommunicationSettingConfig)
{
    pugi::xml_node theCommunicationSettingsNode = theMotherNode.append_child(COMMUNICATIONSETTINGS_NODE_NAME);

    auto dumpCommunicationSetting = [&theCommunicationSettingsNode](CommunicationSettingConfig::CommunicationSetting theCommunicationSetting, const std::string& nodeName)
    {
        if(theCommunicationSetting.fIP != "")
        {
            pugi::xml_node theSettingNode                                                          = theCommunicationSettingsNode.append_child(nodeName.c_str());
            theSettingNode.append_attribute(COMMUNICATIONSETTINGS_IP_ATTRIBUTE_NAME)               = theCommunicationSetting.fIP.c_str();
            theSettingNode.append_attribute(COMMUNICATIONSETTINGS_PORT_ATTRIBUTE_NAME)             = std::to_string(theCommunicationSetting.fPort).c_str();
            theSettingNode.append_attribute(COMMUNICATIONSETTINGS_ENABLECONNECTION_ATTRIBUTE_NAME) = theCommunicationSetting.fEnable ? "1" : "0";
        }
    };

    dumpCommunicationSetting(theCommunicationSettingConfig->fControllerCommunication, COMMUNICATIONSETTINGS_CONTROLLER_NODE_NAME);
    dumpCommunicationSetting(theCommunicationSettingConfig->fDQMCommunication, COMMUNICATIONSETTINGS_DQM_NODE_NAME);
    dumpCommunicationSetting(theCommunicationSettingConfig->fMonitorDQMCommunication, COMMUNICATIONSETTINGS_MONITORDQM_NODE_NAME);
    dumpCommunicationSetting(theCommunicationSettingConfig->fPowerSupplyDQMCommunication, COMMUNICATIONSETTINGS_POWERSUPPLYCLIENT_NODE_NAME);
}
