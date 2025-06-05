#ifndef __FILE_DUMPER__
#define __FILE_DUMPER__

#include "pugixml.hpp"
#include <boost/any.hpp>
#include <map>
#include <string>

class DetectorContainer;
namespace Ph2_HwDescription
{
class BeBoard;
class OpticalGroup;
class Hybrid;
class ReadoutChip;
} // namespace Ph2_HwDescription

class CommunicationSettingConfig;
class DetectorMonitorConfig;

class FileDumper
{
  public:
    FileDumper(const std::string& outputDirectory);
    ~FileDumper();

    std::stringstream dumpConfigurationFiles(DetectorContainer*                       theDetectorContainer,
                                             const std::map<std::string, boost::any>& theSettingMap,
                                             CommunicationSettingConfig*              theCommunicationSettingConfig,
                                             DetectorMonitorConfig*                   theDetectorMonitorConfig);

  private:
    void        dumpBoardConfigurationFile(pugi::xml_node theMotherNode, Ph2_HwDescription::BeBoard* theBoardContainer);
    void        dumpOpticalGroupConfigurationFile(pugi::xml_node theMotherNode, Ph2_HwDescription::OpticalGroup* theOpticalGroupContainer);
    void        dumpHybridConfigurationFile(pugi::xml_node theMotherNode, Ph2_HwDescription::Hybrid* theHybridContainer);
    void        dumpLpGBTphasesForBypass(pugi::xml_node theMotherNode, Ph2_HwDescription::Hybrid* theHybridContainer);
    void        dumpChipConfigurationFile(pugi::xml_node theMotherNode, Ph2_HwDescription::ReadoutChip* theReadoutChip);
    void        dumpSettings(pugi::xml_node theMotherNode, const std::map<std::string, boost::any>& theSettingMap);
    void        dumpMonitorSettings(pugi::xml_node theMotherNode, DetectorMonitorConfig* theDetectorMonitorConfig);
    void        dumpCommunicationSettings(pugi::xml_node theMotherNode, CommunicationSettingConfig* theCommunicationSettingConfig);
    std::string fOutputDirectory;
};

#endif