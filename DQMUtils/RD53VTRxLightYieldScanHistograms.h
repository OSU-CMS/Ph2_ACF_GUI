/*!
  \file                  RD53VTRxLightYieldScanHistograms.h
  \brief                 Header file of VTRx light yield scan histograms
  \author                Mauro DINARDO
  \version               1.0
  \date                  29/11/24
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef RD53VTRxLightYieldScanHistograms_H
#define RD53VTRxLightYieldScanHistograms_H

#include "DQMHistogramBase.h"
#include "Utils/ContainerFactory.h"

#include <TH2F.h>

class VTRxLightYieldScanHistograms : public DQMHistogramBase
{
  public:
    void book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& settingsMap) override;
    void process() override;
    bool fill(std::string& inputStream) override;
    void reset() override {};

    void fillIntensity(const DetectorDataContainer& IntensityContainer);

    bool AreHistoBooked = false;

  private:
    DetectorContainer* fDetectorContainer;

    DetectorDataContainer Intensity2D;

    size_t biasStart;
    size_t biasStop;
    size_t biasStep;
    size_t modulationStart;
    size_t modulationStop;
    size_t modulationStep;
};

#endif
