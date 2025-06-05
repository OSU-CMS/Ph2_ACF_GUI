/*!
  \file                  RD53ThrMinimization.h
  \brief                 Header of threshold minimization
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef RD53ThrMinimization_H
#define RD53ThrMinimization_H

#include "RD53PixelAlive.h"

#ifdef __USE_ROOT__
#include "DQMUtils/RD53ThresholdHistograms.h"
#else
typedef bool ThresholdHistograms;
#endif

// #####################################
// # Threshold minimization test suite #
// #####################################
class ThrMinimization : public PixelAlive
{
  public:
    ~ThrMinimization()
    {
        this->WriteRootFile();
        delete histos;
    }

    void   Running() override;
    void   Stop() override;
    void   ConfigureCalibration() override;
    void   sendData() override;
    void   localConfigure(const std::string& histoFileName, int currentRun) override;
    void   run() override;
    void   draw(bool saveData = true) override;
    size_t getNumberIterations() override
    {
        const uint16_t nIterationsThr = floor(log2(stopValue - startValue + 1) + 2);
        const uint16_t moreIterations = 1;
        return PixelAlive::getNumberIterations() * (nIterationsThr + moreIterations);
    }

    void analyze();

    ThresholdHistograms* histos;

  private:
    void fillHisto() override;

    void bitWiseScanGlobal(const std::vector<const char*>& regNames, float target, float threshold, uint16_t startValue, uint16_t stopValue);

    DetectorDataContainer theThrContainer;

  protected:
    // ######################################
    // # Parameters from configuration file #
    // ######################################
    float  targetOccupancy;
    float  maxMaskedPixels;
    size_t startValue;
    size_t stopValue;
    bool   doDisplay;
    bool   doUpdateChip;
};

#endif
