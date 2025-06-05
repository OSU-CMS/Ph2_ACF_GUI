/*!
  \file                  RD53BERtest.h
  \brief                 Header of Bit Error Rate test
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef RD53BERtest_H
#define RD53BERtest_H

#include "RD53CalibBase.h"

#ifdef __USE_ROOT__
#include "DQMUtils/RD53BERtestHistograms.h"
#else
typedef bool BERtestHistograms;
#endif

// ##################
// # BER test suite #
// ##################
class BERtest : public CalibBase
{
  public:
    ~BERtest()
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

    BERtestHistograms* histos;

  private:
    void fillHisto() override;

  protected:
    // ######################################
    // # Parameters from configuration file #
    // ######################################
    size_t chain2test;
    bool   given_time;
    double frames_or_time;
    bool   doDisplay;

    DetectorDataContainer theBERtestContainer;
};

#endif
