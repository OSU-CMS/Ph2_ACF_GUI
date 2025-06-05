/*!
  \file                  DQMMetadataIT.h
  \brief                 Header file of DQM Metadata
  \author                Mauro DINARDO
  \version               1.0
  \date                  09/03/23
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef DQMMetadataIT_H
#define DQMMEtadataIT_H

#include "DQMUtils/DQMMetadata.h"
#include "RootUtils/StringContainer.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/EmptyContainer.h"

class DQMMetadataIT : public DQMMetadata
{
  public:
    DQMMetadataIT();
    ~DQMMetadataIT();

    void book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& pSettingsMap) override;

    void fillBeginOfCalib(const DetectorDataContainer& theDetectorData);
    void fillEndOfCalib(const DetectorDataContainer& theDetectorData);

    bool fill(std::string& inputStream) override;
    void process() override;
    void reset(void) override;

  private:
    DetectorDataContainer                                      fBeginOfCalibContainer;
    std::array<DetectorDataContainer, RD53Shared::NENDOFCALIB> fEndOfCalibContainerArray;
};

#endif
