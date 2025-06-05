/*!
  \file                  RD53DataReadbackOptimizationHistograms.h
  \brief                 Header file of data readback optimization histograms
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef RD53DataReadbackOptimizationHistograms_H
#define RD53DataReadbackOptimizationHistograms_H

#include "DQMHistogramBase.h"
#include "Utils/ContainerFactory.h"
#include "Utils/RD53Shared.h"

#include <TH1F.h>

class DataReadbackOptimizationHistograms : public DQMHistogramBase
{
  public:
    void book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& settingsMap) override;
    void process() override;
    bool fill(std::string& inputStream) override;
    void reset() override {};

    void fillScanTAP0(const DetectorDataContainer& TAP0scanContainer);
    void fillTAP0(const DetectorDataContainer& TAP0Container);

    void fillScanTAP1(const DetectorDataContainer& TAP1scanContainer);
    void fillTAP1(const DetectorDataContainer& TAP1Container);

    void fillScanTAP2(const DetectorDataContainer& TAP2scanContainer);
    void fillTAP2(const DetectorDataContainer& TAP2Container);

    bool AreHistoBooked = false;

  private:
    DetectorContainer* fDetectorContainer;

    DetectorDataContainer TAP0scan;
    DetectorDataContainer TAP0;
    DetectorDataContainer TAP1scan;
    DetectorDataContainer TAP1;
    DetectorDataContainer TAP2scan;
    DetectorDataContainer TAP2;

    size_t startValueTAP0;
    size_t stopValueTAP0;
    size_t startValueTAP1;
    size_t stopValueTAP1;
    size_t startValueTAP2;
    size_t stopValueTAP2;
};

#endif
