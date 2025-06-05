#include "tools/MetadataHandler.h"
#include "HWDescription/VTRx.h"
#include "HWInterface/VTRxInterface.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include <filesystem>

#ifdef __USE_ROOT__
#include "DQMUtils/DQMMetadata.h"
#endif

MetadataHandler::MetadataHandler(std::string startOfTestTime) : Tool() { fStartOfTest = startOfTestTime; }

MetadataHandler::~MetadataHandler()
{
#ifdef __USE_ROOT__
    if(fResultFile != nullptr && fDQMMetadata != nullptr)
    {
        delete fDQMMetadata;
        fDQMMetadata = nullptr;
    }
#endif
}

void MetadataHandler::initMetadata() { initMetadataHardwareSpecific(); }

void MetadataHandler::fillInitialConditions()
{
    fillNameContainerWithChipIDs();

    std::string theUsername;
    try
    {
        theUsername = std::string(std::getenv("USER"));
    }
    catch(const std::exception& e)
    {
        LOG(WARNING) << e.what();
        LOG(WARNING) << __PRETTY_FUNCTION__ << " Username not set, using dummy name";
        theUsername = "user";
    }

    DetectorDataContainer theUsernameContainer;
    ContainerFactory::copyAndInitDetector<std::string>(*fDetectorContainer, theUsernameContainer);
    theUsernameContainer.getSummary<std::string>() = theUsername;

    std::string theHostName;
    try
    {
        theHostName = std::string(std::getenv("HOSTNAME"));
    }
    catch(const std::exception& e)
    {
        LOG(WARNING) << e.what();
        LOG(WARNING) << __PRETTY_FUNCTION__ << " Hostname not set, using dummy name";
        theHostName = "host";
    }
    DetectorDataContainer theHostNameContainer;
    ContainerFactory::copyAndInitDetector<std::string>(*fDetectorContainer, theHostNameContainer);
    theHostNameContainer.getSummary<std::string>() = theHostName;

    std::string gitRepositoryDirectory;
    if(std::getenv("OTSDAQ_CMSTRACKER_DIR") != nullptr)
    {
        try
        {
            std::filesystem::path gitLinkPath   = std::string(std::getenv("OTSDAQ_CMSTRACKER_DIR")) + "/src/otsdaq-cmstracker/Ph2_ACF/.git";
            std::filesystem::path gitTargetPath = std::filesystem::read_symlink(gitLinkPath);

            std::string gitTargetPathString = gitTargetPath.string();
            gitRepositoryDirectory          = gitTargetPathString.substr(0, gitTargetPathString.length() - 4);
        }
        catch(const std::exception& e)
        {
            LOG(WARNING) << e.what();
        }
    }
    else { gitRepositoryDirectory = std::string(std::getenv("PH2ACF_BASE_DIR")); }
    std::string theGitTag = runCommand("git -C " + gitRepositoryDirectory + " describe --tags HEAD");
    theGitTag.erase(theGitTag.find_last_not_of(" \n\r\t") + 1);

    DetectorDataContainer theGitTagContainer;
    ContainerFactory::copyAndInitDetector<std::string>(*fDetectorContainer, theGitTagContainer);
    theGitTagContainer.getSummary<std::string>() = theGitTag;

    std::string theGitCommitHash = runCommand("git -C " + gitRepositoryDirectory + " rev-parse HEAD");
    theGitCommitHash.erase(theGitCommitHash.find_last_not_of(" \n\r\t") + 1);
    DetectorDataContainer theGitCommitHashContainer;
    ContainerFactory::copyAndInitDetector<std::string>(*fDetectorContainer, theGitCommitHashContainer);
    theGitCommitHashContainer.getSummary<std::string>() = theGitCommitHash;

    DetectorDataContainer theCalibrationNameContainer;
    ContainerFactory::copyAndInitDetector<std::string>(*fDetectorContainer, theCalibrationNameContainer);
    theCalibrationNameContainer.getSummary<std::string>() = fCalibrationName;

    DetectorDataContainer theDetectorInitialConfigurationContainer;
    ContainerFactory::copyAndInitDetector<std::string>(*fDetectorContainer, theDetectorInitialConfigurationContainer);
    theDetectorInitialConfigurationContainer.getSummary<std::string>() = fInitialConfigurationFileContent;

    DetectorDataContainer theCalibrationTimestampContainer;
    ContainerFactory::copyAndInitDetector<std::string>(*fDetectorContainer, theCalibrationTimestampContainer);
    theCalibrationTimestampContainer.getSummary<std::string>() = fStartOfTest;

    DetectorDataContainer theBoardConfigurationContainer;
    ContainerFactory::copyAndInitBoard<std::string>(*fDetectorContainer, theBoardConfigurationContainer);
    fillBoardConfigurationContainer(theBoardConfigurationContainer);

    DetectorDataContainer theReadoutChipConfigurationContainer;
    ContainerFactory::copyAndInitChip<std::string>(*fDetectorContainer, theReadoutChipConfigurationContainer);
    fillReadoutChipConfigurationContainer(theReadoutChipConfigurationContainer);

    DetectorDataContainer theLpGBTConfigurationContainer;
    ContainerFactory::copyAndInitOpticalGroup<std::string>(*fDetectorContainer, theLpGBTConfigurationContainer);
    fillLpGBTConfigurationContainer(theLpGBTConfigurationContainer);

    DetectorDataContainer theLpGBTFuseIdContainer;
    ContainerFactory::copyAndInitOpticalGroup<std::string>(*fDetectorContainer, theLpGBTFuseIdContainer);
    fillLpGBTFuseIdContainer(theLpGBTFuseIdContainer);

    DetectorDataContainer theVTRxFuseIdContainer;
    ContainerFactory::copyAndInitOpticalGroup<std::string>(*fDetectorContainer, theVTRxFuseIdContainer);
    fillVTRxFuseIdContainer(theVTRxFuseIdContainer);
    bool isInitialValue = true;

#ifdef __USE_ROOT__
    fDQMMetadata->book(fResultFile, *fDetectorContainer, fSettingsMap);
    if(fNameContainer != nullptr) fDQMMetadata->fillObjectNames(*fNameContainer);
    fDQMMetadata->fillUsername(theUsernameContainer);
    fDQMMetadata->fillHostName(theHostNameContainer);
    fDQMMetadata->fillGitTag(theGitTagContainer);
    fDQMMetadata->fillGitCommitHash(theGitCommitHashContainer);
    fDQMMetadata->fillCalibrationName(theCalibrationNameContainer);
    fDQMMetadata->fillDetectorConfiguration(theDetectorInitialConfigurationContainer, isInitialValue);
    fDQMMetadata->fillRunTimestamp(theCalibrationTimestampContainer, isInitialValue);
    fDQMMetadata->fillBoardConfiguration(theBoardConfigurationContainer, isInitialValue);
    fDQMMetadata->fillReadoutChipConfiguration(theReadoutChipConfigurationContainer, isInitialValue);
    fDQMMetadata->fillLpGBTConfiguration(theLpGBTConfigurationContainer, isInitialValue);
    fDQMMetadata->fillLpGBTFuseId(theLpGBTFuseIdContainer);
    fDQMMetadata->fillVTRxFuseId(theVTRxFuseIdContainer);
#else
    if(fDQMStreamerEnabled)
    {
        ContainerSerialization theObjectNameSerialization("MetadataObjectNames");
        theObjectNameSerialization.streamByDetectorContainer(fDQMStreamer, *fNameContainer);

        ContainerSerialization theUsernameSerialization("MetadataUsername");
        theUsernameSerialization.streamByDetectorContainer(fDQMStreamer, theUsernameContainer);

        ContainerSerialization theHostNameSerialization("MetadataHostName");
        theHostNameSerialization.streamByDetectorContainer(fDQMStreamer, theHostNameContainer);

        ContainerSerialization theGitCommitHashSerialization("MetadataGitCommitHash");
        theGitCommitHashSerialization.streamByDetectorContainer(fDQMStreamer, theGitCommitHashContainer);

        ContainerSerialization theGitTagSerialization("MetadataGitTag");
        theGitTagSerialization.streamByDetectorContainer(fDQMStreamer, theGitTagContainer);

        ContainerSerialization theCalibrationNameSerialization("MetadataCalibrationName");
        theCalibrationNameSerialization.streamByDetectorContainer(fDQMStreamer, theCalibrationNameContainer);

        ContainerSerialization theDetectorConfigurationSerialization("MetadataDetectorConfiguration");
        theDetectorConfigurationSerialization.streamByDetectorContainer(fDQMStreamer, theDetectorInitialConfigurationContainer, isInitialValue);

        ContainerSerialization theCalibrationTimestampSerialization("MetadataCalibrationTimestamp");
        theCalibrationTimestampSerialization.streamByDetectorContainer(fDQMStreamer, theCalibrationTimestampContainer, isInitialValue);

        ContainerSerialization theBoardConfigurationSerialization("MetadataBoardConfiguration");
        theBoardConfigurationSerialization.streamByBoardContainer(fDQMStreamer, theBoardConfigurationContainer, isInitialValue);

        ContainerSerialization theReadoutChipConfigurationSerialization("MetadataReadoutChipConfiguration");
        theReadoutChipConfigurationSerialization.streamByChipContainer(fDQMStreamer, theReadoutChipConfigurationContainer, isInitialValue);

        ContainerSerialization theLpGBTConfigurationSerialization("MetadataLpGBTConfiguration");
        theLpGBTConfigurationSerialization.streamByOpticalGroupContainer(fDQMStreamer, theLpGBTConfigurationContainer, isInitialValue);

        ContainerSerialization theLpGBTFuseIdSerialization("MetadataLpGBTFuseId");
        theLpGBTFuseIdSerialization.streamByBoardContainer(fDQMStreamer, theLpGBTFuseIdContainer);

        ContainerSerialization theVTRxFuseIdSerialization("MetadataVTRxFuseId");
        theVTRxFuseIdSerialization.streamByBoardContainer(fDQMStreamer, theVTRxFuseIdContainer);
    }
#endif

    fillInitialConditionsHardwareSpecific();
}

