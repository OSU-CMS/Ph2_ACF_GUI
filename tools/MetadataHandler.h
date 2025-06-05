#ifndef __METADATA_HANDLER_H__
#define __METADATA_HANDLER_H__

#include "tools/Tool.h"

class DQMMetadata;

class MetadataHandler : public Tool
{
  public:
    MetadataHandler(std::string startOfTestTime);
    ~MetadataHandler();

    void initMetadata();
    void fillInitialConditions();
    void fillFinalConditions();

    virtual void initMetadataHardwareSpecific()          = 0;
    virtual void fillInitialConditionsHardwareSpecific() = 0;
    virtual void fillFinalConditionsHardwareSpecific()   = 0;

    void setFinalConfigurationFileContent(const std::string& theConfigurationFileContent) { fFinalConfigurationFileContent = theConfigurationFileContent; }

    void fillSubCalibrationNameAndTimeContainer(std::string subCalibrationName);

    void justBookDQMMetadata();

  protected:
    DQMMetadata* fDQMMetadata{nullptr};

  private:
    void fillNameContainerWithChipIDs();
    void fillReadoutChipConfigurationContainer(DetectorDataContainer& theReadoutChipConfigurationContainer);
    void fillLpGBTConfigurationContainer(DetectorDataContainer& theLpGBTConfigurationContainer);
    void fillLpGBTFuseIdContainer(DetectorDataContainer& theLpGBTFuseIdContainer);
    void fillVTRxFuseIdContainer(DetectorDataContainer& theVTRxFuseIdContainer);
    void fillBoardConfigurationContainer(DetectorDataContainer& theBoardConfigurationContainer);

    std::string fFinalConfigurationFileContent{""};
    std::string fStartOfTest;
};

#endif
