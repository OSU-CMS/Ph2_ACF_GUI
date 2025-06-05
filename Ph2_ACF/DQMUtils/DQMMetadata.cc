/*!
  \file                  DQMMetadataIT.cc
  \brief                 Implementationof DQM Metadata
  \author                Mauro DINARDO
  \version               1.0
  \date                  09/03/23
  Support:               email to mauro.dinardo@cern.ch
*/

#include "DQMUtils/DQMMetadata.h"
#include "RootUtils/StringContainer.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/EmptyContainer.h"

DQMMetadata::DQMMetadata() : DQMHistogramBase() {}

DQMMetadata::~DQMMetadata() {}

void DQMMetadata::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    fDetectorContainer = &theDetectorStructure;

    EmptyContainer  theEmpty;
    StringContainer theNameStringContainer("NameId");
    RootContainerFactory::bookHistogramsFromStructure<EmptyContainer, StringContainer, StringContainer, StringContainer, StringContainer, EmptyContainer>(
        theOutputFile, theDetectorStructure, fNameContainer, theEmpty, theNameStringContainer, theNameStringContainer, theNameStringContainer, theNameStringContainer, theEmpty);

    StringContainer theUsernameStringContainer("Username");
    RootContainerFactory::bookDetectorHistograms<StringContainer>(theOutputFile, theDetectorStructure, fUsernameContainer, theUsernameStringContainer);

    StringContainer theHostNameStringContainer("HostName");
    RootContainerFactory::bookDetectorHistograms<StringContainer>(theOutputFile, theDetectorStructure, fHostNameContainer, theHostNameStringContainer);

    StringContainer theGitTagStringContainer("GitTag");
    RootContainerFactory::bookDetectorHistograms<StringContainer>(theOutputFile, theDetectorStructure, fGitTagContainer, theGitTagStringContainer);

    StringContainer theGitCommitHashStringContainer("GitCommitHash");
    RootContainerFactory::bookDetectorHistograms<StringContainer>(theOutputFile, theDetectorStructure, fGitCommitHashContainer, theGitCommitHashStringContainer);

    StringContainer theFirmwareVersionStringContainer("FirmwareVersion");
    RootContainerFactory::bookBoardHistograms<StringContainer>(theOutputFile, theDetectorStructure, fFirmwareVersionContainer, theFirmwareVersionStringContainer);

    StringContainer theCalibrationNameStringContainer("CalibrationName");
    RootContainerFactory::bookDetectorHistograms<StringContainer>(theOutputFile, theDetectorStructure, fCalibrationNameContainer, theCalibrationNameStringContainer);

    StringContainer theInitialDetectorConfigurationStringContainer("InitialDetectorConfiguration");
    RootContainerFactory::bookDetectorHistograms<StringContainer>(theOutputFile, theDetectorStructure, fInitialDetectorConfigurationContainer, theInitialDetectorConfigurationStringContainer);

    StringContainer theFinalDetectorConfigurationStringContainer("FinalDetectorConfiguration");
    RootContainerFactory::bookDetectorHistograms<StringContainer>(theOutputFile, theDetectorStructure, fFinalDetectorConfigurationContainer, theFinalDetectorConfigurationStringContainer);

    StringContainer theCalibrationStartTimestampStringContainer("CalibrationStartTimestamp");
    RootContainerFactory::bookDetectorHistograms<StringContainer>(theOutputFile, theDetectorStructure, fCalibrationStartTimestampContainer, theCalibrationStartTimestampStringContainer);

    StringContainer theCalibrationStopTimestampStringContainer("CalibrationStopTimestamp");
    RootContainerFactory::bookDetectorHistograms<StringContainer>(theOutputFile, theDetectorStructure, fCalibrationStopTimestampContainer, theCalibrationStopTimestampStringContainer);

    StringContainer theInitialBoardConfigurationStringContainer("InitialBoardConfiguration");
    RootContainerFactory::bookBoardHistograms<StringContainer>(theOutputFile, theDetectorStructure, fInitialBoardConfigurationContainer, theInitialBoardConfigurationStringContainer);

    StringContainer theFinalBoardConfigurationStringContainer("FinalBoardConfiguration");
    RootContainerFactory::bookBoardHistograms<StringContainer>(theOutputFile, theDetectorStructure, fFinalBoardConfigurationContainer, theFinalBoardConfigurationStringContainer);

    StringContainer theInitialReadoutChipConfigurationStringContainer("InitialReadoutChipConfiguration");
    RootContainerFactory::bookChipHistograms<StringContainer>(theOutputFile, theDetectorStructure, fInitialReadoutChipConfigurationContainer, theInitialReadoutChipConfigurationStringContainer);

    StringContainer theFinalReadoutChipConfigurationStringContainer("FinalReadoutChipConfiguration");
    RootContainerFactory::bookChipHistograms<StringContainer>(theOutputFile, theDetectorStructure, fFinalReadoutChipConfigurationContainer, theFinalReadoutChipConfigurationStringContainer);

    StringContainer theInitialLpGBTConfigurationStringContainer("InitialLpGBTConfiguration");
    RootContainerFactory::bookOpticalGroupHistograms<StringContainer>(theOutputFile, theDetectorStructure, fInitialLpGBTConfigurationContainer, theInitialLpGBTConfigurationStringContainer);

    StringContainer theFinalLpGBTConfigurationStringContainer("FinalLpGBTConfiguration");
    RootContainerFactory::bookOpticalGroupHistograms<StringContainer>(theOutputFile, theDetectorStructure, fFinalLpGBTConfigurationContainer, theFinalLpGBTConfigurationStringContainer);

    StringContainer theLpGBTFuseIdStringContainer("LpGBTFuseId");
    RootContainerFactory::bookOpticalGroupHistograms<StringContainer>(theOutputFile, theDetectorStructure, fLpGBTFuseIdContainer, theLpGBTFuseIdStringContainer);

    StringContainer theVTRxFuseIdStringContainer("VTRxFuseId");
    RootContainerFactory::bookOpticalGroupHistograms<StringContainer>(theOutputFile, theDetectorStructure, fVTRxFuseIdContainer, theVTRxFuseIdStringContainer);

    StringContainer theSubCalibrationNameAndTimeStringContainer("SubCalibrationNameAndTime");
    RootContainerFactory::bookDetectorHistograms<StringContainer>(theOutputFile, theDetectorStructure, fSubCalibrationNameAndTimeContainer, theSubCalibrationNameAndTimeStringContainer);
}

