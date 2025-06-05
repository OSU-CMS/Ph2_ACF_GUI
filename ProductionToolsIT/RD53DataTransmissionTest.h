/*!
  \file                  RD53DataTransmissionTest.h
  \brief                 TAP0 scan to measure the Bit Error Rate and determine data transmission quality
  \author                Marijus AMBROZAS
  \version               1.0
  \date                  26/04/20
  Support:               email to marijus.ambrozas@cern.ch
*/

#ifndef RD53DataTransmissionTest_H
#define RD53DataTransmissionTest_H

#include "tools/RD53BERtest.h"

#ifdef __USE_ROOT__
#include "DQMUtils/ProductionITRD53A/RD53DataTransmissionTestGraphs.h"
#else
typedef bool DataTransmissionTestGraphs;
#endif

// #########################################
// # Data Readback Optimization test suite #
// #########################################
class DataTransmissionTest : public BERtest
{
  public:
    void Running() override;
    void Stop() override;
    void ConfigureCalibration() override;
    void sendData() override;

    void localConfigure(const std::string& histoFileName, int currentRun) override;
    void run() override;
    void draw(bool saveData = true) override;

    void analyze(const DetectorDataContainer& theTAP0scanContainer, DetectorDataContainer& theTAP0tgtContainer);

    DataTransmissionTestGraphs* histos;

  private:
    void fillHisto() override;

    void binSearch(DetectorDataContainer* theTAP0scanContainer);

    DetectorDataContainer theTAP0scanContainer;
    DetectorDataContainer theTAP0tgtContainer;

  protected:
    double BERtarget;
    bool   given_time;
    double frames_or_time;
};

#endif
