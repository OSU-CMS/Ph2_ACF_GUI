/*!
  \file                  RD53Monitor.h
  \brief                 Implementaion of monitoring process
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef RD53Monitor_H
#define RD53Monitor_H

#include "MonitorUtils/DetectorMonitor.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/Utilities.h"
#include "Utils/ValueAndTime.h"

// ######################
// # Specific libraries #
// ######################
#include "HWDescription/lpGBT.h"
#include "HWInterface/RD53Interface.h"
#include "HWInterface/RD53lpGBTInterface.h"
#include "HWInterface/lpGBTInterface.h"

#include <array>

#ifdef __USE_ROOT__
#include "MonitorDQM/MonitorDQMPlotRD53.h"
#endif

class RD53Monitor : public DetectorMonitor
{
  public:
    RD53Monitor(const Ph2_System::SystemController* theSystemController, const DetectorMonitorConfig& theDetectorMonitorConfig);

  private:
    void runMonitor() override;

    void runRD53RegisterMonitor(const std::string& registerName);
    void runLpGBTRegisterMonitor(const std::string& registerName);
    void sendData(DetectorDataContainer& theRegisterContainer, const std::string& registerName, const std::string& type);

#ifdef __USE_ROOT__
    MonitorDQMPlotRD53* fMonitorDQM{nullptr};
#endif
};

#endif
