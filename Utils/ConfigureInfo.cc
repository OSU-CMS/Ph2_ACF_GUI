#include "Utils/ConfigureInfo.h"
#include "HWDescription/BeBoard.h"
#include "Utils/Container.h"
#include "Utils/DataContainer.h"

ConfigureInfo::ConfigureInfo() {}

ConfigureInfo::~ConfigureInfo() {}

void ConfigureInfo::setEnabledObjects(DetectorContainer* theDetectorContainer) const
{
    std::set<uint16_t> enabledBoardIdList;
    for(const auto& theBoardStructure: fEnabledObjectStructure)
    {
        uint16_t           theBoardId = theBoardStructure.first;
        std::set<uint16_t> enabledOpticalGroupIdList;
        enabledBoardIdList.insert(theBoardId);
        for(const auto& theOpticalGroupStructure: theBoardStructure.second.second)
        {
            uint16_t           theOpticalGroupId = theOpticalGroupStructure.first;
            std::set<uint16_t> enabledHybridIdList;
            enabledOpticalGroupIdList.insert(theOpticalGroupId);

            for(const auto& theHybridStructure: theOpticalGroupStructure.second.second)
            {
                uint16_t           theHybridId = theHybridStructure.first;
                std::set<uint16_t> enabledReadoutChipIdList;
                enabledHybridIdList.insert(theHybridId);
                for(const auto& theReadoutChipStructure: theHybridStructure.second.second)
                {
                    uint16_t theReadoutChipId = theReadoutChipStructure.first;
                    enabledReadoutChipIdList.insert(theReadoutChipId);
                }
                addQueryFunction<ChipContainer>(theDetectorContainer->getObject(theBoardId)->getObject(theOpticalGroupId)->getObject(theHybridId), enabledReadoutChipIdList);
            }
            addQueryFunction<HybridContainer>(theDetectorContainer->getObject(theBoardId)->getObject(theOpticalGroupId), enabledHybridIdList);
        }
        addQueryFunction<OpticalGroupContainer>(theDetectorContainer->getObject(theBoardId), enabledOpticalGroupIdList);
    }
    addQueryFunction<BoardContainer>(theDetectorContainer, enabledBoardIdList);
}

void ConfigureInfo::parseProtobufMessage(const std::string& theConfigureInfoString)
{
    MessageUtils::ConfigurationMessage theConfigureMessage;
    theConfigureMessage.ParseFromString(theConfigureInfoString);

    fCalibrationName   = theConfigureMessage.data().calibration_name();
    fConfigurationFile = theConfigureMessage.data().configuration_file();
    fSettingsFile      = theConfigureMessage.data().settings_file();

    for(const auto& board: theConfigureMessage.data().object_list())
    {
        uint16_t boardId                 = uint16_t(board.id());
        fEnabledObjectStructure[boardId] = std::make_pair(board.name(), OpticalGroupMap());
        auto& theBoardStructure          = fEnabledObjectStructure[boardId].second;

        for(const auto& opticalGroup: board.object_list())
        {
            uint16_t opticalGroupId           = uint16_t(opticalGroup.id());
            theBoardStructure[opticalGroupId] = std::make_pair(opticalGroup.name(), HybridMap());
            auto& theOpticalGroupStructure    = theBoardStructure[opticalGroupId].second;

            for(const auto& hybrid: opticalGroup.object_list())
            {
                uint16_t hybridId                  = uint16_t(hybrid.id());
                theOpticalGroupStructure[hybridId] = std::make_pair(hybrid.name(), ChipMap());
                auto& theHybridStructure           = theOpticalGroupStructure[hybridId].second;

                for(const auto& readoutChip: hybrid.object_list())
                {
                    uint16_t readoutChipId            = uint16_t(readoutChip.id());
                    theHybridStructure[readoutChipId] = readoutChip.name();
                }
            }
        }
    }
}

std::string ConfigureInfo::createProtobufMessage() const
{
    MessageUtils::ConfigurationMessage theConfigureMessage;
    theConfigureMessage.mutable_query_type()->set_type(MessageUtils::QueryType::CONFIGURE);
    theConfigureMessage.mutable_data()->set_calibration_name(fCalibrationName);
    theConfigureMessage.mutable_data()->set_configuration_file(fConfigurationFile);
    theConfigureMessage.mutable_data()->set_settings_file(fSettingsFile);

    for(const auto& theBoardStructure: fEnabledObjectStructure)
    {
        auto theBoardMessage = theConfigureMessage.mutable_data()->add_object_list();
        theBoardMessage->mutable_object_type()->set_type(MessageUtils::ObjectType::BOARD);
        theBoardMessage->set_id(theBoardStructure.first);
        theBoardMessage->set_name(theBoardStructure.second.first);
        for(const auto& theOpticalGroupStructure: theBoardStructure.second.second)
        {
            auto theOpticalGroupMessage = theBoardMessage->add_object_list();
            theOpticalGroupMessage->mutable_object_type()->set_type(MessageUtils::ObjectType::OPTICALGROUP);
            theOpticalGroupMessage->set_id(theOpticalGroupStructure.first);
            theOpticalGroupMessage->set_name(theOpticalGroupStructure.second.first);
            for(const auto& theHybridStructure: theOpticalGroupStructure.second.second)
            {
                auto theHybridMessage = theOpticalGroupMessage->add_object_list();
                theHybridMessage->mutable_object_type()->set_type(MessageUtils::ObjectType::HYBRID);
                theHybridMessage->set_id(theHybridStructure.first);
                theHybridMessage->set_name(theHybridStructure.second.first);
                for(const auto& theReadoutChipStructure: theHybridStructure.second.second)
                {
                    auto theReadoutChipMessage = theHybridMessage->add_object_list();
                    theReadoutChipMessage->mutable_object_type()->set_type(MessageUtils::ObjectType::CHIP);
                    theReadoutChipMessage->set_id(theReadoutChipStructure.first);
                    theReadoutChipMessage->set_name(theReadoutChipStructure.second);
                }
            }
        }
    }

    std::string theConfigurationMessageString;
    theConfigureMessage.SerializeToString(&theConfigurationMessageString);
    return theConfigurationMessageString;
}

