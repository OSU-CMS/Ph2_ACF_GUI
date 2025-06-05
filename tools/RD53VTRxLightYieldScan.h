/*!
  \file                  RD53VTRxLightYieldScan.h
  \brief                 Header of VTRx light yield scan
  \author                Mauro DINARDO
  \version               1.0
  \date                  29/11/24
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef RD53VTRxLightYieldScan_H
#define RD53VTRxLightYieldScan_H

#include "MonitorUtils/DetectorMonitor.h"
#include "RD53CalibBase.h"

#ifdef __USE_ROOT__
#include "DQMUtils/RD53VTRxLightYieldScanHistograms.h"
#else
typedef bool VTRxLightYieldScanHistograms;
#endif

// ###################
// # VTRx test suite #
// ###################
class VTRxLightYieldScan : public CalibBase
{
  public:
    ~VTRxLightYieldScan()
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

    VTRxLightYieldScanHistograms* histos;

  private:
    void fillHisto() override;

    std::vector<uint16_t> dac1List;
    std::vector<uint16_t> dac2List;
    DetectorDataContainer theVTRxLightYieldScanContainer;

  protected:
    // ######################################
    // # Parameters from configuration file #
    // ######################################
    size_t biasStart;
    size_t biasStop;
    size_t biasStep;
    size_t modulationStart;
    size_t modulationStop;
    size_t modulationStep;
    bool   doDisplay;
    bool   doUpdateChip;
};

#endif
