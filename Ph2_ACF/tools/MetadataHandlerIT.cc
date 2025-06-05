#include "tools/MetadataHandlerIT.h"

MetadataHandlerIT::MetadataHandlerIT(std::string startOfTestTime) : MetadataHandler(startOfTestTime) {}

MetadataHandlerIT::~MetadataHandlerIT() {}

void MetadataHandlerIT::initMetadataHardwareSpecific()
{
#ifdef __USE_ROOT__
    fDQMMetadata = new DQMMetadataIT();
#endif
}

void MetadataHandlerIT::fillInitialConditionsHardwareSpecific()
{
    __attribute__((unused)) const bool isInitialValue = true;
    DetectorDataContainer              theBeginOfCalibContainer;
    ContainerFactory::copyAndInitBoard<std::string>(*fDetectorContainer, theBeginOfCalibContainer);
    MetadataHandlerIT::fillBeginOfCalib(theBeginOfCalibContainer);

#ifdef __USE_ROOT__
    auto* theITDQMMetadata = static_cast<DQMMetadataIT*>(fDQMMetadata);
    theITDQMMetadata->fillBeginOfCalib(theBeginOfCalibContainer);
#else
    if(fDQMStreamerEnabled)
    {
        ContainerSerialization theMetadataSerialization("MetadataITBeginOfCalib");
        theMetadataSerialization.streamByHybridContainer(fDQMStreamer, theBeginOfCalibContainer, isInitialValue);
    }
#endif
}

void MetadataHandlerIT::fillFinalConditionsHardwareSpecific()
{
    __attribute__((unused)) const bool isInitialValue = false;
    DetectorDataContainer              theEndOfCalibContainer;
    ContainerFactory::copyAndInitBoard<std::array<std::string, RD53Shared::NENDOFCALIB>>(*fDetectorContainer, theEndOfCalibContainer);
    MetadataHandlerIT::fillEndOfCalib(theEndOfCalibContainer);

#ifdef __USE_ROOT__
    auto* theITDQMMetadata = static_cast<DQMMetadataIT*>(fDQMMetadata);
    theITDQMMetadata->fillEndOfCalib(theEndOfCalibContainer);
#else
    if(fDQMStreamerEnabled)
    {
        ContainerSerialization theMetadataSerialization("MetadataITEndOfCalib");
        theMetadataSerialization.streamByHybridContainer(fDQMStreamer, theEndOfCalibContainer, isInitialValue);
    }
#endif
}

void MetadataHandlerIT::fillBeginOfCalib(DetectorDataContainer& theBeginOfCalibContainer)
{
    for(auto cBoard: *fDetectorContainer)
        theBeginOfCalibContainer.getObject(cBoard->getId())->getSummary<std::string>() =
            std::to_string(static_cast<Ph2_HwInterface::RD53FWInterface*>(this->fBeBoardFWMap[cBoard->getId()])->getChipCommunicationStatus());
}

void MetadataHandlerIT::fillEndOfCalib(DetectorDataContainer& theEndOfCalibContainer)
{
    for(auto cBoard: *fDetectorContainer)
        theEndOfCalibContainer.getObject(cBoard->getId())->getSummary<std::array<std::string, RD53Shared::NENDOFCALIB>>() = {
            std::to_string(static_cast<Ph2_HwInterface::RD53FWInterface*>(this->fBeBoardFWMap[cBoard->getId()])->getNcorruptedNevents()),
            std::to_string(static_cast<Ph2_HwInterface::RD53FWInterface*>(this->fBeBoardFWMap[cBoard->getId()])->getNtrialsNevents())};
}
