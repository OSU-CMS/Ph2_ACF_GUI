/*!
  \file                  RD53EyeScanOptimizationHistograms.h
  \brief                 Header file of EyeScan optimization histograms
  \author                Sergio Sanchez CRUZ
  \version               1.0
  \date                  28/06/22
  Support:               email to sergio.sanchez.cruz@cern.ch
*/

#ifndef RD53EyeScanOptimizationHistograms_H
#define RD53EyeScanOptimizationHistograms_H

#include "../DQMHistogramBase.h"
#include "Utils/ContainerFactory.h"
#include "Utils/GenericDataArray.h"
#include "Utils/RD53Shared.h"

#include <TH1F.h>
#include <TH3F.h>

class EyeScanOptimizationHistograms : public DQMHistogramBase
{
  public:
    void book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& settingsMap) override;
    void process() override;
    bool fill(std::string& inputStream) override;
    void reset() override {};

    void fillScanTAP0(const DetectorDataContainer& TAP0scanContainer);
    void fillScanTAP1(const DetectorDataContainer& TAP1scanContainer);
    void fillScanTAP2(const DetectorDataContainer& TAP2scanContainer);
    void fillScan3D(const DetectorDataContainer& the3DContainer);

  private:
    DetectorDataContainer DetectorData;

    std::unordered_map<std::string, DetectorDataContainer*> TAP0scan;
    std::unordered_map<std::string, DetectorDataContainer*> TAP1scan;
    std::unordered_map<std::string, DetectorDataContainer*> TAP2scan;
    std::unordered_map<std::string, DetectorDataContainer*> ThreeDscan;

    size_t                   startValueTAP0;
    size_t                   stopValueTAP0;
    size_t                   startValueTAP1;
    size_t                   stopValueTAP1;
    size_t                   startValueTAP2;
    size_t                   stopValueTAP2;
    std::vector<std::string> observables = {"EHEight", "EWIDth", "JITTer RMS", "QFACtor", "CROSsing"};
};

#endif
