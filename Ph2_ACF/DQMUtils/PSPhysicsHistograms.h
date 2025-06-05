/*!
  \file                  PSPhysicsHistograms.h
  \brief                 Header file of Physics histograms
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef PSPhysicsHistograms_H
#define PSPhysicsHistograms_H

#include "DQMHistogramBase.h"

#include <TH1F.h>
#include <TH2F.h>

class PSPhysicsHistograms : public DQMHistogramBase
{
  public:
    void book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& settingsMap) override;
    void process() override;
    bool fill(std::string& inputStream) override;
    void reset() override {};

    // void fillSync(const DetectorDataContainer& DataContainer);
    void fillOccupancy(const DetectorDataContainer& DataContainer);
    void fillStub(const DetectorDataContainer& DataContainer);

  private:
    DetectorContainer*    fDetectorContainer;
    DetectorDataContainer fIsMPAContainer;
    // DetectorDataContainer fSClusterHistograms;
    // DetectorDataContainer fPClusterHistograms;
    // DetectorDataContainer fStubHistograms;

    DetectorDataContainer fStubHistogramContainer;
    DetectorDataContainer fOccupancyHistogramContainer;
    DetectorDataContainer fStripOccupancyHistogramContainer;
};

#endif
