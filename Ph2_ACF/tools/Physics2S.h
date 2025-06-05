/*!
  \file                  Physics2S.h
  \brief                 Implementaion of Physics2S data taking
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef Physics2S_H
#define Physics2S_H

#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "Utils/MPAChannelGroupHandler.h"
#include "tools/Tool.h"
// #include "Utils/PSSharedConstants.h"
// #include "HWInterface/PSFWInterface.h"

#ifdef __USE_ROOT__
#include "DQMUtils/Physics2SHistograms.h"
#include "TApplication.h"
#endif

// #############
// # CONSTANTS #
// #############
#define RESULTDIR "Results" // Directory containing the results

// #######################
// # Physics2S data taking #
// #######################
class Physics2S : public Tool
{
  public:
    void Running() override;
    void Stop() override;
    void ConfigureCalibration() override;

    void initialize(const std::string fileRes_, const std::string fileReg_);
    void run();
    void draw();
    // void fillDataContainer(BoardContainer* const& cBoard);
    void fillDataContainer(BoardContainer* cBoard, const std::vector<Ph2_HwInterface::Event*> eventList);

    static std::string fCalibrationDescription;

  private:
    // DetectorDataContainer f2SDataContainer;
    DetectorDataContainer fStubContainer;
    DetectorDataContainer fOccupancyContainer;

    void         initHisto();
    void         fillHisto();
    void         display();
    void         chipErrorReport();
    unsigned int getDataFromBoards();
    void         clearContainers(BoardContainer* theBoard);

    // ########
    // # ROOT #
    // ########
#ifdef __USE_ROOT__
    Physics2SHistograms histos;
    TApplication*       myApp;
#endif

  protected:
    std::string  fileRes;
    std::string  fileReg;
    bool         doUpdateChip;
    bool         doDisplay;
    bool         saveRawData;
    bool         doLocal;
    unsigned int fTotalDataSize = 0;
};

#endif
