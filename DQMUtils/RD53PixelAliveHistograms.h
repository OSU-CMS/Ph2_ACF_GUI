/*!
  \file                  RD53PixelAliveHistograms.h
  \brief                 Header file of PixelAlive calibration histograms
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to alkiviadis.papadopoulos@cern.ch
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef RD53PixelAliveHistograms_H
#define RD53PixelAliveHistograms_H

#include "DQMHistogramBase.h"
#include "Utils/ContainerFactory.h"

#include <TH1F.h>
#include <TH2F.h>

class PixelAliveHistograms : public DQMHistogramBase
{
  public:
    void book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& settingsMap) override;
    void process() override;
    bool fill(std::string& inputStream) override;
    void reset() override {};

    void fill(const DetectorDataContainer& DataContainer);
    void fillBCID(const DetectorDataContainer& DataContainer);
    void fillTrgID(const DetectorDataContainer& DataContainer);

    bool AreHistoBooked = false;

  private:
    DetectorContainer* fDetectorContainer;

    DetectorDataContainer Occupancy1D;
    DetectorDataContainer Occupancy2D;
    DetectorDataContainer ErrorReadOut2D;
    DetectorDataContainer Mask1Dcol;
    DetectorDataContainer Mask1Drow;
    DetectorDataContainer ToT1D;
    DetectorDataContainer ToT2D;
    DetectorDataContainer BCID;
    DetectorDataContainer TriggerID;
    DetectorDataContainer Masked2D;

    size_t nEvents;
    size_t nRows;
    size_t nCols;
};

#endif