void DQMMetadata::fillObjectNames(const DetectorDataContainer& theNameContainer)
{
    // Try and catch are required because this is (hopefully) the only case in which the container is created before the histogram booking
    for(const auto board: theNameContainer)
    {
        BoardDataContainer* theTreeContainerBoard;
        try
        {
            theTreeContainerBoard = fNameContainer.getObject(board->getId());
        }
        catch(const std::exception& e)
        {
            continue;
        }

        theTreeContainerBoard->getSummary<StringContainer, StringContainer>().saveString(board->getSummary<std::string, std::string>().c_str());

        for(const auto opticalGroup: *board)
        {
            OpticalGroupDataContainer* theTreeContainerOpticalGroup;
            try
            {
                theTreeContainerOpticalGroup = theTreeContainerBoard->getObject(opticalGroup->getId());
            }
            catch(const std::exception& e)
            {
                continue;
            }

            theTreeContainerOpticalGroup->getSummary<StringContainer, StringContainer>().saveString(opticalGroup->getSummary<std::string, std::string>().c_str());

            for(const auto hybrid: *opticalGroup)
            {
                HybridDataContainer* theTreeContainerHybrid;
                try
                {
                    theTreeContainerHybrid = theTreeContainerOpticalGroup->getObject(hybrid->getId());
                }
                catch(const std::exception& e)
                {
                    continue;
                }

                theTreeContainerHybrid->getSummary<StringContainer, StringContainer>().saveString(hybrid->getSummary<std::string, std::string>().c_str());

                for(const auto chip: *hybrid)
                {
                    ChipDataContainer* theTreeContainerChip;
                    try
                    {
                        theTreeContainerChip = theTreeContainerHybrid->getObject(chip->getId());
                    }
                    catch(const std::exception& e)
                    {
                        continue;
                    }
                    theTreeContainerChip->getSummary<StringContainer>().saveString(chip->getSummary<std::string>().c_str());
                }
            }
        }
    }
}

void DQMMetadata::fillUsername(const DetectorDataContainer& theUsernameContainer) { fUsernameContainer.getSummary<StringContainer>().saveString(theUsernameContainer.getSummary<std::string>()); }

void DQMMetadata::fillHostName(const DetectorDataContainer& theHostNameContainer) { fHostNameContainer.getSummary<StringContainer>().saveString(theHostNameContainer.getSummary<std::string>()); }

void DQMMetadata::fillGitTag(const DetectorDataContainer& theGitTagContainer) { fGitTagContainer.getSummary<StringContainer>().saveString(theGitTagContainer.getSummary<std::string>()); }

