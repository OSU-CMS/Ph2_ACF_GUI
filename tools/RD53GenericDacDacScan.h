/*!
  \file                  RD53GenericDacDacScan.h
  \brief                 Header of a generic DAC-DAC scan
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/05/21
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef RD53GenericDacDacScan_H
#define RD53GenericDacDacScan_H

#include "RD53PixelAlive.h"

#ifdef __USE_ROOT__
#include "DQMUtils/RD53GenericDacDacScanHistograms.h"
#else
typedef bool GenericDacDacScanHistograms;
#endif

// ##############################
// # Generic DAC-DAC test suite #
// ##############################
class GenericDacDacScan : public PixelAlive
{
  public:
    ~GenericDacDacScan()
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
        const uint16_t nIterationsDAC1 = (stopValueDAC1 - startValueDAC1) / stepDAC1 + 1;
        const uint16_t nIterationsDAC2 = (stopValueDAC2 - startValueDAC2) / stepDAC2 + 1;
        return PixelAlive::getNumberIterations() * nIterationsDAC1 * nIterationsDAC2;
    }

    void analyze();

    GenericDacDacScanHistograms* histos;

  private:
    void fillHisto() override;

    void scanDacDac(const std::string& regNameDAC1, const std::string& regNameDAC2, const std::vector<uint16_t>& dac1List, const std::vector<uint16_t>& dac2List, DetectorDataContainer* theContainer);

    std::vector<uint16_t> dac1List;
    std::vector<uint16_t> dac2List;
    DetectorDataContainer theOccContainer;
    DetectorDataContainer theGenericDacDacContainer;

  protected:
    // ######################################
    // # Parameters from configuration file #
    // ######################################
    std::string regNameDAC1;
    size_t      startValueDAC1;
    size_t      stopValueDAC1;
    size_t      stepDAC1;
    std::string regNameDAC2;
    size_t      startValueDAC2;
    size_t      stopValueDAC2;
    size_t      stepDAC2;

    bool isDAC1ChipReg;
    bool isDAC2ChipReg;
};

#endif
