/*!
  \file                  RD53LpGBTeyeOpeningHistograms.h
  \brief                 Header file of VTRx light yield scan histograms
  \author                Mauro DINARDO
  \version               1.0
  \date                  29/11/24
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef RD53LpGBTeyeOpeningHistograms_H
#define RD53LpGBTeyeOpeningHistograms_H

#include "DQMHistogramBase.h"
#include "Utils/ContainerFactory.h"
#include "Utils/GenericDataArray.h"

#include <TH2F.h>

// #############
// # CONSTANTS #
// #############
#define TIMEMAX 64 // Max value of time register
#define VOLTMAX 31 // Max value of voltage register

class LpGBTeyeOpeningHistograms : public DQMHistogramBase
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

    size_t lpGBTattenuation;
};

#endif
