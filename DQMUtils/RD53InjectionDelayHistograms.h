/*!
  \file                  RD53InjectionDelayHistograms.h
  \brief                 Header file of InjectionDelay calibration histograms
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef RD53InjectionDelayHistograms_H
#define RD53InjectionDelayHistograms_H

#include "DQMHistogramBase.h"
#include "Utils/ContainerFactory.h"
#include "Utils/RD53Shared.h"

#include <TH1F.h>

class InjectionDelayHistograms : public DQMHistogramBase
{
  public:
    void book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& settingsMap) override;
    void process() override;
    bool fill(std::string& inputStream) override;
    void reset() override {};

    void fillOccupancy(const DetectorDataContainer& OccupancyContainer);
    void fillInjectionDelay(const DetectorDataContainer& InjectionDelayContainer);

    bool AreHistoBooked = false;

  private:
    DetectorContainer* fDetectorContainer;

    DetectorDataContainer Occupancy1D;
    DetectorDataContainer InjectionDelay;

    size_t startValue;
    size_t stopValue;
};

#endif
