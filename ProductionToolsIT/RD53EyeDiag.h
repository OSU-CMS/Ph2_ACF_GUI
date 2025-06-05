/*!
  \file                  RD53EyeDiag.h
  \brief                 Implementaion of Bit Error Rate test
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef RD53EyeDiag_H
#define RD53EyeDiag_H

#include "Utils/Container.h"
#include "tools/Tool.h"
#include <boost/algorithm/string.hpp>
#include <unordered_map>

#ifdef __USE_ROOT__
#include "DQMUtils/RD53DataReadbackOptimizationHistograms.h"
#include "TApplication.h"
#endif

// ##################
// # BER test suite #
// ##################
class EyeDiag : public Tool
{
  public:
    ~EyeDiag() override;
    void Running() override;
    void Stop() override;
    void ConfigureCalibration() override;
    void sendData() override;

    void localConfigure(const std::string fileRes_, int currentRun);
    void initializeFiles(const std::string fileRes_, int currentRun);
    void run(std::string runName = "");
    void draw();

#ifdef __USE_ROOT__
    std::unordered_map<std::string, TH1*> histos;
#endif

  private:
    size_t chain2test;
    bool   given_time;
    double frames_or_time;
    void   fillHisto();

  protected:
    DetectorDataContainer theEyeDiagContainer;

    std::string              fileRes;
    int                      theCurrentRun;
    bool                     doDisplay;
    std::vector<std::string> observables = {"EHEight", "EWIDth", "JITTer RMS", "QFACtor", "CROSsing"};
};

#endif
