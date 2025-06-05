#ifndef RD53EyeScanOptimization_H
#define RD53EyeScanOptimization_H

#include "RD53EyeDiag.h"
#include "Utils/GenericDataArray.h"
#include <array>

#ifdef __USE_ROOT__
#include "DQMUtils/ProductionITRD53A/RD53EyeScanOptimizationHistograms.h"
#include "TApplication.h"
#include "TH2F.h"
#endif

// #########################################
// # Data Readback Optimization test suite #
// #########################################
class EyeScanOptimization : public EyeDiag
{
  public:
    void Running();
    void Stop() override;
    void ConfigureCalibration() override;

    void localConfigure(const std::string fileRes_, int currentRun, bool is2D = false);
    void initializeFiles(const std::string fileRes_, int currentRun);
    void run();
    void run2d();
    void draw();
    void saveChipRegisters(int currentRun);

#ifdef __USE_ROOT__
    EyeScanOptimizationHistograms* histos;
#endif

  private:
    size_t rowStart;
    size_t rowStop;
    size_t colStart;
    size_t colStop;
    size_t startValueTAP0;
    size_t stopValueTAP0;
    size_t startValueTAP1;
    size_t stopValueTAP1;
    size_t startValueTAP2;
    size_t stopValueTAP2;
    size_t nEvents;

    std::vector<uint16_t> dacListTAP0;
    std::vector<uint16_t> dacListTAP1;
    std::vector<uint16_t> dacListTAP2;

    DetectorDataContainer theTAP0scanContainer;
    DetectorDataContainer theTAP0Container;
    DetectorDataContainer theTAP1scanContainer;
    DetectorDataContainer theTAP1Container;
    DetectorDataContainer theTAP2scanContainer;
    DetectorDataContainer theTAP2Container;
    DetectorDataContainer the3DContainer;

    void fillHisto();
    void scanDac(const std::string& regName, const std::vector<uint16_t>& dacList, uint32_t nEvents, DetectorDataContainer* theContainer);
    void scanDac3D(const std::string&           regName1,
                   const std::string&           regName2,
                   const std::string&           regName3,
                   const std::vector<uint16_t>& dacList1,
                   const std::vector<uint16_t>& dacList2,
                   const std::vector<uint16_t>& dacList3,
                   uint32_t                     nEvents,
                   DetectorDataContainer*       theContainer);
    void chipErrorReport() const;

  protected:
    std::string fileRes;
    int         theCurrentRun;
    bool        doUpdateChip;
    bool        doDisplay;
    bool        saveBinaryData;
    bool        fIs2D;
};

#endif
