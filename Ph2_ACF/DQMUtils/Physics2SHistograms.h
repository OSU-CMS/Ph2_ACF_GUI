/*!
  \file                  Physics2SHistograms.h
  \brief                 Header file of Physics histograms
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef Physics2SHistograms_H
#define Physics2SHistograms_H

#include "DQMHistogramBase.h"

#include <TH1F.h>
#include <TH2F.h>

class Physics2SHistograms : public DQMHistogramBase
{
  public:
    void book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& settingsMap) override;
    void process() override;
    bool fill(std::string& inputStream) override;
    void reset() override {};

    // void fillData(const DetectorDataContainer& DataContainer);
    void fillOccupancy(const DetectorDataContainer& DataContainer);
    void fillStub(const DetectorDataContainer& DataContainer);

  private:
    DetectorContainer* fDetectorContainer;

    DetectorDataContainer fTopSensorHistogramContainer;
    DetectorDataContainer fBottomSensorHistogramContainer;
    DetectorDataContainer fStubHistogramContainer;
};

#endif
