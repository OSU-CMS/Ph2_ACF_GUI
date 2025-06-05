/*!
  \file                  RD53BERtestHistograms.h
  \brief                 Header file of BERtest calibration histograms
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef RD53BERtestHistograms_H
#define RD53BERtestHistograms_H

#include "DQMHistogramBase.h"
#include "Utils/ContainerFactory.h"

#include <TH1F.h>

class BERtestHistograms : public DQMHistogramBase
{
  public:
    void book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& settingsMap) override;
    void process() override;
    bool fill(std::string& inputStream) override;
    void reset() override {};

    void fillBERtest(const DetectorDataContainer& BERtestContainer);

    bool AreHistoBooked = false;

  private:
    DetectorContainer* fDetectorContainer;

    DetectorDataContainer BERtest;
};

#endif
