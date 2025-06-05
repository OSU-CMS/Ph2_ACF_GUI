/*!
  \file                  RD53SCurveHistograms.h
  \brief                 Header file of SCurve calibration histograms
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to alkiviadis.papadopoulos@cern.ch
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef RD53SCurveHistograms_H
#define RD53SCurveHistograms_H

#include "DQMHistogramBase.h"
#include "Utils/ContainerFactory.h"
#include "Utils/RD53Shared.h"
#include "Utils/ThresholdAndNoise.h"

#include <TH1F.h>
#include <TH2F.h>
#include <TH3F.h>

// #############
// # CONSTANTS #
// #############
#define NBINS_THR 1000
#define NBINS_NOISE 200

class SCurveHistograms : public DQMHistogramBase
{
  public:
    void book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const Ph2_Parser::SettingsMap& settingsMap) override;
    void process() override;
    bool fill(std::string& inputStream) override;
    void reset() override {};

    void fillOccupancy(const DetectorDataContainer& OccupancyContainer, uint16_t DELTA_VCAL);
    void fillThrAndNoise(const DetectorDataContainer& ThrAndNoiseContainer);

    bool AreHistoBooked = false;

  private:
    DetectorContainer* fDetectorContainer;

    DetectorDataContainer Occupancy2D;
    DetectorDataContainer Occupancy3D;
    DetectorDataContainer ErrorReadOut2D;
    DetectorDataContainer ErrorFit2D;
    DetectorDataContainer Threshold1D;
    DetectorDataContainer Noise1D;
    DetectorDataContainer Threshold2D;
    DetectorDataContainer Noise2D;
    DetectorDataContainer ToT2D;
    DetectorDataContainer ThrNoise2D;

    size_t nEvents;
    size_t nSteps;
    size_t startValue;
    size_t stopValue;
    size_t offset;

    size_t nRows;
    size_t nCols;
};

#endif