void MetadataHandler::fillFinalConditions()
{
    bool                  isInitialValue = false;
    DetectorDataContainer theDetectorFinalConfigurationContainer;
    ContainerFactory::copyAndInitDetector<std::string>(*fDetectorContainer, theDetectorFinalConfigurationContainer);
    theDetectorFinalConfigurationContainer.getSummary<std::string>() = fFinalConfigurationFileContent;

    DetectorDataContainer theBoardConfigurationContainer;
    ContainerFactory::copyAndInitBoard<std::string>(*fDetectorContainer, theBoardConfigurationContainer);
    fillBoardConfigurationContainer(theBoardConfigurationContainer);

    DetectorDataContainer theReadoutChipConfigurationContainer;
    ContainerFactory::copyAndInitChip<std::string>(*fDetectorContainer, theReadoutChipConfigurationContainer);
    fillReadoutChipConfigurationContainer(theReadoutChipConfigurationContainer);

    DetectorDataContainer theLpGBTConfigurationContainer;
    ContainerFactory::copyAndInitOpticalGroup<std::string>(*fDetectorContainer, theLpGBTConfigurationContainer);
    fillLpGBTConfigurationContainer(theLpGBTConfigurationContainer);

    DetectorDataContainer theCalibrationTimestampContainer;
    ContainerFactory::copyAndInitDetector<std::string>(*fDetectorContainer, theCalibrationTimestampContainer);
    theCalibrationTimestampContainer.getSummary<std::string>() = getTimeStampString();

#ifdef __USE_ROOT__
    fDQMMetadata->fillDetectorConfiguration(theDetectorFinalConfigurationContainer, isInitialValue);
    fDQMMetadata->fillBoardConfiguration(theBoardConfigurationContainer, isInitialValue);
    fDQMMetadata->fillReadoutChipConfiguration(theReadoutChipConfigurationContainer, isInitialValue);
    fDQMMetadata->fillLpGBTConfiguration(theLpGBTConfigurationContainer, isInitialValue);
    fDQMMetadata->fillRunTimestamp(theCalibrationTimestampContainer, isInitialValue);
#else
    if(fDQMStreamerEnabled)
    {
        ContainerSerialization theDetectorConfigurationSerialization("MetadataDetectorConfiguration");
        theDetectorConfigurationSerialization.streamByDetectorContainer(fDQMStreamer, theDetectorFinalConfigurationContainer, isInitialValue);

        ContainerSerialization theBoardConfigurationSerialization("MetadataBoardConfiguration");
        theBoardConfigurationSerialization.streamByBoardContainer(fDQMStreamer, theBoardConfigurationContainer, isInitialValue);

        ContainerSerialization theReadoutChipConfigurationSerialization("MetadataReadoutChipConfiguration");
        theReadoutChipConfigurationSerialization.streamByChipContainer(fDQMStreamer, theReadoutChipConfigurationContainer, isInitialValue);

        ContainerSerialization theLpGBTConfigurationSerialization("MetadataLpGBTConfiguration");
        theLpGBTConfigurationSerialization.streamByOpticalGroupContainer(fDQMStreamer, theLpGBTConfigurationContainer, isInitialValue);

        ContainerSerialization theCalibrationTimestampSerialization("MetadataCalibrationTimestamp");
        theCalibrationTimestampSerialization.streamByDetectorContainer(fDQMStreamer, theCalibrationTimestampContainer, isInitialValue);
    }
#endif

    fillFinalConditionsHardwareSpecific();
}

