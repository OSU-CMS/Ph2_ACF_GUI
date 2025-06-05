#ifndef __DQM_METADATA__
#define __DQM_METADATA__

#include "DQMUtils/DQMHistogramBase.h"

class TTree;

class DQMMetadata : public DQMHistogramBase
{
  public:
    DQMMetadata();
    ~DQMMetadata();

    virtual void book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap) override;

    void fillObjectNames(const DetectorDataContainer& theNameContainer);
    void fillUsername(const DetectorDataContainer& theUsernameContainer);
    void fillHostName(const DetectorDataContainer& theHostNameContainer);
    void fillGitTag(const DetectorDataContainer& theGitTagContainer);
    void fillGitCommitHash(const DetectorDataContainer& theGitCommitHashContainer);
    void fillFirmwareVersion(const DetectorDataContainer& theFirmwareVersionContainer);
    void fillCalibrationName(const DetectorDataContainer& theCalibrationNameContainer);
    void fillDetectorConfiguration(const DetectorDataContainer& theDetectorConfigurationContainer, bool initialValue);
    void fillBoardConfiguration(const DetectorDataContainer& theBoardConfigurationContainer, bool initialValue);
    void fillRunTimestamp(const DetectorDataContainer& theCalibrationTimestampContainer, bool start);
    void fillReadoutChipConfiguration(const DetectorDataContainer& theReadoutChipConfigurationContainer, bool initialValue);
    void fillLpGBTConfiguration(const DetectorDataContainer& theLpGBTConfigurationContainer, bool initialValue);
    void fillLpGBTFuseId(const DetectorDataContainer& theLpGBTFuseIdContainer);
    void fillVTRxFuseId(const DetectorDataContainer& theVTRxFuseIdContainer);
    void fillSubCalibrationNameAndTime(const DetectorDataContainer& theSubCalibrationNameAndTimeContainer);

    virtual bool fill(std::string& inputStream) override;
    virtual void process() override;
    virtual void reset() override;

  protected:
    DetectorContainer* fDetectorContainer;

  private:
    DetectorDataContainer fNameContainer;
    DetectorDataContainer fUsernameContainer;
    DetectorDataContainer fHostNameContainer;
    DetectorDataContainer fGitTagContainer;
    DetectorDataContainer fGitCommitHashContainer;
    DetectorDataContainer fFirmwareVersionContainer;
    DetectorDataContainer fCalibrationNameContainer;
    DetectorDataContainer fInitialDetectorConfigurationContainer;
    DetectorDataContainer fFinalDetectorConfigurationContainer;
    DetectorDataContainer fCalibrationStartTimestampContainer;
    DetectorDataContainer fCalibrationStopTimestampContainer;
    DetectorDataContainer fInitialBoardConfigurationContainer;
    DetectorDataContainer fFinalBoardConfigurationContainer;
    DetectorDataContainer fInitialReadoutChipConfigurationContainer;
    DetectorDataContainer fFinalReadoutChipConfigurationContainer;
    DetectorDataContainer fInitialLpGBTConfigurationContainer;
    DetectorDataContainer fFinalLpGBTConfigurationContainer;
    DetectorDataContainer fLpGBTFuseIdContainer;
    DetectorDataContainer fVTRxFuseIdContainer;
    DetectorDataContainer fSubCalibrationNameAndTimeContainer;
};

#endif