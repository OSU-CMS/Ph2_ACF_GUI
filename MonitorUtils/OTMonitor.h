#ifndef __MONITOR_OT_H__
#define __MONITOR_OT_H__

#include "MonitorUtils/DetectorMonitor.h"

namespace Ph2_HwDescription
{
class lpGBT;
}
class OTMonitor : public DetectorMonitor
{
  public:
    OTMonitor(const Ph2_System::SystemController* theSystemController, const DetectorMonitorConfig& theDetectorMonitorConfig);

  protected:
    virtual void          runMonitor() override;
    void                  runMonitorLpGBT(const std::string& monitorValueName);
    virtual void          readChipMonitorValue(const std::string& monitorValueName, Ph2_HwDescription::ReadoutChip* theChip, DetectorDataContainer& theDataContainer) = 0;
    DetectorDataContainer getReadoutChipMonitorValues(const std::string& monitorValueName, FrontEndType theFrontEndType);

  private:
    float readLpGBTmonitorValue(Ph2_HwDescription::OpticalGroup* theOpticalGroup, const std::string& monitorValueName);
};

#endif