void MetadataHandler::fillNameContainerWithChipIDs()
{
    if(fNameContainer == nullptr) return;
    for(auto cBoard: *fDetectorContainer)
    {
        fNameContainer->getObject(cBoard->getId())->getSummary<std::string, std::string>() = cBoard->getConnectionUri();
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    uint32_t chipFuseId = fReadoutChipInterface->ReadChipFuseID(cChip);
                    fNameContainer->getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getObject(cChip->getId())->getSummary<std::string>() =
                        std::to_string(chipFuseId);
                }
            }
        }
    }
}

void MetadataHandler::fillReadoutChipConfigurationContainer(DetectorDataContainer& theReadoutChipConfigurationContainer)
{
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                for(auto cChip: *cHybrid)
                {
                    theReadoutChipConfigurationContainer.getObject(cBoard->getId())
                        ->getObject(cOpticalGroup->getId())
                        ->getObject(cHybrid->getId())
                        ->getObject(cChip->getId())
                        ->getSummary<std::string, EmptyContainer>() = cChip->getRegMapStream().str();
                }
            }
        }
    }
}

void MetadataHandler::fillLpGBTConfigurationContainer(DetectorDataContainer& theLpGBTConfigurationContainer)
{
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            auto theLpGBT = cOpticalGroup->flpGBT;
            if(theLpGBT == nullptr) continue;
            theLpGBTConfigurationContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getSummary<std::string, EmptyContainer>() = theLpGBT->getRegMapStream().str();
        }
    }
}

