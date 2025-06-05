/*!
  \file                  RD53SCurve.h
  \brief                 Header of SCurve scan
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef RD53SCurve_H
#define RD53SCurve_H

#include "HWDescription/RD53.h"
#include "RD53CalibBase.h"
#include "Utils/ContainerRecycleBin.h"
#include "Utils/ThresholdAndNoise.h"

#include <algorithm>

#ifdef __USE_ROOT__
#include "DQMUtils/RD53SCurveHistograms.h"
#else
typedef bool SCurveHistograms;
#endif

// #####################
// # SCurve test suite #
// #####################
class SCurve : public CalibBase
{
  public:
    ~SCurve()
    {
        for(auto container: detectorContainerVector) theRecyclingBin.free(container);
        if(doSaveData == true) this->WriteRootFile();
        delete histos;
    }

    void   Running() override;
    void   Stop() override;
    void   ConfigureCalibration() override;
    void   sendData() override;
    void   localConfigure(const std::string& histoFileName, int currentRun) override;
    void   run() override;
    void   draw(bool saveData = true) override;
    size_t getNumberIterations() override { return theChnGroupHandler->getNumberOfGroups() * nSteps * fDetectorContainer->size(); }

    std::shared_ptr<DetectorDataContainer> analyze();

    SCurveHistograms* histos;

  private:
    void fillHisto() override;

    void computeStats(std::vector<float>& measurements, int offset, float& nHits, float& mean, float& rms);

    std::vector<uint16_t>                  dacList;
    std::vector<DetectorDataContainer*>    detectorContainerVector;
    std::shared_ptr<DetectorDataContainer> theThresholdAndNoiseContainer;
    ContainerRecycleBin<OccupancyAndPh>    theRecyclingBin;

  protected:
    // ######################################
    // # Parameters from configuration file #
    // ######################################
    RD53Shared::INJtype injType;
    size_t              startValue;
    size_t              stopValue;
    float               nSteps;
    size_t              offset;
    size_t              nHITxCol;
    size_t              doOnlyNGroups;
    bool                doDisplay;
    bool                doUpdateChip;
    bool                saveBinaryData;

    bool                                     doSaveData;
    const Ph2_HwDescription::RD53::FrontEnd* frontEnd;
    std::shared_ptr<RD53ChannelGroupHandler> theChnGroupHandler;
};

#endif
