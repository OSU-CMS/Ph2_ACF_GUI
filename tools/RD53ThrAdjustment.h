/*!
  \file                  RD53ThrAdjustment.h
  \brief                 Header of threshold adjustment
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef RD53ThrAdjustment_H
#define RD53ThrAdjustment_H

#include "RD53PixelAlive.h"

#ifdef __USE_ROOT__
#include "DQMUtils/RD53ThresholdHistograms.h"
#else
typedef bool ThresholdHistograms;
#endif

// #############
// # CONSTANTS #
// #############
#define TARGETEFF 0.50 // Target efficiency for optimization algorithm

// #####################################
// # Threshold minimization test suite #
// #####################################
class ThrAdjustment : public PixelAlive
{
  public:
    ~ThrAdjustment()
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
        uint16_t nIterationsChip = 0;
        for(const auto cBoard: *fDetectorContainer)
            for(const auto cOpticalGroup: *cBoard)
                for(const auto cHybrid: *cOpticalGroup)
                    if(cHybrid->fullSize() > nIterationsChip) nIterationsChip = cHybrid->fullSize();
        const uint16_t nIterationsThrZero = floor(log2(stopValue - startValue + 1) + 3);
        const uint16_t moreIterations     = 1;
        return PixelAlive::getNumberIterations() * (nIterationsThrZero * nIterationsChip + moreIterations);
    }

    void analyze();

    ThresholdHistograms* histos;

  private:
    void fillHisto() override;
    void establishStartingPoint(DetectorDataContainer& chargeContainer);

    void bitWiseScanGlobal(const std::vector<const char*>& regNames, float targetThreshold, uint16_t startValue, uint16_t stopValue);

    DetectorDataContainer theThrContainer;

  protected:
    // ######################################
    // # Parameters from configuration file #
    // ######################################
    float  targetThreshold;
    size_t startValue;
    size_t stopValue;
    bool   doUpdateChip;
    bool   doDisplay;
};

#endif
