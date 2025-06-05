#ifndef PS_MONITOR_H
#define PS_MONITOR_H

#include "MonitorUtils/OTMonitor.h"

class PSMonitor : public OTMonitor
{
  public:
    PSMonitor(Ph2_System::SystemController* theSystemController, const DetectorMonitorConfig& theDetectorMonitorConfig);

  protected:
    void runMonitor() override;

  private:
    void runMonitorSSA(const std::string& monitorValueName);
    void runMonitorMPA(const std::string& monitorValueName);
    void readChipMonitorValue(const std::string& monitorValueName, Ph2_HwDescription::ReadoutChip* theChip, DetectorDataContainer& theDataContainer) override;
};

#endif