void DQMMetadata::fillGitCommitHash(const DetectorDataContainer& theGitCommitHashContainer)
{
    fGitCommitHashContainer.getSummary<StringContainer>().saveString(theGitCommitHashContainer.getSummary<std::string>());
}

void DQMMetadata::fillFirmwareVersion(const DetectorDataContainer& theFirmwareVersionContainer)
{
    for(const auto board: theFirmwareVersionContainer) { fFirmwareVersionContainer.getObject(board->getId())->getSummary<StringContainer>().saveString(board->getSummary<std::string>().c_str()); }
}

void DQMMetadata::fillCalibrationName(const DetectorDataContainer& theCalibrationNameContainer)
{
    fCalibrationNameContainer.getSummary<StringContainer>().saveString(theCalibrationNameContainer.getSummary<std::string>());
}

void DQMMetadata::fillDetectorConfiguration(const DetectorDataContainer& theDetectorConfigurationContainer, bool initialValue)
{
    if(initialValue)
        fInitialDetectorConfigurationContainer.getSummary<StringContainer>().saveString(theDetectorConfigurationContainer.getSummary<std::string>());
    else
        fFinalDetectorConfigurationContainer.getSummary<StringContainer>().saveString(theDetectorConfigurationContainer.getSummary<std::string>());
}

void DQMMetadata::fillRunTimestamp(const DetectorDataContainer& theCalibrationTimestampContainer, bool start)
{
    DetectorDataContainer* theTimestampPlotContainer;
    if(start) { theTimestampPlotContainer = &fCalibrationStartTimestampContainer; }
    else { theTimestampPlotContainer = &fCalibrationStopTimestampContainer; }
    theTimestampPlotContainer->getSummary<StringContainer>().saveString(theCalibrationTimestampContainer.getSummary<std::string>());
}

void DQMMetadata::fillBoardConfiguration(const DetectorDataContainer& theBoardConfigurationContainer, bool initialValue)
{
    for(const auto board: theBoardConfigurationContainer)
    {
        BoardDataContainer* theTreeContainerBoard;
        if(initialValue)
            theTreeContainerBoard = fInitialBoardConfigurationContainer.getObject(board->getId());
        else
            theTreeContainerBoard = fFinalBoardConfigurationContainer.getObject(board->getId());

        theTreeContainerBoard->getSummary<StringContainer>().saveString(board->getSummary<std::string>().c_str());
    }
}

void DQMMetadata::fillReadoutChipConfiguration(const DetectorDataContainer& theReadoutChipConfigurationContainer, bool initialValue)
{
    for(const auto board: theReadoutChipConfigurationContainer)
    {
        BoardDataContainer* theTreeContainerBoard;
        if(initialValue)
            theTreeContainerBoard = fInitialReadoutChipConfigurationContainer.getObject(board->getId());
        else
            theTreeContainerBoard = fFinalReadoutChipConfigurationContainer.getObject(board->getId());

        for(const auto opticalGroup: *board)
        {
            auto* theTreeContainerOpticalGroup = theTreeContainerBoard->getObject(opticalGroup->getId());

            for(const auto hybrid: *opticalGroup)
            {
                auto* theTreeContainerHybrid = theTreeContainerOpticalGroup->getObject(hybrid->getId());

                for(const auto chip: *hybrid)
                {
                    if(!chip->hasSummary()) continue;
                    auto* theTreeContainerChip = theTreeContainerHybrid->getObject(chip->getId());
                    theTreeContainerChip->getSummary<StringContainer>().saveString(chip->getSummary<std::string>().c_str());
                }
            }
        }
    }
}

void DQMMetadata::fillLpGBTConfiguration(const DetectorDataContainer& theLpGBTConfigurationContainer, bool initialValue)
{
    for(const auto board: theLpGBTConfigurationContainer)
    {
        BoardDataContainer* theTreeContainerBoard;
        if(initialValue)
            theTreeContainerBoard = fInitialLpGBTConfigurationContainer.getObject(board->getId());
        else
            theTreeContainerBoard = fFinalLpGBTConfigurationContainer.getObject(board->getId());

        for(const auto opticalGroup: *board)
        {
            auto* theTreeContainerOpticalGroup = theTreeContainerBoard->getObject(opticalGroup->getId());
            if(!opticalGroup->hasSummary()) continue;
            theTreeContainerOpticalGroup->getSummary<StringContainer>().saveString(opticalGroup->getSummary<std::string>().c_str());
        }
    }
}

