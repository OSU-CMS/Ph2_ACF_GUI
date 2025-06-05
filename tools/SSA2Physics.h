/*!
  \file                  SSA2Physics.h
  \brief                 Implementaion of SSAPhysics data taking
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef SSAPhysics_H
#define SSAPhysics_H

#include "Utils/Container.h"
#include "Utils/ContainerFactory.h"
#include "tools/Tool.h"

#ifdef __USE_ROOT__
#include "DQMUtils/SSAPhysicsHistograms.h"
#include "TApplication.h"
#endif

// #############
// # CONSTANTS #
// #############
#define RESULTDIR "Results" // Directory containing the results

// #######################
// # SSAPhysics data taking #
// #######################
class SSAPhysics : public Tool
{
  public:
    void Running() override;
    void Stop() override;
    void ConfigureCalibration() override;

    void initialize(const std::string fileRes_, const std::string fileReg_);
    void run();
    void draw();
    void fillDataContainer(BoardContainer* const& cBoard);

  private:
    DetectorDataContainer fOccContainer;

    void initHisto();
    void fillHisto();
    void display();
    void chipErrorReport();

    // ########
    // # ROOT #
    // ########
#ifdef __USE_ROOT__
    SSAPhysicsHistograms histos;
    TApplication*        myApp;
#endif

  protected:
    std::string fileRes;
    std::string fileReg;
    bool        doUpdateChip;
    bool        doDisplay;
    bool        saveRawData;
    bool        doLocal;
};

#endif
