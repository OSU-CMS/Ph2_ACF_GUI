/*!
  \file                  RD53GenericDacDacScanHistograms.h
  \brief                 Header file of generic DAC DAC scan histograms
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/05/21
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef RD53GenericDacDacScanHistograms_H
#define RD53GenericDacDacScanHistograms_H

#include "DQMHistogramBase.h"
#include "Utils/ContainerFactory.h"
#include "Utils/RD53Shared.h"

#include <TH1F.h>
#include <TH2F.h>

class GenericDacDacScanHistograms : public DQMHistogramBase
{
  public:
    void book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& settingsMap) override;
    void process() override;
    bool fill(std::string& inputStream) override;
    void reset() override {};

    void fillOccupancy(const DetectorDataContainer& OccupancyContainer);
    void fillGenericDacDac(const DetectorDataContainer& GenericDacDacContainer);

    bool AreHistoBooked = false;

  private:
    DetectorContainer* fDetectorContainer;

    DetectorDataContainer Occupancy2D;
    DetectorDataContainer GenericDac1;
    DetectorDataContainer GenericDac2;

    std::string regNameDAC1;
    size_t      startValueDAC1;
    size_t      stopValueDAC1;
    size_t      stepDAC1;
    std::string regNameDAC2;
    size_t      startValueDAC2;
    size_t      stopValueDAC2;
    size_t      stepDAC2;
};

#endif
