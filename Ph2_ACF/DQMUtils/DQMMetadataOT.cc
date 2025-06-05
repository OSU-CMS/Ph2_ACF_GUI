#include "DQMUtils/DQMMetadataOT.h"
#include "RootUtils/StringContainer.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/EmptyContainer.h"

DQMMetadataOT::DQMMetadataOT() : DQMMetadata() {}

DQMMetadataOT::~DQMMetadataOT() {}

void DQMMetadataOT::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    DQMMetadata::book(theOutputFile, theDetectorStructure, pSettingsMap);

    // child book here
    StringContainer theCICFuseIdStringContainer("CICFuseId");
    RootContainerFactory::bookHybridHistograms<StringContainer>(theOutputFile, theDetectorStructure, fCICFuseIdContainer, theCICFuseIdStringContainer);

    StringContainer theInitialCICConfigurationStringContainer("InitialCICConfiguration");
    RootContainerFactory::bookHybridHistograms<StringContainer>(theOutputFile, theDetectorStructure, fInitialCICConfigurationContainer, theInitialCICConfigurationStringContainer);

    StringContainer theFinalCICConfigurationStringContainer("FinalCICConfiguration");
    RootContainerFactory::bookHybridHistograms<StringContainer>(theOutputFile, theDetectorStructure, fFinalCICConfigurationContainer, theFinalCICConfigurationStringContainer);
}

void DQMMetadataOT::fillCICFuseId(const DetectorDataContainer& theCICFuseIdContainer)
{
    for(const auto board: theCICFuseIdContainer)
    {
        auto* theTreeContainerBoard = fCICFuseIdContainer.getObject(board->getId());

        for(const auto opticalGroup: *board)
        {
            auto* theTreeContainerOpticalGroup = theTreeContainerBoard->getObject(opticalGroup->getId());

            for(const auto hybrid: *opticalGroup)
            {
                auto* theTreeContainerHybrid = theTreeContainerOpticalGroup->getObject(hybrid->getId());
                if(!hybrid->hasSummary()) continue;
                theTreeContainerHybrid->getSummary<StringContainer>().saveString(hybrid->getSummary<std::string>().c_str());
            }
        }
    }
}

void DQMMetadataOT::fillCICConfiguration(const DetectorDataContainer& theCICConfigurationContainer, bool initialValue)
{
    for(const auto board: theCICConfigurationContainer)
    {
        BoardDataContainer* theTreeContainerBoard;
        if(initialValue)
            theTreeContainerBoard = fInitialCICConfigurationContainer.getObject(board->getId());
        else
            theTreeContainerBoard = fFinalCICConfigurationContainer.getObject(board->getId());

        for(const auto opticalGroup: *board)
        {
            auto* theTreeContainerOpticalGroup = theTreeContainerBoard->getObject(opticalGroup->getId());

            for(const auto hybrid: *opticalGroup)
            {
                auto* theTreeContainerHybrid = theTreeContainerOpticalGroup->getObject(hybrid->getId());
                if(!hybrid->hasSummary()) continue;
                theTreeContainerHybrid->getSummary<StringContainer>().saveString(hybrid->getSummary<std::string>().c_str());
            }
        }
    }
}

bool DQMMetadataOT::fill(std::string& inputStream)
{
    bool motherClassFillResult = DQMMetadata::fill(inputStream);
    if(motherClassFillResult) { return true; }
    else
    {
        // child fill here

        ContainerSerialization theCICFuseIdSerialization("MetadataCICFuseId");
        ContainerSerialization theCICConfigurationSerialization("MetadataCICConfiguration");

        if(theCICFuseIdSerialization.attachDeserializer(inputStream))
        {
            // std::cout << "Matched Metadata CICFuseId!!!!!\n";
            DetectorDataContainer theDetectorData =
                theCICFuseIdSerialization.deserializeBoardContainer<EmptyContainer, EmptyContainer, std::string, EmptyContainer, EmptyContainer>(fDetectorContainer);
            fillCICFuseId(theDetectorData);
            return true;
        }
        if(theCICConfigurationSerialization.attachDeserializer(inputStream))
        {
            // std::cout << "Matched Metadata CICConfiguration!!!!!\n";
            bool                  isInitial;
            DetectorDataContainer theDetectorData = theCICConfigurationSerialization.deserializeHybridContainer<EmptyContainer, EmptyContainer, std::string>(fDetectorContainer, isInitial);
            fillCICConfiguration(theDetectorData, isInitial);
            return true;
        }
    }

    return false;
}

void DQMMetadataOT::process()
{
    DQMMetadata::process();

    // child process here
}

void DQMMetadataOT::reset(void)
{
    DQMMetadata::reset();

    // child reset here
}
