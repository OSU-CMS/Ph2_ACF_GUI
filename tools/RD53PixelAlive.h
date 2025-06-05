/*!
  \file                  RD53PixelAlive.h
  \brief                 Header of PixelAlive scan
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef RD53PixelAlive_H
#define RD53PixelAlive_H

#include "RD53CalibBase.h"

#ifdef __USE_ROOT__
#include "DQMUtils/RD53PixelAliveHistograms.h"
#else
typedef bool PixelAliveHistograms;
#endif

// #############
// # CONSTANTS #
// #############
#define NPIXELS_PRINTOUT 216 // Number of tested pixels before next printout

// #########################
// # PixelAlive test suite #
// #########################
class PixelAlive : public CalibBase
{
  public:
    ~PixelAlive()
    {
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
    size_t getNumberIterations() override { return theChnGroupHandler->getNumberOfGroups() * nEvents / nEvtsBurst * fDetectorContainer->size(); }

    std::shared_ptr<DetectorDataContainer> analyze();

    PixelAliveHistograms* histos;

  private:
    void fillHisto() override;
    void runPixelAlive();

    std::shared_ptr<DetectorDataContainer> theOccContainer;
    DetectorDataContainer                  theBCIDContainer;
    DetectorDataContainer                  theTrgIDContainer;

  protected:
    void SetInjectionType();

    // ######################################
    // # Parameters from configuration file #
    // ######################################
    RD53Shared::INJtype injType;
    size_t              nHITxCol;
    float               occPerPixel;
    bool                unstuckPixels;
    int                 doDataIntegrity;
    size_t              doOnlyNGroups;
    bool                doDisplay;
    bool                doUpdateChip;
    bool                saveBinaryData;

    bool                                     doSaveData;
    bool                                     doSilentRunning{false};
    const Ph2_HwDescription::RD53::FrontEnd* frontEnd;
    std::shared_ptr<RD53ChannelGroupHandler> theChnGroupHandler;
};

#endif
