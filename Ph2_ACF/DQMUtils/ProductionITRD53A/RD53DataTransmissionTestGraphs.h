/*!
  \file                  RD53DataTransmissionTestGraphs.h
  \brief                 Header file of TAP scan graphs
  \author                Marijus AMBROZAS
  \version               1.0
  \date                  26/04/21
  Support:               email to marijus.ambrozas@cern.ch
*/

#ifndef RD53DataTransmissionTestGraphs_H
#define RD53DataTransmissionTestGraphs_H

#include "DQMUtils/DQMHistogramBase.h"
#include "Utils/ContainerFactory.h"
#include "Utils/RD53Shared.h"

#include <TGraphAsymmErrors.h>
#include <TH1F.h>

class DataTransmissionTestGraphs : public DQMHistogramBase
{
  public:
    void book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& settingsMap) override;
    void process() override;
    bool fill(std::string& inputStream) override;
    void reset() override {};

    void fillTAP0scan(const DetectorDataContainer& TAP0scanContainer);
    void fillTAP0tgt(const DetectorDataContainer& TAP0tgtContainer);

    bool AreHistoBooked = false;

  private:
    DetectorContainer* fDetectorContainer;

    DetectorDataContainer TAP0scan;
    DetectorDataContainer TAP0tgt;

    double BERtarget;
    bool   given_time;
    double frames_or_time;
};

#endif
