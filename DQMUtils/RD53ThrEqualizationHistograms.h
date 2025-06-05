/*!
  \file                  RD53ThrEqualizationHistograms.h
  \brief                 Header file of ThrEqualization calibration histograms
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to alkiviadis.papadopoulos@cern.ch
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef RD53ThrEqualizationHistograms_H
#define RD53ThrEqualizationHistograms_H

#include "DQMHistogramBase.h"
#include "Utils/ContainerFactory.h"
#include "Utils/RD53Shared.h"

#include <TH1F.h>
#include <TH2F.h>

class ThrEqualizationHistograms : public DQMHistogramBase
{
  public:
    void book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& settingsMap) override;
    void process() override;
    bool fill(std::string& inputStream) override;
    void reset() override {};

    void fillOccupancy(const DetectorDataContainer& OccupancyContainer);
    void fillTDAC(const DetectorDataContainer& TDACContainer);
    void fillOccupancyScan(const DetectorDataContainer& OccupancyContainer);
    void fillTDACGain(const DetectorDataContainer& TDACGainContainer);

    bool AreHistoBooked = false;

  private:
    DetectorContainer* fDetectorContainer;

    DetectorDataContainer ThrEqualization;
    DetectorDataContainer TDAC1D;
    DetectorDataContainer TDAC2D;
    DetectorDataContainer Occupancy1D;
    DetectorDataContainer TDACGain;

    size_t startValue;
    size_t stopValue;
    size_t nEvents;

    size_t nRows;
    size_t nCols;

    const Ph2_HwDescription::RD53::FrontEnd* frontEnd;
};

#endif