void MetadataHandler::fillLpGBTFuseIdContainer(DetectorDataContainer& theLpGBTFuseIdContainer)
{
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            auto theLpGBT = cOpticalGroup->flpGBT;
            if(theLpGBT == nullptr) continue;
            uint32_t chipFuseId = flpGBTInterface->ReadChipFuseID(theLpGBT);

            theLpGBTFuseIdContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getSummary<std::string, EmptyContainer>() = convertToString(chipFuseId);
        }
    }
}

void MetadataHandler::fillVTRxFuseIdContainer(DetectorDataContainer& theVTRxFuseIdContainer)
{
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            auto theVTRx = cOpticalGroup->fVTRx;
            if(theVTRx == nullptr) continue;
            uint32_t chipFuseId                                                                                                             = fVTRxInterface->ReadChipFuseID(theVTRx);
            theVTRxFuseIdContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getSummary<std::string, EmptyContainer>() = convertToString(chipFuseId);
        }
    }
}

void MetadataHandler::fillBoardConfigurationContainer(DetectorDataContainer& theBoardConfigurationContainer)
{
    for(auto cBoard: *fDetectorContainer) { theBoardConfigurationContainer.getObject(cBoard->getId())->getSummary<std::string, EmptyContainer>() = cBoard->getRegMapStream().str(); }
}

void MetadataHandler::fillSubCalibrationNameAndTimeContainer(std::string subCalibrationName)
{
    DetectorDataContainer theSubCalibrationNameAndTimeContainer;
    ContainerFactory::copyAndInitDetector<std::pair<std::string, std::string>>(*fDetectorContainer, theSubCalibrationNameAndTimeContainer);

    theSubCalibrationNameAndTimeContainer.getSummary<std::pair<std::string, std::string>>() = std::make_pair(subCalibrationName, getTimeStampString());

#ifdef __USE_ROOT__
    fDQMMetadata->fillSubCalibrationNameAndTime(theSubCalibrationNameAndTimeContainer);
#else
    if(fDQMStreamerEnabled)
    {
        ContainerSerialization theSubCalibrationNameAndTimeSerialization("MetadataSubCalibrationNameAndTime");
        theSubCalibrationNameAndTimeSerialization.streamByDetectorContainer(fDQMStreamer, theSubCalibrationNameAndTimeContainer);
    }
#endif
}

void MetadataHandler::justBookDQMMetadata()
{
#ifdef __USE_ROOT__
    fDQMMetadata->book(fResultFile, *fDetectorContainer, fSettingsMap);
#endif
}
