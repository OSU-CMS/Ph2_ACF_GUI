#ifndef __DQM_METADATA_OT__
#define __DQM_METADATA_OT__

#include "DQMUtils/DQMMetadata.h"

class DQMMetadataOT : public DQMMetadata
{
  public:
    DQMMetadataOT();
    ~DQMMetadataOT();

    void book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap) override;

    void fillCICFuseId(const DetectorDataContainer& theCICFuseIdContainer);
    void fillCICConfiguration(const DetectorDataContainer& theCICConfigurationContainer, bool initialValue);

    bool fill(std::string& inputStream) override;
    void process() override;
    void reset(void) override;

  private:
    DetectorDataContainer fCICFuseIdContainer;
    DetectorDataContainer fInitialCICConfigurationContainer;
    DetectorDataContainer fFinalCICConfigurationContainer;
};

#endif