void DQMMetadata::fillLpGBTFuseId(const DetectorDataContainer& theLpGBTFuseIdContainer)
{
    for(const auto board: theLpGBTFuseIdContainer)
    {
        auto* theTreeContainerBoard = fLpGBTFuseIdContainer.getObject(board->getId());
        for(const auto opticalGroup: *board)
        {
            auto* theTreeContainerOpticalGroup = theTreeContainerBoard->getObject(opticalGroup->getId());
            if(!opticalGroup->hasSummary()) continue;
            theTreeContainerOpticalGroup->getSummary<StringContainer>().saveString(opticalGroup->getSummary<std::string>().c_str());
        }
    }
}

void DQMMetadata::fillVTRxFuseId(const DetectorDataContainer& theVTRxFuseIdContainer)
{
    for(const auto board: theVTRxFuseIdContainer)
    {
        auto* theTreeContainerBoard = fVTRxFuseIdContainer.getObject(board->getId());
        for(const auto opticalGroup: *board)
        {
            auto* theTreeContainerOpticalGroup = theTreeContainerBoard->getObject(opticalGroup->getId());
            if(!opticalGroup->hasSummary()) continue;
            theTreeContainerOpticalGroup->getSummary<StringContainer>().saveString(opticalGroup->getSummary<std::string>().c_str());
        }
    }
}

void DQMMetadata::fillSubCalibrationNameAndTime(const DetectorDataContainer& theSubCalibrationNameAndTimeContainer)
{
    auto        theSubCalibrationNameAndTimePair   = theSubCalibrationNameAndTimeContainer.getSummary<std::pair<std::string, std::string>>();
    std::string theSubCalibrationNameAndTimeString = theSubCalibrationNameAndTimePair.first + " " + theSubCalibrationNameAndTimePair.second;
    fSubCalibrationNameAndTimeContainer.getSummary<StringContainer>().appendString(theSubCalibrationNameAndTimeString);
}

void DQMMetadata::process() {}

void DQMMetadata::reset() {}

