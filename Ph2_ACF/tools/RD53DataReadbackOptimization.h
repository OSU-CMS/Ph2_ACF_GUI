/*!
  \file                  RD53DataReadbackOptimization.h
  \brief                 Header of data readback optimization scan
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef RD53DataReadbackOptimization_H
#define RD53DataReadbackOptimization_H

#include "RD53BERtest.h"

#ifdef __USE_ROOT__
#include "DQMUtils/RD53DataReadbackOptimizationHistograms.h"
#else
typedef bool DataReadbackOptimizationHistograms;
#endif

// #########################################
// # Data Readback Optimization test suite #
// #########################################
class DataReadbackOptimization : public BERtest
{
  public:
    ~DataReadbackOptimization()
    {
        this->WriteRootFile();
        delete histos;
    }

    void Running() override;
    void Stop() override;
    void ConfigureCalibration() override;
    void sendData() override;
    void localConfigure(const std::string& hstoFileName, int currentRun) override;
    void run() override;
    void draw(bool saveData = true) override;

    void analyze(const std::string& regName, const std::vector<uint16_t>& dacListTAP, const DetectorDataContainer& theTAPscanContainer, DetectorDataContainer& theTAPContainer);

    DataReadbackOptimizationHistograms* histos;

  private:
    void fillHisto() override;

    void scanDac(const std::string& regName, const std::vector<uint16_t>& dacList, DetectorDataContainer* theContainer);

    std::vector<uint16_t> dacListTAP0;
    std::vector<uint16_t> dacListTAP1;
    std::vector<uint16_t> dacListTAP2;
    DetectorDataContainer theTAP0scanContainer;
    DetectorDataContainer theTAP0Container;
    DetectorDataContainer theTAP1scanContainer;
    DetectorDataContainer theTAP1Container;
    DetectorDataContainer theTAP2scanContainer;
    DetectorDataContainer theTAP2Container;

  protected:
    // ######################################
    // # Parameters from configuration file #
    // ######################################
    size_t startValueTAP0;
    size_t stopValueTAP0;
    size_t startValueTAP1;
    size_t stopValueTAP1;
    bool   invTAP1;
    size_t startValueTAP2;
    size_t stopValueTAP2;
    bool   invTAP2;
    bool   doUpdateChip;
};

#endif
