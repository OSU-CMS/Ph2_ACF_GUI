/*!
  \file                  MonitorDQMPlotRD53.h
  \brief                 Implementaion of DQM monitoring process
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef MonitorDQMPlotRD53_H
#define MonitorDQMPlotRD53_H

#include "MonitorDQM/MonitorDQMPlotBase.h"
#include "RootUtils/GraphContainer.h"
#include "Utils/ContainerFactory.h"
#include "Utils/ContainerSerialization.h"
#include "Utils/ValueAndTime.h"

class MonitorDQMPlotRD53 : public MonitorDQMPlotBase
{
  public:
    MonitorDQMPlotRD53() {};
    ~MonitorDQMPlotRD53() {};

    void book(TFile* theOutputFile, DetectorContainer& theDetectorStructure, const DetectorMonitorConfig& detectorMonitorConfig) override;
    bool fill(std::string& inputStream) override;
    void process() override {};
    void reset(void) override {};

    void fillChipPlots(DetectorDataContainer& DataContainer, const std::string& registerName);
    void fillOptoPlots(DetectorDataContainer& DataContainer, const std::string& registerName);

  private:
    const DetectorContainer*                     fDetectorContainer;
    std::map<std::string, DetectorDataContainer> fRegisterMonitorPlotMap;
};
#endif
