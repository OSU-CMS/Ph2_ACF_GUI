#ifndef MONITOR_2S_H
#define MONITOR_2S_H

#include "MonitorUtils/OTMonitor.h"

class Monitor2S : public OTMonitor
{
  public:
    Monitor2S(const Ph2_System::SystemController* theSystemController, const DetectorMonitorConfig& theDetectorMonitorConfig);

  protected:
    void runMonitor() override;

  private:
    void runMonitorCBC(const std::string& monitorValueName);
    void readChipMonitorValue(const std::string& monitorValueName, Ph2_HwDescription::ReadoutChip* theChip, DetectorDataContainer& theDataContainer) override;
};

#endif
