/*!
  \file                  RD53LpGBTeyeOpening.h
  \brief                 Header of LpGBT eye opening scan
  \author                Mauro DINARDO
  \version               1.0
  \date                  29/11/24
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef RD53LpGBTeyeOpening_H
#define RD53LpGBTeyeOpening_H

#include "MonitorUtils/DetectorMonitor.h"
#include "RD53CalibBase.h"
#include "Utils/GenericDataArray.h"

#ifdef __USE_ROOT__
#include "DQMUtils/RD53LpGBTeyeOpeningHistograms.h"
#else
typedef bool LpGBTeyeOpeningHistograms;
#endif

// #############
// # CONSTANTS #
// #############
#define TIMEMAX 64 // Max value of time register
#define VOLTMAX 31 // Max value of voltage register

// ########################
// # LpGBT eye test suite #
// ########################
class LpGBTeyeOpening : public CalibBase
{
  public:
    ~LpGBTeyeOpening()
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

    LpGBTeyeOpeningHistograms* histos;

  private:
    void fillHisto() override;

    DetectorDataContainer theLpGBTeyeOpeningContainer;

  protected:
    // ######################################
    // # Parameters from configuration file #
    // ######################################
    int  lpGBTattenuation;
    bool doDisplay;
    bool doUpdateChip;
};

#endif
