#include "tools/MetadataHandlerOT.h"
#include "HWDescription/OuterTrackerHybrid.h"
#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#ifdef __USE_ROOT__
#include "DQMUtils/DQMMetadataOT.h"
#endif

using namespace Ph2_HwDescription;

MetadataHandlerOT::MetadataHandlerOT(std::string startOfTestTime) : MetadataHandler(startOfTestTime) {}

MetadataHandlerOT::~MetadataHandlerOT() {}

void MetadataHandlerOT::initMetadataHardwareSpecific()
{
#ifdef __USE_ROOT__
    fDQMMetadata = new DQMMetadataOT();
#endif
}

void MetadataHandlerOT::fillInitialConditionsHardwareSpecific()
{
    bool                  isInitialValue = true;
    DetectorDataContainer theCICFuseIdContainer;
    ContainerFactory::copyAndInitHybrid<std::string>(*fDetectorContainer, theCICFuseIdContainer);
    fillCICFuseIdContainer(theCICFuseIdContainer);

    DetectorDataContainer theCICConfigurationContainer;
    ContainerFactory::copyAndInitHybrid<std::string>(*fDetectorContainer, theCICConfigurationContainer);
    fillCICConfigurationContainer(theCICConfigurationContainer);

#ifdef __USE_ROOT__
    auto* theOTDQMMetadata = static_cast<DQMMetadataOT*>(fDQMMetadata);
    theOTDQMMetadata->fillCICFuseId(theCICFuseIdContainer);
    theOTDQMMetadata->fillCICConfiguration(theCICConfigurationContainer, isInitialValue);
#else
    if(fDQMStreamerEnabled)
    {
        ContainerSerialization theCICFuseIdSerialization("MetadataCICFuseId");
        theCICFuseIdSerialization.streamByBoardContainer(fDQMStreamer, theCICFuseIdContainer);

        ContainerSerialization theCICConfigurationSerialization("MetadataCICConfiguration");
        theCICConfigurationSerialization.streamByHybridContainer(fDQMStreamer, theCICConfigurationContainer, isInitialValue);
    }
#endif
}

void MetadataHandlerOT::fillFinalConditionsHardwareSpecific()
{
    bool isInitialValue = false;

    DetectorDataContainer theCICConfigurationContainer;
    ContainerFactory::copyAndInitHybrid<std::string>(*fDetectorContainer, theCICConfigurationContainer);
    fillCICConfigurationContainer(theCICConfigurationContainer);

#ifdef __USE_ROOT__
    auto* theOTDQMMetadata = static_cast<DQMMetadataOT*>(fDQMMetadata);
    theOTDQMMetadata->fillCICConfiguration(theCICConfigurationContainer, isInitialValue);
#else
    if(fDQMStreamerEnabled)
    {
        ContainerSerialization theCICConfigurationSerialization("MetadataCICConfiguration");
        theCICConfigurationSerialization.streamByHybridContainer(fDQMStreamer, theCICConfigurationContainer, isInitialValue);
    }
#endif
}

void MetadataHandlerOT::fillCICFuseIdContainer(DetectorDataContainer& theCICFuseIdContainer)
{
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                uint32_t chipFuseId = 0;
                if(static_cast<OuterTrackerHybrid*>(cHybrid)->fCic->getFrontEndType() == FrontEndType::CIC2)
                    chipFuseId = fCicInterface->ReadChipFuseID(static_cast<OuterTrackerHybrid*>(cHybrid)->fCic);
                theCICFuseIdContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getSummary<std::string>() = std::to_string(chipFuseId);
            }
        }
    }
}

void MetadataHandlerOT::fillCICConfigurationContainer(DetectorDataContainer& theCICConfigurationContainer)
{
    for(auto cBoard: *fDetectorContainer)
    {
        for(auto cOpticalGroup: *cBoard)
        {
            for(auto cHybrid: *cOpticalGroup)
            {
                theCICConfigurationContainer.getObject(cBoard->getId())->getObject(cOpticalGroup->getId())->getObject(cHybrid->getId())->getSummary<std::string>() =
                    static_cast<OuterTrackerHybrid*>(cHybrid)->fCic->getRegMapStream().str();
            }
        }
    }
}