void ConfigureInfo::extractObjectNames(DetectorDataContainer* theNameContainer) const
{
    for(const auto& theBoardStructure: fEnabledObjectStructure)
    {
        auto theBoardNameContainer                                    = theNameContainer->getObject(theBoardStructure.first);
        theBoardNameContainer->getSummary<std::string, std::string>() = theBoardStructure.second.first;

        for(const auto& theOpticalGroupStructure: theBoardStructure.second.second)
        {
            auto theOpticalGroupNameContainer                                    = theBoardNameContainer->getObject(theOpticalGroupStructure.first);
            theOpticalGroupNameContainer->getSummary<std::string, std::string>() = theOpticalGroupStructure.second.first;

            for(const auto& theHybridStructure: theOpticalGroupStructure.second.second)
            {
                auto theHybridNameContainer                                    = theOpticalGroupNameContainer->getObject(theHybridStructure.first);
                theHybridNameContainer->getSummary<std::string, std::string>() = theHybridStructure.second.first;

                for(const auto& theReadoutChipStructure: theHybridStructure.second.second)
                {
                    auto theReadoutChipNameContainer                       = theHybridNameContainer->getObject(theReadoutChipStructure.first);
                    theReadoutChipNameContainer->getSummary<std::string>() = theReadoutChipStructure.second;
                }
            }
        }
    }
}

void ConfigureInfo::enableBoard(uint16_t boardId, const std::string boardName)
{
    if(fEnabledObjectStructure.find(boardId) == fEnabledObjectStructure.end()) { fEnabledObjectStructure[boardId] = std::make_pair(boardName, OpticalGroupMap()); }
    else
        fEnabledObjectStructure[boardId].first = boardName;
}

void ConfigureInfo::enableOpticalGroup(uint16_t boardId, uint16_t opticalGroupId, const std::string opticalGroupName)
{
    if(fEnabledObjectStructure.find(boardId) == fEnabledObjectStructure.end()) enableBoard(boardId, "");
    auto& opticalGroupMap = fEnabledObjectStructure[boardId].second;
    if(opticalGroupMap.find(opticalGroupId) == opticalGroupMap.end()) { opticalGroupMap[opticalGroupId] = std::make_pair(opticalGroupName, HybridMap()); }
    else
        opticalGroupMap[opticalGroupId].first = opticalGroupName;
}

void ConfigureInfo::enableHybrid(uint16_t boardId, uint16_t opticalGroupId, uint16_t hybridId, const std::string hybridName)
{
    if(fEnabledObjectStructure.find(boardId) == fEnabledObjectStructure.end()) enableBoard(boardId, "");
    auto& opticalGroupMap = fEnabledObjectStructure[boardId].second;
    if(opticalGroupMap.find(opticalGroupId) == opticalGroupMap.end()) enableOpticalGroup(boardId, opticalGroupId, "");
    auto& hybridMap = opticalGroupMap[opticalGroupId].second;

    if(hybridMap.find(hybridId) == hybridMap.end()) { hybridMap[hybridId] = std::make_pair(hybridName, ChipMap()); }
    else
        hybridMap[hybridId].first = hybridName;
}

void ConfigureInfo::enableReadoutChip(uint16_t boardId, uint16_t opticalGroupId, uint16_t hybridId, uint16_t readoutChipId, const std::string readoutChipName)
{
    if(fEnabledObjectStructure.find(boardId) == fEnabledObjectStructure.end()) enableBoard(boardId, "");
    auto& opticalGroupMap = fEnabledObjectStructure[boardId].second;
    if(opticalGroupMap.find(opticalGroupId) == opticalGroupMap.end()) enableOpticalGroup(boardId, opticalGroupId, "");
    auto& hybridMap = opticalGroupMap[opticalGroupId].second;
    if(hybridMap.find(hybridId) == hybridMap.end()) enableHybrid(boardId, opticalGroupId, hybridId, "");
    auto& readoutChipMap          = hybridMap[hybridId].second;
    readoutChipMap[readoutChipId] = readoutChipName;
}

std::string ConfigureInfo::getConfigFileStream(const std::string& fileName) const
{
    std::ifstream     fileHandler(fileName);
    std::stringstream fileStream;
    fileStream << fileHandler.rdbuf();
    return fileStream.str();
}
