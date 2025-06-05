/*!
  \file                  RD53VoltageTuning.h
  \brief                 Implementaion of Voltage Tuning
  \author                Yuta TAKAHASHI
  \version               1.0
  \date                  03/05/21
  Support:               email to Yuta.Takahashi@cern.ch
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef RD53VoltageTuning_H
#define RD53VoltageTuning_H

#include "MonitorUtils/DetectorMonitor.h"
#include "RD53CalibBase.h"

#ifdef __USE_ROOT__
#include "DQMUtils/RD53VoltageTuningHistograms.h"
#else
typedef bool VoltageTuningHistograms;
#endif

// #############
// # CONSTANTS #
// #############
#define CONVERSIONfactor 2   // Conversion factor from DAC voltage to actual voltage
#define NSIGMA 2             // Number of sigmas for voltage tolerance
#define STARTfraction 2. / 4 // Fraction of full voltage range

// #############################
// # Voltage tuning test suite #
// #############################
class VoltageTuning : public CalibBase
{
  public:
    ~VoltageTuning()
    {
        this->WriteRootFile();
        delete histos;
    }

    void Running() override;
    void Stop() override;
    void ConfigureCalibration() override;
    void sendData() override;
    void localConfigure(const std::string& histoFileName, int currentRun) override;
    void run() override;
    void draw(bool saveData = true) override;

    void analyze();

    VoltageTuningHistograms* histos;

  private:
    void fillHisto() override;

    std::vector<int> createScanRange(Ph2_HwDescription::Chip* pChip, const std::string regName, float target, float initial);

    DetectorDataContainer theAnaContainer;
    DetectorDataContainer theDigContainer;

  protected:
    // ######################################
    // # Parameters from configuration file #
    // ######################################
    float targetDig;
    float targetAna;
    float toleranceDig;
    float toleranceAna;
    bool  doDisplay;
};

#endif
