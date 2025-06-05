/*!
  \file                  RD53VoltageTuningHistograms.h
  \brief                 Header file of Voltage Tuning histograms
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef RD53VoltageTuningHistograms_H
#define RD53VoltageTuningHistograms_H

#include "DQMHistogramBase.h"
#include "Utils/ContainerFactory.h"

#include <TH1F.h>

// #############
// # CONSTANTS #
// #############
#define NBINS_V 32

class VoltageTuningHistograms : public DQMHistogramBase
{
  public:
    void book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& settingsMap) override;
    void process() override;
    bool fill(std::string& inputStream) override;
    void reset() override {};

    void fillDig(const DetectorDataContainer& DataContainer);
    void fillAna(const DetectorDataContainer& DataContainer);

    bool AreHistoBooked = false;

  private:
    DetectorContainer*    fDetectorContainer;
    DetectorDataContainer VoltageDig;
    DetectorDataContainer VoltageAna;
};

#endif
