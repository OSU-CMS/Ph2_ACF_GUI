#ifndef __CONFIGURATION_INFO__
#define __CONFIGURATION_INFO__

#include "MessageUtils/cpp/QueryMessage.pb.h"
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

class DetectorContainer;
class DetectorDataContainer;

class ConfigureInfo
{
  public:
    ConfigureInfo();
    ~ConfigureInfo();

    void setConfigurationFiles(const std::string& theConfigurationFile, const std::string& theSettingsFile = "")
    {
        fConfigurationFile = theConfigurationFile;
        fSettingsFile      = (theSettingsFile == "" ? theConfigurationFile : theSettingsFile);
    }
    std::string getConfigurationFile() const { return fConfigurationFile; }
    std::string getSettingsFile() const { return fSettingsFile; }
    std::string getConfigFileStream(const std::string& fileName) const;

    void        setCalibrationName(const std::string& theCalibrationName) { fCalibrationName = theCalibrationName; }
    std::string getCalibrationName() const { return fCalibrationName; }

    void setEnabledObjects(DetectorContainer* theDetectorContainer) const;

    void parseProtobufMessage(const std::string& theConfigureInfoString);

    std::string createProtobufMessage() const;

    std::string getQueryFunctionName() { return fQueryName; };

    void enableBoard(uint16_t boardId, const std::string boardName);
    void enableOpticalGroup(uint16_t boardId, uint16_t opticalGroupId, const std::string opticalGroupName);
    void enableHybrid(uint16_t boardId, uint16_t opticalGroupId, uint16_t hybridId, const std::string hybridName);
    void enableReadoutChip(uint16_t boardId, uint16_t opticalGroupId, uint16_t hybridId, uint16_t readoutChipId, const std::string readoutChipName);

    void extractObjectNames(DetectorDataContainer* theNameContainer) const;

  private:
    std::string       fConfigurationFile{""};
    std::string       fSettingsFile{""};
    std::string       fCalibrationName{""};
    const std::string fQueryName{"ConfigurationQueryFunction"};

    typedef std::unordered_map<uint16_t, std::string>                             ChipMap;
    typedef std::unordered_map<uint16_t, std::pair<std::string, ChipMap>>         HybridMap;
    typedef std::unordered_map<uint16_t, std::pair<std::string, HybridMap>>       OpticalGroupMap;
    typedef std::unordered_map<uint16_t, std::pair<std::string, OpticalGroupMap>> BoardMap;

    BoardMap fEnabledObjectStructure;

    template <typename Content, typename Container>
    void addQueryFunction(Container* theContainer, const std::set<uint16_t>& theEnabledSet) const
    {
        std::string functionName = "ConfigureInfoEnabledFunction";

        if(theEnabledSet.size() > 0)
        {
            auto enabledListFunction = [theEnabledSet](const Content* theContainer) { return theEnabledSet.find(theContainer->getId()) != theEnabledSet.end(); };
            theContainer->addQueryFunction(enabledListFunction, functionName);
        }
    }
};

#endif
