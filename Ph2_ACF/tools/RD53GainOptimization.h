/*!
  \file                  RD53GainOptimization.h
  \brief                 Header of gain optimization
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef RD53GainOptimization_H
#define RD53GainOptimization_H

#include "RD53Gain.h"

#ifdef __USE_ROOT__
#include "DQMUtils/RD53GainOptimizationHistograms.h"
#else
typedef bool GainOptimizationHistograms;
#endif

// #############
// # CONSTANTS #
// #############
#define NSTDEV 4. // Number of standard deviations for gain tolerance

// ################################
// # Gain optimization test suite #
// ################################
class GainOptimization : public Gain
{
  public:
    ~GainOptimization()
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
        uint16_t nIterationsKrumCurr = floor(log2(KrumCurrStop - KrumCurrStart + 1) + 2);
        uint16_t moreIterations      = 1;
        return Gain::getNumberIterations() * (nIterationsKrumCurr + moreIterations);
    }

    void analyze();

    GainOptimizationHistograms* histos;

  private:
    void fillHisto() override;

    void bitWiseScanGlobal(const std::string& regName, float target, uint16_t startValue, uint16_t stopValue);

    DetectorDataContainer theKrumCurrContainer;

  protected:
    // ######################################
    // # Parameters from configuration file #
    // ######################################
    size_t KrumCurrStart;
    size_t KrumCurrStop;
    bool   doUpdateChip;
    bool   doDisplay;
};

#endif
