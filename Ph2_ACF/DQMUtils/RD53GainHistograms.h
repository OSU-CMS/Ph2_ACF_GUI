/*!
  \file                  RD53GainHistograms.h
  \brief                 Header file of Gain calibration histograms
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to alkiviadis.papadopoulos@cern.ch
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef RD53GainHistograms_H
#define RD53GainHistograms_H

#include "DQMHistogramBase.h"
#include "Utils/ContainerFactory.h"
#include "Utils/GainFit.h"
#include "Utils/RD53Shared.h"

#include <TH1F.h>
#include <TH2F.h>
#include <TH3F.h>

// #############
// # CONSTANTS #
// #############
#define INTERCEPT_HALFRANGE 15 // [ToT]
#define SLOPE_RANGE 8e-2       // [ToT / VCal]
#define NBINS_G 100            // Number of histogram bins

class GainHistograms : public DQMHistogramBase
{
  public:
    void book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& settingsMap) override;
    void process() override;
    bool fill(std::string& inputStream) override;
    void reset() override {};

    void fillOccupancy(const DetectorDataContainer& OccupancyContainer, uint16_t DELTA_VCAL);
    void fillGain(const DetectorDataContainer& GainContainer);

    bool AreHistoBooked = false;

  private:
    DetectorContainer* fDetectorContainer;

    DetectorDataContainer Occupancy2D;
    DetectorDataContainer Occupancy3D;
    DetectorDataContainer ErrorReadOut2D;
    DetectorDataContainer ErrorFit2D;

    DetectorDataContainer InterceptHighQ1D;
    DetectorDataContainer SlopeHighQ1D;
    DetectorDataContainer InterceptLowQ1D;
    DetectorDataContainer SlopeLowQ1D;
    DetectorDataContainer Chi2DoF1D;

    DetectorDataContainer InterceptHighQ2D;
    DetectorDataContainer SlopeHighQ2D;
    DetectorDataContainer InterceptLowQ2D;
    DetectorDataContainer SlopeLowQ2D;
    DetectorDataContainer Chi2DoF2D;

    size_t nEvents;
    size_t nSteps;
    size_t startValue;
    size_t stopValue;
    size_t offset;

    size_t nRows;
    size_t nCols;
};

#endif
