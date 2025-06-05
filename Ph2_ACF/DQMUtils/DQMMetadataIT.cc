#include "DQMUtils/DQMMetadataIT.h"

DQMMetadataIT::DQMMetadataIT() : DQMMetadata() {}

DQMMetadataIT::~DQMMetadataIT() {}

void DQMMetadataIT::book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap)
{
    DQMMetadata::book(theOutputFile, theDetectorStructure, pSettingsMap);

    // child book here
    StringContainer fBeginOfCalibStringContainer("ITBeginOfCalib");
    RootContainerFactory::bookBoardHistograms<StringContainer>(theOutputFile, theDetectorStructure, fBeginOfCalibContainer, fBeginOfCalibStringContainer);

    std::array<StringContainer, RD53Shared::NENDOFCALIB> fEndOfCalibStringContainerArray;
    fEndOfCalibStringContainerArray[0].setName("ITEndOfCalibNcorruptedPackets");
    fEndOfCalibStringContainerArray[1].setName("ITEndOfCalibNtrialsPackets");
    for(auto i = 0; i < RD53Shared::NENDOFCALIB; i++)
        RootContainerFactory::bookBoardHistograms<StringContainer>(theOutputFile, theDetectorStructure, fEndOfCalibContainerArray[i], fEndOfCalibStringContainerArray[i]);
}

void DQMMetadataIT::fillBeginOfCalib(const DetectorDataContainer& theDetectorData)
{
    for(const auto cBoard: theDetectorData)
    {
        if(cBoard->hasSummary() == false) continue;
        fBeginOfCalibContainer.getObject(cBoard->getId())->getSummary<StringContainer>().saveString(cBoard->getSummary<std::string>().c_str());
    }
}

void DQMMetadataIT::fillEndOfCalib(const DetectorDataContainer& theDetectorData)
{
    for(const auto cBoard: theDetectorData)
    {
        if(cBoard->hasSummary() == false) continue;
        for(auto i = 0; i < RD53Shared::NENDOFCALIB; i++)
            fEndOfCalibContainerArray[i].getObject(cBoard->getId())->getSummary<StringContainer>().saveString(cBoard->getSummary<std::array<std::string, RD53Shared::NENDOFCALIB>>()[i]);
    }
}

bool DQMMetadataIT::fill(std::string& inputStream)
{
    const bool motherClassFillResult = DQMMetadata::fill(inputStream);

    if(motherClassFillResult == true)
        return true;
    else
    {
        // child fill here
        ContainerSerialization theMetadataBeginOfCalibSerialization("MetadataITBeginOfCalib");
        ContainerSerialization theMetadataEndOfCalibSerialization("MetadataITEndOfCalib");

        if(theMetadataBeginOfCalibSerialization.attachDeserializer(inputStream))
        {
            DetectorDataContainer theDetectorData =
                theMetadataBeginOfCalibSerialization.deserializeBoardContainer<EmptyContainer, EmptyContainer, EmptyContainer, std::string, EmptyContainer>(fDetectorContainer);
            DQMMetadataIT::fillBeginOfCalib(theDetectorData);
            return true;
        }

        if(theMetadataEndOfCalibSerialization.attachDeserializer(inputStream))
        {
            DetectorDataContainer theDetectorData =
                theMetadataEndOfCalibSerialization.deserializeBoardContainer<EmptyContainer, EmptyContainer, EmptyContainer, std::array<std::string, RD53Shared::NENDOFCALIB>, EmptyContainer>(
                    fDetectorContainer);
            DQMMetadataIT::fillEndOfCalib(theDetectorData);
            return true;
        }
    }

    return false;
}

void DQMMetadataIT::process()
{
    DQMMetadata::process();

    // child process here
}

void DQMMetadataIT::reset(void)
{
    DQMMetadata::reset();

    // child reset here
}