bool DQMMetadata::fill(std::string& inputStream)
{
    ContainerSerialization theNameSerialization("MetadataObjectNames");
    ContainerSerialization theUsernameSerialization("MetadataUsername");
    ContainerSerialization theHostNameSerialization("MetadataHostName");
    ContainerSerialization theGitTagSerialization("MetadataGitTag");
    ContainerSerialization theGitCommitHashSerialization("MetadataGitCommitHash");
    ContainerSerialization theFirmwareVersionSerialization("MetadataFirmwareVersion");
    ContainerSerialization theCalibrationNameSerialization("MetadataCalibrationName");
    ContainerSerialization theDetectorConfigurationSerialization("MetadataDetectorConfiguration");
    ContainerSerialization theCalibrationTimestampSerialization("MetadataCalibrationTimestamp");
    ContainerSerialization theBoardConfigurationSerialization("MetadataBoardConfiguration");
    ContainerSerialization theReadoutChipConfigurationSerialization("MetadataReadoutChipConfiguration");
    ContainerSerialization theLpGBTConfigurationSerialization("MetadataLpGBTConfiguration");
    ContainerSerialization theLpGBTFuseIdSerialization("MetadataLpGBTFuseId");
    ContainerSerialization theVTRxFuseIdSerialization("MetadataVTRxFuseId");
    ContainerSerialization theSubCalibrationNameAndTimeSerialization("MetadataSubCalibrationNameAndTime");

    if(theNameSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched Metadata ObjectNames!!!!!\n";
        DetectorDataContainer theDetectorData =
            theNameSerialization.deserializeDetectorContainer<EmptyContainer, std::string, std::string, std::string, std::string, EmptyContainer>(fDetectorContainer);
        fillObjectNames(theDetectorData);

        return true;
    }
    if(theUsernameSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched Metadata Username!!!!!\n";
        DetectorDataContainer theDetectorData =
            theUsernameSerialization.deserializeDetectorContainer<EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer, std::string>(fDetectorContainer);
        fillUsername(theDetectorData);

        return true;
    }
    if(theHostNameSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched Metadata HostName!!!!!\n";
        DetectorDataContainer theDetectorData =
            theHostNameSerialization.deserializeDetectorContainer<EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer, std::string>(fDetectorContainer);
        fillHostName(theDetectorData);

        return true;
    }
    if(theGitTagSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched Metadata GitTag!!!!!\n";
        DetectorDataContainer theDetectorData =
            theGitTagSerialization.deserializeDetectorContainer<EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer, std::string>(fDetectorContainer);
        fillGitTag(theDetectorData);
        return true;
    }
    if(theGitCommitHashSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched Metadata GitCommitHash!!!!!\n";
        DetectorDataContainer theDetectorData =
            theGitCommitHashSerialization.deserializeDetectorContainer<EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer, std::string>(fDetectorContainer);
        fillGitCommitHash(theDetectorData);
        return true;
    }
    if(theFirmwareVersionSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched Metadata FirmwareVersion!!!!!\n";
        DetectorDataContainer theDetectorData =
            theFirmwareVersionSerialization.deserializeDetectorContainer<EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer, std::string, EmptyContainer>(fDetectorContainer);
        fillFirmwareVersion(theDetectorData);
        return true;
    }
    if(theCalibrationNameSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched Metadata CalibrationName!!!!!\n";
        DetectorDataContainer theDetectorData =
            theCalibrationNameSerialization.deserializeDetectorContainer<EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer, std::string>(fDetectorContainer);
        fillCalibrationName(theDetectorData);
        return true;
    }
    if(theDetectorConfigurationSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched Metadata DetectorConfiguration!!!!!\n";
        bool                  isInitial;
        DetectorDataContainer theDetectorData =
            theDetectorConfigurationSerialization.deserializeDetectorContainer<EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer, std::string>(fDetectorContainer,
                                                                                                                                                                            isInitial);
        fillDetectorConfiguration(theDetectorData, isInitial);
        return true;
    }
    if(theCalibrationTimestampSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched Metadata CalibrationTimestamp!!!!!\n";
        bool                  isInitial;
        DetectorDataContainer theDetectorData =
            theCalibrationTimestampSerialization.deserializeDetectorContainer<EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer, std::string>(fDetectorContainer,
                                                                                                                                                                           isInitial);
        fillRunTimestamp(theDetectorData, isInitial);
        return true;
    }
    if(theBoardConfigurationSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched Metadata BoardConfiguration!!!!!\n";
        bool                  isInitial;
        DetectorDataContainer theDetectorData =
            theBoardConfigurationSerialization.deserializeBoardContainer<EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer, std::string>(fDetectorContainer, isInitial);
        fillBoardConfiguration(theDetectorData, isInitial);
        return true;
    }
    if(theReadoutChipConfigurationSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched Metadata ReadoutChipConfiguration!!!!!\n";
        bool                  isInitial;
        DetectorDataContainer theDetectorData = theReadoutChipConfigurationSerialization.deserializeChipContainer<EmptyContainer, std::string>(fDetectorContainer, isInitial);
        fillReadoutChipConfiguration(theDetectorData, isInitial);
        return true;
    }
    if(theLpGBTConfigurationSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched Metadata LpGBTConfiguration!!!!!\n";
        bool                  isInitial;
        DetectorDataContainer theDetectorData =
            theLpGBTConfigurationSerialization.deserializeOpticalGroupContainer<EmptyContainer, EmptyContainer, EmptyContainer, std::string>(fDetectorContainer, isInitial);
        fillLpGBTConfiguration(theDetectorData, isInitial);
        return true;
    }
    if(theLpGBTFuseIdSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched Metadata LpGBTFuseId!!!!!\n";
        DetectorDataContainer theDetectorData = theLpGBTFuseIdSerialization.deserializeBoardContainer<EmptyContainer, EmptyContainer, EmptyContainer, std::string, EmptyContainer>(fDetectorContainer);
        fillLpGBTFuseId(theDetectorData);
        return true;
    }
    if(theVTRxFuseIdSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched Metadata VTRxFuseId!!!!!\n";
        DetectorDataContainer theDetectorData = theVTRxFuseIdSerialization.deserializeBoardContainer<EmptyContainer, EmptyContainer, EmptyContainer, std::string, EmptyContainer>(fDetectorContainer);
        fillVTRxFuseId(theDetectorData);
        return true;
    }
    if(theSubCalibrationNameAndTimeSerialization.attachDeserializer(inputStream))
    {
        // std::cout << "Matched Metadata SubCalibrationNameAndTime!!!!!\n";
        DetectorDataContainer theDetectorData =
            theSubCalibrationNameAndTimeSerialization.deserializeDetectorContainer<EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer, EmptyContainer, std::pair<std::string, std::string>>(
                fDetectorContainer);
        fillSubCalibrationNameAndTime(theDetectorData);
        return true;
    }

    return false;
}
